#include "AnimNode_AltaiContacts.h"
#include "AltaiBodyDynamics.h"
#include "AltaiSwimming.h"
#include "Animation/AnimSequence.h"
#include "Animation/AttributeTypes.h"
#include "AltaiHands.h"
#include "AltaiWallClimbing.h"
#include "AltaiTraversal.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimInstanceProxy.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/PrimitiveComponent.h"
#include "GameFramework/Actor.h"
#include "TwoBoneIK.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
FAnimNode_AltaiContacts::~FAnimNode_AltaiContacts()=default;
FAnimNode_AltaiContacts::FAnimNode_AltaiContacts()
{
 Pelvis.BoneName=TEXT("pelvis");Spine.BoneName=TEXT("spine_01");
 Ends[0].BoneName=TEXT("hand_l");Ends[1].BoneName=TEXT("hand_r");
 Ends[2].BoneName=TEXT("foot_l");Ends[3].BoneName=TEXT("foot_r");
}
void FAnimNode_AltaiContacts::InitializeBoneReferences(const FBoneContainer& B)
{
 for(auto& E:Ends)E.Initialize(B);Pelvis.Initialize(B);Spine.Initialize(B);
 const auto& Ref=B.GetReferenceSkeleton();
 // The climbing solver is built in the rig's reference frame. Joint limits are
 // relative to this frame, never to the previous already-corrected animation.
 ClimbSolver.Reset();SolverBones.Reset();ClimbReferenceLocal.Reset();ClimbBasePose.Reset();
 TArray<FTransform> Reference;
 for(int32 I=0;I<Ref.GetNum();++I){FTransform T=Ref.GetRefBonePose()[I];const int32 Parent=Ref.GetParentIndex(I);if(Parent!=INDEX_NONE)T=T*Reference[Parent];Reference.Add(T);}
 for(int32 I=0;I<B.GetCompactPoseNumBones();++I){
  const FCompactPoseBoneIndex Compact(I);const int32 MeshIndex=B.MakeMeshPoseIndex(Compact).GetInt();
  const auto& T=Reference[MeshIndex];const FName Name=Ref.GetBoneName(MeshIndex);
  ClimbReferenceLocal.Add(Ref.GetRefBonePose()[MeshIndex]);
  SolverBones.Add(ClimbSolver.AddBone(Name,B.GetParentBoneIndex(Compact).GetInt(),T.GetLocation(),T.GetRotation(),Name==Pelvis.BoneName));
 }
 for(int32 I=0;I<4;++I){ClimbEffectors[I]=ClimbSolver.AddEffector(Ends[I].BoneName);
  RecoveryEndUp[I]=Reference[Ref.FindBoneIndex(Ends[I].BoneName)].GetRotation().Inverse().RotateVector(FVector::UpVector);
 }
 PelvisEffector=ClimbSolver.AddEffector(Pelvis.BoneName);
 if(ClimbSolver.Initialize()){
  for(int32 I=0;I<4;++I){
   const int32 End=Ref.FindBoneIndex(Ends[I].BoneName),Joint=Ref.GetParentIndex(End);
   const int32 SolverIndex=ClimbSolver.GetBoneIndex(Ref.GetBoneName(Joint));
   if(auto* Settings=ClimbSolver.GetBoneSettings(SolverIndex)){
    // Derive the signed hinge from the actual left/right reference bones.
    // Knee flexion brings the heel backward; elbow flexion brings the hand forward.
    const FVector Desired=FVector(0,I>=2?-1:1,0);
    float Best=-1;int32 Axis=0;float Sign=1;
    const FVector LocalEnd=Ref.GetRefBonePose()[End].GetLocation();
    for(int32 J=0;J<3;++J){FRotator Angle=FRotator::ZeroRotator;if(J==0)Angle.Roll=5;else if(J==1)Angle.Pitch=5;else Angle.Yaw=5;
     const FVector Delta=Reference[Joint].GetRotation().RotateVector(Angle.Quaternion().RotateVector(LocalEnd)-LocalEnd);
     const float Score=FVector::DotProduct(Delta,Desired);
     if(FMath::Abs(Score)>Best){Best=FMath::Abs(Score);Axis=J;Sign=Score>=0?1.f:-1.f;}
    }
    Settings->X=Settings->Y=Settings->Z=PBIK::ELimitType::Locked;
    const int32 Root=Ref.GetParentIndex(Joint);
    const FVector Upper=(Reference[Joint].GetLocation()-Reference[Root].GetLocation()).GetSafeNormal();
    const FVector Lower=(Reference[End].GetLocation()-Reference[Joint].GetLocation()).GetSafeNormal();
    const float ReferenceFlex=FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(float(FVector::DotProduct(Upper,Lower)),-1.f,1.f)));
    const float A=Sign*(2.f-ReferenceFlex),Z=Sign*(145.f-ReferenceFlex);
    const float Min=FMath::Min(A,Z),Max=FMath::Max(A,Z),Preferred=Sign*(55.f-ReferenceFlex);
    if(Axis==0){Settings->X=PBIK::ELimitType::Limited;Settings->MinX=Min;Settings->MaxX=Max;Settings->PreferredAngles.Roll=Preferred;}
    else if(Axis==1){Settings->Y=PBIK::ELimitType::Limited;Settings->MinY=Min;Settings->MaxY=Max;Settings->PreferredAngles.Pitch=Preferred;}
    else {Settings->Z=PBIK::ELimitType::Limited;Settings->MinZ=Min;Settings->MaxZ=Max;Settings->PreferredAngles.Yaw=Preferred;}
    HingeBones[I]=SolverIndex;HingeAxes[I]=Axis;HingeMin[I]=Min;HingeMax[I]=Max;
    Settings->bUsePreferredAngles=true;
    UE_LOG(LogTemp,Display,TEXT("Altai climbing hinge %s axis=%d sign=%.0f"),*Ref.GetBoneName(Joint).ToString(),Axis,Sign);
    UE_LOG(LogTemp,Display,TEXT("Altai climbing reference %s %s"),*Ref.GetBoneName(Joint).ToString(),*Ref.GetRefBonePose()[Joint].GetRotation().ToString());
   }
  }
  for(int32 I=0;I<ClimbSolver.GetNumBones();++I)if(auto* Settings=ClimbSolver.GetBoneSettings(I)){
   const FString Name=Ref.GetBoneName(B.MakeMeshPoseIndex(FCompactPoseBoneIndex(I)).GetInt()).ToString();
   if(Name==TEXT("pelvis")){Settings->RotationStiffness=.9f;Settings->PositionStiffness=.65f;}
   if(Name.StartsWith(TEXT("thigh_"))){
    Settings->X=Settings->Y=Settings->Z=PBIK::ELimitType::Limited;
    Settings->MinX=-45;Settings->MaxX=45;Settings->MinY=-50;Settings->MaxY=50;
    Settings->MinZ=-120;Settings->MaxZ=25;Settings->bUsePreferredAngles=true;Settings->PreferredAngles.Yaw=-25;
   }
   if(Name.StartsWith(TEXT("spine"))){Settings->RotationStiffness=.6f;Settings->X=Settings->Y=Settings->Z=PBIK::ELimitType::Limited;Settings->MinX=Settings->MinY=Settings->MinZ=-18;Settings->MaxX=Settings->MaxY=Settings->MaxZ=18;}
  }
 }

 for(int I=0;I<2;++I){Fingers[I].Reset();FingerForward[I].Reset();for(const TCHAR* Digit:{TEXT("index"),TEXT("middle"),TEXT("ring"),TEXT("pinky"),TEXT("thumb")}){for(const TCHAR* Segment:{TEXT("metacarpal"),TEXT("01"),TEXT("02"),TEXT("03")}){FBoneReference R;R.BoneName=FName(*FString::Printf(TEXT("%s_%s_%s"),Digit,Segment,I==0?TEXT("l"):TEXT("r")));if(Ref.FindBoneIndex(R.BoneName)==INDEX_NONE)continue;R.Initialize(B);if(R.IsValidToEvaluate(B)){Fingers[I].Add(R);FVector Direction=FingerForward[I].IsEmpty()?FVector::ForwardVector:FingerForward[I].Last();
 const int32 Index=Ref.FindBoneIndex(R.BoneName);for(int32 J=Index+1;J<Ref.GetNum();++J)if(Ref.GetParentIndex(J)==Index){Direction=Ref.GetRefBonePose()[J].GetTranslation().GetSafeNormal();break;}FingerForward[I].Add(Direction);}}}}
 for(int I=0;I<2;++I){
  HandBasis[I]=FQuat::Identity;
  const FString Side=I==0?TEXT("_l"):TEXT("_r");
  const int32 Index=Ref.FindBoneIndex(FName(*(TEXT("index_metacarpal")+Side))),Pinky=Ref.FindBoneIndex(FName(*(TEXT("pinky_metacarpal")+Side)));
  if(Index!=INDEX_NONE && Pinky!=INDEX_NONE){
   const FVector A=Ref.GetRefBonePose()[Index].GetTranslation(),P=Ref.GetRefBonePose()[Pinky].GetTranslation();
   const FVector Forward=(A+P).GetSafeNormal();
   HandBasis[I]=FRotationMatrix::MakeFromXZ(Forward,FVector::CrossProduct(A-P,Forward).GetSafeNormal()*(I==0?1.f:-1.f)).ToQuat();
  }
 }
 for(int32 I=0;I<2;++I){
  FingerFlexAxis[I].Reset();FingerReference[I].Reset();FingerDigits[I].Reset();FingerSegments[I].Reset();
  const int32 Hand=Ref.FindBoneIndex(Ends[I].BoneName);WristReference[I]=Ref.GetRefBonePose()[Hand].GetRotation();
  for(int32 J=0;J<Fingers[I].Num();++J){
   const FName Name=Fingers[I][J].BoneName;const FString S=Name.ToString();const int32 Index=Ref.FindBoneIndex(Name);
   FTransform InHand=Ref.GetRefBonePose()[Index];int32 Parent=Ref.GetParentIndex(Index);
   while(Parent!=Hand && Parent!=INDEX_NONE){InHand=InHand*Ref.GetRefBonePose()[Parent];Parent=Ref.GetParentIndex(Parent);}
   const FVector Palm=InHand.InverseTransformVectorNoScale(HandBasis[I].GetAxisZ());
   FingerFlexAxis[I].Add(FVector::CrossProduct(FingerForward[I][J],Palm).GetSafeNormal());
   FingerReference[I].Add(Ref.GetRefBonePose()[Index]);
   FingerDigits[I].Add(S.StartsWith(TEXT("index"))?0:S.StartsWith(TEXT("middle"))?1:S.StartsWith(TEXT("ring"))?2:S.StartsWith(TEXT("pinky"))?3:4);
   FingerSegments[I].Add(S.Contains(TEXT("metacarpal"))?-1:S.Contains(TEXT("_01"))?0:S.Contains(TEXT("_02"))?1:2);
  }
 }

}
void FAnimNode_AltaiContacts::PreUpdate(const UAnimInstance* Instance)
{
 Holding=false;OrientHands=false;RecoveringBody=false;SimulatingBody=false;StumbleAlpha=0;
 auto* Mesh=Instance->GetSkelMeshComponent();auto* Owner=Mesh?Mesh->GetOwner():nullptr;
 SwimBlend=WadeBlend=DrownProgress=0;
 if(auto* Swim=Owner?Owner->FindComponentByClass<UAltaiSwimming>():nullptr){
  SwimBlend=Swim->SwimAlpha;WadeBlend=Swim->WadeAlpha;SwimPhase=Swim->Phase;SwimFatigue=Swim->Fatigue;SwimTravel=Swim->TravelBlend;TreadSwimMotion=Swim->TreadMotion;
  DrownSwimMotion=Swim->DrownMotion;DrownProgress=Swim->Dead?Swim->DeathProgress:0;
  SwimMotion=Swim->Breaststroke;EasySwimMotion=Swim->EasyStroke;
  const auto* C=Cast<ACharacter>(Owner);const float DesiredPitch=Swim->State==EAltaiSwimState::Diving && C?FMath::Clamp(FRotator::NormalizeAxis(C->GetControlRotation().Pitch),-55.f,55.f):0.f;SwimPitch=FMath::FInterpTo(SwimPitch,DesiredPitch,Owner->GetWorld()->GetDeltaSeconds(),3.f);
 }
 if(auto* Body=Owner?Owner->FindComponentByClass<UAltaiBodyDynamics>():nullptr;Body && Body->OwnsBody()){
  RecoveringBody=Body->State==EAltaiBodyState::GettingUp;SimulatingBody=!RecoveringBody;
  BodyAnimation=Body->RecoveryMotion();BodySnapshot=Body->RecoverySnapshot;BodyRecovery=Body->RecoveryProgress;BodyAcquire=Body->RecoveryAcquire;
  const FTransform Transform=Mesh->GetComponentTransform();
  RecoveryOffset=Transform.InverseTransformVectorNoScale(Body->RecoveryGroundOffset);
  RecoveryTilt=Transform.GetRotation().Inverse()*Body->RecoveryGroundRotation*Transform.GetRotation();
  for(int32 I=0;I<4;++I){RecoveryWeights[I]=0;if(Body->RecoveryContacts.IsValidIndex(I)){
   const auto& Contact=Body->RecoveryContacts[I];RecoveryWeights[I]=Contact.Weight;
   RecoveryGoals[I]=Transform.InverseTransformPosition(Contact.Goal);RecoveryNormals[I]=Transform.InverseTransformVectorNoScale(Contact.Normal);
  }}
  ClimbingPose=ClimbWasActive=MantlePose=WallPose=false;ClimbPoseAlpha=0;ClimbBasePose.Reset();return;
 }
 auto* Hands=Owner?Owner->FindComponentByClass<UAltaiHands>():nullptr;
 if(!Hands || Hands->ContactGoals.Num()!=4){for(float& Weight:Weights)Weight=0;return;}
 // The lab mesh evaluates after physics: sample the solved rigid-body pose here.
 Hands->RefreshGripGoals();
 const FTransform World=Mesh->GetComponentTransform();
 BodyForward=World.InverseTransformVectorNoScale(Owner->GetActorForwardVector());
 Holding=IsValid(Hands->Held);const bool Wall=Owner->ActorHasTag(TEXT("AltaiWallAttached"));
 const bool Mantle=Owner->ActorHasTag(TEXT("AltaiMantling"));
 const bool ActiveClimb=Wall || Mantle;
 if(ActiveClimb && !ClimbWasActive){ClimbEntryElapsed=0;ClimbBasePose.Reset();for(float& Weight:Weights)Weight=0;}
 const bool ClimbTail=ClimbingPose && !Holding && ClimbPoseAlpha>.01f;
 ClimbingPose=ActiveClimb || ClimbTail;MantlePose=Mantle;WallPose=Wall;
 ClimbWasActive=ActiveClimb;
 auto* Climb=Owner->FindComponentByClass<UAltaiWallClimbing>();
 auto* Traverse=Owner->FindComponentByClass<UAltaiTraversal>();
 if(Wall && Climb)for(int32 I=0;I<4;++I)if(Climb->ContactActive[I]){WallPlane=World.InverseTransformPosition(Hands->ContactGoals[I]+Owner->GetActorForwardVector()*(I<2?7.f:16.f));break;}
 const float Dt=Instance->GetWorld()->GetDeltaSeconds();PoseDt=Dt;
 if(ActiveClimb){ClimbEntryElapsed+=Dt;ClimbPoseAlpha=FMath::SmoothStep(0.f,.4f,ClimbEntryElapsed);}else ClimbPoseAlpha=FMath::FInterpTo(ClimbPoseAlpha,0.f,Dt,8.f);
 const auto* Character=Cast<ACharacter>(Owner);GroundBrace=(!Wall && !Mantle && Character && Character->GetCharacterMovement()->IsMovingOnGround())?Hands->CarryPose:0;
 CrouchAlpha=FMath::FInterpTo(CrouchAlpha,Character && Character->bIsCrouched && !Wall && !Mantle?1.f:0.f,Dt,8.f);
 GroundBrace=FMath::Max(GroundBrace,CrouchAlpha);
 if(Hands->ConstrainedGrip && !Hands->FurnitureBodyOffset.IsNearlyZero())GroundBrace=1.f;
 CurlAlpha=FMath::FInterpTo(CurlAlpha,(Holding || ClimbingPose)?1.f:0.f,Dt,8.f);
 float GripSize=0;
 if(Holding && !Hands->ConstrainedGrip){const FVector Extent=Hands->Held->CalcBounds(FTransform(FQuat::Identity,FVector::ZeroVector,Hands->Held->GetComponentScale())).BoxExtent;GripSize=1.f-FMath::Clamp((Extent.GetMin()-3.f)/9.f,0.f,1.f);}
 SmallGrip=FMath::FInterpTo(SmallGrip,GripSize,Dt,8.f);
 FAltaiFingerGrasp Desired;
 for(int32 I=0;I<5;++I){const float S=SmallGrip;const FVector A=I==4?FVector(12,20,12):FMath::Lerp(FVector(18,35,25),FVector(38+I*3,60,35),S);
  if(I==0)Desired.Index=A;else if(I==1)Desired.Middle=A;else if(I==2)Desired.Ring=A;else if(I==3)Desired.Little=A;else Desired.Thumb=A;}
 if(Holding)if(auto* Static=Cast<UStaticMeshComponent>(Hands->Held))if(auto* Asset=Static->GetStaticMesh().Get())if(auto* Profile=Cast<UAltaiGripProfile>(Asset->GetAssetUserDataOfClass(UAltaiGripProfile::StaticClass())))Desired=Hands->TwoHands?Profile->TwoHands:Profile->OneHand;
 if(ClimbingPose){Desired.Index=FVector(12,48,22);Desired.Middle=FVector(15,52,25);Desired.Ring=FVector(18,50,25);Desired.Little=FVector(22,45,25);Desired.Thumb=FVector(8,15,8);Desired.ThumbOpposition=5;}
 FingerPose.Index=FMath::VInterpTo(FingerPose.Index,Desired.Index,Dt,10);FingerPose.Middle=FMath::VInterpTo(FingerPose.Middle,Desired.Middle,Dt,10);FingerPose.Ring=FMath::VInterpTo(FingerPose.Ring,Desired.Ring,Dt,10);FingerPose.Little=FMath::VInterpTo(FingerPose.Little,Desired.Little,Dt,10);FingerPose.Thumb=FMath::VInterpTo(FingerPose.Thumb,Desired.Thumb,Dt,10);FingerPose.ThumbOpposition=FMath::FInterpTo(FingerPose.ThumbOpposition,Desired.ThumbOpposition,Dt,10);
 Hands->WristTrackingError=LastWristError;Hands->ForearmRoll=LastForearmRoll;Hands->SignedForearmRoll=LastSignedRoll;
 OrientHands=Holding || Wall || Mantle || Hands->ContactWeights[0]>.001f || Hands->ContactWeights[1]>.001f;
 FVector Offset=Hands->FurnitureBodyOffset+FVector(0,0,-Hands->CarryPose*3.f-CrouchAlpha*56.f);
 if(Climb && Wall)Offset=Climb->BodyOffset;
 if(Traverse && Mantle){Offset=Traverse->BodyOffset;MantleLip=World.InverseTransformPosition(Traverse->LedgePoint);MantleForward=World.InverseTransformVectorNoScale(Owner->GetActorForwardVector());MantleUp=World.InverseTransformVectorNoScale(FVector::UpVector);}
 BodyTranslation=FMath::VInterpTo(BodyTranslation,World.InverseTransformVectorNoScale(Offset),Dt,ClimbingPose?15.f:5.f);
 // Keep the feet anchored while the pelvis returns after releasing a low handle.
 if(!Wall && !Mantle && Character && Character->GetCharacterMovement()->IsMovingOnGround() && !BodyTranslation.IsNearlyZero(.1f))GroundBrace=1.f;
 LeanAxis=World.InverseTransformVectorNoScale(Owner->GetActorRightVector());
 const float TargetLean=ClimbingPose?FMath::DegreesToRadians(Mantle && Traverse?Traverse->BodyLean:-4.f):FMath::DegreesToRadians(-5.f)*Hands->CarryPose+FMath::DegreesToRadians(12.f)*CrouchAlpha+FMath::DegreesToRadians(-5.f)*Hands->ThrowPose;
 Lean=FMath::FInterpTo(Lean,TargetLean,Dt,ClimbingPose?12.f:5.f);
 TwistAxis=World.InverseTransformVectorNoScale(FVector::UpVector);TorsoTwist=FMath::FInterpTo(TorsoTwist,FMath::DegreesToRadians(Wall && Climb?Climb->BodyTwist:0.f),Dt,6.f);
 for(int32 I=0;I<4;++I){
  if(I<2){float Curl=1;
   if(Wall && Climb){Curl=Climb->ContactActive[I]?1.f:Climb->MovingLimb==I?FMath::SmoothStep(.7f,1.f,Climb->ContactProgress):0.f;}
   if(Mantle && Traverse)Curl=1-.85f*Traverse->PalmDown;
   const float Orient=Wall && Climb && !Climb->ContactActive[I]?(Climb->MovingLimb==I?FMath::SmoothStep(.2f,.8f,Climb->ContactProgress):0.f):1.f;
   HandOrientationAlpha[I]=FMath::FInterpTo(HandOrientationAlpha[I],Orient,Dt,10.f);
   FingerContact[I]=FMath::FInterpTo(FingerContact[I],Curl,Dt,12.f);}
  SupportAlpha[I]=Wall && Climb?(Climb->ContactActive[I]?1.f:0.f):Mantle?1.f:0.f;
  Goals[I]=World.InverseTransformPosition(Hands->ContactGoals[I]);Weights[I]=FMath::Lerp(Weights[I],Hands->ContactWeights[I],1.f-FMath::Exp(-Dt*(ClimbingPose?24.f:10.f)));
  if(I>=2 && GroundBrace>0)Weights[I]=GroundBrace;
  if(ActiveClimb)Weights[I]=FMath::Min(Weights[I],ClimbPoseAlpha);
  if(Mantle && Traverse && Traverse->Descending && I>=2)Weights[I]=1.f;
  if((Holding || Wall || Mantle) && I<2){
   FQuat Frame=Holding?Hands->GripOrientation(I):FRotationMatrix::MakeFromXZ(FVector::VectorPlaneProject(FVector::UpVector,Owner->GetActorForwardVector()).GetSafeNormal(),Owner->GetActorForwardVector()).ToQuat();
   if(Mantle && Traverse){const FQuat Press=FRotationMatrix::MakeFromXZ(Owner->GetActorForwardVector(),-FVector::UpVector).ToQuat();Frame=FQuat::Slerp(Frame,Press,Traverse->PalmDown);}
   const FQuat Target=World.GetRotation().Inverse()*Frame*HandBasis[I].Inverse();
   Orientations[I]=WasOrienting?FQuat::Slerp(Orientations[I],Target,1.f-FMath::Exp(-Dt*12.f)):Target;
  }
  FVector Pole=Owner->GetActorLocation()+Owner->GetActorRightVector()*((I%2)?65:-65)+Owner->GetActorForwardVector()*(I<2?-30:70)+FVector(0,0,I<2?15:-40);
  if(Holding && I<2){
   const FVector Shoulder=Mesh->GetSocketLocation(I==0?TEXT("upperarm_l"):TEXT("upperarm_r"));
   if(Hands->ConstrainedGrip)Pole=Shoulder+Owner->GetActorRightVector()*((I%2)?22.f:-22.f)+FVector(0,0,-55.f)+Owner->GetActorForwardVector()*8.f;
   else {
    // Put the elbow behind the grasp's finger direction. A hook above a handle needs
    // a different elbow plane from an upright bottle; a fixed downward pole cannot do both.
    const FVector GraspDirection=Hands->GripOrientation(I).GetAxisX();
    Pole=Hands->ContactGoals[I]-GraspDirection*38.f+Owner->GetActorRightVector()*((I%2)?12.f:-12.f);
   }
  }
  if(ClimbingPose){
   const FVector Root=Mesh->GetSocketLocation(I==0?TEXT("upperarm_l"):I==1?TEXT("upperarm_r"):I==2?TEXT("thigh_l"):TEXT("thigh_r"));
   Pole=Root+Owner->GetActorRightVector()*((I%2?1.f:-1.f)*(I<2?32.f:36.f))+Owner->GetActorForwardVector()*(I<2?-16.f:40.f)+FVector(0,0,I<2?-28.f:(Mantle?35.f:-20.f));
  }
  if(Mantle && Traverse && I>=2){
   const float OverLip=FMath::SmoothStep(Traverse->LedgePoint.Z-20.f,Traverse->LedgePoint.Z+8.f,Hands->ContactGoals[I].Z);
   const FVector Outside=Hands->ContactGoals[I]-Owner->GetActorForwardVector()*45.f+FVector(0,0,35.f);
   Pole=FMath::Lerp(Outside,Pole,OverLip);
  }
  Poles[I]=WasOrienting?FMath::VInterpTo(Poles[I],World.InverseTransformPosition(Pole),Dt,10.f):World.InverseTransformPosition(Pole);
 }
 if(auto* Body=Owner->FindComponentByClass<UAltaiBodyDynamics>())StumbleAlpha=ActiveClimb?0:Body->Reaction;
 WasOrienting=OrientHands;
}
void FAnimNode_AltaiContacts::EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output,TArray<FBoneTransform>& Out)
{
 if(SimulatingBody)return;
 const auto& Bones=Output.Pose.GetPose().GetBoneContainer();
 if(SwimBlend>.001f && SwimMotion && !RecoveringBody){
  FCompactPose Tread;Tread.SetBoneContainer(&Bones);
  FCompactPose Pose,Easy,Drown;Pose.SetBoneContainer(&Bones);Easy.SetBoneContainer(&Bones);Drown.SetBoneContainer(&Bones);
  FBlendedCurve Curve,EasyCurve;Curve.InitFrom(Bones);EasyCurve.InitFrom(Bones);
  UE::Anim::FStackAttributeContainer Attr,EasyAttr;FAnimationPoseData Data(Pose,Curve,Attr),EasyData(Easy,EasyCurve,EasyAttr);
  SwimMotion->GetAnimationPose(Data,FAnimExtractContext(double(SwimPhase*SwimMotion->GetPlayLength()),false));
  if(EasySwimMotion)EasySwimMotion->GetAnimationPose(EasyData,FAnimExtractContext(double(SwimPhase*EasySwimMotion->GetPlayLength()),false));
  FBlendedCurve TreadCurve;TreadCurve.InitFrom(Bones);UE::Anim::FStackAttributeContainer TreadAttr;FAnimationPoseData TreadData(Tread,TreadCurve,TreadAttr);
  if(TreadSwimMotion)TreadSwimMotion->GetAnimationPose(TreadData,FAnimExtractContext(double(SwimPhase*TreadSwimMotion->GetPlayLength()),false));
  FBlendedCurve DrownCurve;DrownCurve.InitFrom(Bones);UE::Anim::FStackAttributeContainer DrownAttr;FAnimationPoseData DrownData(Drown,DrownCurve,DrownAttr);
  if(DrownProgress>0 && DrownSwimMotion)DrownSwimMotion->GetAnimationPose(DrownData,FAnimExtractContext(double(FMath::Min(.999f,DrownProgress)*DrownSwimMotion->GetPlayLength()),false));
  TArray<FTransform> World;
  for(int32 I=0;I<Bones.GetCompactPoseNumBones();++I){
   const FCompactPoseBoneIndex Index(I),Parent=Bones.GetParentBoneIndex(Index);FTransform Local=Pose[Index];
   if(EasySwimMotion)Local.Blend(Local,Easy[Index],SwimFatigue);
   if(TreadSwimMotion)Local.Blend(Tread[Index],Local,SwimTravel);
   if(DrownProgress>0 && DrownSwimMotion)Local.Blend(Local,Drown[Index],FMath::SmoothStep(0.f,.6f,DrownProgress));
   Local.Blend(Output.Pose.GetLocalSpaceTransform(Index),Local,SwimBlend);
   // A safe clip can still inherit an over-flexed knee from the outgoing
   // locomotion pose. Enforce the same calibrated hinge bounds after blending.
   for(int32 J=0;J<4;++J)if(I==HingeBones[J] && ClimbReferenceLocal.IsValidIndex(I)){
    const FQuat Delta=Local.GetRotation()*ClimbReferenceLocal[I].GetRotation().Inverse();
    const FVector Axis=HingeAxes[J]==0?FVector::ForwardVector:HingeAxes[J]==1?FVector::RightVector:FVector::UpVector;
    FQuat Swing,Twist;Delta.ToSwingTwist(Axis,Swing,Twist);
    const FRotator Angles=Twist.Rotator();
    const float Angle=HingeAxes[J]==0?Angles.Roll:HingeAxes[J]==1?Angles.Pitch:Angles.Yaw;
    const float Limited=FMath::Clamp(Angle,HingeMin[J]+.1f,HingeMax[J]-.1f);
    if(!FMath::IsNearlyEqual(Angle,Limited))Local.SetRotation((Swing*FQuat(Axis,FMath::DegreesToRadians(Limited))*ClimbReferenceLocal[I].GetRotation()).GetNormalized());
   }
   World.Add(Parent==INDEX_NONE?Local:Local*World[Parent.GetInt()]);
  }
  const FVector Pivot=World[Pelvis.GetCompactPoseIndex(Bones).GetInt()].GetLocation();
  const FQuat Pitch(FVector::ForwardVector,FMath::DegreesToRadians(SwimPitch*SwimBlend));
  for(int32 I=0;I<World.Num();++I){auto T=World[I];T.SetLocation(Pivot+Pitch.RotateVector(T.GetLocation()-Pivot));T.SetRotation(Pitch*T.GetRotation());Out.Add(FBoneTransform(FCompactPoseBoneIndex(I),T));}
  return;
 }
 if(WadeBlend>.01f && !Holding && !ClimbingPose && !RecoveringBody){
  TArray<FTransform> World;
  for(int32 I=0;I<Bones.GetCompactPoseNumBones();++I){
   const FCompactPoseBoneIndex Index(I),Parent=Bones.GetParentBoneIndex(Index);FTransform Local=Output.Pose.GetLocalSpaceTransform(Index);
   const FName Name=Bones.GetReferenceSkeleton().GetBoneName(Bones.MakeMeshPoseIndex(Index).GetInt());
   if(Name==TEXT("lowerarm_l") || Name==TEXT("lowerarm_r"))Local.SetRotation(Local.GetRotation()*FQuat(FVector::UpVector,FMath::DegreesToRadians(22*WadeBlend)));
   if(Name==TEXT("spine_01"))Local.SetRotation(Local.GetRotation()*FQuat(FVector::ForwardVector,FMath::DegreesToRadians(-6*WadeBlend)));
   World.Add(Parent==INDEX_NONE?Local:Local*World[Parent.GetInt()]);Out.Add(FBoneTransform(Index,World.Last()));
  }
  return;
 }
 if(RecoveringBody && BodyAnimation){
  FCompactPose Pose;Pose.SetBoneContainer(&Bones);FBlendedCurve Curve;Curve.InitFrom(Bones);
  UE::Anim::FStackAttributeContainer Attributes;FAnimationPoseData Data(Pose,Curve,Attributes);
  const float Length=BodyAnimation->GetPlayLength(),Time=BodyRecovery*(Length+BodyAcquire+.2f);
  BodyAnimation->GetAnimationPose(Data,FAnimExtractContext(FMath::Clamp(Time-BodyAcquire,0.f,Length),false));
  const float Acquire=FMath::SmoothStep(0.f,BodyAcquire,Time),Stand=FMath::SmoothStep(Length+BodyAcquire,Length+BodyAcquire+.2f,Time);
  TArray<FTransform> World;
  for(int32 I=0;I<Bones.GetCompactPoseNumBones();++I){const FCompactPoseBoneIndex Index(I),Parent=Bones.GetParentBoneIndex(Index);FTransform Local=Pose[Index];
   const int32 SnapshotIndex=BodySnapshot.BoneNames.IndexOfByKey(Bones.GetReferenceSkeleton().GetBoneName(Bones.MakeMeshPoseIndex(Index).GetInt()));
   if(BodySnapshot.bIsValid && BodySnapshot.LocalTransforms.IsValidIndex(SnapshotIndex))Local.Blend(BodySnapshot.LocalTransforms[SnapshotIndex],Local,Acquire);
   Local.Blend(Local,Output.Pose.GetLocalSpaceTransform(Index),Stand);
   const FName BoneName=Bones.GetReferenceSkeleton().GetBoneName(Bones.MakeMeshPoseIndex(Index).GetInt());
   if(BoneName==TEXT("hand_l") || BoneName==TEXT("hand_r") || BoneName==TEXT("foot_l") || BoneName==TEXT("foot_r")){
    // A physical snapshot can carry residual wrist/ankle rotation beyond the
    // authored envelope. Bound acquisition too, before terrain contact fitting.
    const FQuat Rest=ClimbReferenceLocal[I].GetRotation();FQuat Delta=Rest.Inverse()*Local.GetRotation();
    const float Angle=Delta.AngularDistance(FQuat::Identity),Limit=FMath::DegreesToRadians(BoneName.ToString().StartsWith(TEXT("hand"))?70.f:65.f);
    Local.SetRotation(Rest*FQuat::Slerp(FQuat::Identity,Delta,FMath::Min(1.f,Limit/FMath::Max(Angle,.0001f))));
   }
   World.Add(Parent==INDEX_NONE?Local:Local*World[Parent.GetInt()]);
  }
  const float TerrainAlpha=Acquire*(1-Stand);
  const FVector Pivot=World[Pelvis.GetCompactPoseIndex(Bones).GetInt()].GetLocation();
  const FQuat Tilt=FQuat::Slerp(FQuat::Identity,RecoveryTilt,TerrainAlpha);
  for(auto& T:World){T.SetLocation(Pivot+RecoveryOffset*TerrainAlpha+Tilt.RotateVector(T.GetLocation()-Pivot));T.SetRotation(Tilt*T.GetRotation());}
  auto SetBone=[&](int32 Index,const FTransform& Transform){
   const FTransform Before=World[Index];World[Index]=Transform;
   for(int32 J=Index+1;J<World.Num();++J){int32 Parent=Bones.GetParentBoneIndex(FCompactPoseBoneIndex(J)).GetInt();
    while(Parent>=0 && Parent!=Index)Parent=Bones.GetParentBoneIndex(FCompactPoseBoneIndex(Parent)).GetInt();
    if(Parent==Index)World[J]=World[J].GetRelativeTransform(Before)*Transform;
   }
  };
  auto LimitTwist=[](FQuat Delta,FVector Axis,float Limit){
   Axis.Normalize();const float Projection=FVector::DotProduct(FVector(Delta.X,Delta.Y,Delta.Z),Axis);
   FQuat Twist(Axis.X*Projection,Axis.Y*Projection,Axis.Z*Projection,Delta.W);
   if(Twist.SizeSquared()<SMALL_NUMBER)return Delta;Twist.Normalize();
   const float Angle=FMath::UnwindRadians(2*FMath::Atan2(FVector::DotProduct(FVector(Twist.X,Twist.Y,Twist.Z),Axis),Twist.W));
   return (Delta*Twist.Inverse()*FQuat(Axis,FMath::Clamp(Angle,-FMath::DegreesToRadians(Limit),FMath::DegreesToRadians(Limit)))).GetNormalized();
  };
  for(int32 I=0;I<4;++I){
   const float Weight=RecoveryWeights[I]*TerrainAlpha;if(Weight<.001f)continue;
   const int32 E=Ends[I].GetCompactPoseIndex(Bones).GetInt(),J=Bones.GetParentBoneIndex(FCompactPoseBoneIndex(E)).GetInt(),A=Bones.GetParentBoneIndex(FCompactPoseBoneIndex(J)).GetInt();
   const int32 Parent=Bones.GetParentBoneIndex(FCompactPoseBoneIndex(A)).GetInt();
   FTransform Upper=World[A],Lower=World[J],End=World[E];
   const FVector UL=ClimbReferenceLocal[J].GetLocation(),LL=ClimbReferenceLocal[E].GetLocation();
   const float L1=UL.Size(),L2=LL.Size();
   FVector Target=FMath::Lerp(End.GetLocation(),RecoveryGoals[I],double(Weight));const FVector Axis=(Target-Upper.GetLocation()).GetSafeNormal();
   const float Min=FMath::Sqrt(L1*L1+L2*L2+2*L1*L2*FMath::Cos(FMath::DegreesToRadians(140.f)));
   const float Max=FMath::Sqrt(L1*L1+L2*L2+2*L1*L2*FMath::Cos(FMath::DegreesToRadians(2.f)));
   const float D=FMath::Clamp(float(FVector::Dist(Target,Upper.GetLocation())),Min,Max);Target=Upper.GetLocation()+Axis*D;
   const FVector Hinge=FVector::VectorPlaneProject(Upper.GetRotation().GetAxisZ(),Axis).GetSafeNormal();if(Hinge.IsNearlyZero())continue;
   const FVector Bend=FVector::CrossProduct(Axis,Hinge).GetSafeNormal();const float Along=(L1*L1-L2*L2+D*D)/(2*D);
   const FVector Joint=Upper.GetLocation()+Axis*Along+Bend*FMath::Sqrt(FMath::Max(0.f,L1*L1-Along*Along));
   FQuat U=FRotationMatrix::MakeFromXZ(Joint-Upper.GetLocation(),Hinge).ToQuat()*FRotationMatrix::MakeFromXZ(UL,FVector::UpVector).ToQuat().Inverse();
   FQuat L=FRotationMatrix::MakeFromXZ(Target-Joint,Hinge).ToQuat()*FRotationMatrix::MakeFromXZ(LL,FVector::UpVector).ToQuat().Inverse();
   const FQuat Reference=ClimbReferenceLocal[A].GetRotation(),ParentQ=World[Parent].GetRotation();
   // Find a nearby anatomical bend plane that reaches the contact exactly.
   // Clamping a single proposed plane can leave a loaded palm floating above it.
   float BestCost=FLT_MAX;FQuat BestU=U,BestL=L;
   for(int32 Sample=-15;Sample<=15;++Sample){
    const FVector Normal=FQuat(Axis,FMath::DegreesToRadians(Sample*3.f)).RotateVector(Hinge);
    const FVector K=Upper.GetLocation()+Axis*Along+FVector::CrossProduct(Axis,Normal).GetSafeNormal()*FMath::Sqrt(FMath::Max(0.f,L1*L1-Along*Along));
    const FQuat CandidateU=FRotationMatrix::MakeFromXZ(K-Upper.GetLocation(),Normal).ToQuat()*FRotationMatrix::MakeFromXZ(UL,FVector::UpVector).ToQuat().Inverse();
    const FQuat CandidateL=FRotationMatrix::MakeFromXZ(Target-K,Normal).ToQuat()*FRotationMatrix::MakeFromXZ(LL,FVector::UpVector).ToQuat().Inverse();
    const FQuat Delta=Reference.Inverse()*ParentQ.Inverse()*CandidateU;
    if(Delta.AngularDistance(LimitTwist(Delta,UL,I<2?100.f:75.f))>.001f || Upper.GetRotation().AngularDistance(CandidateU)>FMath::DegreesToRadians(30.f))continue;
    const float Cost=FMath::Square(Upper.GetRotation().AngularDistance(CandidateU))+.25f*FMath::Square(Lower.GetRotation().AngularDistance(CandidateL));
    if(Cost<BestCost){BestCost=Cost;BestU=CandidateU;BestL=CandidateL;}
   }
   if(BestCost<FLT_MAX){U=BestU;L=BestL;}
   else{
    const FQuat Safe=ParentQ*Reference*LimitTwist(Reference.Inverse()*ParentQ.Inverse()*U,UL,I<2?100.f:75.f);
    const float Change=Upper.GetRotation().AngularDistance(Safe);const FQuat Bounded=FQuat::Slerp(Upper.GetRotation(),Safe,FMath::Min(1.f,FMath::DegreesToRadians(25.f)/FMath::Max(Change,.0001f)));
    L=Bounded*U.Inverse()*L;U=Bounded;
   }
   Upper.SetRotation(U);Lower.SetRotation(L);Lower.SetLocation(Upper.TransformPosition(UL));End.SetLocation(Lower.TransformPosition(LL));
   // Rotate toward the support normal within the wrist/ankle envelope.
   const FVector CurrentNormal=End.GetRotation().RotateVector(I<2?HandBasis[I].GetAxisZ():RecoveryEndUp[I]);
   const FVector DesiredNormal=RecoveryNormals[I]*(I<2?-1.f:1.f);
   const FQuat Desired=FQuat::FindBetweenNormals(CurrentNormal,DesiredNormal)*End.GetRotation();
   const FQuat EndReference=ClimbReferenceLocal[E].GetRotation();
   FQuat Delta=EndReference.Inverse()*L.Inverse()*Desired;
   if(I<2)Delta=LimitTwist(Delta,HandBasis[I].GetAxisX(),15.f);
   const float Angle=Delta.AngularDistance(FQuat::Identity),Limit=FMath::DegreesToRadians(I<2?70.f:50.f);
   Delta=FQuat::Slerp(FQuat::Identity,Delta,FMath::Min(1.f,Limit/FMath::Max(Angle,.0001f)));
   End.SetRotation(FQuat::Slerp(End.GetRotation(),L*EndReference*Delta,Weight));
   // The lower limb changed frame during IK. A partial blend from the old world
   // orientation must also be constrained in this new parent frame.
   const FQuat BlendedDelta=EndReference.Inverse()*L.Inverse()*End.GetRotation();
   const float BlendedAngle=BlendedDelta.AngularDistance(FQuat::Identity);
   End.SetRotation(L*EndReference*FQuat::Slerp(FQuat::Identity,BlendedDelta,FMath::Min(1.f,Limit/FMath::Max(BlendedAngle,.0001f))));
   SetBone(A,Upper);SetBone(J,Lower);SetBone(E,End);
  }
  for(int32 I=0;I<World.Num();++I)Out.Emplace(FCompactPoseBoneIndex(I),World[I]);
  return;
 }
 TMap<int32,FTransform> Poses;LastWristError=LastForearmRoll=0;
 FVector ClimbInputGoals[4]={Goals[0],Goals[1],Goals[2],Goals[3]};
 if(ClimbingPose && ClimbBasePose.Num()!=Bones.GetCompactPoseNumBones()){
  for(auto& Bend:BendHistory)Bend=FVector::ZeroVector;
  ClimbBasePose.Reset();for(int32 I=0;I<Bones.GetCompactPoseNumBones();++I)ClimbBasePose.Add(Output.Pose.GetLocalSpaceTransform(FCompactPoseBoneIndex(I)));
 }else if(!ClimbingPose)ClimbBasePose.Reset();

 // The loaded shoulders follow anchored hands during a mantle. Solve the torso
 // before limb IK, rather than stretching an arm to chase a capsule trajectory.
 FVector SupportedTranslation=BodyTranslation;
 // Propagate a small brace/support adjustment through the hierarchy before solving anchored limbs.
 // Bone lengths are never scaled; the IK solver retains the rig's anatomical reach.
 for(int32 I=0;I<Bones.GetCompactPoseNumBones();++I){
  const FCompactPoseBoneIndex Index(I),Parent=Bones.GetParentBoneIndex(Index);
  FTransform Local=Output.Pose.GetLocalSpaceTransform(Index);
  if(ClimbingPose && ClimbBasePose.IsValidIndex(I))Local.Blend(Local,ClimbBasePose[I],ClimbWasActive?1.f:ClimbPoseAlpha);
  FTransform Pose=Parent==INDEX_NONE?Local:Local*(Poses.Contains(Parent.GetInt())?Poses[Parent.GetInt()]:Output.Pose.GetComponentSpaceTransform(Parent));bool Changed=ClimbingPose || Poses.Contains(Parent.GetInt());
  if(Pelvis.IsValidToEvaluate(Bones) && Index==Pelvis.GetCompactPoseIndex(Bones)){Pose.AddToTranslation(SupportedTranslation);Pose.SetRotation(FQuat(LeanAxis,ClimbingPose?Lean*.2f:0.f)*FQuat(TwistAxis,TorsoTwist)*Pose.GetRotation());Changed=true;}
  const FString BoneName=Bones.GetReferenceSkeleton().GetBoneName(Bones.MakeMeshPoseIndex(Index).GetInt()).ToString();
  if((ClimbingPose && BoneName.StartsWith(TEXT("spine_"))) || (!ClimbingPose && Spine.IsValidToEvaluate(Bones) && Index==Spine.GetCompactPoseIndex(Bones))){Pose.SetRotation(FQuat(LeanAxis,ClimbingPose?Lean*.16f:Lean+FMath::DegreesToRadians(22.f)*StumbleAlpha)*Pose.GetRotation());Changed=true;}
  if(StumbleAlpha>.001f && !ClimbingPose && !Holding && BoneName.StartsWith(TEXT("upperarm_"))){Pose.SetRotation(FQuat(LeanAxis,-FMath::DegreesToRadians(35.f)*StumbleAlpha)*Pose.GetRotation());Changed=true;}
  if(Changed)Poses.Add(I,Pose);
 }
 auto GetPose=[&](FCompactPoseBoneIndex Index){if(const auto* P=Poses.Find(Index.GetInt()))return *P;return Output.Pose.GetComponentSpaceTransform(Index);};
 if(ClimbingPose && ClimbSolver.IsReadyToSimulate() && SolverBones.Num()==Bones.GetCompactPoseNumBones()){
  for(int32 I=0;I<SolverBones.Num();++I)ClimbSolver.SetBoneTransform(SolverBones[I],GetPose(FCompactPoseBoneIndex(I)));
  PBIK::FEffectorSettings BodySettings;BodySettings.PositionAlpha=1;BodySettings.RotationAlpha=1;BodySettings.StrengthAlpha=.7f;BodySettings.PinRotation=1;
  const FTransform Hip=GetPose(Pelvis.GetCompactPoseIndex(Bones));
  ClimbSolver.SetEffectorGoal(PelvisEffector,Hip.GetLocation(),Hip.GetRotation(),BodySettings);
  for(int32 I=0;I<4;++I){
   PBIK::FEffectorSettings Settings;Settings.PositionAlpha=Weights[I];Settings.RotationAlpha=0;Settings.StrengthAlpha=I<2?1.f:.9f;Settings.ChainDepth=3;Settings.PullChainAlpha=.2f;Settings.PinRotation=I<2?0.f:1.f;
   const FTransform Effector=GetPose(Ends[I].GetCompactPoseIndex(Bones));ClimbInputGoals[I]=Effector.GetLocation();
   ClimbSolver.SetEffectorGoal(ClimbEffectors[I],Goals[I],Effector.GetRotation(),Settings);
  }
  FPBIKSolverSettings Settings;Settings.Iterations=60;Settings.SubIterations=10;Settings.bAllowStretch=false;Settings.RootBehavior=EPBIKRootBehavior::Free;Settings.MaxAngle=15;Settings.OverRelaxation=1;
  ClimbSolver.Solve(Settings);
  for(int32 I=0;I<SolverBones.Num();++I){FTransform Pose;ClimbSolver.GetBoneGlobalTransform(SolverBones[I],Pose);Poses.Add(I,Pose);}
  // FBIK constraints are iterative. Project hinge rotations exactly before
  // propagating children, so an unreachable target can NEVER invert a joint.
  const TMap<int32,FTransform> Unconstrained=Poses;
  for(int32 I=0;I<SolverBones.Num();++I){
   const int32 Parent=Bones.GetParentBoneIndex(FCompactPoseBoneIndex(I)).GetInt();
   if(Parent==INDEX_NONE)continue;
   FTransform Local=Unconstrained[I].GetRelativeTransform(Unconstrained[Parent]);
   for(int32 J=0;J<4;++J)if(I==HingeBones[J]){
    const FQuat Delta=Local.GetRotation()*ClimbReferenceLocal[I].GetRotation().Inverse();
    const FVector Axis=HingeAxes[J]==0?FVector::ForwardVector:HingeAxes[J]==1?FVector::RightVector:FVector::UpVector;
    FQuat Swing,Twist;Delta.ToSwingTwist(Axis,Swing,Twist);
    const FRotator Angles=Twist.Rotator();FRotator Limited=FRotator::ZeroRotator;
    if(HingeAxes[J]==0)Limited.Roll=FMath::Clamp(float(Angles.Roll),HingeMin[J],HingeMax[J]);
    else if(HingeAxes[J]==1)Limited.Pitch=FMath::Clamp(float(Angles.Pitch),HingeMin[J],HingeMax[J]);
    else Limited.Yaw=FMath::Clamp(float(Angles.Yaw),HingeMin[J],HingeMax[J]);
    Local.SetRotation((Limited.Quaternion()*ClimbReferenceLocal[I].GetRotation()).GetNormalized());
   }
   const FString Name=Bones.GetReferenceSkeleton().GetBoneName(Bones.MakeMeshPoseIndex(FCompactPoseBoneIndex(I)).GetInt()).ToString();
   if(Name.StartsWith(TEXT("thigh_"))){
    FRotator Angles=(Local.GetRotation()*ClimbReferenceLocal[I].GetRotation().Inverse()).Rotator();
    Angles.Roll=FMath::Clamp(Angles.Roll,-45.,45.);Angles.Pitch=FMath::Clamp(Angles.Pitch,-50.,50.);Angles.Yaw=FMath::Clamp(Angles.Yaw,-120.,25.);
    Local.SetRotation((Angles.Quaternion()*ClimbReferenceLocal[I].GetRotation()).GetNormalized());
   }
   if(I!=Pelvis.GetCompactPoseIndex(Bones).GetInt())Local.SetTranslation(ClimbReferenceLocal[I].GetTranslation());
   Poses.Add(I,Local*Poses[Parent]);
  }

 }
 for(int32 I=0;I<4;++I){
  if(Weights[I]<.001f || !Ends[I].IsValidToEvaluate(Bones))continue;
  const auto End=Ends[I].GetCompactPoseIndex(Bones),Joint=Bones.GetParentBoneIndex(End),Root=Bones.GetParentBoneIndex(Joint);
  if(Joint==INDEX_NONE || Root==INDEX_NONE)continue;
  FTransform A=GetPose(Root),B=GetPose(Joint),C=GetPose(End);
  const FTransform OA=A,OB=B,OC=C;
  FVector Goal=(I>=2 && GroundBrace>0)?Output.Pose.GetComponentSpaceTransform(End).GetLocation():Goals[I];
  if((Holding || ClimbingPose) && I<2){
   const float Length=FVector::Dist(A.GetLocation(),B.GetLocation())+FVector::Dist(B.GetLocation(),C.GetLocation());
   Goal=A.GetLocation()+(Goal-A.GetLocation()).GetClampedToMaxSize(Length*.97f);
  }
  if(ClimbingPose){
   // Refit the two rigid segments to the contact using an anatomical hinge frame.
   // FindBetween rotations for separate bones leave an uncontrolled twist; both
   // segment frames here share ONE hinge normal and positive flexion.
   const FVector UpperLocal=OB.GetRelativeTransform(OA).GetLocation();
   const FVector LowerLocal=OC.GetRelativeTransform(OB).GetLocation();
   const float L1=UpperLocal.Size(),L2=LowerLocal.Size();
   FVector Target=FMath::Lerp(ClimbInputGoals[I],Goals[I],double(Weights[I]));
   const float MinD=FMath::Sqrt(L1*L1+L2*L2+2*L1*L2*FMath::Cos(FMath::DegreesToRadians(145.f)));
   const float MaxD=FMath::Sqrt(L1*L1+L2*L2+2*L1*L2*FMath::Cos(FMath::DegreesToRadians(2.f)));
   const FVector Axis=(Target-A.GetLocation()).GetSafeNormal();
   const float D=FMath::Clamp(float(FVector::Dist(Target,A.GetLocation())),MinD,MaxD);
   Target=A.GetLocation()+Axis*D;
   const float Along=(L1*L1-L2*L2+D*D)/(2*D),Height=FMath::Sqrt(FMath::Max(0.f,L1*L1-Along*Along));
   FVector Preferred=FVector::VectorPlaneProject(B.GetLocation()-A.GetLocation(),Axis).GetSafeNormal();
   if(Preferred.IsNearlyZero())Preferred=FVector::VectorPlaneProject(BodyForward,Axis).GetSafeNormal();
   const FQuat UpperBasis=FRotationMatrix::MakeFromXZ(UpperLocal.GetSafeNormal(),FVector::UpVector).ToQuat();
   const FQuat LowerBasis=FRotationMatrix::MakeFromXZ(LowerLocal.GetSafeNormal(),FVector::UpVector).ToQuat();
   FQuat BestA=A.GetRotation(),BestB=B.GetRotation();FVector BestK=B.GetLocation();float BestCost=FLT_MAX;
   const int32 Samples=I>=2?72:1;
   for(int32 Sample=0;Sample<Samples;++Sample){
    const float Angle=Samples==1?0.f:2*PI*Sample/Samples;
    const FVector Bend=FQuat(Axis,Angle).RotateVector(Preferred);
    const FVector Knee=A.GetLocation()+Axis*Along+Bend*Height;
    const FVector Upper=(Knee-A.GetLocation()).GetSafeNormal(),Lower=(Target-Knee).GetSafeNormal();
    const FVector Hinge=FVector::CrossProduct(Upper,Lower).GetSafeNormal();
    const FQuat QR=(FRotationMatrix::MakeFromXZ(Upper,Hinge).ToQuat()*UpperBasis.Inverse()).GetNormalized();
    const FQuat QJ=(FRotationMatrix::MakeFromXZ(Lower,Hinge).ToQuat()*LowerBasis.Inverse()).GetNormalized();
    float Cost=1.f-FVector::DotProduct(Bend,Preferred);
    const FVector Previous=FVector::VectorPlaneProject(BendHistory[I],Axis).GetSafeNormal();
    if(!Previous.IsNearlyZero())Cost+=2.f*(1.f-FVector::DotProduct(Bend,Previous));
    if(I>=2){
     const FVector Outward=FVector::CrossProduct(TwistAxis,BodyForward)*(I%2?1.f:-1.f);
     const FVector Center=GetPose(Pelvis.GetCompactPoseIndex(Bones)).GetLocation();
     Cost+=1000.f*FMath::Max(0.f,8.f-float(FVector::DotProduct(Knee-Center,Outward)));
     const auto Parent=Bones.GetParentBoneIndex(Root);
     const FQuat Local=GetPose(Parent).GetRotation().Inverse()*QR;
     const FRotator HipDelta=(Local*ClimbReferenceLocal[Root.GetInt()].GetRotation().Inverse()).Rotator();
     Cost+=100.f*(FMath::Abs(HipDelta.Roll-FMath::Clamp(HipDelta.Roll,-45.,45.))+FMath::Abs(HipDelta.Pitch-FMath::Clamp(HipDelta.Pitch,-50.,50.))+FMath::Abs(HipDelta.Yaw-FMath::Clamp(HipDelta.Yaw,-120.,25.)));
     if(WallPose)Cost+=1000.f*FMath::Max(0.f,8.f+float(FVector::DotProduct(Knee-WallPlane,BodyForward)));
     if(MantlePose){
      const float Inside=FVector::DotProduct(Knee-MantleLip,MantleForward)+10.f;
      const float Below=10.f-FVector::DotProduct(Knee-MantleLip,MantleUp);
      if(Inside>0 && Below>0)Cost+=1000.f*FMath::Min(Inside,Below);
     }
    }
    if(Cost<BestCost){BestCost=Cost;BestA=QR;BestB=QJ;BestK=Knee;}
   }
   BendHistory[I]=FVector::VectorPlaneProject(BestK-A.GetLocation(),Axis).GetSafeNormal();
   A.SetRotation(BestA);B.SetRotation(BestB);B.SetLocation(BestK);C.SetLocation(Target);
   // Retain the hip's anatomical envelope even if no valid contact pose exists.
   if(I>=2){const auto Parent=Bones.GetParentBoneIndex(Root);const FQuat ParentQ=GetPose(Parent).GetRotation();
    FRotator Delta=(ParentQ.Inverse()*A.GetRotation()*ClimbReferenceLocal[Root.GetInt()].GetRotation().Inverse()).Rotator();
    Delta.Roll=FMath::Clamp(Delta.Roll,-45.,45.);Delta.Pitch=FMath::Clamp(Delta.Pitch,-50.,50.);Delta.Yaw=FMath::Clamp(Delta.Yaw,-120.,25.);
    const FQuat Limited=ParentQ*Delta.Quaternion()*ClimbReferenceLocal[Root.GetInt()].GetRotation();
    B.SetRotation(Limited*A.GetRotation().Inverse()*B.GetRotation());A.SetRotation(Limited);
   }
  }else AnimationCore::SolveTwoBoneIK(A,B,C,Poles[I],Goal,false,1.,1.);
  if(OrientHands && I<2){
   const FQuat Desired=Holding?Orientations[I]:FQuat::Slerp((B.GetRotation()*WristReference[I]).GetNormalized(),Orientations[I],HandOrientationAlpha[I]);
   // Pronation/supination belongs to the forearm, not a swivelling wrist.
   const FVector LongAxis=(C.GetLocation()-B.GetLocation()).GetSafeNormal();
   FQuat Neutral=(B.GetRotation()*WristReference[I]).GetNormalized();
   FQuat Difference=Desired*Neutral.Inverse();if(Difference.W<0)Difference=Difference*-1;
   FQuat Swing,Twist;Difference.ToSwingTwist(LongAxis,Swing,Twist);
   FVector Axis;float Radians;Twist.ToAxisAndAngle(Axis,Radians);
   float Roll=FMath::Clamp(FMath::UnwindRadians(Radians)*FVector::DotProduct(Axis,LongAxis),-FMath::DegreesToRadians(75.f),FMath::DegreesToRadians(75.f));
   if(ClimbingPose)Roll=FMath::FInterpConstantTo(ClimbForearmRoll[I],Roll,PoseDt,FMath::DegreesToRadians(180.f));ClimbForearmRoll[I]=Roll;
   B.SetRotation(FQuat(LongAxis,Roll)*B.GetRotation());if(I==1)LastSignedRoll=FMath::RadiansToDegrees(Roll);LastForearmRoll=FMath::Max(LastForearmRoll,FMath::Abs(FMath::RadiansToDegrees(Roll)));
   Neutral=(B.GetRotation()*WristReference[I]).GetNormalized();
   Difference=Desired*Neutral.Inverse();if(Difference.W<0)Difference=Difference*-1;
   Difference.ToSwingTwist(LongAxis,Swing,Twist);Swing.ToAxisAndAngle(Axis,Radians);
   const FQuat Limited=FQuat(Axis,FMath::Min(Radians,FMath::DegreesToRadians(65.f)))*Neutral;
   LastWristError=FMath::Max(LastWristError,FMath::RadiansToDegrees(float(Limited.AngularDistance(Desired))));
   C.SetRotation(Limited);
  }
  if(!ClimbingPose){A.Blend(OA,A,Weights[I]);B.Blend(OB,B,Weights[I]);C.Blend(OC,C,Weights[I]);}
  // Blending component-space translations shortens limbs during entry/release.
  // Blend rotations, then reconstruct joints from their unchanged local offsets.
  B.SetLocation(A.TransformPosition(OB.GetRelativeTransform(OA).GetLocation()));
  C.SetLocation(B.TransformPosition(OC.GetRelativeTransform(OB).GetLocation()));
  Poses.Add(Root.GetInt(),A);Poses.Add(Joint.GetInt(),B);Poses.Add(End.GetInt(),C);
  // Preserve all descendants, including twist and finger bones, under the corrected chain.
  for(int32 J=Root.GetInt()+1;J<Bones.GetCompactPoseNumBones();++J){
   const FCompactPoseBoneIndex Child(J),Parent=Bones.GetParentBoneIndex(Child);
   if(Child==Joint || Child==End)continue;
   FCompactPoseBoneIndex Ancestor=Parent;bool Descendant=false;
   while(Ancestor!=INDEX_NONE){if(Ancestor==Root){Descendant=true;break;}Ancestor=Bones.GetParentBoneIndex(Ancestor);}
   if(Descendant)Poses.Add(J,Output.Pose.GetLocalSpaceTransform(Child)*GetPose(Parent));
  }
  if(CurlAlpha>.001f && I<2){
   TMap<int32,FTransform> Solved;Solved.Add(End.GetInt(),C);
   const FVector Palm=C.GetRotation().RotateVector(HandBasis[I].GetAxisZ());
   for(int32 J=0;J<Fingers[I].Num();++J){
    const auto Index=Fingers[I][J].GetCompactPoseIndex(Bones),Parent=Bones.GetParentBoneIndex(Index);
    const FTransform* ParentPose=Solved.Find(Parent.GetInt());if(!ParentPose)continue;
    const int32 Digit=FingerDigits[I][J],Segment=FingerSegments[I][J];
    FTransform Local=FingerReference[I][J];
    if(Segment>=0){
     const float Angle=FMath::Clamp(float(FingerPose.Digit(Digit)[Segment]),0.f,Segment==1?100.f:80.f);
     Local.SetRotation(Local.GetRotation()*FQuat(FingerFlexAxis[I][J],FMath::DegreesToRadians(Angle)));
     if(Digit==4 && Segment==0){
      const FVector OpposeAxis=ParentPose->InverseTransformVectorNoScale(C.GetRotation().RotateVector(HandBasis[I].GetAxisX()));
      Local.SetRotation(FQuat(OpposeAxis,FMath::DegreesToRadians(FingerPose.ThumbOpposition)*(I==0?1.f:-1.f))*Local.GetRotation());
     }
    }
    // Absolute authored pose, blended from locomotion; fixed local flexion axes never flip mid-curl.
    Local.Blend(Output.Pose.GetLocalSpaceTransform(Index),Local,Weights[I]*CurlAlpha*FingerContact[I]);
    const FTransform Pose=Local*(*ParentPose);Solved.Add(Index.GetInt(),Pose);Poses.Add(Index.GetInt(),Pose);
   }
  }
 }
 for(const auto& Entry:Poses)Out.Emplace(FCompactPoseBoneIndex(Entry.Key),Entry.Value);
 Out.Sort(FCompareBoneTransformIndex());
}
