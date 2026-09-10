#include "AltaiLabController.h"
#include "AltaiBodyDynamics.h"
#include "AltaiDeveloperPanel.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Misc/App.h"
#include "HAL/IConsoleManager.h"
#include "AltaiWeather.h"
#include "AltaiSurface.h"
#include "AltaiTraversal.h"
#include "AltaiHands.h"
#include "AltaiWetSurface.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "AltaiWallClimbing.h"
#include "Components/InputComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "InputCoreTypes.h"

AAltaiLabController::AAltaiLabController()
{
 AutoInitializeScreens=false;
 InventoryClass=LoadClass<UAltaiScreen>(nullptr,TEXT("/Game/Altai/UI/WBP_FieldInventory.WBP_FieldInventory_C"));
 PauseClass=LoadClass<UAltaiScreen>(nullptr,TEXT("/Game/Altai/UI/WBP_Pause.WBP_Pause_C"));
 ConfirmExitClass=LoadClass<UAltaiScreen>(nullptr,TEXT("/Game/Altai/UI/WBP_ConfirmExit.WBP_ConfirmExit_C"));
}
const TCHAR* AAltaiLabController::DeveloperShortcut()
{
#if PLATFORM_MAC
 return TEXT("⌘D");
#else
 return TEXT("Ctrl+D");
#endif
}
void AAltaiLabController::ShowScreen(EAltaiScreenKind Kind)
{
 if(DeveloperPanel)return;
 const bool Opening=CurrentScreen==EAltaiScreenKind::None && Kind!=EAltaiScreenKind::None;
 const bool Closing=CurrentScreen!=EAltaiScreenKind::None && Kind==EAltaiScreenKind::None;
 if(Opening){
  PreviouslyPaused=UGameplayStatics::IsGamePaused(this);
  MouseGrab=false;if(Hands)Hands->CancelManipulation();
  if(GetPawn())GetPawn()->Tags.AddUnique(TEXT("AltaiDeveloperPanelOpen"));
  if(WallClimbing){PreviousWallInput=WallClimbing->InputFromPlayer;WallClimbing->InputFromPlayer=false;}
  SetIgnoreMoveInput(true);SetIgnoreLookInput(true);FlushPressedKeys();
 }
 Super::ShowScreen(Kind);
 UGameplayStatics::SetGamePaused(this,Kind!=EAltaiScreenKind::None || PreviouslyPaused);
 if(Closing){
  if(GetPawn())GetPawn()->Tags.Remove(TEXT("AltaiDeveloperPanelOpen"));
  if(WallClimbing)WallClimbing->InputFromPlayer=PreviousWallInput;
  SetIgnoreMoveInput(false);SetIgnoreLookInput(false);FlushPressedKeys();
 }
}

