#pragma once
#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/HUD.h"
#include "AltaiLabController.generated.h"

/** Opt-in controller on the sandbox map only. Campaign controller stays independent. */
UCLASS()
class ALTAIENVIRONMENT_API AAltaiLabController : public APlayerController
{
 GENERATED_BODY()
public:
 virtual void BeginPlay() override;
 virtual void SetupInputComponent() override;
 UPROPERTY(BlueprintReadOnly) TObjectPtr<class UAltaiSurfaceResponse> SurfaceResponse;
 UPROPERTY(BlueprintReadOnly) TObjectPtr<class AAltaiWeatherRig> Weather;
 UPROPERTY(BlueprintReadOnly) TObjectPtr<class UAltaiTraversal> Traversal;
 UPROPERTY(BlueprintReadOnly) TObjectPtr<class UAltaiHands> Hands;
 UPROPERTY(BlueprintReadOnly) TObjectPtr<class UAltaiWallClimbing> WallClimbing;
 UPROPERTY(BlueprintReadWrite) bool ShowLabHUD=true;
 UFUNCTION(BlueprintCallable) void CycleBodyMass();
private:
 void PlaceHand();void ReleaseHand();void StartCrouch();void StopCrouch();void NextLimb();void Grab();void Climb();void ReleaseLedge();
 void Preset0();void Preset1();void Preset2();void Preset3();void Preset4();void Preset5();void Preset6();
 void Day();void Night();void AutoWeather();void Clock();void Later();void Earlier();void Reset();void ToggleHUD();void ToggleLightning();
};
UCLASS()
class ALTAIENVIRONMENT_API AAltaiLabHUD : public AHUD
{
 GENERATED_BODY()
public: virtual void DrawHUD() override;
};
