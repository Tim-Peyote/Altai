#include "AltaiSkeletalMesh.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/Actor.h"

void UAltaiSkeletalMesh::FinalizeBoneTransform()
{
 // Physical constraints are also enforced on the live bodies by BodyDynamics.
 // Chaos's interpolation can reintroduce an overshoot from its previous pose;
 // clamp that final interpolated pose before buffers flip and sockets are published.
 if(GetOwner() && GetOwner()->ActorHasTag(TEXT("AltaiBodyUnbalanced")) && GetSkeletalMeshAsset()){
  auto& Pose=GetEditableComponentSpaceTransforms();const auto& Ref=GetSkeletalMeshAsset()->GetRefSkeleton();
  for(const TCHAR* Name:{TEXT("calf_l"),TEXT("calf_r"),TEXT("lowerarm_l"),TEXT("lowerarm_r")}){
   const int32 Joint=Ref.FindBoneIndex(Name);if(Joint<0 || !Pose.IsValidIndex(Joint))continue;
   const int32 Parent=Ref.GetParentIndex(Joint);if(Parent<0)continue;
   const FName EndName(*(FString(FString(Name).StartsWith(TEXT("calf"))?TEXT("foot_"):TEXT("hand_"))+FString(Name).Right(1)));
   const int32 End=Ref.FindBoneIndex(EndName);if(End<0)continue;
   const auto& LocalRef=Ref.GetRefBonePose()[Joint];
   const FVector Upper=LocalRef.GetTranslation().GetSafeNormal();
   const FVector Lower=LocalRef.GetRotation().RotateVector(Ref.GetRefBonePose()[End].GetTranslation()).GetSafeNormal();
   const float RestFlex=FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(float(FVector::DotProduct(Upper,Lower)),-1.f,1.f)));
   const FTransform Before=Pose[Joint];
   const FRotator Delta=(Pose[Parent].GetRotation().Inverse()*Before.GetRotation()*LocalRef.GetRotation().Inverse()).Rotator();
   FTransform Limited=Before;
   Limited.SetRotation((Pose[Parent].GetRotation()*FQuat(FVector::UpVector,FMath::DegreesToRadians(FMath::Clamp(float(Delta.Yaw),2.f-RestFlex,140.f-RestFlex)))*LocalRef.GetRotation()).GetNormalized());
   Limited.SetLocation(Pose[Parent].TransformPosition(LocalRef.GetTranslation()));
   Pose[Joint]=Limited;
   for(int32 I=Joint+1;I<Pose.Num() && I<Ref.GetNum();++I){
    int32 Ancestor=Ref.GetParentIndex(I);while(Ancestor>=0 && Ancestor!=Joint)Ancestor=Ref.GetParentIndex(Ancestor);
    if(Ancestor==Joint){
     Pose[I]=Pose[I].GetRelativeTransform(Before)*Limited;
     // Physics interpolation can also displace an ankle/wrist relative to its parent.
     // Reconstruct joint positions from rigid anatomical segments, not that displaced endpoint.
     const int32 P=Ref.GetParentIndex(I);Pose[I].SetLocation(Pose[P].TransformPosition(Ref.GetRefBonePose()[I].GetTranslation()));
    }
   }
  }
 }
 Super::FinalizeBoneTransform();
}