void AAltaiLabController::BeginPlay()
{
 Super::BeginPlay();
#if WITH_EDITOR
 if(GetWorld()->WorldType==EWorldType::PIE)if(auto* V=IConsoleManager::Get().FindConsoleVariable(TEXT("r.Editor.Viewport.OverridePIEScreenPercentage"))){OldPIEScreenOverride=V->GetInt();OldPIEScreenFlags=V->GetFlags();V->Set(0,ECVF_SetByCode);}
#endif
 SetInputMode(FInputModeGameOnly());bShowMouseCursor=false;
 if(TActorIterator<AAltaiWeatherRig> It(GetWorld());It){Weather=*It;}
 for(TActorIterator<AStaticMeshActor> A(GetWorld());A;++A)if(A->ActorHasTag(TEXT("AltaiClimbable")) || A->ActorHasTag(TEXT("AltaiGripVisual")) || A->GetStaticMeshComponent()->IsSimulatingPhysics()){auto* Wet=NewObject<UAltaiWetSurface>(*A,TEXT("LocalWetSurface"));Wet->RegisterComponent();}
 if(auto* C=Cast<ACharacter>(GetPawn()))
 {
  C->GetCharacterMovement()->GetNavAgentPropertiesRef().bCanCrouch=true;
  SurfaceResponse=NewObject<UAltaiSurfaceResponse>(C,TEXT("LabSurfaceResponse"));
  SurfaceResponse->WetFootprint=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Altai/Environment/Materials/M_FootprintMud.M_FootprintMud"));
  SurfaceResponse->SnowFootprint=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Altai/Environment/Materials/M_FootprintSnow.M_FootprintSnow"));
  SurfaceResponse->RippleMaterial=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Altai/Environment/Materials/M_WaterRipple.M_WaterRipple"));
  SurfaceResponse->RegisterComponent();
  Hands=NewObject<UAltaiHands>(C,TEXT("LabHands"));Hands->RegisterComponent();
  WallClimbing=NewObject<UAltaiWallClimbing>(C,TEXT("LabWallClimbing"));WallClimbing->RegisterComponent();
  Traversal=NewObject<UAltaiTraversal>(C,TEXT("LabTraversal"));Traversal->RegisterComponent();
  BodyDynamics=NewObject<UAltaiBodyDynamics>(C,TEXT("LabBodyDynamics"));BodyDynamics->RegisterComponent();
 }
}
void AAltaiLabController::SetupInputComponent()
{
 Super::SetupInputComponent();
 InputComponent->BindKey(EKeys::LeftMouseButton,IE_Pressed,this,&AAltaiLabController::PlaceHand);
 InputComponent->BindKey(EKeys::LeftMouseButton,IE_Released,this,&AAltaiLabController::ReleaseHand);
 InputComponent->BindKey(EKeys::LeftControl,IE_Pressed,this,&AAltaiLabController::StartCrouch);
 InputComponent->BindKey(EKeys::LeftControl,IE_Released,this,&AAltaiLabController::StopCrouch);
 InputComponent->BindKey(EKeys::RightMouseButton,IE_Pressed,this,&AAltaiLabController::NextLimb);
 InputComponent->BindKey(EKeys::RightMouseButton,IE_Released,this,&AAltaiLabController::ReleaseThrow);
 InputComponent->BindKey(EKeys::R,IE_Pressed,this,&AAltaiLabController::StartRotateHeld);
 InputComponent->BindKey(EKeys::R,IE_Released,this,&AAltaiLabController::StopRotateHeld);
 InputComponent->BindKey(EKeys::T,IE_Pressed,this,&AAltaiLabController::ToggleHeldGrip);
 InputComponent->BindKey(EKeys::MouseScrollUp,IE_Pressed,this,&AAltaiLabController::HoldFarther);
 InputComponent->BindKey(EKeys::MouseScrollDown,IE_Pressed,this,&AAltaiLabController::HoldCloser);
 InputComponent->BindKey(FInputChord(EKeys::D,false,true,false,false),IE_Pressed,this,&AAltaiLabController::ToggleDeveloperPanel);
 InputComponent->BindKey(EKeys::F,IE_Pressed,this,&AAltaiLabController::Grab);
 InputComponent->BindKey(EKeys::E,IE_Pressed,this,&AAltaiLabController::Climb);
 InputComponent->BindKey(EKeys::C,IE_Pressed,this,&AAltaiLabController::ReleaseLedge);

}
void AAltaiLabController::CycleBodyMass(){if(auto* C=Cast<ACharacter>(GetPawn())){auto* M=C->GetCharacterMovement();M->Mass=M->Mass>=119?60:M->Mass+20;}}
void AAltaiLabController::PlaceHand(){
 if(WallClimbing && WallClimbing->Attached){WallClimbing->PlaceContact();return;}
 if(!Hands)return;
 if(!Hands->Held && !Hands->TryGrab(false))return;
 MouseGrab=true;if(!Hands->ConstrainedGrip)Hands->BeginHandMotion();
}
void AAltaiLabController::ReleaseHand(){if(Hands && MouseGrab)Hands->Release();MouseGrab=false;}
void AAltaiLabController::StartCrouch(){if(BodyDynamics && BodyDynamics->OwnsBody())return;if((WallClimbing && WallClimbing->Attached)||(Traversal && Traversal->Climbing))return;if(auto* C=Cast<ACharacter>(GetPawn()))C->Crouch();}
void AAltaiLabController::StopCrouch(){if(auto* C=Cast<ACharacter>(GetPawn()))C->UnCrouch();}
void AAltaiLabController::NextLimb(){if(WallClimbing && WallClimbing->Attached){WallClimbing->SelectNextLimb();return;}if(Hands)Hands->BeginChargeThrow();}
void AAltaiLabController::ReleaseThrow(){if(Hands && Hands->ReleaseChargedThrow())MouseGrab=false;}
void AAltaiLabController::StartRotateHeld(){if(Hands)Hands->SetHeldRotationMode(true);}
void AAltaiLabController::StopRotateHeld(){if(Hands)Hands->SetHeldRotationMode(false);}
void AAltaiLabController::ToggleHeldGrip(){if(Hands)Hands->SetTwoHandGrip(!Hands->TwoHands);}
void AAltaiLabController::HoldCloser(){if(Hands)Hands->AdjustHoldDistance(-4.f);}
void AAltaiLabController::HoldFarther(){if(Hands)Hands->AdjustHoldDistance(4.f);}
void AAltaiLabController::Grab(){MouseGrab=false;if(Hands)Hands->ToggleGrab();}
void AAltaiLabController::Climb(){if(auto* C=Cast<ACharacter>(GetPawn());C && C->bIsCrouched){if(Hands)Hands->Hint=TEXT("Stand up before climbing");return;}if(Hands && Hands->Held){Hands->Hint=TEXT("Put the object down before climbing");return;}if(WallClimbing && WallClimbing->Attached){if(Traversal)Traversal->TryClimbFromWall();return;}if(WallClimbing && WallClimbing->AttachWall())return;if(Traversal)Traversal->TryClimb();}
void AAltaiLabController::ReleaseLedge(){if(WallClimbing && WallClimbing->Attached){WallClimbing->ReleaseWall();return;}if(Traversal){if(Traversal->Climbing)Traversal->CancelClimb();else Traversal->TryDescend();}}
#define LAB_PRESET(N) void AAltaiLabController::Preset##N(){if(Weather){Weather->AutomaticWeather=false;Weather->SelectPreset(N);}}
LAB_PRESET(0) LAB_PRESET(1) LAB_PRESET(2) LAB_PRESET(3) LAB_PRESET(4) LAB_PRESET(5) LAB_PRESET(6)
#undef LAB_PRESET
void AAltaiLabController::Day(){if(Weather)Weather->SetHour(14);}
void AAltaiLabController::Night(){if(Weather)Weather->SetHour(23);}
void AAltaiLabController::AutoWeather(){if(Weather)Weather->AutomaticWeather=!Weather->AutomaticWeather;}
void AAltaiLabController::Clock(){if(Weather)Weather->CycleTime=!Weather->CycleTime;}
void AAltaiLabController::Earlier(){if(Weather)Weather->SetHour(Weather->Hour-2);}
void AAltaiLabController::Later(){if(Weather)Weather->SetHour(Weather->Hour+2);}
void AAltaiLabController::Reset(){UGameplayStatics::OpenLevel(this,FName(*UGameplayStatics::GetCurrentLevelName(this,true)));}
void AAltaiLabController::ToggleHUD(){ShowLabHUD=!ShowLabHUD;}
void AAltaiLabController::ToggleLightning(){if(Weather)Weather->LightningEnabled=!Weather->LightningEnabled;}
void AAltaiLabController::Tick(float Dt)
{Super::Tick(Dt);FrameSeconds=FMath::Lerp(FrameSeconds,float(FApp::GetDeltaTime()),.08f);}
void AAltaiLabController::ToggleDeveloperPanel()
{
 if(DeveloperPanel){CloseDeveloperPanel();return;}
 if(CurrentScreen!=EAltaiScreenKind::None)return;
 MouseGrab=false;if(Hands)Hands->CancelManipulation();
 PreviouslyPaused=UGameplayStatics::IsGamePaused(this);
 if(auto* C=Cast<ACharacter>(GetPawn())){C->Tags.AddUnique(TEXT("AltaiDeveloperPanelOpen"));C->GetCharacterMovement()->StopMovementImmediately();if(!Hands || !Hands->Held)C->UnCrouch();}
 if(WallClimbing){PreviousWallInput=WallClimbing->InputFromPlayer;WallClimbing->InputFromPlayer=false;}
 SetIgnoreMoveInput(true);SetIgnoreLookInput(true);FlushPressedKeys();
 DeveloperPanel=CreateWidget<UAltaiDeveloperPanel>(this,UAltaiDeveloperPanel::StaticClass());DeveloperPanel->AddToViewport(100);bShowMouseCursor=true;
 FInputModeUIOnly Mode;Mode.SetWidgetToFocus(DeveloperPanel->TakeWidget());Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);SetInputMode(Mode);DeveloperPanel->SetKeyboardFocus();
}
void AAltaiLabController::CloseDeveloperPanel()
{
 if(!DeveloperPanel)return;
 DeveloperPanel->RemoveFromParent();DeveloperPanel=nullptr;
 if(GetPawn())GetPawn()->Tags.Remove(TEXT("AltaiDeveloperPanelOpen"));
 if(WallClimbing)WallClimbing->InputFromPlayer=PreviousWallInput;
 UGameplayStatics::SetGamePaused(this,PreviouslyPaused);
 SetIgnoreMoveInput(false);SetIgnoreLookInput(false);FlushPressedKeys();bShowMouseCursor=false;SetInputMode(FInputModeGameOnly());
}
void AAltaiLabController::EndPlay(const EEndPlayReason::Type R){CloseDeveloperPanel();
#if WITH_EDITOR
 if(OldPIEScreenOverride>=0)if(auto* V=IConsoleManager::Get().FindConsoleVariable(TEXT("r.Editor.Viewport.OverridePIEScreenPercentage"))){V->Set(OldPIEScreenOverride,ECVF_SetByCode);V->ClearFlags(ECVF_SetByMask);V->SetFlags(EConsoleVariableFlags(OldPIEScreenFlags & ECVF_SetByMask));}
#endif
 Super::EndPlay(R);}
