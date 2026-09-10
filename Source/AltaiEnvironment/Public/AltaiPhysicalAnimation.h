#pragma once
#include "CoreMinimal.h"
#include "PhysicsEngine/PhysicalAnimationComponent.h"
#include "AltaiPhysicalAnimation.generated.h"

/** Defer native drive updates until the skeletal pose is actually available. */
UCLASS()
class ALTAIENVIRONMENT_API UAltaiPhysicalAnimation : public UPhysicalAnimationComponent
{
 GENERATED_BODY()
public:
 virtual void TickComponent(float Dt,ELevelTick Type,FActorComponentTickFunction* Tick) override;
};
