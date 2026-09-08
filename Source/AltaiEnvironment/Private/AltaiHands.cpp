#include "AltaiHands.h"
#include "AltaiArticulatedProp.h"
#include "Components/StaticMeshComponent.h"
#include "AltaiTraversal.h"
#include "AltaiWeather.h"
#include "AltaiWetSurface.h"
#include "Components/CapsuleComponent.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "Animation/AnimInstance.h"
#include "Engine/SkeletalMesh.h"
#include "PhysicsEngine/PhysicsSettings.h"

UAltaiHands::UAltaiHands()
{
 PrimaryComponentTick.bCanEverTick=true;PrimaryComponentTick.TickGroup=TG_PrePhysics;
 LimbReach={53,53,85,85};ContactGoals.Init(FVector::ZeroVector,4);ContactWeights.Init(0,4);
}
void UAltaiHands::BeginPlay()
{
 Super::BeginPlay();Character=Cast<ACharacter>(GetOwner());
 if(Character.IsValid()){
  auto* Mesh=Character->GetMesh();
  AddTickPrerequisiteComponent(Character->GetCharacterMovement());
  OldMeshTickGroup=Mesh->PrimaryComponentTick.TickGroup;
  Mesh->SetTickGroup(TG_PostPhysics);
  // Measure the actual rig in reference pose, not the current bent/IK pose.
  if(auto* Asset=Mesh->GetSkeletalMeshAsset()){
   const auto& Ref=Asset->GetRefSkeleton();
   const FName Ends[]={TEXT("hand_l"),TEXT("hand_r"),TEXT("foot_l"),TEXT("foot_r")};
   for(int I=0;I<4;++I){const int End=Ref.FindBoneIndex(Ends[I]);if(End==INDEX_NONE)continue;
    const int Joint=Ref.GetParentIndex(End);if(Joint==INDEX_NONE)continue;
    LimbReach[I]=(Ref.GetRefBonePose()[End].GetTranslation().Size()+Ref.GetRefBonePose()[Joint].GetTranslation().Size())*Mesh->GetComponentScale().GetMin()*.97f;
   }
  }
 }
 if(Character.IsValid())if(auto* Anim=LoadClass<UAnimInstance>(nullptr,TEXT("/Game/Altai/Player/ABP_AltaiContacts.ABP_AltaiContacts_C")))
  Character->GetMesh()->SetOverridePostProcessAnimBP(Anim,true);
}
void UAltaiHands::EndPlay(const EEndPlayReason::Type R){Release();RestoreReleasedCollision(true);if(Character.IsValid())Character->GetMesh()->SetTickGroup(OldMeshTickGroup);Super::EndPlay(R);}
void UAltaiHands::ToggleGrab(){if(Held)Release();else TryGrab();}
bool UAltaiHands::TryGrab(bool FurnitureOnly)
{
 if(Held || WallContacts || !Character.IsValid())return false;
 if(auto* Traversal=GetOwner()->FindComponentByClass<UAltaiTraversal>();Traversal && Traversal->Climbing){Hint=TEXT("Finish the mantle before grabbing");return false;}
 auto* C=Character.Get();FVector Eye;FRotator Look;C->GetActorEyesViewPoint(Eye,Look);
 if(auto* PC=Cast<APlayerController>(C->GetController()))Look=PC->GetControlRotation();
 // Aim from the character, never from the distant third-person camera.
 const FVector Start=C->GetActorLocation()+FVector(0,0,55);
 FHitResult Hit;FCollisionQueryParams Q(SCENE_QUERY_STAT(AltaiGrab),false,C);
 float SweepRadius=14;
 // Resolve the visible cursor target first, then verify reach and occlusion from the body.
 if(auto* PC=Cast<APlayerController>(C->GetController());PC && PC->PlayerCameraManager){
  const FVector Camera=PC->PlayerCameraManager->GetCameraLocation();FHitResult Aim;
  if(GetWorld()->LineTraceSingleByChannel(Aim,Camera,Camera+Look.Vector()*(MaxReach+FVector::Dist(Camera,Start)),ECC_Visibility,Q)
    && Aim.GetComponent() && Aim.GetComponent()->IsSimulatingPhysics() && FVector::Dist(Start,Aim.ImpactPoint)<=MaxReach){Look=(Aim.ImpactPoint-Start).Rotation();SweepRadius=4;}
 }
 if(!GetWorld()->SweepSingleByChannel(Hit,Start,Start+Look.Vector()*MaxReach,FQuat::Identity,ECC_Visibility,FCollisionShape::MakeSphere(SweepRadius),Q))
 {Hint=TEXT("Aim at a loose object within reach");return false;}
 auto* P=Hit.GetComponent();
 auto* Fixture=P?Cast<AAltaiArticulatedProp>(P->GetOwner()):nullptr;
 const bool Articulated=Fixture && P==Fixture->Part;
 if(FurnitureOnly && !Articulated)return false;
 if(!P || !P->IsSimulatingPhysics() || (!Articulated && P->GetMass()>MaxMass)){Hint=TEXT("Object is fixed or too heavy");return false;}
 if(Articulated && FVector::Dist(C->GetMesh()->GetSocketLocation(TEXT("upperarm_r")),P->GetSocketLocation(TEXT("Grip_One")))>LimbReach[1]+8){Hint=TEXT("Move closer to the handle (Ctrl crouch)");return false;}
 ConstrainedGrip=Articulated;
 PreviousBodyLocation=C->GetActorLocation();Held=P;GrabAge=0;OldPawnResponse=P->GetCollisionResponseToChannel(ECC_Pawn);for(int32 I=PendingCollision.Num()-1;I>=0;--I)if(PendingCollision[I].Component==P){OldPawnResponse=PendingCollision[I].Response;PendingCollision.RemoveAt(I);}if(!ConstrainedGrip)P->SetCollisionResponseToChannel(ECC_Pawn,ECR_Ignore);HeldMass=P->GetMass();TwoHands=!ConstrainedGrip && HeldMass>3;
 OldLinearDamping=P->GetLinearDamping();OldAngularDamping=P->GetAngularDamping();
 P->SetLinearDamping(FMath::Max(OldLinearDamping,.8f));P->SetAngularDamping(FMath::Max(OldAngularDamping,3.f));
 LocalContact=P->GetComponentTransform().InverseTransformPosition(Hit.ImpactPoint);
 const FVector Right=C->GetActorRightVector(),Forward=C->GetActorForwardVector();
 const float Radius=FMath::Clamp(P->Bounds.BoxExtent.GetMin(),5.f,22.f);
 const FVector Center=P->GetCenterOfMass();
 LeftLocal=P->GetComponentTransform().InverseTransformPosition(Center-Right*(Radius+5)-Forward*Radius*.25f);
 RightLocal=P->GetComponentTransform().InverseTransformPosition(Center+Right*(Radius+5)-Forward*Radius*.25f);
 LeftGripSocket=TEXT("Grip_L");RightGripSocket=TwoHands?FName(TEXT("Grip_R")):FName(TEXT("Grip_One"));
 AuthoredGrip=P->DoesSocketExist(RightGripSocket) && (!TwoHands || P->DoesSocketExist(LeftGripSocket));
 if(AuthoredGrip){
  RightLocal=P->GetComponentTransform().InverseTransformPosition(P->GetSocketLocation(RightGripSocket));
  if(TwoHands)LeftLocal=P->GetComponentTransform().InverseTransformPosition(P->GetSocketLocation(LeftGripSocket));
 }else{LeftGripSocket=NAME_None;RightGripSocket=NAME_None;}
 HoldRotation=AuthoredGrip?FRotator(0,C->GetActorRotation().Yaw,0).Quaternion():P->GetComponentQuat();OverreachTime=0;Hint=TEXT("F: put down / release");
 if(ConstrainedGrip){C->Tags.AddUnique(TEXT("AltaiFurnitureGrip"));DesiredOpening=Fixture->GetOpening();if(auto* PC=Cast<APlayerController>(C->GetController())){PC->SetIgnoreLookInput(true);LockedLook=true;}Hint=TEXT("Drag mouse down/up: pull/push | release LMB or F");}
 return true;
}
void UAltaiHands::DragInteraction(float Delta)
{
 if(ConstrainedGrip && IsValid(Held))if(auto* Prop=Cast<AAltaiArticulatedProp>(Held->GetOwner()))DesiredOpening=FMath::Clamp(DesiredOpening+Delta,0.f,Prop->Travel);
}
void UAltaiHands::Release()
{
 if(Character.IsValid())Character->Tags.Remove(TEXT("AltaiFurnitureGrip"));
 if(LockedLook && Character.IsValid())if(auto* PC=Cast<APlayerController>(Character->GetController()))PC->SetIgnoreLookInput(false);LockedLook=false;ConstrainedGrip=false;
 if(IsValid(Held)){PendingCollision.Add({Held.Get(),OldPawnResponse});Held->SetLinearDamping(OldLinearDamping);Held->SetAngularDamping(OldAngularDamping);}
 Held=nullptr;HeldMass=0;CarrySpeedScale=CarryAccelerationScale=1;BalanceDemand=0;AuthoredGrip=false;LeftGripSocket=RightGripSocket=NAME_None;AppliedForce=0;PositionError=0;TwoHands=false;OverreachTime=0;
 Hint=TEXT("F: take object");
}
void UAltaiHands::RefreshGripGoals()
{
 if(!IsValid(Held))return;
 ContactGoals[0]=Held->GetComponentTransform().TransformPosition(LeftLocal);
 ContactGoals[1]=Held->GetComponentTransform().TransformPosition(RightLocal);
}
void UAltaiHands::SetWallContacts(const TArray<FVector>& Goals,bool Active)
{
 if(Held)return;WallContacts=Active;
 if(Active && Goals.Num()==4){ContactGoals=Goals;for(float& W:ContactWeights)W=FMath::FInterpTo(W,1.f,GetWorld()->GetDeltaSeconds(),10.f);}
 // Keep the last pose during release; Tick fades the contact weights out.
}
void UAltaiHands::TickComponent(float Dt,ELevelTick T,FActorComponentTickFunction* F)
{
 Super::TickComponent(Dt,T,F);RestoreReleasedCollision(false);if(!Character.IsValid())return;
 BodyMass=FMath::Max(1.f,Character->GetCharacterMovement()->Mass);
 CarryPose=FMath::FInterpTo(CarryPose,IsValid(Held) && !ConstrainedGrip?FMath::Clamp(HeldMass/BodyMass*4.f,0.f,1.f):0.f,Dt,5.f);
 if(WallContacts)return;
 float Rain=0;if(TActorIterator<AAltaiWeatherRig> W(GetWorld());W)Rain=W->Rain;
 FHitResult Roof;FCollisionQueryParams RainQuery(SCENE_QUERY_STAT(AltaiHandRain),false,Character.Get());
 if(Held)RainQuery.AddIgnoredComponent(Held.Get());
 const FVector P=Character->GetActorLocation()+FVector(0,0,90);
 const bool Covered=GetWorld()->LineTraceSingleByChannel(Roof,P,P+FVector(0,0,2500),ECC_Visibility,RainQuery);
 Wetness=FMath::Clamp(Wetness+Dt*((Covered?0:Rain)*.07f-(Rain<.1f?.025f:0)),0.f,1.f);
 if(!IsValid(Held) || !Held->IsSimulatingPhysics()){if(Held)Release();Blend=FMath::FInterpTo(Blend,0.f,Dt,10);for(float& W:ContactWeights)W=FMath::FInterpTo(W,0.f,Dt,10);Stamina=FMath::Min(1.f,Stamina+Dt*.09f);return;}
 auto* C=Character.Get();
 if(ConstrainedGrip){
  auto* Prop=Cast<AAltaiArticulatedProp>(Held->GetOwner());if(!Prop){Release();return;}
  if(auto* PC=Cast<APlayerController>(C->GetController())){float X=0,Y=0;PC->GetInputMouseDelta(X,Y);DragInteraction((-Y+X*.35f)*(Prop->Sliding?.5f:.8f));}
  RefreshGripGoals();
  const float Reach=FVector::Dist(C->GetMesh()->GetSocketLocation(TEXT("upperarm_r")),ContactGoals[1]);
  OverreachTime=Reach>LimbReach[1]+12?OverreachTime+Dt:0;
  if(OverreachTime>.18f){Release();Hint=TEXT("Handle out of reach - step closer");return;}
  Prop->DriveGrip(DesiredOpening,OneHandForce*FMath::Lerp(.65f,1.f,Stamina));
  PositionError=FVector::Dist(Prop->GripAt(DesiredOpening),ContactGoals[1]);CarrySpeedScale=CarryAccelerationScale=1;BalanceDemand=0;
  ContactWeights[0]=FMath::FInterpTo(ContactWeights[0],0.f,Dt,10);ContactWeights[1]=FMath::FInterpTo(ContactWeights[1],1.f,Dt,10);
  Stamina=FMath::Clamp(Stamina-Dt*FMath::Clamp(PositionError/100.f,0.f,1.f)*.025f,0.f,1.f);return;
 }
 const FVector Forward=C->GetActorForwardVector();
 if(AuthoredGrip)HoldRotation=FQuat::Slerp(HoldRotation,FRotator(0,C->GetActorRotation().Yaw,0).Quaternion(),1.f-FMath::Exp(-Dt*7.f));
 // Keep a heavy load near the trunk, with clearance for the actual authored prop depth.
 float CarryDistance=FMath::Lerp(46.f,40.f,CarryPose);
 if(AuthoredGrip){const float Depth=Held->CalcBounds(FTransform(FQuat::Identity,FVector::ZeroVector,Held->GetComponentScale())).BoxExtent.X;
  CarryDistance=FMath::Max(CarryDistance,Depth+17.f);}
 FVector Target=C->GetActorLocation()+Forward*CarryDistance+FVector(0,0,45.f-CarryPose*12.f);
 if(AuthoredGrip){const FVector GripCenter=Held->GetComponentTransform().TransformPosition(TwoHands?(LeftLocal+RightLocal)*.5f:RightLocal);Target-=GripCenter-Held->GetCenterOfMass();}
 BodyMass=FMath::Max(1.f,C->GetCharacterMovement()->Mass);
 const float Ratio=HeldMass/BodyMass;
 CarrySpeedScale=1.f/(1.f+2.f*Ratio);CarryAccelerationScale=1.f/(1.f+3.f*Ratio);
 const FVector Acceleration=(C->GetVelocity()-PreviousVelocity)/FMath::Max(Dt,.001f);PreviousVelocity=C->GetVelocity();
 const float Lever=FVector::Dist2D(Held->GetCenterOfMass(),C->GetActorLocation())/52.f;
 BalanceDemand=FMath::Clamp(Ratio*(Lever+Acceleration.Size2D()/600.f),0.f,1.f);
 const FVector Current=Held->GetCenterOfMass();
 const FVector Error=Target-Current;PositionError=Error.Size();
 const float Step=FMath::Clamp(Dt,.001f,GetDefault<UPhysicsSettings>()->MaxPhysicsDeltaTime);
 // Walking velocity omits capsule step-up/down displacement. Feed the actual body travel
 // over the physics step, so a carried load follows stairs and capped low-FPS simulation.
 const FVector BodyDelta=C->GetActorLocation()-PreviousBodyLocation;PreviousBodyLocation=C->GetActorLocation();
 const FVector BodyVelocity=BodyDelta.Size()<100.f?BodyDelta/Step:C->GetVelocity();
 const FVector Velocity=Held->GetPhysicsLinearVelocity()-BodyVelocity;
 float ContactWetness=Wetness;if(auto* Surface=Held->GetOwner()->FindComponentByClass<UAltaiWetSurface>())ContactWetness=FMath::Max(ContactWetness,Surface->Wetness);
 const float Limit=(TwoHands?TwoHandForce:OneHandForce)*FMath::Lerp(.55f,1.f,Stamina)*FMath::Lerp(1.f,.7f,ContactWetness);
 // UE forces use kg*cm/s^2. Gravity compensation still consumes the bounded force budget.
 const float K=650.f,D=2.f*FMath::Sqrt(K*HeldMass);
 const float Denom=1.f+D/HeldMass*Step+K/HeldMass*Step*Step;
 FVector Force=(Error*K-Velocity*(D+K*Step))/Denom+FVector(0,0,-GetWorld()->GetGravityZ()*HeldMass);
 Force=Force.GetClampedToMaxSize(Limit);AppliedForce=Force.Size();
 Held->AddForce(Force,NAME_None,false);
 // Bounded angular drive: an off-centre grip has to work against the object's inertia.
 FQuat Delta=HoldRotation*Held->GetComponentQuat().Inverse();if(Delta.W<0)Delta=Delta*-1;
 FVector Axis;float Angle;Delta.ToAxisAndAngle(Axis,Angle);
 // A thin rod has very different roll and yaw inertia. Stabilize each local axis separately.
 const FQuat Rotation=Held->GetComponentQuat();
 const FVector Inertia=Held->GetInertiaTensor(),AngularError=Rotation.UnrotateVector(Axis*Angle);
 const FVector Omega=Rotation.UnrotateVector(Held->GetPhysicsAngularVelocityInRadians());
 FVector LocalTorque;
 for(int32 I=0;I<3;++I){const float Moment=FMath::Max(.1f,static_cast<float>(Inertia[I]));const float Spring=Moment*36.f,Damping=Moment*12.f;
  LocalTorque[I]=(AngularError[I]*Spring-Omega[I]*(Damping+Spring*Step))/(1.f+Damping/Moment*Step+Spring/Moment*Step*Step);}
 Held->AddTorqueInRadians(Rotation.RotateVector(LocalTorque).GetClampedToMaxSize(Limit*12),NAME_None,false);
 Stamina=FMath::Clamp(Stamina-Step*(HeldMass*.0015f+FMath::Max(0.f,AppliedForce/Limit-.8f)*.08f),0.f,1.f);
 GrabAge+=Step;
 OverreachTime=GrabAge>1.5f && PositionError>100?OverreachTime+Step:0;
 if(OverreachTime>.45f || FVector::Dist(Current,C->GetActorLocation())>240){Release();Hint=TEXT("Grip lost: object blocked or out of reach");return;}
 RefreshGripGoals();
 Blend=FMath::FInterpTo(Blend,1.f,Dt,9);
 ContactWeights[0]=TwoHands?Blend:0;ContactWeights[1]=Blend;ContactWeights[2]=ContactWeights[3]=0;
}

void UAltaiHands::RestoreReleasedCollision(bool Force)
{
 for(int32 I=PendingCollision.Num()-1;I>=0;--I){
  auto& Entry=PendingCollision[I];auto* P=Entry.Component.Get();if(!P){PendingCollision.RemoveAt(I);continue;}
  bool Safe=Force || !Character.IsValid();
  if(!Safe){FVector Closest;const float Distance=Character->GetCapsuleComponent()->GetClosestPointOnCollision(P->Bounds.Origin,Closest);Safe=Distance>P->Bounds.SphereRadius+3;}
  if(Safe){P->SetCollisionResponseToChannel(ECC_Pawn,Entry.Response);PendingCollision.RemoveAt(I);}
 }
}