void AAltaiLabController::ResetLab(){CloseDeveloperPanel();Reset();}
void AAltaiLabHUD::DrawHUD()
{
 Super::DrawHUD();auto* PC=Cast<AAltaiLabController>(PlayerOwner);if(!Canvas || !PC || !PC->ShowLabHUD || PC->DeveloperPanel)return;
 const FLinearColor HUDAccent(.75,.67,.46),White(.8,.85,.82);
 DrawRect(HUDAccent,Canvas->ClipX*.5f-1,Canvas->ClipY*.5f-1,2,2);
 DrawText(FString::Printf(TEXT("%s · Панель разработчика    I / Tab · Инвентарь"),AAltaiLabController::DeveloperShortcut()),White,24,Canvas->ClipY-34,GEngine->GetSmallFont(),1.1f);
 FString ActionHint=PC->Hands?PC->Hands->Hint:FString();
 if(PC->Traversal && PC->Traversal->Climbing)ActionHint=PC->Traversal->Hint;
 else if(PC->WallClimbing && PC->WallClimbing->Attached){
  auto* Wall=PC->WallClimbing.Get();const TCHAR* Names[]={TEXT("Left hand"),TEXT("Right hand"),TEXT("Left foot"),TEXT("Right foot")};
  ActionHint=FString::Printf(TEXT("%s: %s | RMB select / LMB reach / Q rest | S down / E top"),Names[FMath::Clamp(Wall->SelectedLimb,0,3)],Wall->MovingLimb==Wall->SelectedLimb?TEXT("reaching"):Wall->ContactActive[Wall->SelectedLimb]?TEXT("support"):TEXT("free"));
 }else if(PC->Traversal && PC->Traversal->CanClimb && (!PC->Hands || !PC->Hands->Held))ActionHint=PC->Traversal->Hint;
 if(!ActionHint.IsEmpty())DrawText(ActionHint,White,Canvas->ClipX*.5f-220,Canvas->ClipY*.5f+40,GEngine->GetSmallFont(),1.1f);
 if(PC->Hands && PC->Hands->ChargingThrow){DrawRect(FLinearColor(.04,.05,.05,.8),Canvas->ClipX*.5f-70,Canvas->ClipY*.5f+60,140,4);DrawRect(HUDAccent,Canvas->ClipX*.5f-70,Canvas->ClipY*.5f+60,140*PC->Hands->ThrowCharge,4);}
 if(PC->ShowDiagnostics){
  DrawRect(FLinearColor(.015,.022,.021,.85),24,24,340,82);
  DrawText(FString::Printf(TEXT("%.0f FPS  |  %.1f ms"),1.f/FMath::Max(PC->FrameSeconds,.001f),PC->FrameSeconds*1000),HUDAccent,36,34,GEngine->GetSmallFont());
  if(PC->Hands)DrawText(FString::Printf(TEXT("Load %.1f kg  |  Stamina %.0f%%  |  Speed %.0f%%"),PC->Hands->HeldMass,PC->Hands->Stamina*100,PC->Hands->CarrySpeedScale*100),White,36,55,GEngine->GetSmallFont());
  if(PC->SurfaceResponse)DrawText(FString::Printf(TEXT("Steps %d  |  Stumbles %d"),PC->SurfaceResponse->StepCount,PC->SurfaceResponse->StumbleCount),White,36,76,GEngine->GetSmallFont());
 }
}
