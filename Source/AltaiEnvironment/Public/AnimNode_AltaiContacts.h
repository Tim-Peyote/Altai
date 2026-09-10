#pragma once
#include "CoreMinimal.h"
#include "AltaiGripProfile.h"
#include "Animation/PoseSnapshot.h"
#include "BoneControllers/AnimNode_SkeletalControlBase.h"
#include "Core/PBIKBody.h"
#include "Core/PBIKSolver.h"
#include "AnimNode_AltaiContacts.generated.h"

USTRUCT(BlueprintInternalUseOnly)
struct ALTAIENVIRONMENT_API FAnimNode_AltaiContacts : public FAnimNode_SkeletalControlBase
{
 GENERATED_BODY()
 FAnimNode_AltaiContacts();
 virtual ~FAnimNode_AltaiContacts() override;
 virtual bool HasPreUpdate() const override{return true;}
 virtual void PreUpdate(const UAnimInstance* Instance) override;
 virtual void InitializeBoneReferences(const FBoneContainer& RequiredBones) override;
 virtual bool IsValidToEvaluate(const USkeleton*,const FBoneContainer&) override{return true;}
 virtual void EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output,TArray<FBoneTransform>& Out) override;
private:
 FPoseSnapshot BodySnapshot;
 FVector RecoveryOffset=FVector::ZeroVector,RecoveryGoals[4],RecoveryNormals[4],RecoveryEndUp[4];
 FQuat RecoveryTilt=FQuat::Identity;
 float RecoveryWeights[4]={0,0,0,0};
 bool RecoveringBody=false,SimulatingBody=false;
 float BodyRecovery=0,BodyAcquire=.45f,StumbleAlpha=0;
 UPROPERTY() TObjectPtr<class UAnimSequence> BodyAnimation;
 FPBIKSolver ClimbSolver;
 TArray<int32> SolverBones;
 TArray<FTransform> ClimbReferenceLocal,ClimbBasePose;
 bool ClimbWasActive=false;
 float ClimbEntryElapsed=0,ClimbPoseAlpha=0,PoseDt=0,ClimbForearmRoll[2]={0,0};
 FVector BendHistory[4]={FVector::ZeroVector,FVector::ZeroVector,FVector::ZeroVector,FVector::ZeroVector};
 int32 HingeBones[4]={-1,-1,-1,-1},HingeAxes[4]={2,2,2,2};
 float HingeMin[4]={0,0,0,0},HingeMax[4]={145,145,145,145};
 int32 ClimbEffectors[4]={-1,-1,-1,-1};
 int32 PelvisEffector=-1;
 FVector BodyForward=FVector::ForwardVector;
 FBoneReference Ends[4],Pelvis,Spine;
 FVector BodyTranslation=FVector::ZeroVector,LeanAxis=FVector::RightVector;
 float TorsoTwist=0;
 FVector TwistAxis=FVector::UpVector;
 float FingerContact[2]={0,0};
 float HandOrientationAlpha[2]={1,1};
 float SupportAlpha[4]={0,0,0,0};
 bool ClimbingPose=false,MantlePose=false,WallPose=false;
 FVector WallPlane=FVector::ZeroVector;
 FVector MantleLip=FVector::ZeroVector,MantleForward=FVector::ForwardVector,MantleUp=FVector::UpVector;
 float Lean=0,CurlAlpha=0,GroundBrace=0,CrouchAlpha=0,SmallGrip=0;
 bool WasOrienting=false;
 TArray<FBoneReference> Fingers[2];
 TArray<FVector> FingerForward[2],FingerFlexAxis[2];
 TArray<FTransform> FingerReference[2];
 TArray<int32> FingerDigits[2],FingerSegments[2];
 FQuat WristReference[2];
 FAltaiFingerGrasp FingerPose;
 float LastWristError=0,LastForearmRoll=0,LastSignedRoll=0;
 FVector Goals[4],Poles[4];
 FQuat Orientations[2],HandBasis[2];
 bool Holding=false,OrientHands=false;
 float Weights[4]={0,0,0,0};
};
