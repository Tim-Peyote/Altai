#include "AnimNode_AltaiContacts.h"
#include "AltaiHands.h"
#include "AltaiWallClimbing.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimInstanceProxy.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/PrimitiveComponent.h"
#include "GameFramework/Actor.h"
#include "TwoBoneIK.h"
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
}
void FAnimNode_AltaiContacts::PreUpdate(const UAnimInstance* Instance)
{
 Holding=false;OrientHands=false;
 auto* Mesh=Instance->GetSkelMeshComponent();auto* Owner=Mesh?Mesh->GetOwner():nullptr;
 auto* Hands=Owner?Owner->FindComponentByClass<UAltaiHands>():nullptr;
 if(!Hands || Hands->ContactGoals.Num()!=4){for(float& Weight:Weights)Weight=0;return;}
 // The lab mesh evaluates after physics: sample the solved rigid-body pose here.
 Hands->RefreshGripGoals();
 const FTransform World=Mesh->GetComponentTransform();
 Holding=IsValid(Hands->Held);const bool Wall=Owner->ActorHasTag(TEXT("AltaiWallAttached"));
 const bool Mantle=Owner->ActorHasTag(TEXT("AltaiMantling"));
 const float Dt=Instance->GetWorld()->GetDeltaSeconds();
 const auto* Character=Cast<ACharacter>(Owner);GroundBrace=(!Wall && !Mantle && Character && Character->GetCharacterMovement()->IsMovingOnGround())?Hands->CarryPose:0;
 CrouchAlpha=FMath::FInterpTo(CrouchAlpha,Character && Character->bIsCrouched && !Wall && !Mantle?1.f:0.f,Dt,8.f);
 GroundBrace=FMath::Max(GroundBrace,CrouchAlpha);
 CurlAlpha=FMath::FInterpTo(CurlAlpha,Holding?1.f:0.f,Dt,8.f);
 OrientHands=Holding || Wall || Mantle || Hands->ContactWeights[0]>.001f || Hands->ContactWeights[1]>.001f;
 FVector Offset(0,0,-Hands->CarryPose*3.f-CrouchAlpha*56.f);
 if(auto* Climb=Owner->FindComponentByClass<UAltaiWallClimbing>();Climb && Wall)Offset=Climb->BodyOffset;
 BodyTranslation=FMath::VInterpTo(BodyTranslation,World.InverseTransformVectorNoScale(Offset),Dt,5.f);
 LeanAxis=World.InverseTransformVectorNoScale(Owner->GetActorRightVector());
 Lean=FMath::FInterpTo(Lean,FMath::DegreesToRadians(-5.f)*Hands->CarryPose+FMath::DegreesToRadians(12.f)*CrouchAlpha,Dt,5.f);
 for(int32 I=0;I<4;++I){
  Goals[I]=World.InverseTransformPosition(Hands->ContactGoals[I]);Weights[I]=FMath::Lerp(Weights[I],Hands->ContactWeights[I],1.f-FMath::Exp(-Dt*10.f));
  if(I>=2 && GroundBrace>0)Weights[I]=GroundBrace;
  if((Holding || Wall || Mantle) && I<2){
   FVector Palm=Holding?(Hands->Held->GetCenterOfMass()-Hands->ContactGoals[I]).GetSafeNormal():Owner->GetActorForwardVector();
   FVector FingerDirection=FVector::VectorPlaneProject(FVector::UpVector,Palm).GetSafeNormal();
   const FName Socket=I==0?Hands->LeftGripSocket:Hands->RightGripSocket;
   if(Holding && !Socket.IsNone()){const FTransform Grip=Hands->Held->GetSocketTransform(Socket);Palm=Grip.GetUnitAxis(EAxis::Z);FingerDirection=Grip.GetUnitAxis(EAxis::X);}
   const FQuat Target=World.GetRotation().Inverse()*FRotationMatrix::MakeFromXZ(FingerDirection,Palm).ToQuat()*HandBasis[I].Inverse();
   Orientations[I]=WasOrienting?FQuat::Slerp(Orientations[I],Target,1.f-FMath::Exp(-Dt*12.f)):Target;
  }
  const FVector Pole=Owner->GetActorLocation()+Owner->GetActorRightVector()*((I%2)?65:-65)+Owner->GetActorForwardVector()*(I<2?-30:70)+FVector(0,0,I<2?15:-40);
  Poles[I]=World.InverseTransformPosition(Pole);
 }
 WasOrienting=OrientHands;
}
void FAnimNode_AltaiContacts::EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output,TArray<FBoneTransform>& Out)
{
 const auto& Bones=Output.Pose.GetPose().GetBoneContainer();
 TMap<int32,FTransform> Poses;
 // Propagate a small brace/support adjustment through the hierarchy before solving anchored limbs.
 // Bone lengths are never scaled; the IK solver retains the rig's anatomical reach.
 for(int32 I=0;I<Bones.GetCompactPoseNumBones();++I){
  const FCompactPoseBoneIndex Index(I),Parent=Bones.GetParentBoneIndex(Index);
  FTransform Pose=Output.Pose.GetComponentSpaceTransform(Index);bool Changed=false;
  if(const auto* ParentPose=Poses.Find(Parent.GetInt())){Pose=Output.Pose.GetLocalSpaceTransform(Index)*(*ParentPose);Changed=true;}
  if(Pelvis.IsValidToEvaluate(Bones) && Index==Pelvis.GetCompactPoseIndex(Bones)){Pose.AddToTranslation(BodyTranslation);Changed=true;}
  if(Spine.IsValidToEvaluate(Bones) && Index==Spine.GetCompactPoseIndex(Bones)){Pose.SetRotation(FQuat(LeanAxis,Lean)*Pose.GetRotation());Changed=true;}
  if(Changed)Poses.Add(I,Pose);
 }
 auto GetPose=[&](FCompactPoseBoneIndex Index){if(const auto* P=Poses.Find(Index.GetInt()))return *P;return Output.Pose.GetComponentSpaceTransform(Index);};
 for(int32 I=0;I<4;++I){
  if(Weights[I]<.001f || !Ends[I].IsValidToEvaluate(Bones))continue;
  const auto End=Ends[I].GetCompactPoseIndex(Bones),Joint=Bones.GetParentBoneIndex(End),Root=Bones.GetParentBoneIndex(Joint);
  if(Joint==INDEX_NONE || Root==INDEX_NONE)continue;
  FTransform A=GetPose(Root),B=GetPose(Joint),C=GetPose(End);
  const FTransform OA=A,OB=B,OC=C;
  const FVector Goal=(I>=2 && GroundBrace>0)?Output.Pose.GetComponentSpaceTransform(End).GetLocation():Goals[I];
  AnimationCore::SolveTwoBoneIK(A,B,C,Poles[I],Goal,false,1.,1.);
  if(OrientHands && I<2)C.SetRotation(Orientations[I]);
  A.Blend(OA,A,Weights[I]);B.Blend(OB,B,Weights[I]);C.Blend(OC,C,Weights[I]);
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
   int32 FingerNumber=0;
   for(const auto& Finger:Fingers[I]){
    const auto Index=Finger.GetCompactPoseIndex(Bones),Parent=Bones.GetParentBoneIndex(Index);
    const FTransform* ParentPose=Solved.Find(Parent.GetInt());if(!ParentPose)continue;
    FTransform Pose=Output.Pose.GetLocalSpaceTransform(Index)*(*ParentPose);
    const FString Name=Finger.BoneName.ToString();
    if(!Name.Contains(TEXT("metacarpal"))){
     const FVector Direction=Pose.TransformVectorNoScale(FingerForward[I][FingerNumber]);
     const FVector Axis=FVector::CrossProduct(Direction,Palm).GetSafeNormal();
     const float Angle=Name.Contains(TEXT("_01"))?18.f:35.f;
     Pose.SetRotation(FQuat(Axis,FMath::DegreesToRadians(Angle)*Weights[I]*CurlAlpha)*Pose.GetRotation());
    }
    Solved.Add(Index.GetInt(),Pose);Poses.Add(Index.GetInt(),Pose);++FingerNumber;
   }
  }
 }
 for(const auto& Entry:Poses)Out.Emplace(FCompactPoseBoneIndex(Entry.Key),Entry.Value);
 Out.Sort(FCompareBoneTransformIndex());
}
