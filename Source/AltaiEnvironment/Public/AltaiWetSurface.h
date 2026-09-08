#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AltaiWetSurface.generated.h"
UCLASS(ClassGroup=(Altai),meta=(BlueprintSpawnableComponent))
class ALTAIENVIRONMENT_API UAltaiWetSurface : public UActorComponent
{
 GENERATED_BODY()
public:
 UAltaiWetSurface();
 virtual void BeginPlay() override;
 virtual void TickComponent(float Dt,ELevelTick T,FActorComponentTickFunction* F) override;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) float Wetness=0;
 UPROPERTY(EditAnywhere,BlueprintReadWrite) float DryGrip=.95f;
 UPROPERTY(EditAnywhere,BlueprintReadWrite) float WetGrip=.65f;
 float Grip() const{return FMath::Lerp(DryGrip,WetGrip,Wetness);}
private:
 TWeakObjectPtr<class AAltaiWeatherRig> Weather;
 TWeakObjectPtr<class UMeshComponent> Mesh;
 UPROPERTY(Transient) TObjectPtr<class UMaterialInstanceDynamic> Material;
};
