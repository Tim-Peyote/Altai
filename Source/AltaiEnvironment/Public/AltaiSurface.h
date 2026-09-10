#pragma once
#include "CoreMinimal.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Components/ActorComponent.h"
#include "GameFramework/Actor.h"
#include "AltaiSurface.generated.h"

UENUM(BlueprintType)
enum class EAltaiSurface : uint8 { Soil, Stone, Mud, Water, Snow, Wood };

UCLASS(BlueprintType)
class ALTAIENVIRONMENT_API UAltaiPhysicalMaterial : public UPhysicalMaterial
{
 GENERATED_BODY()
public:
 UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Surface") EAltaiSurface Kind=EAltaiSurface::Soil;
 UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Surface", meta=(ClampMin="0.1",ClampMax="1")) float SpeedMultiplier=1;
 UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Surface") TObjectPtr<class UMaterialInterface> Footprint;
};

/** Water/mud depth is measured at feet, not camera or capsule overlap. */
UCLASS(Blueprintable)
class ALTAIENVIRONMENT_API AAltaiSurfaceZone : public AActor
{
 GENERATED_BODY()
public:
 AAltaiSurfaceZone();
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<class UBoxComponent> Bounds;
 UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<UAltaiPhysicalMaterial> Surface;
 UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 Priority=10;
 UPROPERTY(EditAnywhere, BlueprintReadWrite) float SurfaceHeight=0;
 UPROPERTY(EditAnywhere, BlueprintReadWrite) float FullResistanceDepth=40;
 bool ContainsFeet(const FVector& Feet) const;
 float ResistanceAt(const FVector& Feet) const;
};

UCLASS(ClassGroup=(Altai), meta=(BlueprintSpawnableComponent))
class ALTAIENVIRONMENT_API UAltaiSurfaceResponse : public UActorComponent
{
 GENERATED_BODY()
public:
 UAltaiSurfaceResponse();
 virtual void BeginPlay() override;
 virtual void EndPlay(const EEndPlayReason::Type Reason) override;
 virtual void TickComponent(float DeltaTime,ELevelTick TickType,FActorComponentTickFunction* Function) override;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement",meta=(ClampMin="300",ClampMax="2500")) float GroundAcceleration=1200;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement",meta=(ClampMin="300",ClampMax="2500")) float GroundBraking=1000;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Surface") float StepDistance=65;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Surface") int32 MaxFootprints=96;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Surface") float FootprintLifetime=60;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Surface") bool EnableStumble=true;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Surface") TObjectPtr<class UMaterialInterface> WetFootprint;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Surface") TObjectPtr<class UMaterialInterface> SnowFootprint;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Surface") TObjectPtr<class UMaterialInterface> RippleMaterial;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Surface") EAltaiSurface CurrentSurface=EAltaiSurface::Soil;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Surface") float SpeedScale=1;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Surface") int32 StepCount=0;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Surface") int32 StumbleCount=0;
 UFUNCTION(BlueprintCallable) void ClearFootprints();
private:
 void MakeStep(const FVector& Feet, UAltaiPhysicalMaterial* Material, AAltaiSurfaceZone* Zone);
 TWeakObjectPtr<class ACharacter> Character;
 TArray<TWeakObjectPtr<AAltaiSurfaceZone>> Zones;
 TArray<TWeakObjectPtr<class UDecalComponent>> Marks;
 TArray<TWeakObjectPtr<AActor>> Ripples;
 TWeakObjectPtr<class AAltaiWeatherRig> Weather;
 FVector LastPosition=FVector::ZeroVector;
 float BaseSpeed=500,BaseAcceleration=2048,BaseBraking=2048,BaseBrakingFriction=2;
 float BaseGroundFriction=8,BaseBrakeDrag=0;
 bool BaseSeparateBraking=false;
 float DistanceSinceStep=0;
 float StumbleTimer=0;
 float StumbleCooldown=0;
 int32 WetSteps=0;
};
