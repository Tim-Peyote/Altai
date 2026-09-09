#include "AltaiHands.h"
#include "AltaiCharacter.h"
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
  OldMeshVisibilityTick=uint8(Mesh->VisibilityBasedAnimTickOption);OldUpdateRateOptimizations=Mesh->bEnableUpdateRateOptimizations;
  Mesh->VisibilityBasedAnimTickOption=EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
  Mesh->bEnableUpdateRateOptimizations=false;
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
void UAltaiHands::EndPlay(const EEndPlayReason::Type R){Release();RestoreReleasedCollision(true);if(Character.IsValid()){auto* Mesh=Character->GetMesh();Mesh->SetTickGroup(OldMeshTickGroup);Mesh->VisibilityBasedAnimTickOption=EVisibilityBasedAnimTickOption(OldMeshVisibilityTick);Mesh->bEnableUpdateRateOptimizations=OldUpdateRateOptimizations;}Super::EndPlay(R);}
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
 FVector ReachOffset=FVector::ZeroVector;
 if(Articulated){
  const FTransform Grip=P->GetSocketTransform(TEXT("Grip_One"));
  const FVector Wrist=Grip.GetLocation()-Grip.GetUnitAxis(EAxis::X)*7.f-Grip.GetUnitAxis(EAxis::Z)*2.f;
  const FVector Delta=Wrist-C->GetMesh()->GetSocketLocation(TEXT("upperarm_r"));
  // Bend knees and shift the torso for low handles, retaining a bent elbow.
  // The capsule stays in place; the eyes smoothly follow the supported body adjustment.
  ReachOffset=Delta.GetSafeNormal()*FMath::Max(0.f,float(Delta.Size())-LimbReach[1]*.80f);
  const FVector Horizontal=FVector(ReachOffset.X,ReachOffset.Y,0).GetClampedToMaxSize(25.f);
  ReachOffset=Horizontal+FVector(0,0,FMath::Clamp(ReachOffset.Z,-50.,0.));
  if((Delta-ReachOffset).Size()>LimbReach[1]*.95f){Hint=TEXT("Move closer to the handle");return false;}
 }
 FurnitureBodyOffset=ReachOffset;
 if(auto* Player=Cast<AAltaiCharacter>(C))Player->InteractionEyeOffset=ReachOffset;
 ConstrainedGrip=Articulated;
 PreviousBodyLocation=C->GetActorLocation();PreviousVelocity=C->GetVelocity();Held=P;GrabAge=0;
 OldPawnResponse=P->GetCollisionResponseToChannel(ECC_Pawn);OldCCD=P->GetBodyInstance()?P->GetBodyInstance()->bUseCCD:false;
 for(int32 I=PendingCollision.Num()-1;I>=0;--I)if(PendingCollision[I].Component==P){OldPawnResponse=PendingCollision[I].Response;OldCCD=PendingCollision[I].OldCCD;PendingCollision.RemoveAt(I);}
 if(!ConstrainedGrip){P->SetCollisionResponseToChannel(ECC_Pawn,ECR_Ignore);P->SetUseCCD(true);}
 HeldMass=P->GetMass();TwoHands=!ConstrainedGrip && (HeldMass>3 || P->GetOwner()->ActorHasTag(TEXT("AltaiTwoHandOnly")));
 HandMotionOffset=SmoothedHandOffset=FVector::ZeroVector;HoldDistanceOffset=0;FollowThroughRemaining=0;ThrowPose=0;
 OldLinearDamping=P->GetLinearDamping();OldAngularDamping=P->GetAngularDamping();
 P->SetLinearDamping(FMath::Max(OldLinearDamping,.8f));P->SetAngularDamping(FMath::Max(OldAngularDamping,3.f));
 LocalContact=P->GetComponentTransform().InverseTransformPosition(Hit.ImpactPoint);
 ConfigureGrip();
 const FQuat BodyYaw=FRotator(0,C->GetActorRotation().Yaw,0).Quaternion();
 RelativeHoldRotation=BodyYaw.Inverse()*P->GetComponentQuat();NeutralHoldRotation=RelativeHoldRotation;HoldRotation=P->GetComponentQuat();
 GripAngles=FVector::ZeroVector;AnatomicalFrame=FQuat::Identity;AnatomicalFrameReady=false;GripReturningToNeutral=false;GripRotationLimited=false;GripRotationEffort=0;WristTrackingError=ForearmRoll=TrackingLossTime=0;
 OverreachTime=0;Hint=TEXT("F: put down / release");
 if(ConstrainedGrip){C->Tags.AddUnique(TEXT("AltaiFurnitureGrip"));DesiredOpening=Fixture->GetOpening();UpdateLookLock();Hint=TEXT("Drag mouse down/up: pull/push | release LMB or F");}
 return true;
}
void UAltaiHands::DragInteraction(float Delta)
{
 if(ConstrainedGrip && IsValid(Held))if(auto* Prop=Cast<AAltaiArticulatedProp>(Held->GetOwner()))DesiredOpening=FMath::Clamp(DesiredOpening+Delta,0.f,Prop->Travel);
}
void UAltaiHands::Release()
{
 const bool Gesture=MovingHand && !ConstrainedGrip && IsValid(Held);
 const FVector ReleasedVelocity=IsValid(Held)?Held->GetPhysicsLinearVelocity():FVector::ZeroVector;
 const bool Both=TwoHands;
 LastReleaseSpeed=ReleasedVelocity.Size();
 CancelManipulation();
 FollowThroughRemaining=Gesture && LastReleaseSpeed>150.f?.18f:0.f;
 FollowThroughDirection=ReleasedVelocity.GetSafeNormal();FollowThroughTwoHands=Both;
 if(Character.IsValid()){Character->Tags.Remove(TEXT("AltaiFurnitureGrip"));if(auto* Player=Cast<AAltaiCharacter>(Character.Get()))Player->InteractionEyeOffset=FVector::ZeroVector;}
 if(LockedLook && Character.IsValid())if(auto* PC=Cast<APlayerController>(Character->GetController()))PC->SetIgnoreLookInput(false);LockedLook=false;ConstrainedGrip=false;
 if(IsValid(Held)){PendingCollision.Add({Held.Get(),OldPawnResponse,OldCCD});Held->SetLinearDamping(OldLinearDamping);Held->SetAngularDamping(OldAngularDamping);}
 FurnitureBodyOffset=FVector::ZeroVector;Held=nullptr;HeldMass=0;CarrySpeedScale=CarryAccelerationScale=1;BalanceDemand=0;AuthoredGrip=false;LeftGripSocket=RightGripSocket=NAME_None;AppliedForce=0;PositionError=0;TwoHands=false;OverreachTime=0;
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
 if(!IsValid(Held) || !Held->IsSimulatingPhysics()){
  if(Held)Release();
  if(FollowThroughRemaining>0){
   FollowThroughRemaining=FMath::Max(0.f,FollowThroughRemaining-Dt);
   ContactGoals[1]+=FollowThroughDirection*(Dt*45.f);if(FollowThroughTwoHands)ContactGoals[0]+=FollowThroughDirection*(Dt*45.f);
  }
  GripAngles=FVector::ZeroVector;GripRotationEffort=0;GripRotationLimited=false;WristTrackingError=0;
  ThrowPose=FMath::FInterpTo(ThrowPose,0.f,Dt,8.f);
  Blend=FMath::FInterpTo(Blend,0.f,Dt,10);for(float& W:ContactWeights)W=FMath::FInterpTo(W,0.f,Dt,10);
  Stamina=FMath::Min(1.f,Stamina+Dt*.09f);return;
 }
 auto* C=Character.Get();
 if(ConstrainedGrip){
  auto* Prop=Cast<AAltaiArticulatedProp>(Held->GetOwner());if(!Prop){Release();return;}
  if(auto* PC=Cast<APlayerController>(C->GetController());PC && !C->ActorHasTag(TEXT("AltaiDeveloperPanelOpen"))){float X=0,Y=0;PC->GetInputMouseDelta(X,Y);DragInteraction((-Y+X*.35f)*(Prop->Sliding?.5f:.8f));}
  RefreshGripGoals();
  const float Reach=FVector::Dist(C->GetMesh()->GetSocketLocation(TEXT("upperarm_r")),ContactGoals[1]);
  GrabAge+=Dt;
  OverreachTime=GrabAge>.8f && Reach>LimbReach[1]+4?OverreachTime+Dt:0;
  if(OverreachTime>.18f){Release();Hint=TEXT("Handle out of reach - step closer");return;}
  if(GrabAge>.35f)Prop->DriveGrip(DesiredOpening,OneHandForce*FMath::Lerp(.65f,1.f,Stamina));
  PositionError=FVector::Dist(Prop->GripAt(DesiredOpening),ContactGoals[1]);CarrySpeedScale=CarryAccelerationScale=1;BalanceDemand=0;
  ContactWeights[0]=FMath::FInterpTo(ContactWeights[0],0.f,Dt,10);ContactWeights[1]=FMath::FInterpTo(ContactWeights[1],1.f,Dt,10);
  Stamina=FMath::Clamp(Stamina-Dt*FMath::Clamp(PositionError/100.f,0.f,1.f)*.025f,0.f,1.f);return;
 }
 TickManipulation(Dt);if(!IsValid(Held))return;
 TrackingLossTime=GrabAge>1.5f && WristTrackingError>28.f?TrackingLossTime+Dt:0;
 if(TrackingLossTime>.3f){Release();Hint=TEXT("Grip lost: wrist cannot follow the object - move closer or release the obstruction");return;}
 const FVector Forward=C->GetActorForwardVector(),Right=C->GetActorRightVector();
 const FQuat RotationGoal=FRotator(0,C->GetActorRotation().Yaw,0).Quaternion()*RelativeHoldRotation;
 HoldRotation=FQuat::Slerp(HoldRotation,RotationGoal,1.f-FMath::Exp(-Dt*7.f));
 // Keep a heavy load near the trunk, with clearance for the actual authored prop depth.
 float CarryDistance=FMath::Lerp(36.f,32.f,CarryPose);
 if(AuthoredGrip){const float Depth=Held->CalcBounds(FTransform(FQuat::Identity,FVector::ZeroVector,Held->GetComponentScale())).BoxExtent.X;
  CarryDistance=FMath::Max(CarryDistance,Depth+17.f);}
 CarryDistance+=HoldDistanceOffset;
 FVector Target=C->GetActorLocation()+Forward*(CarryDistance+SmoothedHandOffset.X)+Right*((TwoHands?0.f:14.f)+SmoothedHandOffset.Y)+FVector(0,0,53.f+GripHeightOffset-CarryPose*7.f+SmoothedHandOffset.Z);
 {const FVector GripCenter=Held->GetComponentTransform().TransformPosition(TwoHands?(LeftLocal+RightLocal)*.5f:RightLocal);Target-=GripCenter-Held->GetCenterOfMass();}
 // Keep the physical target inside both arm reach volumes, including during swings and winding up.
 // Clamping only the IK would leave the prop floating beyond the fingertips.
 for(int32 Pass=0;Pass<3;++Pass)for(int32 I=TwoHands?0:1;I<2;++I){
  const FVector Shoulder=C->GetMesh()->GetSocketLocation(I==0?TEXT("upperarm_l"):TEXT("upperarm_r"));
  const FVector Local=I==0?LeftLocal:RightLocal;
  const FVector Offset=Held->GetComponentTransform().TransformPosition(Local)-Held->GetCenterOfMass();
  const FVector DesiredWrist=Target+Offset;
  Target+=Shoulder+(DesiredWrist-Shoulder).GetClampedToMaxSize(LimbReach[I]*.92f)-DesiredWrist;
 }
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
 const float K=FMath::Max(650.f,HeldMass*180.f),D=2.f*FMath::Sqrt(K*HeldMass);
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
 OverreachTime=GrabAge>1.5f && PositionError>100?OverreachTime+Step:OverreachTime;
 if(OverreachTime>.45f || FVector::Dist(Current,C->GetActorLocation())>240){Release();Hint=TEXT("Grip lost: object blocked or out of reach");return;}
 RefreshGripGoals();
 bool BeyondArm=false;
 for(int32 I=TwoHands?0:1;I<2;++I)BeyondArm|=FVector::Dist(C->GetMesh()->GetSocketLocation(I==0?TEXT("upperarm_l"):TEXT("upperarm_r")),ContactGoals[I])>LimbReach[I]+8.f;
 if(GrabAge>1.5f && BeyondArm){OverreachTime+=Step;if(OverreachTime>.4f){Release();Hint=TEXT("Grip lost: move the object closer");return;}}else if(PositionError<=100)OverreachTime=0;
 Blend=FMath::FInterpTo(Blend,1.f,Dt,9);
 ContactWeights[0]=TwoHands?Blend:0;ContactWeights[1]=Blend;ContactWeights[2]=ContactWeights[3]=0;
}

void UAltaiHands::RestoreReleasedCollision(bool Force)
{
 for(int32 I=PendingCollision.Num()-1;I>=0;--I){
  auto& Entry=PendingCollision[I];auto* P=Entry.Component.Get();if(!P){PendingCollision.RemoveAt(I);continue;}
  const bool Safe=Force || !Character.IsValid() || !P->OverlapComponent(Character->GetActorLocation(),Character->GetActorQuat(),Character->GetCapsuleComponent()->GetCollisionShape(2.f));
  if(Safe)P->SetCollisionResponseToChannel(ECC_Pawn,Entry.Response);
  // Keep CCD during flight; restore the original body setting once slow and clear of the player.
  if(Safe && (Force || P->GetPhysicsLinearVelocity().Size()<150.f)){P->SetUseCCD(Entry.OldCCD);PendingCollision.RemoveAt(I);}
 }
}
