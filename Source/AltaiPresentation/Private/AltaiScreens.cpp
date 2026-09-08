#include "AltaiScreens.h"
#include "AltaiInventoryScreen.h"
#include "AltaiSession.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/ScrollBox.h"
#include "Components/EditableTextBox.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "GameFramework/GameUserSettings.h"
#include "GameFramework/Pawn.h"
#include "InputCoreTypes.h"
#include "Engine/Engine.h"

void UAltaiItemCell::NativeOnInitialized()
{ Super::NativeOnInitialized(); SelectButton->OnClicked.AddDynamic(this,&UAltaiItemCell::Select); }
void UAltaiItemCell::Select() { if(OwnerScreen) OwnerScreen->SelectItem(ItemIndex); }
void UAltaiScreen::NativeOnInitialized()
{
 Super::NativeOnInitialized();
 if(LoadButton) LoadButton->OnClicked.AddDynamic(this,&UAltaiScreen::OpenLoad);
 if(PrimaryButton) PrimaryButton->OnClicked.AddDynamic(this,&UAltaiScreen::Primary);
 if(SecondaryButton) SecondaryButton->OnClicked.AddDynamic(this,&UAltaiScreen::Secondary);
 if(SettingsButton) SettingsButton->OnClicked.AddDynamic(this,&UAltaiScreen::Settings);
 if(QuitButton) QuitButton->OnClicked.AddDynamic(this,&UAltaiScreen::Quit);
 if(BackButton) BackButton->OnClicked.AddDynamic(this,&UAltaiScreen::Back);
}
void UAltaiScreen::NativeConstruct() { Super::NativeConstruct(); Refresh(); }
void UAltaiScreen::Refresh()
{
 UAltaiSession* S=UAltaiSession::Find(this);
 if(Kind==EAltaiScreenKind::MainMenu && PrimaryButton) PrimaryButton->SetIsEnabled(S && S->HasSave());
 if(Kind==EAltaiScreenKind::MainMenu && LoadButton)LoadButton->SetIsEnabled(S && S->HasSave());
 if(Kind==EAltaiScreenKind::Pause && StatusText && S)StatusText->SetText(FText::FromString(S->GetActiveProfileName()));
 if(Kind==EAltaiScreenKind::LoadGame && ProfileList && ProfileRowClass && S)
 {
  ProfileList->ClearChildren();const auto Profiles=S->GetProfiles();
  for(const auto& Info:Profiles)
  {
   auto* Row=CreateWidget<UAltaiProfileRow>(GetOwningPlayer(),ProfileRowClass);
   Row->OwnerScreen=this;Row->ProfileId=Info.Id;
   Row->ProfileName->SetText(FText::FromString(Info.Name));
   const int32 Minutes=static_cast<int32>(Info.PlaySeconds/60);
   const FString Date=FText::AsDateTime(Info.SavedAt,EDateTimeStyle::Short,EDateTimeStyle::Short).ToString();
   Row->ProfileDetails->SetText(FText::FromString(FString::Printf(TEXT("Мир • %d ч %02d мин • %s%s"),Minutes/60,Minutes%60,*Date,Info.Recovered?TEXT(" • резервная копия"):TEXT(""))));
   ProfileList->AddChild(Row);
  }
  if(StatusText)StatusText->SetText(FText::FromString(Profiles.IsEmpty()?TEXT("Сохранённых прохождений пока нет."):TEXT("Выберите прохождение. Остальные не изменятся.")));
 }
 if(Kind==EAltaiScreenKind::Inventory && ItemGrid && ItemCellClass && S)
 {
  ItemGrid->ClearChildren();const auto& Items=S->GetItems();
  for(int32 I=0;I<Items.Num();++I)
  {
   auto* Cell=CreateWidget<UAltaiItemCell>(GetOwningPlayer(),ItemCellClass);
   Cell->OwnerScreen=this;Cell->ItemIndex=I;
   Cell->ItemName->SetText(Items.IsValidIndex(I)?Items[I].Name:FText::GetEmpty());
   Cell->ItemQuantity->SetText(Items.IsValidIndex(I)?FText::AsNumber(Items[I].Quantity):FText::GetEmpty());
   Cell->SelectButton->SetIsEnabled(Items.IsValidIndex(I));
   auto* Slot=ItemGrid->AddChildToUniformGrid(Cell,I/4,I%4);
   Slot->SetHorizontalAlignment(HAlign_Fill); Slot->SetVerticalAlignment(VAlign_Fill);
  }
  if(StatusText) StatusText->SetText(FText::FromString(FString::Printf(TEXT("Предметов: %d"),Items.Num())));
  if(Items.Num()) SelectItem(0);
  else if(DetailText) DetailText->SetText(FText::FromString(TEXT("Сумка пуста")));
 }
 if(Kind==EAltaiScreenKind::Settings && StatusText)
 {
  auto* GS=UGameUserSettings::GetGameUserSettings();
  const TCHAR* Names[]={TEXT("Низкое"),TEXT("Среднее"),TEXT("Высокое"),TEXT("Очень высокое"),TEXT("Кино")};
  int32 Q=GS->GetOverallScalabilityLevel();
  StatusText->SetText(FText::FromString(FString::Printf(TEXT("Качество: %s\nРежим: %s"),Q>=0 && Q<5?Names[Q]:TEXT("Пользовательское"),GS->GetFullscreenMode()==EWindowMode::Windowed?TEXT("Оконный"):TEXT("Полный экран"))));
 }
}
void UAltaiScreen::SelectItem(int32 Index)
{
 if(auto* S=UAltaiSession::Find(this)) if(S->GetItems().IsValidIndex(Index) && DetailText)
 {
  if(ItemGrid) for(int32 N=0;N<ItemGrid->GetChildrenCount();++N)
   if(auto* Cell=Cast<UAltaiItemCell>(ItemGrid->GetChildAt(N)))
    Cell->SelectButton->SetBackgroundColor(N==Index?FLinearColor(0.65f,0.51f,0.31f,1):FLinearColor::White);
  const auto& I=S->GetItems()[Index];
  DetailText->SetText(FText::FromString(FString::Printf(TEXT("%s\n\n%s\n\nКоличество: %d"),*I.Name.ToString(),*I.Description.ToString(),I.Quantity)));
 }
}
void UAltaiScreen::Primary()
{
 auto* PC=Cast<AAltaiPlayerController>(GetOwningPlayer());auto* S=UAltaiSession::Find(this);if(!PC || !S)return;
 switch(Kind)
 {
 case EAltaiScreenKind::MainMenu:
  if(!S->ContinueExpedition() && StatusText) StatusText->SetText(FText::FromString(TEXT("Не удалось прочитать сохранение")));break;
 case EAltaiScreenKind::Pause:PC->ShowScreen(EAltaiScreenKind::None);break;
 case EAltaiScreenKind::ConfirmNew:
 case EAltaiScreenKind::NewGame:
  if(!S->CreateProfile(ProfileNameInput?ProfileNameInput->GetText().ToString():TEXT("Новое прохождение")) && StatusText)StatusText->SetText(FText::FromString(S->GetSaveStatus()));break;
 case EAltaiScreenKind::ConfirmExit:
  UKismetSystemLibrary::QuitGame(this,GetOwningPlayer(),EQuitPreference::Quit,false);break;
 case EAltaiScreenKind::Settings:
 { auto* GS=UGameUserSettings::GetGameUserSettings();GS->SetOverallScalabilityLevel((FMath::Max(GS->GetOverallScalabilityLevel(),0)+1)%4);GS->ApplySettings(false);Refresh();break; }
 default:break;
 }
}
void UAltaiScreen::Secondary()
{
 auto* PC=Cast<AAltaiPlayerController>(GetOwningPlayer());auto* S=UAltaiSession::Find(this);if(!PC || !S)return;
 if(Kind==EAltaiScreenKind::MainMenu)
 { PC->ShowScreen(EAltaiScreenKind::NewGame); }
 else if(Kind==EAltaiScreenKind::Pause)
 { if(S->SaveExpedition())S->ReturnToMenu();else if(StatusText)StatusText->SetText(FText::FromString(S->GetSaveStatus())); }
 else if(Kind==EAltaiScreenKind::Settings)
 { auto* GS=UGameUserSettings::GetGameUserSettings();GS->SetFullscreenMode(GS->GetFullscreenMode()==EWindowMode::Windowed?EWindowMode::WindowedFullscreen:EWindowMode::Windowed);GS->ApplySettings(false);Refresh(); }
}
void UAltaiScreen::Settings() { if(auto* PC=Cast<AAltaiPlayerController>(GetOwningPlayer()))PC->ShowScreen(EAltaiScreenKind::Settings); }
void UAltaiScreen::Quit()
{
 if(Kind==EAltaiScreenKind::Pause)
 {if(auto* PC=Cast<AAltaiPlayerController>(GetOwningPlayer()))PC->ShowScreen(EAltaiScreenKind::ConfirmExit);}
 else UKismetSystemLibrary::QuitGame(this,GetOwningPlayer(),EQuitPreference::Quit,false);
}
void UAltaiScreen::Back() { if(auto* PC=Cast<AAltaiPlayerController>(GetOwningPlayer()))PC->GoBack(); }
FReply UAltaiScreen::NativeOnKeyDown(const FGeometry& G,const FKeyEvent& E)
{
 const FKey K=E.GetKey();
 if(K==EKeys::Escape || K==EKeys::Gamepad_FaceButton_Right || (Kind==EAltaiScreenKind::Inventory && (K==EKeys::I || K==EKeys::Tab)))
 { Back();return FReply::Handled(); }
 return Super::NativeOnKeyDown(G,E);
}
void AAltaiPlayerController::BeginPlay()
{
 Super::BeginPlay();
 GetWorldTimerManager().SetTimerForNextTick([this]()
 {
  auto* S=UAltaiSession::Find(this);
  if(S && S->IsExpedition())
  {S->WorldReady();if(HUDClass){HUDWidget=CreateWidget<UUserWidget>(this,HUDClass);HUDWidget->AddToViewport();} ShowScreen(EAltaiScreenKind::None);}
  else ShowScreen(EAltaiScreenKind::MainMenu);
 });
}
void AAltaiPlayerController::SetupInputComponent()
{
 Super::SetupInputComponent();
 InputComponent->BindKey(EKeys::M,IE_Pressed,this,&AAltaiPlayerController::ToggleMap);
 InputComponent->BindKey(EKeys::I,IE_Pressed,this,&AAltaiPlayerController::ToggleInventory);
 InputComponent->BindKey(EKeys::Tab,IE_Pressed,this,&AAltaiPlayerController::ToggleInventory);
 InputComponent->BindKey(EKeys::Escape,IE_Pressed,this,&AAltaiPlayerController::TogglePause).bExecuteWhenPaused=true;
 InputComponent->BindKey(EKeys::Gamepad_Special_Right,IE_Pressed,this,&AAltaiPlayerController::TogglePause).bExecuteWhenPaused=true;
}
void AAltaiPlayerController::ShowScreen(EAltaiScreenKind Kind)
{
 if(Kind==EAltaiScreenKind::Settings || Kind==EAltaiScreenKind::ConfirmNew || Kind==EAltaiScreenKind::LoadGame || Kind==EAltaiScreenKind::NewGame || Kind==EAltaiScreenKind::ConfirmExit)ReturnScreen=CurrentScreen;
 if(ActiveScreen){ActiveScreen->RemoveFromParent();ActiveScreen=nullptr;}
 CurrentScreen=Kind;
 const bool bUI=Kind!=EAltaiScreenKind::None;
 if(auto* S=UAltaiSession::Find(this))UGameplayStatics::SetGamePaused(this,bUI && S->IsExpedition());
 bShowMouseCursor=bUI;
 if(HUDWidget)HUDWidget->SetVisibility(bUI?ESlateVisibility::Collapsed:ESlateVisibility::HitTestInvisible);
 if(!bUI){FInputModeGameOnly M;SetInputMode(M);return;}
 TSubclassOf<UAltaiScreen> Class;
 switch(Kind){case EAltaiScreenKind::MainMenu:Class=MainMenuClass;break;case EAltaiScreenKind::Pause:Class=PauseClass;break;case EAltaiScreenKind::Inventory:Class=InventoryClass;break;case EAltaiScreenKind::Settings:Class=SettingsClass;break;case EAltaiScreenKind::ConfirmNew:Class=ConfirmNewClass;break;case EAltaiScreenKind::LoadGame:Class=LoadGameClass;break;case EAltaiScreenKind::NewGame:Class=NewGameClass;break;case EAltaiScreenKind::ConfirmExit:Class=ConfirmExitClass;break;default:break;}
 if(!Class)return;
 ActiveScreen=CreateWidget<UAltaiScreen>(this,Class);ActiveScreen->AddToViewport(10);
 FInputModeUIOnly M;M.SetWidgetToFocus(ActiveScreen->TakeWidget());M.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);SetInputMode(M);
 ActiveScreen->SetKeyboardFocus();
}
void AAltaiPlayerController::GoBack()
{
 if(CurrentScreen==EAltaiScreenKind::Settings || CurrentScreen==EAltaiScreenKind::ConfirmNew || CurrentScreen==EAltaiScreenKind::LoadGame || CurrentScreen==EAltaiScreenKind::NewGame || CurrentScreen==EAltaiScreenKind::ConfirmExit)ShowScreen(ReturnScreen);
 else if(CurrentScreen==EAltaiScreenKind::Inventory || CurrentScreen==EAltaiScreenKind::Pause)ShowScreen(EAltaiScreenKind::None);
}
void AAltaiPlayerController::ToggleInventory(){if(CurrentScreen==EAltaiScreenKind::None)ShowScreen(EAltaiScreenKind::Inventory);}
void AAltaiPlayerController::TogglePause(){if(CurrentScreen==EAltaiScreenKind::None)ShowScreen(EAltaiScreenKind::Pause);else GoBack();}

