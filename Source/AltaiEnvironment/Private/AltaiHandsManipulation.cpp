#include "AltaiHands.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "InputCoreTypes.h"
#include "AltaiGripProfile.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"

void UAltaiHands::UpdateLookLock()
{
 const bool Needed=IsValid(Held) && (ConstrainedGrip || MovingHand || RotatingHeld);
 if(Needed==LockedLook || !Character.IsValid())return;
 if(auto* PC=Cast<APlayerController>(Character->GetController())){PC->SetIgnoreLookInput(Needed);LockedLook=Needed;}
}
void UAltaiHands::ConfigureGrip()
{
 if(!IsValid(Held) || !Character.IsValid())return;
 GripHeightOffset=0;
 if(auto* Static=Cast<UStaticMeshComponent>(Held))if(auto* Asset=Static->GetStaticMesh().Get())if(auto* Profile=Cast<UAltaiGripProfile>(Asset->GetAssetUserDataOfClass(UAltaiGripProfile::StaticClass())))GripHeightOffset=FMath::Clamp(Profile->CarryHeightOffset,-28.f,8.f);
 auto* C=Character.Get();const FVector Right=C->GetActorRightVector(),Forward=C->GetActorForwardVector();
 const float Radius=FMath::Clamp(Held->Bounds.BoxExtent.GetMin(),5.f,22.f);
 const FVector Center=Held->GetCenterOfMass();const FTransform Transform=Held->GetComponentTransform();
 LeftLocal=Transform.InverseTransformPosition(Center-Right*(Radius+5)-Forward*Radius*.25f);
 RightLocal=Transform.InverseTransformPosition(Center+Right*(Radius+5)-Forward*Radius*.25f);
 LeftGripSocket=TEXT("Grip_L");RightGripSocket=TwoHands?FName(TEXT("Grip_R")):FName(TEXT("Grip_One"));
 AuthoredGrip=Held->DoesSocketExist(RightGripSocket) && (!TwoHands || Held->DoesSocketExist(LeftGripSocket));
 if(AuthoredGrip){
  const FTransform Grip=Held->GetSocketTransform(RightGripSocket);
  const FVector Wrist=Grip.GetLocation()-(ConstrainedGrip?Grip.GetUnitAxis(EAxis::X)*7.f+Grip.GetUnitAxis(EAxis::Z)*2.f:FVector::ZeroVector);
  RightLocal=Transform.InverseTransformPosition(Wrist);
  if(TwoHands)LeftLocal=Transform.InverseTransformPosition(Held->GetSocketLocation(LeftGripSocket));
 }else{LeftGripSocket=NAME_None;RightGripSocket=NAME_None;}
 for(int32 I=0;I<2;++I){const FName Socket=I==0?LeftGripSocket:RightGripSocket;
  const FVector Wrist=Transform.TransformPosition(I==0?LeftLocal:RightLocal),Palm=(Center-Wrist).GetSafeNormal();
  const FQuat Frame=!Socket.IsNone()?Held->GetSocketQuaternion(Socket):FRotationMatrix::MakeFromXZ(FVector::VectorPlaneProject(FVector::UpVector,Palm).GetSafeNormal(),Palm).ToQuat();
  LocalGripFrames[I]=Transform.GetRotation().Inverse()*Frame;
 }
 RefreshGripGoals();
}
FQuat UAltaiHands::GripOrientation(int32 Hand) const
{
 return IsValid(Held)?Held->GetComponentQuat()*LocalGripFrames[FMath::Clamp(Hand,0,1)]:FQuat::Identity;
}
bool UAltaiHands::SetTwoHandGrip(bool Enabled)
{
 if(!IsValid(Held) || ConstrainedGrip || ChargingThrow || ThrowPending || RotatingHeld)return false;
 
 const float Limit=OneHandMassLimit*FMath::Clamp(FMath::Sqrt(BodyMass/100.f),.7f,1.25f);
 if(!Enabled && (HeldMass>Limit || Held->GetOwner()->ActorHasTag(TEXT("AltaiTwoHandOnly")))){
  Hint=TEXT("Too heavy for one hand - keep both hands on the object");return false;
 }
 if(!GripAngles.IsNearlyZero(2.f)){PendingGripMode=Enabled?1:0;GripReturningToNeutral=true;Hint=TEXT("Returning hand to neutral to change grip");return true;}
 if(TwoHands!=Enabled){TwoHands=Enabled;ConfigureGrip();Blend=0;AnatomicalFrameReady=false;}
 return true;
}
bool UAltaiHands::BeginHandMotion()
{
 if(!IsValid(Held) || ConstrainedGrip || ChargingThrow || ThrowPending)return false;
 MovingHand=true;UpdateLookLock();return true;
}
void UAltaiHands::MoveHeldHand(FVector2D Delta)
{
 if(!MovingHand || RotatingHeld || !IsValid(Held) || ConstrainedGrip || ChargingThrow || ThrowPending)return;
 // An upward stroke moves the hand forward and up; sideways strokes sweep laterally.
 // These are bounded target displacements, never velocities or impulses applied to the object.
 HandMotionOffset.X=FMath::Clamp(HandMotionOffset.X+Delta.Y*.55f,-18.f,18.f);
 HandMotionOffset.Y=FMath::Clamp(HandMotionOffset.Y+Delta.X*.55f,-26.f,26.f);
 HandMotionOffset.Z=FMath::Clamp(HandMotionOffset.Z+Delta.Y*.4f,-20.f,20.f);
}
void UAltaiHands::SetHeldRotationMode(bool Enabled)
{
 if(Enabled && (!IsValid(Held) || ConstrainedGrip || ChargingThrow || ThrowPending || GrabAge<.35f))return;
 RotatingHeld=Enabled;
 if(Enabled && !AnatomicalFrameReady){
  const FQuat Yaw=FRotator(0,Character->GetActorRotation().Yaw,0).Quaternion();
  const FVector Forearm=(Character->GetMesh()->GetSocketLocation(TEXT("hand_r"))-Character->GetMesh()->GetSocketLocation(TEXT("lowerarm_r"))).GetSafeNormal();
  FVector Palm=Held->DoesSocketExist(RightGripSocket)?Held->GetSocketTransform(RightGripSocket).GetUnitAxis(EAxis::Z):Character->GetActorForwardVector();
  if(FMath::Abs(FVector::DotProduct(Forearm,Palm))>.95f)Palm=Character->GetActorRightVector();
  AnatomicalFrame=Yaw.Inverse()*FRotationMatrix::MakeFromXZ(Forearm,Palm).ToQuat();AnatomicalFrameReady=true;NeutralForearmRoll=SignedForearmRoll;
 }
 if(Enabled)GripReturningToNeutral=false;
 UpdateLookLock();
}
void UAltaiHands::RotateHeld(FVector Degrees)
{
 if(!RotatingHeld || !IsValid(Held) || ConstrainedGrip)return;
 // Do not accumulate hidden mouse travel beyond a limit. Reversing the mouse responds immediately.
 Degrees=Degrees.GetClampedToMaxSize(6.f);
 const FVector Before=GripAngles;
 GripAngles+=Degrees;
 UpdateAnatomicalRotation();
 GripRotationLimited=!(GripAngles-Before).Equals(Degrees,.01f);
}
void UAltaiHands::UpdateAnatomicalRotation()
{
 const float Load=FMath::Clamp(HeldMass/(TwoHands?20.f:6.f),0.f,1.f);
 const float Strength=FMath::Lerp(1.f,.65f,Load)*FMath::Lerp(.8f,1.f,Stamina);
 FVector Positive(45,20,12),Negative(35,20,12),Both(20,20,10);
 if(auto* Static=Cast<UStaticMeshComponent>(Held))if(auto* Asset=Static->GetStaticMesh().Get())if(auto* Profile=Cast<UAltaiGripProfile>(Asset->GetAssetUserDataOfClass(UAltaiGripProfile::StaticClass()))){Positive=Profile->OneHandPositiveLimits;Negative=Profile->OneHandNegativeLimits;Both=Profile->TwoHandLimits;}
 for(int32 I=0;I<3;++I)GripAngleLimits[I]=FMath::Clamp(TwoHands?Both[I]:(GripAngles[I]>=0?Positive[I]:Negative[I]),1.,I==0?65.:I==1?40.:25.)*Strength;
 if(AnatomicalFrameReady){const float Available=TwoHands?FMath::Max(.1f,75.f-FMath::Abs(NeutralForearmRoll)):FMath::Max(.1f,75.f-(GripAngles.X>=0?NeutralForearmRoll:-NeutralForearmRoll));GripAngleLimits.X=FMath::Min(GripAngleLimits.X,Available);}
 // Combined motions share an ellipsoidal comfort budget rather than allowing all extremes at once.
 FVector N=GripAngles/GripAngleLimits;
 N=N.GetClampedToMaxSize(1.f);GripAngles=N*GripAngleLimits;GripRotationEffort=N.Size();
 const FQuat Local=FQuat(FVector::UpVector,FMath::DegreesToRadians(GripAngles.Z))*FQuat(FVector::RightVector,FMath::DegreesToRadians(GripAngles.Y))*FQuat(FVector::ForwardVector,FMath::DegreesToRadians(GripAngles.X));
 RelativeHoldRotation=(AnatomicalFrame*Local*AnatomicalFrame.Inverse()*NeutralHoldRotation).GetNormalized();
}

