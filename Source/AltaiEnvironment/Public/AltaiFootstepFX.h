#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AltaiFootstepFX.generated.h"
UCLASS(NotBlueprintable,Transient)
class ALTAIENVIRONMENT_API AAltaiFootstepFX : public AActor
{
 GENERATED_BODY()
public:
 AAltaiFootstepFX();
 void Configure(bool Water,bool Snow,class UMaterialInterface* Ripple);
 virtual void Tick(float Dt) override;
private:
 UPROPERTY() TObjectPtr<class UStaticMeshComponent> Ring;
 UPROPERTY() TObjectPtr<class UInstancedStaticMeshComponent> Flecks;
 float Age=0;
 bool WaterStep=false;
};
