#pragma once
#include "CoreMinimal.h"
#include "Components/SkeletalMeshComponent.h"
#include "AltaiSkeletalMesh.generated.h"

/** Final anatomical projection after Chaos interpolation, before publishing the visible pose. */
UCLASS()
class ALTAIGAMEPLAY_API UAltaiSkeletalMesh : public USkeletalMeshComponent
{
 GENERATED_BODY()
public:
 virtual void FinalizeBoneTransform() override;
};
