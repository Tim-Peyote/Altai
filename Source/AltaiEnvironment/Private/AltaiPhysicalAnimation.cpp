#include "AltaiPhysicalAnimation.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"

void UAltaiPhysicalAnimation::TickComponent(float Dt,ELevelTick Type,FActorComponentTickFunction* Tick)
{
 auto* Mesh=GetSkeletalMesh();if(!Mesh || !Mesh->GetSkeletalMeshAsset())return;
 // This accessor joins pending evaluation. During mesh/physics transitions the
 // buffer may still be empty; native local-space drives assume every bone exists.
 // Keep the pending native update for the next valid pose instead of indexing it.
 if(Mesh->GetBoneSpaceTransformsView().Num()<Mesh->GetSkeletalMeshAsset()->GetRefSkeleton().GetNum())return;
 Super::TickComponent(Dt,Type,Tick);
}
