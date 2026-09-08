#include "AltaiArticulatedProp.h"
#include "Components/StaticMeshComponent.h"
#include "PhysicsEngine/PhysicsConstraintComponent.h"
#include "PhysicsEngine/PhysicsSettings.h"

AAltaiArticulatedProp::AAltaiArticulatedProp()
{
 Base=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Base"));SetRootComponent(Base);Base->SetMobility(EComponentMobility::Static);Base->SetCollisionProfileName(TEXT("BlockAll"));
 Part=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Part"));Part->SetupAttachment(Base);Part->SetMobility(EComponentMobility::Movable);Part->SetCollisionProfileName(TEXT("PhysicsActor"));
 Joint=CreateDefaultSubobject<UPhysicsConstraintComponent>(TEXT("Joint"));Joint->SetupAttachment(Base);
}
void AAltaiArticulatedProp::BeginPlay()
{
 Super::BeginPlay();if(!Part->GetStaticMesh())return;
 auto* Anchor=AnchorActor?AnchorActor->FindComponentByClass<UStaticMeshComponent>():Base.Get();
 if(!Anchor || !Anchor->GetStaticMesh()){UE_LOG(LogTemp,Error,TEXT("Articulated prop %s needs a physical anchor"),*GetName());return;}
 Axis=Axis.GetSafeNormal();if(Axis.IsNearlyZero())Axis=FVector::UpVector;
 Travel=FMath::Clamp(Travel,1.f,Sliding?100.f:150.f);
 Closed=Part->GetComponentTransform();GripLocal=Closed.InverseTransformPosition(Part->GetSocketLocation(TEXT("Grip_One")));
 Part->SetMassOverrideInKg(NAME_None,PartMass,true);Part->SetLinearDamping(Resistance);Part->SetAngularDamping(Resistance);Part->SetSimulatePhysics(true);
 Joint->SetWorldTransform(Closed);Joint->SetDisableCollision(true);
 Joint->SetLinearYLimit(LCM_Locked,0);Joint->SetLinearZLimit(LCM_Locked,0);
 // UE shares one linear limit radius across axes; set its value last.
 Joint->SetLinearXLimit(Sliding?LCM_Limited:LCM_Locked,Sliding?Travel*.5f:0);
 Joint->SetAngularSwing1Limit(ACM_Locked,0);Joint->SetAngularSwing2Limit(ACM_Locked,0);Joint->SetAngularTwistLimit(Sliding?ACM_Locked:ACM_Limited,Sliding?0:Travel*.5f);
 Joint->SetConstrainedComponents(Anchor,NAME_None,Part,NAME_None);
 const FQuat Basis=FRotationMatrix::MakeFromX(Axis).ToQuat();
 const FTransform Frame2(Basis,FVector::ZeroVector);
 const FTransform Center(Sliding?Basis:FQuat(Axis,FMath::DegreesToRadians(Travel*.5f))*Basis,Sliding?Axis*Travel*.5f:FVector::ZeroVector);
 const FTransform WorldFrame=Center*Closed;
 Joint->SetConstraintReferenceFrame(EConstraintFrame::Frame1,WorldFrame.GetRelativeTransform(Anchor->GetComponentTransform()));
 Joint->SetConstraintReferenceFrame(EConstraintFrame::Frame2,Frame2);
}
float AAltaiArticulatedProp::GetOpening() const
{
 if(Sliding)return FVector::DotProduct(Closed.InverseTransformPosition(Part->GetComponentLocation()),Axis);
 const FQuat Q=Closed.GetRotation().Inverse()*Part->GetComponentQuat();
 return FMath::RadiansToDegrees(2*FMath::Atan2(FVector::DotProduct(FVector(Q.X,Q.Y,Q.Z),Axis),Q.W));
}
FVector AAltaiArticulatedProp::GripAt(float Opening) const
{
 Opening=FMath::Clamp(Opening,0.f,Travel);
 return Closed.TransformPosition(Sliding?GripLocal+Axis*Opening:FQuat(Axis,FMath::DegreesToRadians(Opening)).RotateVector(GripLocal));
}
void AAltaiArticulatedProp::DriveGrip(float Opening,float Strength)
{
 const FVector Point=Part->GetSocketLocation(TEXT("Grip_One"));
 const FVector Error=GripAt(Opening)-Point;
 // Force at the handle preserves the lever arm; never teleport or rotate the rigid body.
 const float Mass=FMath::Max(Part->GetMass(),1.f),K=Sliding?500.f:350.f,D=2*FMath::Sqrt(K*Mass);
 const float Step=FMath::Clamp(GetWorld()->GetDeltaSeconds(),.001f,UPhysicsSettings::Get()->MaxPhysicsDeltaTime);
 FVector Force=(Error*K-Part->GetPhysicsLinearVelocityAtPoint(Point)*D)/(1+D/Mass*Step+K/Mass*Step*Step);
 if(!Sliding){
  const FVector WorldAxis=Closed.TransformVectorNoScale(Axis),Lever=Point-Part->GetComponentLocation();
  const FVector Tangent=FVector::CrossProduct(WorldAxis,Lever);
  const FVector GravityTorque=FVector::CrossProduct(Part->GetCenterOfMass()-Part->GetComponentLocation(),FVector(0,0,Mass*GetWorld()->GetGravityZ()));
  // Holding a lid supports its weight through the hand; the same force budget still applies.
  Force-=Tangent*(FVector::DotProduct(WorldAxis,GravityTorque)/FMath::Max(Tangent.SizeSquared(),1.f));
 }
 Part->AddForceAtLocation(Force.GetClampedToMaxSize(Strength),Point);Part->WakeAllRigidBodies();
}
