#include "AltaiEditorLibrary.h"
#include "AltaiScreens.h"
#include "WidgetBlueprint.h"
#include "WidgetBlueprintFactory.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Border.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/SizeBox.h"
#include "Components/ScrollBox.h"
#include "Components/EditableTextBox.h"
#include "Components/UniformGridPanel.h"
#include "AssetToolsModule.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "EditorAssetLibrary.h"
#include "Styling/CoreStyle.h"
#include "Engine/Font.h"

namespace AltaiUI
{
const FLinearColor Ink(0.026f,0.032f,0.038f,1), Panel(0.065f,0.075f,0.083f,1), Bone(0.84f,0.82f,0.76f,1), Muted(0.48f,0.51f,0.50f,1), Accent(0.48f,0.36f,0.20f,1);
template<typename T>T* W(UWidgetBlueprint* B,const TCHAR* Name){return B->WidgetTree->ConstructWidget<T>(T::StaticClass(),FName(Name));}
UTextBlock* Text(UWidgetBlueprint* B,const TCHAR* Name,const TCHAR* Value,int32 Size,FLinearColor Color=Bone)
{
 auto* T=W<UTextBlock>(B,Name);T->SetText(FText::FromString(Value));FSlateFontInfo Font=FCoreStyle::GetDefaultFontStyle("Regular",Size);Font.FontObject=LoadObject<UFont>(nullptr,TEXT("/Engine/EngineFonts/Roboto.Roboto"));T->SetFont(Font);T->SetColorAndOpacity(FSlateColor(Color));T->SetAutoWrapText(true);return T;
}
void Append(UVerticalBox* V,UWidget* Child,float Bottom=12)
{auto* S=V->AddChildToVerticalBox(Child);S->SetPadding(FMargin(0,0,0,Bottom));}
void Button(UWidgetBlueprint* B,UVerticalBox* V,const TCHAR* Name,const TCHAR* Label)
{
 auto* Box=B->WidgetTree->ConstructWidget<USizeBox>();Box->SetMinDesiredHeight(48);
 auto* Btn=W<UButton>(B,Name);Btn->bIsVariable=true;
 FButtonStyle Style=Btn->GetStyle();Style.Normal.TintColor=FSlateColor(Panel);Style.Hovered.TintColor=FSlateColor(FLinearColor(0.16,0.18,0.19,1));Style.Pressed.TintColor=FSlateColor(Accent);Style.NormalPadding=FMargin(18,10);Style.PressedPadding=FMargin(18,10);Btn->SetStyle(Style);
 FString TN=FString(Name)+"Label";auto* T=Text(B,*TN,Label,20);T->SetJustification(ETextJustify::Left);T->SetAutoWrapText(false);Btn->AddChild(T);Box->AddChild(Btn);Append(V,Box,8);
}
UWidgetBlueprint* New(const TCHAR* Name,UClass* Parent)
{
 FString Path=FString("/Game/Altai/UI/")+Name;
 if(UEditorAssetLibrary::DoesAssetExist(Path))return nullptr;
 auto* F=NewObject<UWidgetBlueprintFactory>();F->ParentClass=Parent;
 return Cast<UWidgetBlueprint>(FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get().CreateAsset(Name,TEXT("/Game/Altai/UI"),UWidgetBlueprint::StaticClass(),F));
}
void Save(UWidgetBlueprint* B)
{FKismetEditorUtilities::CompileBlueprint(B);UEditorAssetLibrary::SaveLoadedAsset(B,false);}
void Screen(const TCHAR* Name,EAltaiScreenKind Kind,const TCHAR* Heading)
{
 auto* B=New(Name,UAltaiScreen::StaticClass());if(!B)return;
 auto* Canvas=W<UCanvasPanel>(B,TEXT("Root"));B->WidgetTree->RootWidget=Canvas;
 auto* Bg=W<UBorder>(B,TEXT("Background"));Bg->SetBrushColor(Ink);Bg->SetPadding(FMargin(0));
 auto* BS=Canvas->AddChildToCanvas(Bg);BS->SetAnchors(FAnchors(0,0,1,1));BS->SetOffsets(FMargin(0));
 auto* V=W<UVerticalBox>(B,TEXT("Content"));auto* VS=Canvas->AddChildToCanvas(V);
 const bool Inv=Kind==EAltaiScreenKind::Inventory;
 VS->SetAnchors((Inv || Kind==EAltaiScreenKind::LoadGame)?FAnchors(0.07,0.09,0.93,0.91):FAnchors(0.10,0.13,0.48,0.92));VS->SetOffsets(FMargin(0));
 Append(V,Text(B,TEXT("Eyebrow"),TEXT("АЛТАЙ / ЭКСПЕДИЦИЯ"),14,Muted),20);
 Append(V,Text(B,TEXT("Heading"),Heading,Inv?38:54),24);
 if(Inv)
 {
  auto* Row=W<UHorizontalBox>(B,TEXT("InventoryBody"));
  auto* Grid=W<UUniformGridPanel>(B,TEXT("ItemGrid"));Grid->bIsVariable=true;Grid->SetSlotPadding(FMargin(4));Grid->SetMinDesiredSlotHeight(90);Grid->SetMinDesiredSlotWidth(130);
  auto* GS=Row->AddChildToHorizontalBox(Grid);GS->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
  auto* Detail=Text(B,TEXT("DetailText"),TEXT("Выберите предмет"),20);Detail->bIsVariable=true;
  auto* DS=Row->AddChildToHorizontalBox(Detail);FSlateChildSize DSize(ESlateSizeRule::Fill);DSize.Value=0.65;DS->SetSize(DSize);DS->SetPadding(FMargin(32,8,0,0));
  Append(V,Row,20);
 }
 if(Kind==EAltaiScreenKind::MainMenu)
 {Button(B,V,TEXT("PrimaryButton"),TEXT("Продолжить"));Button(B,V,TEXT("SecondaryButton"),TEXT("Новая игра"));Button(B,V,TEXT("SettingsButton"),TEXT("Настройки"));Button(B,V,TEXT("QuitButton"),TEXT("Выход"));}
 if(Kind==EAltaiScreenKind::Pause)
 {Button(B,V,TEXT("PrimaryButton"),TEXT("Вернуться в игру"));Button(B,V,TEXT("SecondaryButton"),TEXT("Сохранить и выйти в меню"));Button(B,V,TEXT("SettingsButton"),TEXT("Настройки"));}
 if(Kind==EAltaiScreenKind::Settings)
 {Button(B,V,TEXT("PrimaryButton"),TEXT("Изменить качество графики"));Button(B,V,TEXT("SecondaryButton"),TEXT("Переключить режим экрана"));Button(B,V,TEXT("BackButton"),TEXT("Назад"));}
 if(Kind==EAltaiScreenKind::ConfirmNew)
 {
  Append(V,Text(B,TEXT("Warning"),TEXT("Будет создано отдельное прохождение."),20),20);
  Button(B,V,TEXT("PrimaryButton"),TEXT("Начать заново"));Button(B,V,TEXT("BackButton"),TEXT("Отмена"));
 }
 if(Kind==EAltaiScreenKind::LoadGame)
 {
  auto* List=W<UScrollBox>(B,TEXT("ProfileList"));List->bIsVariable=true;
  auto* Slot=V->AddChildToVerticalBox(List);Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));Slot->SetPadding(FMargin(0,0,0,16));
  Button(B,V,TEXT("BackButton"),TEXT("Назад"));
 }
 if(Kind==EAltaiScreenKind::NewGame)
 {
  Append(V,Text(B,TEXT("Explanation"),TEXT("Отдельное прохождение со своим прогрессом и автосохранениями."),18),16);
  auto* Input=W<UEditableTextBox>(B,TEXT("ProfileNameInput"));Input->bIsVariable=true;Input->SetHintText(FText::FromString(TEXT("Имя прохождения")));
  auto Style=Input->GetWidgetStyle();FSlateFontInfo Font=FCoreStyle::GetDefaultFontStyle("Regular",20);Font.FontObject=LoadObject<UFont>(nullptr,TEXT("/Engine/EngineFonts/Roboto.Roboto"));Style.TextStyle.SetFont(Font);Style.SetForegroundColor(Bone).SetFocusedForegroundColor(Bone).SetBackgroundColor(Panel).SetPadding(FMargin(12,10));Input->SetWidgetStyle(Style);Append(V,Input,20);
  Button(B,V,TEXT("PrimaryButton"),TEXT("Начать"));Button(B,V,TEXT("BackButton"),TEXT("Назад"));
 }
 if(Kind==EAltaiScreenKind::ConfirmExit)
 {
  Append(V,Text(B,TEXT("Explanation"),TEXT("Выйти из игры без нового сохранения? Изменения после последнего автосохранения будут потеряны. Уже сделанные автосохранения останутся."),20),24);
  Button(B,V,TEXT("PrimaryButton"),TEXT("Выйти без сохранения"));Button(B,V,TEXT("BackButton"),TEXT("Остаться"));
 }
 if(Inv)Button(B,V,TEXT("BackButton"),TEXT("Закрыть сумку"));
 auto* Status=Text(B,TEXT("StatusText"),TEXT(""),16,Muted);Status->bIsVariable=true;Append(V,Status);
 Save(B);
 auto* Defaults=Cast<UAltaiScreen>(B->GeneratedClass->GetDefaultObject());Defaults->Kind=Kind;Defaults->SetIsFocusable(true);
 Defaults->ProfileRowClass=LoadClass<UAltaiProfileRow>(nullptr,TEXT("/Game/Altai/UI/WBP_ProfileRow.WBP_ProfileRow_C"));
 Defaults->ItemCellClass=LoadClass<UAltaiItemCell>(nullptr,TEXT("/Game/Altai/UI/WBP_ItemCell.WBP_ItemCell_C"));
 B->Modify();Save(B);
}
}
void UAltaiEditorLibrary::CreateUIAssets()
{
 using namespace AltaiUI;
 if(auto* B=New(TEXT("WBP_ItemCell"),UAltaiItemCell::StaticClass()))
 {
  auto* Button=W<UButton>(B,TEXT("SelectButton"));Button->bIsVariable=true;B->WidgetTree->RootWidget=Button;
  FButtonStyle Style=Button->GetStyle();Style.Normal.TintColor=FSlateColor(Panel);Style.Hovered.TintColor=FSlateColor(Accent);Style.Pressed.TintColor=FSlateColor(Accent);Button->SetStyle(Style);
  auto* V=W<UVerticalBox>(B,TEXT("CellContent"));Button->AddChild(V);
  auto* N=Text(B,TEXT("ItemName"),TEXT("Предмет"),18);N->bIsVariable=true;Append(V,N,4);
  auto* Q=Text(B,TEXT("ItemQuantity"),TEXT("1"),14,Muted);Q->bIsVariable=true;Append(V,Q,0);Save(B);
 }
 Screen(TEXT("WBP_MainMenu"),EAltaiScreenKind::MainMenu,TEXT("АЛТАЙ"));
 Screen(TEXT("WBP_Pause"),EAltaiScreenKind::Pause,TEXT("Пауза"));
 Screen(TEXT("WBP_Inventory"),EAltaiScreenKind::Inventory,TEXT("Сумка"));
 Screen(TEXT("WBP_Settings"),EAltaiScreenKind::Settings,TEXT("Настройки"));
 Screen(TEXT("WBP_ConfirmNew"),EAltaiScreenKind::ConfirmNew,TEXT("Новая игра"));
 if(auto* B=New(TEXT("WBP_ExpeditionHUD"),UUserWidget::StaticClass()))
 {
  auto* C=W<UCanvasPanel>(B,TEXT("Root"));B->WidgetTree->RootWidget=C;
  auto* T=Text(B,TEXT("Controls"),TEXT("WASD  Движение     Мышь  Камера     Пробел  Прыжок     I / Tab  Сумка     Esc  Пауза"),16);
  auto* S=C->AddChildToCanvas(T);S->SetAnchors(FAnchors(0.04,0.94,0.96,0.99));S->SetOffsets(FMargin(0));Save(B);
 }
}

