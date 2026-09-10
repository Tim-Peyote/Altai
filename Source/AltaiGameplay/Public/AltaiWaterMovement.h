#pragma once
#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AltaiWaterMovement.generated.h"

/** Water-specific mode retaining CharacterMovement acceleration and swept collision. */
UCLASS()
class ALTAIGAMEPLAY_API UAltaiWaterMovement : public UCharacterMovementComponent
{
 GENERATED_BODY()
public:
 bool WaterActive=false,Submerged=false;
 bool PassiveSink=false;
 float WaterHeight=0,WaterSpeed=180,VerticalIntent=0;
 virtual void PhysCustom(float DeltaTime,int32 Iterations) override;
 virtual float GetMaxSpeed() const override;
};
