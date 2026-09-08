#include "AltaiLabController.h"
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

void AAltaiLabController::BeginPlay()
{
 Super::BeginPlay();SetInputMode(FInputModeGameOnly());bShowMouseCursor=false;
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
 InputComponent->BindKey(EKeys::B,IE_Pressed,this,&AAltaiLabController::CycleBodyMass);
 InputComponent->BindKey(EKeys::F,IE_Pressed,this,&AAltaiLabController::Grab);
 InputComponent->BindKey(EKeys::E,IE_Pressed,this,&AAltaiLabController::Climb);
 InputComponent->BindKey(EKeys::C,IE_Pressed,this,&AAltaiLabController::ReleaseLedge);
 InputComponent->BindKey(EKeys::One,IE_Pressed,this,&AAltaiLabController::Preset0);
 InputComponent->BindKey(EKeys::Two,IE_Pressed,this,&AAltaiLabController::Preset1);
 InputComponent->BindKey(EKeys::Three,IE_Pressed,this,&AAltaiLabController::Preset2);
 InputComponent->BindKey(EKeys::Four,IE_Pressed,this,&AAltaiLabController::Preset3);
 InputComponent->BindKey(EKeys::Five,IE_Pressed,this,&AAltaiLabController::Preset4);
 InputComponent->BindKey(EKeys::Six,IE_Pressed,this,&AAltaiLabController::Preset5);
 InputComponent->BindKey(EKeys::Seven,IE_Pressed,this,&AAltaiLabController::Preset6);
 InputComponent->BindKey(EKeys::Eight,IE_Pressed,this,&AAltaiLabController::Day);
 InputComponent->BindKey(EKeys::Nine,IE_Pressed,this,&AAltaiLabController::Night);
 InputComponent->BindKey(EKeys::Zero,IE_Pressed,this,&AAltaiLabController::AutoWeather);
 InputComponent->BindKey(EKeys::P,IE_Pressed,this,&AAltaiLabController::Clock);
 InputComponent->BindKey(EKeys::LeftBracket,IE_Pressed,this,&AAltaiLabController::Earlier);
 InputComponent->BindKey(EKeys::RightBracket,IE_Pressed,this,&AAltaiLabController::Later);
 InputComponent->BindKey(EKeys::BackSpace,IE_Pressed,this,&AAltaiLabController::Reset);
 InputComponent->BindKey(EKeys::H,IE_Pressed,this,&AAltaiLabController::ToggleHUD);
 InputComponent->BindKey(EKeys::L,IE_Pressed,this,&AAltaiLabController::ToggleLightning);
}
void AAltaiLabController::CycleBodyMass(){if(auto* C=Cast<ACharacter>(GetPawn())){auto* M=C->GetCharacterMovement();M->Mass=M->Mass>=119?60:M->Mass+20;}}
void AAltaiLabController::PlaceHand(){if(WallClimbing && WallClimbing->Attached){WallClimbing->PlaceContact();return;}if(Hands)Hands->TryGrab(true);}
void AAltaiLabController::ReleaseHand(){if(Hands && Hands->ConstrainedGrip)Hands->Release();}
void AAltaiLabController::StartCrouch(){if((WallClimbing && WallClimbing->Attached)||(Traversal && Traversal->Climbing))return;if(auto* C=Cast<ACharacter>(GetPawn()))C->Crouch();}
void AAltaiLabController::StopCrouch(){if(auto* C=Cast<ACharacter>(GetPawn()))C->UnCrouch();}
void AAltaiLabController::NextLimb(){if(WallClimbing)WallClimbing->SelectNextLimb();}
void AAltaiLabController::Grab(){if(Hands)Hands->ToggleGrab();}
void AAltaiLabController::Climb(){if(auto* C=Cast<ACharacter>(GetPawn());C && C->bIsCrouched){if(Hands)Hands->Hint=TEXT("Stand up before climbing");return;}if(Hands && Hands->Held){Hands->Hint=TEXT("Put the object down before climbing");return;}if(WallClimbing && WallClimbing->Attached){if(Traversal)Traversal->TryClimbFromWall();return;}if(WallClimbing && WallClimbing->AttachWall())return;if(Traversal)Traversal->TryClimb();}
void AAltaiLabController::ReleaseLedge(){if(WallClimbing)WallClimbing->ReleaseWall();if(Traversal)Traversal->CancelClimb();}
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
void AAltaiLabHUD::DrawHUD()
{
 Super::DrawHUD();auto* PC=Cast<AAltaiLabController>(PlayerOwner);if(!Canvas || !PC || !PC->ShowLabHUD)return;
 const float Scale=FMath::Clamp(Canvas->ClipX/1600.f,.75f,2.f);
 DrawRect(FLinearColor(.015,.022,.021,.8),24,24,650*Scale,260*Scale);
 const FLinearColor Gold(.75,.67,.46),White(.8,.85,.82);
 DrawRect(Gold,Canvas->ClipX*.5f-1,Canvas->ClipY*.5f-1,2,2);
 auto Text=[&](const FString& S,float Y,FLinearColor Color){DrawText(S,Color,40,28+Y*Scale,GEngine->GetSmallFont(),Scale*1.15f);};
 Text(TEXT("ALTAI  /  ENVIRONMENT LAB"),8,Gold);
 auto* W=PC->Weather.Get();
 if(W)
 {
  FString Label=W->Presets.IsValidIndex(W->PresetIndex)&&W->Presets[W->PresetIndex]?W->Presets[W->PresetIndex]->Label.ToString():TEXT("Custom");
  Text(FString::Printf(TEXT("%s  |  %02d:%02d  |  Auto weather %s / clock %s"),*Label,int(W->Hour),int(FMath::Frac(W->Hour)*60),W->AutomaticWeather?TEXT("ON"):TEXT("OFF"),W->CycleTime?TEXT("ON"):TEXT("OFF")),32,White);
 }
 if(PC->SurfaceResponse)
 {
  auto* S=PC->SurfaceResponse.Get();
  Text(FString::Printf(TEXT("Surface: %s   Speed: %.0f%%   Steps: %d   Stumbles: %d"),*StaticEnum<EAltaiSurface>()->GetNameStringByValue(int64(S->CurrentSurface)),S->SpeedScale*100,S->StepCount,S->StumbleCount),54,White);
 }
 Text(TEXT("1 Clear  2 Overcast  3 Rain  4 Fog  5 Storm  6 Snow  7 Sleet"),78,White);
 Text(TEXT("8 Day  9 Night  0 Auto weather  P Clock  [ / ] Time"),100,White);
 Text(TEXT("H Hide panel  L Lightning on/off  Delete/Backspace Reset"),122,White);
 Text(TEXT("V View  Ctrl Crouch  E Climb / top   C Release   Q Free limb   Space Jump off"),144,Gold);
 if(PC->WallClimbing && PC->WallClimbing->Attached)Text(FString::Printf(TEXT("Wall: grip %.0f%% wet %.0f%% stamina %.0f%% limb %d | RMB/LMB"),PC->WallClimbing->Grip*100,PC->WallClimbing->Wetness*100,PC->WallClimbing->Stamina*100,PC->WallClimbing->SelectedLimb+1),166,Gold);
 else if(PC->Traversal)Text(PC->Traversal->Hint,166,White);
 if(PC->WallClimbing && PC->WallClimbing->Attached){auto* Climb=PC->WallClimbing.Get();
  Text(FString::Printf(TEXT("Supports LH %s  RH %s  LF %s  RF %s | %s"),Climb->ContactActive[0]?TEXT("ON"):TEXT("--"),Climb->ContactActive[1]?TEXT("ON"):TEXT("--"),Climb->ContactActive[2]?TEXT("ON"):TEXT("--"),Climb->ContactActive[3]?TEXT("ON"):TEXT("--"),Climb->MovingLimb!=INDEX_NONE?TEXT("Reaching"):TEXT("RMB select / LMB place")),232,White);
 }
 if(PC->Hands)Text(FString::Printf(TEXT("F Take/release | %.1f kg | Stamina %.0f%% | %s"),PC->Hands->HeldMass,PC->Hands->Stamina*100,*PC->Hands->Hint),188,Gold);
 if(PC->Hands)Text(FString::Printf(TEXT("B Body %.0f kg | Load speed %.0f%% | Balance demand %.0f%%"),PC->Hands->BodyMass,PC->Hands->CarrySpeedScale*100,PC->Hands->BalanceDemand*100),210,White);
}
