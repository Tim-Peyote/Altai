#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameFramework/Actor.h"
#include "AltaiWeather.generated.h"

UCLASS(BlueprintType)
class ALTAIENVIRONMENT_API UAltaiWeatherPreset : public UDataAsset
{
 GENERATED_BODY()
public:
 UPROPERTY(EditAnywhere, BlueprintReadOnly) FText Label;
 UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0", ClampMax="1")) float Rain = 0;
 UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0", ClampMax="1")) float Snow = 0;
 UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0", ClampMax="1")) float Mist = .2f;
 UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0", ClampMax="1")) float Cloud = .5f;
 UPROPERTY(EditAnywhere, BlueprintReadOnly) bool Thunder = false;
};

/** Owns weather presentation only; never creates terrain or writes campaign state. */
UCLASS(Blueprintable)
class ALTAIENVIRONMENT_API AAltaiWeatherRig : public AActor
{
 GENERATED_BODY()
public:
 AAltaiWeatherRig();
 virtual void OnConstruction(const FTransform& Transform) override;
 virtual void BeginPlay() override;
 virtual void Tick(float DeltaSeconds) override;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<class UDirectionalLightComponent> Sun;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<class UDirectionalLightComponent> Moon;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<class UDirectionalLightComponent> Lightning;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<class USkyAtmosphereComponent> Atmosphere;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<class USkyLightComponent> Sky;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<class UExponentialHeightFogComponent> Fog;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<class UVolumetricCloudComponent> Clouds;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<class UPostProcessComponent> Grade;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<class UNiagaraComponent> RainFX;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<class UNiagaraComponent> SnowFX;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weather") TArray<TObjectPtr<UAltaiWeatherPreset>> Presets;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weather") int32 PresetIndex = 1;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weather", meta=(ClampMin="0",ClampMax="24")) float Hour = 15;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weather") bool CycleTime = false;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weather", meta=(ClampMin="30")) float DayDurationSeconds = 600;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weather") bool AutomaticWeather = false;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weather", meta=(ClampMin="5")) float WeatherHoldSeconds = 45;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weather", meta=(ClampMin="0.1")) float TransitionSeconds = 5;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weather") int32 RandomSeed = 1809;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weather") TObjectPtr<class UMaterialParameterCollection> SurfaceParameters;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weather") TObjectPtr<class USoundBase> ThunderSound;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weather") bool LightningEnabled = true;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weather") float Rain = 0;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weather") float Snow = 0;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weather") float Mist = .2f;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weather") float Wetness = 0;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weather") float SnowCover = 0;
 UFUNCTION(BlueprintCallable, Category="Weather") void SelectPreset(int32 Index, bool Immediate = false);
 UFUNCTION(BlueprintCallable, Category="Weather") void SetHour(float Value);
 UFUNCTION(BlueprintCallable, Category="Weather") void PreviewWeather();
private:
 UPROPERTY(Transient) TObjectPtr<class UMaterialInstanceDynamic> CloudMaterial;
 void ApplyLighting();
 float Cloud = .5f;
 float WeatherElapsed = 0;
 float LightningCountdown = 12;
 float FlashRemaining = 0;
 FRandomStream Random;
};
