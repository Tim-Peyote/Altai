#include "AltaiEditorLibrary.h"
#include "AltaiInventoryScreen.h"
#include "AltaiInventoryPreview.h"
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
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/ScaleBox.h"
#include "Components/UniformGridPanel.h"
#include "Components/ScrollBox.h"
#include "Components/WidgetSwitcher.h"
#include "AssetToolsModule.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "EditorAssetLibrary.h"
#include "Engine/Font.h"
#include "Engine/FontFace.h"
#include "Factories/FontFactory.h"
#include "Materials/MaterialInterface.h"

namespace FieldUI
{
const FLinearColor Ink(0,0,0,1),Panel(.016,.022,.027,1),TextColor(.84,.82,.75,1),Muted(.43,.48,.49,1);
template<class T>T* W(UWidgetBlueprint* B,const TCHAR* Name){auto* X=B->WidgetTree->ConstructWidget<T>(T::StaticClass(),FName(Name));X->bIsVariable=true;return X;}
UWidgetBlueprint* New(const TCHAR* Name,UClass* Parent)
{
 if(UEditorAssetLibrary::DoesAssetExist(FString("/Game/Altai/UI/")+Name))return nullptr;
 auto* Factory=NewObject<UWidgetBlueprintFactory>();Factory->ParentClass=Parent;
 return Cast<UWidgetBlueprint>(FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get().CreateAsset(Name,TEXT("/Game/Altai/UI"),UWidgetBlueprint::StaticClass(),Factory));
}
void Save(UWidgetBlueprint* B){FKismetEditorUtilities::CompileBlueprint(B);if(B->Status==BS_Error){UE_LOG(LogTemp,Error,TEXT("Inventory widget compile failed: %s"),*B->GetName());return;}UEditorAssetLibrary::SaveLoadedAsset(B,false);}
UTextBlock* Text(UWidgetBlueprint* B,const TCHAR* Name,const TCHAR* Value,int Size,FLinearColor Color=TextColor)
{
 auto* T=W<UTextBlock>(B,Name);T->SetText(FText::FromString(Value));FSlateFontInfo F;F.FontObject=LoadObject<UFont>(nullptr,TEXT("/Engine/EngineFonts/Roboto.Roboto"));F.TypefaceFontName=TEXT("Regular");F.Size=Size;T->SetFont(F);T->SetColorAndOpacity(Color);T->SetAutoWrapText(true);return T;
}
void At(UCanvasPanel* C,UWidget* W,float X,float Y,float Right,float Bottom)
{auto* S=C->AddChildToCanvas(W);S->SetAnchors(FAnchors(X,Y,Right,Bottom));S->SetOffsets(FMargin(0));}
void Append(UVerticalBox* V,UWidget* W,float Pad=12)
{auto* S=V->AddChildToVerticalBox(W);S->SetPadding(FMargin(0,0,0,Pad));}
UButton* Button(UWidgetBlueprint* B,const TCHAR* Name,const TCHAR* Label,int FontSize=16,const TCHAR* LabelName=nullptr)
{
 auto* Btn=W<UButton>(B,Name);auto Style=Btn->GetStyle();Style.Normal.TintColor=FSlateColor(Panel);Style.Hovered.TintColor=FSlateColor(FLinearColor(.13,.15,.16,1));Style.Pressed.TintColor=FSlateColor(FLinearColor(.27,.22,.14,1));Style.NormalPadding=FMargin(10,9);Style.PressedPadding=FMargin(10,9);Btn->SetStyle(Style);
 if(Label && *Label)Btn->AddChild(Text(B,LabelName?LabelName:*(FString(Name)+"Label"),Label,FontSize));return Btn;
}
void Action(UWidgetBlueprint* B,UVerticalBox* V,const TCHAR* Name,const TCHAR* Label,const TCHAR* LabelName=nullptr)
{auto* Size=B->WidgetTree->ConstructWidget<USizeBox>();Size->SetMinDesiredHeight(44);Size->AddChild(Button(B,Name,Label,16,LabelName));Append(V,Size,8);}
UBorder* PanelBox(UWidgetBlueprint* B,const TCHAR* Name)
{auto* P=W<UBorder>(B,Name);P->SetBrushColor(Panel);P->SetPadding(FMargin(16));return P;}
UUserWidget* Instance(UWidgetBlueprint* B,const TCHAR* Name,const TCHAR* Asset)
{auto* C=LoadClass<UUserWidget>(nullptr,*FString::Printf(TEXT("/Game/Altai/UI/%s.%s_C"),Asset,Asset));auto* X=NewObject<UUserWidget>(B->WidgetTree,C,FName(Name),RF_Transactional);X->bIsVariable=true;return X;}
}
void UAltaiEditorLibrary::CreateInventoryAssets()
{
 using namespace FieldUI;
 if(auto* B=New(TEXT("WBP_InventoryItem"),UAltaiItemCell::StaticClass()))
 {
  auto* Btn=Button(B,TEXT("SelectButton"),TEXT(""));Btn->ClearChildren();B->WidgetTree->RootWidget=Btn;
  auto* V=W<UVerticalBox>(B,TEXT("Content"));Btn->AddChild(V);
  auto* Size=W<USizeBox>(B,TEXT("IconSize"));Size->SetHeightOverride(58);Size->SetWidthOverride(58);
  auto* Icon=W<UImage>(B,TEXT("ItemIcon"));Size->AddChild(Icon);auto* S=V->AddChildToVerticalBox(Size);S->SetHorizontalAlignment(HAlign_Center);
  auto* N=Text(B,TEXT("ItemName"),TEXT("Предмет"),14);N->SetJustification(ETextJustify::Center);Append(V,N,2);
  auto* Q=Text(B,TEXT("ItemQuantity"),TEXT("1"),12,Muted);Q->SetJustification(ETextJustify::Right);Append(V,Q,0);Save(B);
 }
 if(auto* B=New(TEXT("WBP_EquipmentCell"),UAltaiEquipmentCell::StaticClass()))
 {
  auto* Btn=Button(B,TEXT("SelectButton"),TEXT(""));Btn->ClearChildren();B->WidgetTree->RootWidget=Btn;
  Btn->SetToolTipText(FText::FromString(TEXT("ЛКМ — назначить выбранный предмет. ПКМ — освободить ячейку.")));
  auto* V=W<UVerticalBox>(B,TEXT("Content"));Btn->AddChild(V);Append(V,Text(B,TEXT("SlotName"),TEXT("Ячейка"),12,Muted),5);Append(V,Text(B,TEXT("EquippedName"),TEXT("—"),15),0);Save(B);
 }
 if(auto* B=New(TEXT("WBP_PreviewSurface"),UAltaiPreviewSurface::StaticClass()))
 {
  auto* I=W<UImage>(B,TEXT("PreviewImage"));B->WidgetTree->RootWidget=I;
  I->SetBrushFromMaterial(LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Altai/UI/Preview/M_InventoryPreview.M_InventoryPreview")));Save(B);
 }
 if(auto* B=New(TEXT("WBP_FieldMap"),UUserWidget::StaticClass()))
 {
  auto* C=W<UCanvasPanel>(B,TEXT("Root"));B->WidgetTree->RootWidget=C;auto* P=PanelBox(B,TEXT("MapPanel"));At(C,P,0,0,1,1);
  auto* V=W<UVerticalBox>(B,TEXT("MapMessage"));At(C,V,.15,.28,.85,.8);
  auto* H=Text(B,TEXT("MapHeading"),TEXT("Карта ещё не составлена"),32);H->SetJustification(ETextJustify::Center);Append(V,H,22);
  auto* T=Text(B,TEXT("MapDescription"),TEXT("Исследованные места появятся здесь.\nСистема исследования мира пока не подключена."),20,Muted);T->SetJustification(ETextJustify::Center);Append(V,T);Save(B);
 }
 if(auto* B=New(TEXT("WBP_FieldInventory"),UAltaiInventoryScreen::StaticClass()))
 {
  auto* C=W<UCanvasPanel>(B,TEXT("Root"));B->WidgetTree->RootWidget=C;auto* Bg=PanelBox(B,TEXT("Background"));Bg->SetBrushColor(Ink);At(C,Bg,0,0,1,1);
  auto* Tabs=W<UHorizontalBox>(B,TEXT("InventoryTabBar"));At(C,Tabs,.035,.035,.48,.10);
  for(auto* Btn:{Button(B,TEXT("InventoryTab"),TEXT("ИНВЕНТАРЬ"),20),Button(B,TEXT("MapTab"),TEXT("КАРТА"),20)}){auto* S=Tabs->AddChildToHorizontalBox(Btn);S->SetPadding(FMargin(0,0,16,0));S->SetSize(FSlateChildSize(ESlateSizeRule::Fill));}
  auto* Pages=W<UWidgetSwitcher>(B,TEXT("Pages"));At(C,Pages,.035,.14,.965,.89);
  auto* Page=W<UCanvasPanel>(B,TEXT("InventoryPage"));Pages->AddChild(Page);
  auto* Preview=Instance(B,TEXT("PreviewSurface"),TEXT("WBP_PreviewSurface"));At(Page,Preview,.30,-.04,.76,1.04);
  auto* Left=PanelBox(B,TEXT("BagPanel"));Left->SetBrushColor(Ink);At(Page,Left,0,0,.30,1);auto* V=W<UVerticalBox>(B,TEXT("BagContent"));Left->AddChild(V);
  auto* Filters=W<UHorizontalBox>(B,TEXT("Filters"));Append(V,Filters,14);
  const TCHAR* Names[]={TEXT("FilterAll"),TEXT("FilterMaterials"),TEXT("FilterEquipment"),TEXT("FilterTools")};const TCHAR* Labels[]={TEXT("Все"),TEXT("Мат."),TEXT("Одежда"),TEXT("Снар.")};
  for(int I=0;I<4;++I){auto* S=Filters->AddChildToHorizontalBox(Button(B,Names[I],Labels[I],13));S->SetSize(FSlateChildSize(ESlateSizeRule::Fill));S->SetPadding(FMargin(0,0,3,0));}
  auto* Grid=W<UUniformGridPanel>(B,TEXT("ItemGrid"));Grid->SetSlotPadding(FMargin(3));Grid->SetMinDesiredSlotHeight(108);Grid->SetMinDesiredSlotWidth(80);auto* Scroll=W<UScrollBox>(B,TEXT("ItemScroll"));Scroll->SetOrientation(Orient_Vertical);Scroll->SetClipping(EWidgetClipping::ClipToBounds);Scroll->AddChild(Grid);auto* GS=V->AddChildToVerticalBox(Scroll);GS->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
  Append(V,Text(B,TEXT("StatusText"),TEXT(""),13,Muted),0);
  auto* Right=PanelBox(B,TEXT("DescriptionPanel"));Right->SetBrushColor(Ink);At(Page,Right,.77,0,1,1);auto* D=W<UVerticalBox>(B,TEXT("DescriptionContent"));Right->AddChild(D);
  Append(D,Text(B,TEXT("SelectedName"),TEXT("Предмет"),27),5);Append(D,Text(B,TEXT("SelectedCategory"),TEXT(""),13,Muted),16);
  auto* Size=W<USizeBox>(B,TEXT("SelectedIconSize"));Size->SetHeightOverride(128);auto* I=W<UImage>(B,TEXT("SelectedIcon"));Size->AddChild(I);Append(D,Size,16);
  auto* DS=D->AddChildToVerticalBox(Text(B,TEXT("DetailText"),TEXT(""),17));DS->SetSize(FSlateChildSize(ESlateSizeRule::Fill));DS->SetPadding(FMargin(0,0,0,12));
  Action(B,D,TEXT("AssignButton"),TEXT("Назначить"),TEXT("AssignLabel"));Action(B,D,TEXT("UnassignButton"),TEXT("Снять назначение"));
  struct SlotSpec{const TCHAR* Name;const TCHAR* Label;EAltaiEquipSlot Slot;float X,Y;};
  const SlotSpec Specs[]={{TEXT("HeadSlot"),TEXT("ГОЛОВА"),EAltaiEquipSlot::Head,.64,.10},{TEXT("BodySlot"),TEXT("ОДЕЖДА"),EAltaiEquipSlot::Body,.31,.34},{TEXT("HandSlot"),TEXT("В РУКЕ"),EAltaiEquipSlot::Hand,.64,.45},{TEXT("QuickSlot1"),TEXT("БЫСТРЫЙ 1"),EAltaiEquipSlot::Quick1,.40,.82},{TEXT("QuickSlot2"),TEXT("БЫСТРЫЙ 2"),EAltaiEquipSlot::Quick2,.54,.82}};
  for(const auto& Spec:Specs){auto* Cell=Cast<UAltaiEquipmentCell>(Instance(B,Spec.Name,TEXT("WBP_EquipmentCell")));Cell->EquipmentSlot=Spec.Slot;At(Page,Cell,Spec.X,Spec.Y,Spec.X+.12,Spec.Y+.13);}
  auto* Hint=Text(B,TEXT("RotationHint"),TEXT("Потяните мышью, чтобы повернуть"),13,Muted);Hint->SetJustification(ETextJustify::Center);At(Page,Hint,.32,.97,.75,1.03);
  Pages->AddChild(Instance(B,TEXT("MapPage"),TEXT("WBP_FieldMap")));
  At(C,Button(B,TEXT("BackButton"),TEXT("Закрыть   ·   Esc / I"),16),.035,.93,.25,.98);
  At(C,Text(B,TEXT("FooterHint"),TEXT("M  Инвентарь / карта     •     ПКМ по ячейке — снять назначение"),13,Muted),.32,.94,.965,.99);
  Save(B);auto* Defaults=Cast<UAltaiInventoryScreen>(B->GeneratedClass->GetDefaultObject());Defaults->Kind=EAltaiScreenKind::Inventory;Defaults->SetIsFocusable(true);
  Defaults->ItemCellClass=LoadClass<UAltaiItemCell>(nullptr,TEXT("/Game/Altai/UI/WBP_InventoryItem.WBP_InventoryItem_C"));
  Defaults->PreviewClass=LoadClass<AAltaiInventoryPreview>(nullptr,TEXT("/Game/Altai/UI/Preview/BP_InventoryPreviewRig.BP_InventoryPreviewRig_C"));Save(B);
 }
}

void UAltaiEditorLibrary::PolishInventoryAssets()
{
 using namespace FieldUI;
 if(auto* B=Cast<UWidgetBlueprint>(UEditorAssetLibrary::LoadAsset(TEXT("/Game/Altai/UI/WBP_PreviewSurface"))))
 {
  if(!B->WidgetTree->FindWidget(TEXT("PortraitAspect")))
  {
   auto* Image=B->WidgetTree->FindWidget(TEXT("PreviewImage"));
   auto* Scale=W<UScaleBox>(B,TEXT("PortraitAspect"));Scale->SetStretch(EStretch::ScaleToFit);
   auto* Size=W<USizeBox>(B,TEXT("PortraitSize"));Size->SetWidthOverride(640);Size->SetHeightOverride(896);
   B->WidgetTree->RootWidget=Scale;Scale->AddChild(Size);Size->AddChild(Image);Save(B);
  }
 }
 if(auto* B=Cast<UWidgetBlueprint>(UEditorAssetLibrary::LoadAsset(TEXT("/Game/Altai/UI/WBP_FieldInventory"))))
 {
  if(auto* Size=Cast<USizeBox>(B->WidgetTree->FindWidget(TEXT("SelectedIconSize"))))
  {Size->SetWidthOverride(128);if(auto* Slot=Cast<UVerticalBoxSlot>(Size->Slot))Slot->SetHorizontalAlignment(HAlign_Center);Save(B);}
 }
}

bool UAltaiEditorLibrary::CreateHeadingFont()
{
 const FString Path=TEXT("/Game/Altai/UI/Fonts/F_Display");
 if(UEditorAssetLibrary::DoesAssetExist(Path))return true;
 auto* Face=LoadObject<UFontFace>(nullptr,TEXT("/Game/Altai/UI/Fonts/F_Heading.F_Heading"));if(!Face)return false;
 auto* Factory=NewObject<UFontFactory>();
 auto* Font=Cast<UFont>(FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get().CreateAsset(TEXT("F_Display"),TEXT("/Game/Altai/UI/Fonts"),UFont::StaticClass(),Factory));if(!Font)return false;
 Font->FontCacheType=EFontCacheType::Runtime;
 FTypefaceEntry Entry;Entry.Name=TEXT("Regular");Entry.Font=FFontData(Face);Font->CompositeFont.DefaultTypeface.Fonts.Add(Entry);
 Font->MarkPackageDirty();return UEditorAssetLibrary::SaveLoadedAsset(Font,false);
}
