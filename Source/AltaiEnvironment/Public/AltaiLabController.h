#pragma once
#include "CoreMinimal.h"
#include "AltaiScreens.h"
#include "GameFramework/HUD.h"
#include "AltaiLabController.generated.h"

/** Opt-in controller on the sandbox map only. Campaign controller stays independent. */
UCLASS()
class ALTAIENVIRONMENT_API AAltaiLabController : public AAltaiPlayerController
{
 GENERATED_BODY()
public:
 AAltaiLabController();
 virtual void ShowScreen(EAltaiScreenKind Kind) override;
 static const TCHAR* DeveloperShortcut();
 virtual void BeginPlay() override;
 virtual void SetupInputComponent() override;
 virtual void Tick(float DeltaSeconds) override;
 virtual void EndPlay(const EEndPlayReason::Type Reason) override;
 UFUNCTION(BlueprintCallable) void ToggleDeveloperPanel();
 UFUNCTION(BlueprintCallable) void CloseDeveloperPanel();
 UFUNCTION(BlueprintCallable) void ResetLab();
 UPROPERTY(BlueprintReadOnly) TObjectPtr<class UAltaiDeveloperPanel> DeveloperPanel;
 UPROPERTY(BlueprintReadWrite) bool ShowDiagnostics=false;
 UPROPERTY(BlueprintReadOnly) float FrameSeconds=.016f;
 UPROPERTY(BlueprintReadOnly) TObjectPtr<class UAltaiBodyDynamics> BodyDynamics;
 UPROPERTY(BlueprintReadOnly) TObjectPtr<class UAltaiSurfaceResponse> SurfaceResponse;
 UPROPERTY(BlueprintReadOnly) TObjectPtr<class AAltaiWeatherRig> Weather;
 UPROPERTY(BlueprintReadOnly) TObjectPtr<class UAltaiTraversal> Traversal;
 UPROPERTY(BlueprintReadOnly) TObjectPtr<class UAltaiHands> Hands;
 UPROPERTY(BlueprintReadOnly) TObjectPtr<class UAltaiWallClimbing> WallClimbing;
 UPROPERTY(BlueprintReadWrite) bool ShowLabHUD=true;
 UFUNCTION(BlueprintCallable) void CycleBodyMass();
private:
 int OldPIEScreenOverride=-1;uint32 OldPIEScreenFlags=0;
 bool PreviousWallInput=true,PreviouslyPaused=false;
 bool MouseGrab=false;
 void ReleaseThrow();void StartRotateHeld();void StopRotateHeld();void ToggleHeldGrip();void HoldCloser();void HoldFarther();
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
