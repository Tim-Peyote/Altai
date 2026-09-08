#pragma once
#include "CoreMinimal.h"
#include "BoneControllers/AnimNode_SkeletalControlBase.h"
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
 FBoneReference Ends[4],Pelvis,Spine;
 FVector BodyTranslation=FVector::ZeroVector,LeanAxis=FVector::RightVector;
 float Lean=0,CurlAlpha=0,GroundBrace=0,CrouchAlpha=0;
 bool WasOrienting=false;
 TArray<FBoneReference> Fingers[2];
 TArray<FVector> FingerForward[2];
 FVector Goals[4],Poles[4];
 FQuat Orientations[2],HandBasis[2];
 bool Holding=false,OrientHands=false;
 float Weights[4]={0,0,0,0};
};