void UAltaiProfileRow::NativeOnInitialized()
{Super::NativeOnInitialized();SelectButton->OnClicked.AddDynamic(this,&UAltaiProfileRow::Select);}
void UAltaiProfileRow::Select(){if(OwnerScreen)OwnerScreen->SelectProfile(ProfileId);}
void UAltaiScreen::SelectProfile(const FString& Id)
{if(auto* S=UAltaiSession::Find(this))if(!S->LoadProfile(Id) && StatusText)StatusText->SetText(FText::FromString(S->GetSaveStatus()));}
void UAltaiScreen::OpenLoad()
{if(auto* PC=Cast<AAltaiPlayerController>(GetOwningPlayer()))PC->ShowScreen(EAltaiScreenKind::LoadGame);}
void UAltaiHUD::NativeTick(const FGeometry& G,float Delta)
{
 Super::NativeTick(G,Delta);
 if(auto* S=UAltaiSession::Find(this))
 {
  const FString Notice=S->GetSaveStatus();
  if(S->GetSaveRevision()!=PreviousRevision){PreviousRevision=S->GetSaveRevision();NoticeAge=0;}
  NoticeAge+=Delta;
  if(SaveNotice)SaveNotice->SetText(FText::FromString((S->HasSaveError() || NoticeAge<4)?Notice:FString()));
 }
}

void AAltaiPlayerController::ToggleMap(){if(CurrentScreen==EAltaiScreenKind::None){ShowScreen(EAltaiScreenKind::Inventory);if(auto* Screen=Cast<UAltaiInventoryScreen>(ActiveScreen))Screen->ShowMap();}}
