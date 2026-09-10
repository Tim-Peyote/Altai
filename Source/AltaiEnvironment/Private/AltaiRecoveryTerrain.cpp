#include "AltaiBodyDynamics.h"
#include "Animation/AnimSequence.h"
#include "Animation/Skeleton.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "PhysicsEngine/PhysicalAnimationComponent.h"
#include "Engine/World.h"

namespace {
const FName ContactNames[]={TEXT("hand_l"),TEXT("hand_r"),TEXT("foot_l"),TEXT("foot_r")};
}

void UAltaiBodyDynamics::CacheRecoveryPose()
{
 auto* Motion=RecoveryMotion();if(CachedRecovery==Motion && !RecoveryPoseCache.IsEmpty())return;
 CachedRecovery=Motion;RecoveryPoseCache.Reset();if(!Motion)return;
 const auto& Ref=Motion->GetSkeleton()->GetReferenceSkeleton();
 const int32 Frames=FMath::CeilToInt(Motion->GetPlayLength()*30)+1;
 for(int32 Frame=0;Frame<Frames;++Frame){
  auto& Pose=RecoveryPoseCache.AddDefaulted_GetRef();Pose.SetNum(Ref.GetNum());
  for(int32 Bone=0;Bone<Ref.GetNum();++Bone){
   Motion->GetBoneTransform(Pose[Bone],FSkeletonPoseBoneIndex(Bone),FAnimExtractContext(FMath::Min(Frame/30.,double(Motion->GetPlayLength())),false),false);
   const int32 Parent=Ref.GetParentIndex(Bone);if(Parent>=0)Pose[Bone]=Pose[Bone]*Pose[Parent];
  }
 }
}

void UAltaiBodyDynamics::SampleRecoveryPose(float Time,TArray<FTransform>& Pose) const
{
 if(RecoveryPoseCache.IsEmpty())return;
 const float Frame=FMath::Clamp(Time*30.f,0.f,float(RecoveryPoseCache.Num()-1));
 const int32 A=FMath::FloorToInt(Frame),B=FMath::Min(A+1,RecoveryPoseCache.Num()-1);
 Pose.SetNum(RecoveryPoseCache[A].Num());
 for(int32 I=0;I<Pose.Num();++I)Pose[I].Blend(RecoveryPoseCache[A][I],RecoveryPoseCache[B][I],Frame-A);
}

bool UAltaiBodyDynamics::EvaluateRecoveryTerrain(float Time,const FTransform& MeshWorld,FVector& Offset,FQuat& Rotation,TArray<FAltaiRecoveryContact>& Contacts,bool Clearance)
{
 auto Reject=[this,Time](FString Reason){RecoveryTerrainIssue=FString::Printf(TEXT("%.2fs: %s"),Time,*Reason);return false;};
 auto* C=Character.Get();auto* Motion=RecoveryMotion();if(!C || !Motion)return Reject(TEXT("Missing motion"));
 TArray<FTransform> Pose;SampleRecoveryPose(Time,Pose);if(Pose.IsEmpty())return Reject(TEXT("Missing pose"));
 const auto& Ref=Motion->GetSkeleton()->GetReferenceSkeleton();
 const int32 HipIndex=Ref.FindBoneIndex(TEXT("pelvis"));const FVector Hip=MeshWorld.TransformPosition(Pose[HipIndex].GetLocation());
 const float Base=MeshWorld.GetLocation().Z;
 FCollisionQueryParams Query(SCENE_QUERY_STAT(AltaiRecoveryTerrain),false,C);
 auto FloorAt=[&](const FVector& XY,FHitResult& Hit){
  return GetWorld()->LineTraceSingleByChannel(Hit,FVector(XY.X,XY.Y,Base+48),FVector(XY.X,XY.Y,Base-65),ECC_Visibility,Query)
   && Hit.ImpactNormal.Z>=.866f && Hit.GetComponent() && !Hit.GetComponent()->IsSimulatingPhysics();
 };
 FHitResult Ground;if(!FloorAt(Hip,Ground))return Reject(TEXT("No stable ground under pelvis"));
 const float Height=Pose[HipIndex].GetLocation().Z;
 Offset=FVector(0,0,Ground.ImpactPoint.Z+2-Base-2*FMath::SmoothStep(60.f,90.f,Height));
 if(FMath::Abs(Offset.Z)>40)return Reject(TEXT("Ground height outside reach"));
 const float Tilt=1-FMath::SmoothStep(40.f,90.f,Height);
 Rotation=FQuat::Slerp(FQuat::Identity,FQuat::FindBetweenNormals(FVector::UpVector,Ground.ImpactNormal),Tilt);
 auto Position=[&](int32 Bone){return Hip+Offset+Rotation.RotateVector(MeshWorld.TransformPosition(Pose[Bone].GetLocation())-Hip);};
 Contacts.SetNum(4);int32 FootSupports=0;
 for(int32 I=0;I<4;++I){
  auto& Contact=Contacts[I];Contact.Weight=0;
  const int32 End=Ref.FindBoneIndex(ContactNames[I]),Joint=Ref.GetParentIndex(End),Root=Ref.GetParentIndex(Joint);
  const FVector Expected=Position(End),Shoulder=Position(Root);
  const float AuthoredHeight=Pose[End].GetLocation().Z;
  const float Weight=1-FMath::SmoothStep(I<2?5.f:16.f,I<2?10.f:32.f,AuthoredHeight);
  if(Weight<.01f)continue;
  const float Length=FVector::Dist(Pose[Root].GetLocation(),Pose[Joint].GetLocation())+FVector::Dist(Pose[Joint].GetLocation(),Pose[End].GetLocation());
  float Best=FLT_MAX;FHitResult BestHit;
  // Search close to the authored contact, rather than stretching toward a distant surface.
  for(int32 Candidate=0;Candidate<9;++Candidate){
   const float Angle=(Candidate-1)*PI/4,Radius=Candidate? (I<2?14.f:8.f):0.f;
   const FVector Point=Expected+FVector(FMath::Cos(Angle)*Radius,FMath::Sin(Angle)*Radius,0);
   FHitResult Hit;if(!FloorAt(Point,Hit))continue;
   const FVector Goal=Hit.ImpactPoint+Hit.ImpactNormal*(I<2?4.f:8.f);
   if(FMath::Abs(Goal.Z-Expected.Z)>32 || FVector::Dist(FMath::Lerp(Expected,Goal,double(Weight)),Shoulder)>Length-.01f)continue;
   // A point hit on the edge of a pebble is not a usable palm/sole patch.
   bool Patch=true;const float Spread=I<2?2.5f:4.f;
   for(const FVector Direction:{FVector(Spread,0,0),FVector(-Spread,0,0),FVector(0,Spread,0),FVector(0,-Spread,0)}){
    FHitResult Edge;if(!FloorAt(Point+Direction,Edge) || FMath::Abs(Edge.ImpactPoint.Z-Hit.ImpactPoint.Z)>5 || FVector::DotProduct(Edge.ImpactNormal,Hit.ImpactNormal)<.94){Patch=false;break;}
   }
   if(!Patch)continue;
   const float Cost=FVector::DistSquared(Goal,Expected);
   if(Cost<Best){Best=Cost;BestHit=Hit;}
  }
  if(Best==FLT_MAX){if(Weight>.95f)return Reject(TEXT("No reachable contact: ")+ContactNames[I].ToString());continue;}
  Contact.Goal=BestHit.ImpactPoint+BestHit.ImpactNormal*(I<2?4.f:8.f);Contact.Normal=BestHit.ImpactNormal;Contact.Weight=Weight;
  if(I>=2 && Weight>.25f)++FootSupports;
 }
 if(!FootSupports)return Reject(TEXT("No supporting foot"));
 if(Clearance){
  // Validate the trunk as well as the final standing capsule. These spheres stay
  // above the authored floor; support limbs are separately fitted to the surface.
  for(const FName Bone:{FName(TEXT("pelvis")),FName(TEXT("spine_03")),FName(TEXT("head"))}){
   const FVector Point=Position(Ref.FindBoneIndex(Bone));
   if(GetWorld()->OverlapBlockingTestByChannel(Point,FQuat::Identity,ECC_Pawn,FCollisionShape::MakeSphere(8),Query))return Reject(TEXT("Trunk clearance: ")+Bone.ToString());
  }
 }
 RecoveryTerrainIssue.Reset();return true;
}