void UAltaiEditorLibrary::UpgradeSaveUI()
{
 using namespace AltaiUI;
 if(auto* B=New(TEXT("WBP_ProfileRow"),UAltaiProfileRow::StaticClass()))
 {
  auto* Box=W<USizeBox>(B,TEXT("RowSize"));Box->SetMinDesiredHeight(86);B->WidgetTree->RootWidget=Box;
  auto* Btn=W<UButton>(B,TEXT("SelectButton"));Btn->bIsVariable=true;Box->AddChild(Btn);
  auto Style=Btn->GetStyle();Style.Normal.TintColor=FSlateColor(Panel);Style.Hovered.TintColor=FSlateColor(Accent);Style.NormalPadding=FMargin(18,12);Btn->SetStyle(Style);
  auto* V=W<UVerticalBox>(B,TEXT("Content"));Btn->AddChild(V);
  auto* Name=Text(B,TEXT("ProfileName"),TEXT("Прохождение"),22);Name->bIsVariable=true;Append(V,Name,6);
  auto* Detail=Text(B,TEXT("ProfileDetails"),TEXT("Мир • 0 ч 00 мин"),16);Detail->bIsVariable=true;Append(V,Detail,0);Save(B);
 }
 Screen(TEXT("WBP_LoadGame"),EAltaiScreenKind::LoadGame,TEXT("Загрузить игру"));
 Screen(TEXT("WBP_NewGame"),EAltaiScreenKind::NewGame,TEXT("Новая игра"));
 Screen(TEXT("WBP_ConfirmExit"),EAltaiScreenKind::ConfirmExit,TEXT("Выйти из игры?"));
 for(const TCHAR* Name:{TEXT("WBP_MainMenu"),TEXT("WBP_Pause")})
 {
  auto* B=Cast<UWidgetBlueprint>(UEditorAssetLibrary::LoadAsset(FString("/Game/Altai/UI/")+Name));if(!B)continue;
  auto* V=Cast<UVerticalBox>(B->WidgetTree->FindWidget(TEXT("Content")));if(!V)continue;
  const bool Main=FString(Name)==TEXT("WBP_MainMenu");const TCHAR* ButtonName=Main?TEXT("LoadButton"):TEXT("QuitButton");
  if(!B->WidgetTree->FindWidget(ButtonName))
  {B->Modify();Button(B,V,ButtonName,Main?TEXT("Загрузить игру"):TEXT("Выйти без сохранения"));V->ShiftChild(Main?3:V->GetChildrenCount()-2,V->GetChildAt(V->GetChildrenCount()-1));Save(B);}
 }
 if(auto* B=New(TEXT("WBP_GameHUD"),UAltaiHUD::StaticClass()))
 {
  auto* C=W<UCanvasPanel>(B,TEXT("Root"));B->WidgetTree->RootWidget=C;
  auto* T=Text(B,TEXT("Controls"),TEXT("WASD  Движение     Мышь  Камера     Пробел  Прыжок     I / Tab  Сумка     Esc  Пауза"),16);
  auto* S=C->AddChildToCanvas(T);S->SetAnchors(FAnchors(0.04,0.94,0.96,0.99));S->SetOffsets(FMargin(0));
  auto* N=Text(B,TEXT("SaveNotice"),TEXT(""),16);N->bIsVariable=true;N->SetJustification(ETextJustify::Right);
  auto* NS=C->AddChildToCanvas(N);NS->SetAnchors(FAnchors(0.55,0.04,0.96,0.16));NS->SetOffsets(FMargin(0));Save(B);
 }
}