void UAltaiHands::AdjustHoldDistance(float Delta)
{
 if(!IsValid(Held) || ConstrainedGrip || ChargingThrow || ThrowPending)return;
 if(RotatingHeld)RotateHeld(FVector(Delta*1.f,0,0));
 else HoldDistanceOffset=FMath::Clamp(HoldDistanceOffset+Delta,-8.f,12.f);
}
bool UAltaiHands::BeginChargeThrow()
{
 if(!IsValid(Held) || ConstrainedGrip || RotatingHeld || ThrowPending || ChargingThrow)return false;
 if(GrabAge<.2f || PositionError>60.f){Hint=TEXT("Let the hand reach the object before throwing");return false;}
 MovingHand=false;GripReturningToNeutral=true;HandMotionOffset=FVector::ZeroVector;UpdateLookLock();
 ChargingThrow=true;ThrowCharge=0;return true;
}
bool UAltaiHands::ReleaseChargedThrow()
{
 if(!ChargingThrow || !IsValid(Held) || ConstrainedGrip)return false;
 ChargingThrow=false;ThrowPending=true;ThrowElapsed=0;
 ThrowDirection=Character->GetActorForwardVector();
 if(auto* PC=Cast<APlayerController>(Character->GetController()))ThrowDirection=PC->GetControlRotation().Vector();
 return true;
}
void UAltaiHands::CancelManipulation()
{
 PendingGripMode=-1;MovingHand=false;RotatingHeld=false;ChargingThrow=false;ThrowPending=false;
 ThrowCharge=0;ThrowElapsed=0;HandMotionOffset=FVector::ZeroVector;UpdateLookLock();
}
void UAltaiHands::TickManipulation(float Dt)
{
 if(!IsValid(Held) || ConstrainedGrip)return;
 if(GripReturningToNeutral){GripAngles=FMath::VInterpTo(GripAngles,FVector::ZeroVector,Dt,7.f);if(GripAngles.IsNearlyZero(.2f)){GripAngles=FVector::ZeroVector;GripReturningToNeutral=false;}}
 if(!GripReturningToNeutral && PendingGripMode>=0){const bool Mode=PendingGripMode==1;PendingGripMode=-1;SetTwoHandGrip(Mode);}
 UpdateAnatomicalRotation();
 const bool Panel=Character->ActorHasTag(TEXT("AltaiDeveloperPanelOpen"));
 if(Panel){CancelManipulation();return;}
 if(auto* PC=Cast<APlayerController>(Character->GetController())){
  float X=0,Y=0;PC->GetInputMouseDelta(X,Y);
  if(RotatingHeld){
   const float Turn=(X-Y)*.25f;
   if(PC->IsInputKeyDown(EKeys::X))RotateHeld(FVector(Turn,0,0));
   else if(PC->IsInputKeyDown(EKeys::Y))RotateHeld(FVector(0,Turn,0));
   else if(PC->IsInputKeyDown(EKeys::Z))RotateHeld(FVector(0,0,Turn));
   else RotateHeld(FVector(0,-Y*.25f,X*.25f));
  }else if(MovingHand)MoveHeldHand(FVector2D(X,-Y));
 }
 if(ChargingThrow)ThrowCharge=FMath::Min(1.f,ThrowCharge+Dt/(.85f+HeldMass*.025f));
 if(ThrowPending){ThrowElapsed+=Dt;if(ThrowElapsed>.14f+FMath::Min(HeldMass*.004f,.12f)){FinishThrow();return;}}
 ThrowPose=FMath::FInterpTo(ThrowPose,ThrowPending?1.f:(ChargingThrow?-ThrowCharge:0.f),Dt,12.f);
 FVector TargetOffset=HandMotionOffset;
 if(ChargingThrow)TargetOffset+=FVector(TwoHands?-3.f:-10.f,TwoHands?0.f:8.f,TwoHands?2.f:6.f)*ThrowCharge;
 if(ThrowPending)TargetOffset+=FVector(16.f,0,10.f)*FMath::Clamp(ThrowElapsed/.14f,0.f,1.f);
 SmoothedHandOffset=FMath::VInterpTo(SmoothedHandOffset,TargetOffset,Dt,12.f);
 if(ChargingThrow)Hint=FString::Printf(TEXT("RMB: release to throw  %.0f%% | F: cancel / put down"),ThrowCharge*100);
 else if(ThrowPending)Hint=TEXT("Throwing");
 else if(RotatingHeld)Hint=GripRotationLimited?TEXT("Hand movement limit - reverse the mouse to return"):TEXT("R + mouse: wrist / forearm | wheel: roll | release R: keep grip");
 else if(MovingHand)Hint=TEXT("Move mouse: swing hand | release LMB: let fly | RMB: charge throw");
 else Hint=FString::Printf(TEXT("%s / %.2f kg | LMB: move hand | RMB: charge | R: rotate | T: grip | F: release"),TwoHands?TEXT("Two hands"):TEXT("One hand"),HeldMass);
}
void UAltaiHands::FinishThrow()
{
 if(!IsValid(Held) || ConstrainedGrip){CancelManipulation();return;}
 auto* Prop=Held.Get();const float Mass=FMath::Max(.05f,HeldMass);
 // Work in joules -> UE kg*cm^2/s^2. Work/velocity caps prevent tiny props from becoming bullets.
 const float Strength=FMath::Clamp(FMath::Sqrt(BodyMass/100.f),.7f,1.25f)*FMath::Lerp(.45f,1.f,Stamina)*FMath::Lerp(1.f,.75f,Wetness);
 const float Energy=(4.f+(TwoHands?TwoHandThrowEnergy:OneHandThrowEnergy)*ThrowCharge*ThrowCharge)*Strength;
 const float Speed=FMath::Min(1400.f,FMath::Sqrt(2.f*Energy*10000.f/Mass));
 const FVector BodyVelocity=Character->GetVelocity(),Relative=Prop->GetPhysicsLinearVelocity()-BodyVelocity;
 const FVector Outgoing=(Relative+ThrowDirection*Speed).GetClampedToMaxSize(FMath::Min(1600.f,FMath::Max(Speed,float(Relative.Size()))));
 const bool Both=TwoHands;const FVector Direction=ThrowDirection;
 Stamina=FMath::Max(0.f,Stamina-(.025f+Energy*.00035f));
 Release();
 Prop->AddImpulse((Outgoing-Relative)*Mass,NAME_None,false);
 LastThrowSpeed=Outgoing.Size();LastThrowMass=Mass;LastThrowEnergy=Energy;
 FollowThroughRemaining=.25f;FollowThroughDirection=Direction;FollowThroughTwoHands=Both;ThrowPose=1.f;
 Hint=TEXT("F / hold LMB: take another object");
}