bool UAltaiBodyDynamics::RecoveryPathClear(const FTransform& MeshWorld)
{
 const float Length=RecoveryMotion()->GetPlayLength();
 for(float Time=0;Time<=Length+.2f;Time+=.2f){
  FVector Offset;FQuat Tilt;TArray<FAltaiRecoveryContact> Contacts;
  if(!EvaluateRecoveryTerrain(FMath::Min(Time,Length),MeshWorld,Offset,Tilt,Contacts,true))return false;
 }
 return true;
}

bool UAltaiBodyDynamics::UpdateRecoveryTerrain(float Time,float Dt)
{
 FVector Offset;FQuat Rotation;TArray<FAltaiRecoveryContact> Contacts;
 if(!EvaluateRecoveryTerrain(Time,Character->GetMesh()->GetComponentTransform(),Offset,Rotation,Contacts,true))return false;
 RecoveryGroundOffset=Dt>0?FMath::VInterpTo(RecoveryGroundOffset,Offset,Dt,12):Offset;
 RecoveryGroundRotation=Dt>0?FQuat::Slerp(RecoveryGroundRotation,Rotation,FMath::Min(Dt*12.f,1.f)):Rotation;
 if(RecoveryContacts.Num()!=4)RecoveryContacts=Contacts;
 RecoverySupportCount=0;
 for(int32 I=0;I<4;++I){
  auto& Old=RecoveryContacts[I];auto& Next=Contacts[I];
  if(Next.Weight>.5f)++RecoverySupportCount;
  if(Dt>0 && Old.Weight>.5f && Next.Weight>.5f && FVector::DistSquaredXY(Old.Goal,Next.Goal)<FMath::Square(I<2?10.f:12.f) && FMath::Abs(Old.Goal.Z-Next.Goal.Z)<2 && FVector::DotProduct(Old.Normal,Next.Normal)>.99f){
   // Keep a planted contact stable while the body moves over it.
   Next.Goal=Old.Goal;
  }
  if(Dt>0 && Old.Weight>.01f && Next.Weight>.01f)Next.Goal=FMath::VInterpTo(Old.Goal,Next.Goal,Dt,16);
  Old=Next;
 }
 return true;
}

void UAltaiBodyDynamics::ResumePhysicalFall()
{
 auto* Mesh=Character->GetMesh();
 Mesh->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);Mesh->SetCollisionProfileName(TEXT("Ragdoll"));Mesh->SetCollisionResponseToChannel(ECC_Pawn,ECR_Ignore);
 Mesh->SetAllBodiesBelowSimulatePhysics(TEXT("pelvis"),true,true);Mesh->SetAllBodiesBelowPhysicsBlendWeight(TEXT("pelvis"),1,false,true);
 ConfigureBodies();Mesh->SetAllPhysicsLinearVelocity(FVector::ZeroVector);
 State=EAltaiBodyState::Falling;Elapsed=QuietTime=RecoveryBlockedTime=RecoveryProgress=0;RecoveryContacts.Reset();RecoverySupportCount=0;++Falls;
 Hint=TEXT("Support lost / returning to physics");
}
