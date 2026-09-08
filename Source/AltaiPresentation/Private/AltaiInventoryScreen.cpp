#include "AltaiInventoryScreen.h"
#include "AltaiInventoryPreview.h"
#include "AltaiInventoryOrder.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/ScrollBox.h"
#include "Components/Spacer.h"
#include "Blueprint/WidgetTree.h"
#include "Components/WidgetSwitcher.h"
#include "Engine/World.h"
#include "Engine/Texture2D.h"
#include "InputCoreTypes.h"

namespace
{
const FLinearColor Normal(.12f,.14f,.16f,1),Selected(.65f,.49f,.27f,1);
void State(UButton* Button,bool Active)
{
 if(!Button)return;auto Style=Button->GetStyle();
 Style.Normal.DrawAs=ESlateBrushDrawType::RoundedBox;
 Style.Normal.TintColor=FSlateColor(Active?FLinearColor(.006,.005,.003,1):FLinearColor(.0005,.0005,.0005,1));
 Style.Normal.OutlineSettings.Width=Active?1.2f:.45f;
 Style.Normal.OutlineSettings.Color=FSlateColor(Active?FLinearColor(.48,.39,.23,1):FLinearColor(.06,.055,.045,1));
 Style.NormalForeground=FSlateColor(Active?FLinearColor(.9,.82,.63,1):FLinearColor(.55,.53,.47,1));
 Button->SetBackgroundColor(FLinearColor::White);Button->SetStyle(Style);
}
FText AltaiCategoryLabel(EAltaiItemCategory C)
{return FText::FromString(C==EAltaiItemCategory::Material?TEXT("Материал"):C==EAltaiItemCategory::Equipment?TEXT("Экипировка"):TEXT("Снаряжение"));}
const FAltaiItem* FindItem(const UAltaiSession* S,FName Id)
{return S?S->GetItems().FindByPredicate([&](const FAltaiItem& I){return I.Id==Id;}):nullptr;}
void Icon(UImage* Image,const FAltaiItem* Item)
{
 if(!Image)return;
 auto* Texture=Item?Item->Icon.LoadSynchronous():nullptr;
 Image->SetBrushFromTexture(Texture);Image->SetVisibility(Texture?ESlateVisibility::HitTestInvisible:ESlateVisibility::Hidden);
}
}
FReply UAltaiPreviewSurface::NativeOnMouseButtonDown(const FGeometry& G,const FPointerEvent& E)
{if(E.GetEffectingButton()==EKeys::LeftMouseButton){Rotating=true;return FReply::Handled().CaptureMouse(TakeWidget());}return Super::NativeOnMouseButtonDown(G,E);}
FReply UAltaiPreviewSurface::NativeOnMouseButtonUp(const FGeometry& G,const FPointerEvent& E)
{if(E.GetEffectingButton()==EKeys::LeftMouseButton){Rotating=false;return FReply::Handled().ReleaseMouseCapture();}return Super::NativeOnMouseButtonUp(G,E);}
FReply UAltaiPreviewSurface::NativeOnMouseMove(const FGeometry& G,const FPointerEvent& E)
{if(Rotating && E.IsMouseButtonDown(EKeys::LeftMouseButton)){if(OwnerInventory)OwnerInventory->RotatePreview(E.GetCursorDelta().X*.45f);return FReply::Handled();}Rotating=false;return Super::NativeOnMouseMove(G,E);}
void UAltaiEquipmentCell::NativeOnInitialized()
{Super::NativeOnInitialized();if(SelectButton)SelectButton->OnClicked.AddDynamic(this,&UAltaiEquipmentCell::SelectSlot);}
void UAltaiEquipmentCell::SelectSlot(){if(OwnerInventory)OwnerInventory->ClickEquipment(EquipmentSlot);}
FReply UAltaiEquipmentCell::NativeOnMouseButtonDown(const FGeometry& G,const FPointerEvent& E)
{return Super::NativeOnMouseButtonDown(G,E);}
void UAltaiInventoryScreen::NativeOnInitialized()
{
 Super::NativeOnInitialized();
 if(SortButton)SortButton->OnClicked.AddDynamic(this,&UAltaiInventoryScreen::CycleSort);
 if(ContextEquip)ContextEquip->OnClicked.AddDynamic(this,&UAltaiInventoryScreen::ContextAssignWear);
 if(ContextQuick1)ContextQuick1->OnClicked.AddDynamic(this,&UAltaiInventoryScreen::ContextAssignQuick1);
 if(ContextQuick2)ContextQuick2->OnClicked.AddDynamic(this,&UAltaiInventoryScreen::ContextAssignQuick2);
 if(ContextRemove)ContextRemove->OnClicked.AddDynamic(this,&UAltaiInventoryScreen::ContextUnassign);
 if(ContextClose)ContextClose->OnClicked.AddDynamic(this,&UAltaiInventoryScreen::CloseContext);
 if(ContextDismiss)ContextDismiss->OnClicked.AddDynamic(this,&UAltaiInventoryScreen::CloseContext);
 CloseContext();
 if(!InventoryTab || !MapTab || !FilterAll || !FilterMaterials || !FilterEquipment || !FilterTools || !AssignButton || !UnassignButton || !HeadSlot || !BodySlot || !HandSlot || !QuickSlot1 || !QuickSlot2 || !PreviewSurface)return;
 InventoryTab->OnClicked.AddDynamic(this,&UAltaiInventoryScreen::ShowInventory);MapTab->OnClicked.AddDynamic(this,&UAltaiInventoryScreen::ShowMap);
 FilterAll->OnClicked.AddDynamic(this,&UAltaiInventoryScreen::All);FilterMaterials->OnClicked.AddDynamic(this,&UAltaiInventoryScreen::Materials);
 FilterEquipment->OnClicked.AddDynamic(this,&UAltaiInventoryScreen::Equipment);FilterTools->OnClicked.AddDynamic(this,&UAltaiInventoryScreen::Tools);
 AssignButton->OnClicked.AddDynamic(this,&UAltaiInventoryScreen::AssignSelected);UnassignButton->OnClicked.AddDynamic(this,&UAltaiInventoryScreen::UnassignSelected);
 const TCHAR* Labels[]={TEXT("ГОЛОВА"),TEXT("ОДЕЖДА"),TEXT("В РУКЕ"),TEXT("БЫСТРЫЙ 1"),TEXT("БЫСТРЫЙ 2")};int32 N=0;
 for(auto* Cell:{HeadSlot.Get(),BodySlot.Get(),HandSlot.Get(),QuickSlot1.Get(),QuickSlot2.Get()}){Cell->OwnerInventory=this;if(Cell->SlotName)Cell->SlotName->SetText(FText::FromString(Labels[N]));++N;}
 PreviewSurface->OwnerInventory=this;
}
void UAltaiInventoryScreen::NativeConstruct()
{
 Super::NativeConstruct();
 if(PreviewClass && GetWorld())
 {FActorSpawnParameters P;P.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;P.ObjectFlags|=RF_Transient;
 Preview=GetWorld()->SpawnActor<AAltaiInventoryPreview>(PreviewClass,FVector(0,0,-50000),FRotator::ZeroRotator,P);InitialCapture=true;}
 ShowInventory();
}
void UAltaiInventoryScreen::NativeDestruct()
{if(Preview){Preview->Destroy();Preview=nullptr;}Super::NativeDestruct();}
void UAltaiInventoryScreen::NativeTick(const FGeometry& G,float Delta)
{Super::NativeTick(G,Delta);if(InitialCapture && Preview){Preview->RefreshPreview();InitialCapture=false;}}
void UAltaiInventoryScreen::RotatePreview(float Degrees){if(Preview && Pages->GetActiveWidgetIndex()==0)Preview->RotateModel(Degrees);}
void UAltaiInventoryScreen::ShowInventory(){CloseContext();Pages->SetActiveWidgetIndex(0);State(InventoryTab,true);State(MapTab,false);if(Preview)Preview->RefreshPreview();}
void UAltaiInventoryScreen::ShowMap(){CloseContext();Pages->SetActiveWidgetIndex(1);State(InventoryTab,false);State(MapTab,true);}
FReply UAltaiInventoryScreen::NativeOnKeyDown(const FGeometry& G,const FKeyEvent& E)
{
 if(E.GetKey()==EKeys::Escape && UWidgetBlueprintLibrary::IsDragDropping()){UWidgetBlueprintLibrary::CancelDragDrop();EndItemDrag();return FReply::Handled();}
 if(E.GetKey()==EKeys::Escape && ContextOpen){CloseContext();SetKeyboardFocus();return FReply::Handled();}
 if(ContextOpen)return Super::NativeOnKeyDown(G,E);
 if(E.GetKey()==EKeys::R){CycleSort();return FReply::Handled();}
 if(E.GetKey()==EKeys::M){if(Pages->GetActiveWidgetIndex()==0)ShowMap();else ShowInventory();return FReply::Handled();}
 if(E.GetKey()==EKeys::Left || E.GetKey()==EKeys::Right){RotatePreview(E.GetKey()==EKeys::Left?-15:15);return FReply::Handled();}
 return Super::NativeOnKeyDown(G,E);
}
void UAltaiInventoryScreen::SetFilter(int32 Value){CloseContext();Filter=Value;Refresh();if(ItemScroll)ItemScroll->ScrollToStart();}
void UAltaiInventoryScreen::All(){SetFilter(-1);}void UAltaiInventoryScreen::Materials(){SetFilter(0);}void UAltaiInventoryScreen::Equipment(){SetFilter(1);}void UAltaiInventoryScreen::Tools(){SetFilter(2);}
void UAltaiInventoryScreen::Refresh()
{
 auto* S=UAltaiSession::Find(this);if(!S || !ItemGrid || !ItemCellClass)return;
 const auto& Items=S->GetItems();TArray<int32> Visible;
 for(int32 I=0;I<Items.Num();++I)if(Filter<0 || static_cast<int32>(Items[I].Category)==Filter)Visible.Add(I);
 if(!Visible.ContainsByPredicate([&](int32 I){return Items[I].Id==SelectedId;}))SelectedId=Visible.IsEmpty()?NAME_None:Items[Visible[0]].Id;
 AltaiSortInventory(Items,Visible,SortMode);
 ItemGrid->ClearChildren();
 for(int32 N=0;N<Visible.Num();++N)
 {
  auto* Cell=CreateWidget<UAltaiItemCell>(GetOwningPlayer(),ItemCellClass);Cell->OwnerScreen=this;
  const FAltaiItem* Item=Visible.IsValidIndex(N)?&Items[Visible[N]]:nullptr;Cell->ItemIndex=Item?Visible[N]:INDEX_NONE;Cell->ItemId=Item?Item->Id:NAME_None;
  Cell->ItemName->SetText(Item?Item->Name:FText::GetEmpty());Cell->ItemQuantity->SetText(Item?FText::AsNumber(Item->Quantity):FText::GetEmpty());Icon(Cell->ItemIcon,Item);
  Cell->SelectButton->SetIsEnabled(Item!=nullptr);State(Cell->SelectButton,Item && Item->Id==SelectedId);
  auto* Slot=ItemGrid->AddChildToUniformGrid(Cell,N/4,N%4);Slot->SetHorizontalAlignment(HAlign_Fill);Slot->SetVerticalAlignment(VAlign_Fill);
 }
 // Preserve four equal columns even when a filter contains only one or two items.
 if(Visible.Num()>0 && Visible.Num()<4)
 {
  auto* Spacer=WidgetTree->ConstructWidget<USpacer>();
  Spacer->SetVisibility(ESlateVisibility::Hidden);
  ItemGrid->AddChildToUniformGrid(Spacer,0,3);
 }
 UButton* Filters[]={FilterAll,FilterMaterials,FilterEquipment,FilterTools};for(int32 I=0;I<4;++I)State(Filters[I],Filter==I-1);
 if(SortLabel){const TCHAR* Names[]={TEXT("Сортировка: тип"),TEXT("Сортировка: имя"),TEXT("Сортировка: количество")};SortLabel->SetText(FText::FromString(Names[SortMode]));}
 StatusText->SetText(FText::FromString(FString::Printf(TEXT("В сумке %d   •   Показано %d"),Items.Num(),Visible.Num())));UpdateDetails();
}
void UAltaiInventoryScreen::SelectItem(int32 Index)
{if(auto* S=UAltaiSession::Find(this))if(S->GetItems().IsValidIndex(Index))SelectId(S->GetItems()[Index].Id);}
void UAltaiInventoryScreen::SelectId(FName Id)
{if(auto* S=UAltaiSession::Find(this))if(S->GetItems().ContainsByPredicate([&](const FAltaiItem& I){return I.Id==Id;})){SelectedId=Id;UpdateSelection();UpdateDetails();}}
void UAltaiInventoryScreen::UpdateSelection()
{if(ItemGrid)for(auto* W:ItemGrid->GetAllChildren())if(auto* Cell=Cast<UAltaiItemCell>(W))State(Cell->SelectButton,Cell->ItemId==SelectedId);}
void UAltaiInventoryScreen::CycleSort(){CloseContext();SortMode=(SortMode+1)%3;Refresh();if(ItemScroll)ItemScroll->ScrollToStart();}

EAltaiEquipSlot UAltaiInventoryScreen::SuggestedSlot(const FAltaiItem& Item) const
{
 if(Item.WearSlot!=EAltaiEquipSlot::None)return Item.WearSlot;
 if(Item.QuickAccess)
 {if(auto* S=UAltaiSession::Find(this)){auto L=S->GetLoadout();if(!L.ContainsByPredicate([](const FAltaiLoadoutEntry& E){return E.Slot==EAltaiEquipSlot::Quick1;}))return EAltaiEquipSlot::Quick1;}
 return EAltaiEquipSlot::Quick2;}
 return EAltaiEquipSlot::None;
}
void UAltaiInventoryScreen::UpdateDetails()
{
 auto* S=UAltaiSession::Find(this);const auto* Item=FindItem(S,SelectedId);const auto Loadout=S?S->GetLoadout():TArray<FAltaiLoadoutEntry>();
 SelectedName->SetText(Item?Item->Name:FText::FromString(TEXT("Нет предметов")));SelectedCategory->SetText(Item?AltaiCategoryLabel(Item->Category):FText::GetEmpty());Icon(SelectedIcon,Item);
 DetailText->SetText(Item?FText::FromString(FString::Printf(TEXT("%s\n\nВ сумке: %d"),*Item->Description.ToString(),Item->Quantity)):FText::FromString(TEXT("В этой категории пока пусто.")));
 const auto Suggested=Item?SuggestedSlot(*Item):EAltaiEquipSlot::None;
 AssignButton->SetIsEnabled(Suggested!=EAltaiEquipSlot::None);AssignButton->SetVisibility(Suggested!=EAltaiEquipSlot::None?ESlateVisibility::Visible:ESlateVisibility::Collapsed);
 AssignLabel->SetText(FText::FromString(Suggested==EAltaiEquipSlot::Quick1?TEXT("В быстрый слот 1"):Suggested==EAltaiEquipSlot::Quick2?TEXT("В быстрый слот 2"):Suggested!=EAltaiEquipSlot::None?TEXT("Надеть / взять"):TEXT("Не экипируется")));
 const bool Assigned=Item && Loadout.ContainsByPredicate([&](const FAltaiLoadoutEntry& E){return E.ItemId==SelectedId;});
 UnassignButton->SetIsEnabled(Assigned);UnassignButton->SetVisibility(Assigned?ESlateVisibility::Visible:ESlateVisibility::Collapsed);
 if(InteractionHint && !DraggingItem)InteractionHint->SetText(FText::FromString(Item && Suggested!=EAltaiEquipSlot::None?TEXT("Перетащите в ячейку • ПКМ — действия"):TEXT("ПКМ — сведения • R — сортировка")));
 for(auto* Cell:{HeadSlot.Get(),BodySlot.Get(),HandSlot.Get(),QuickSlot1.Get(),QuickSlot2.Get()})
 {
  const auto* Entry=Loadout.FindByPredicate([&](const FAltaiLoadoutEntry& E){return E.Slot==Cell->EquipmentSlot;});const auto* Equipped=Entry?FindItem(S,Entry->ItemId):nullptr;
  Cell->EquippedName->SetText(Equipped?Equipped->Name:FText::FromString(TEXT("—")));
  Icon(Cell->EquippedIcon,Equipped);
  if(Cell->EquippedQuantity)Cell->EquippedQuantity->SetText(Equipped?FText::AsNumber(Equipped->Quantity):FText::GetEmpty());
  if(Cell->EquippedName)Cell->EquippedName->SetVisibility(Equipped?ESlateVisibility::Collapsed:ESlateVisibility::HitTestInvisible);
  State(Cell->SelectButton,Item && Item->CanAssign(Cell->EquipmentSlot));
 }
}
void UAltaiInventoryScreen::AssignSelected(){if(auto* S=UAltaiSession::Find(this))if(const auto* Item=FindItem(S,SelectedId)){S->AssignItem(SelectedId,SuggestedSlot(*Item));UpdateSelection();UpdateDetails();}}
void UAltaiInventoryScreen::UnassignSelected()
{if(auto* S=UAltaiSession::Find(this)){for(const auto& E:S->GetLoadout())if(E.ItemId==SelectedId)S->ClearSlot(E.Slot);UpdateSelection();UpdateDetails();}}
void UAltaiInventoryScreen::ClickEquipment(EAltaiEquipSlot Slot,bool Clear)
{
 auto* S=UAltaiSession::Find(this);if(!S)return;
 if(Clear)S->ClearSlot(Slot);
 else if(const auto* Item=FindItem(S,SelectedId);Item && Item->CanAssign(Slot))S->AssignItem(SelectedId,Slot);
 else for(const auto& E:S->GetLoadout())if(E.Slot==Slot){SelectedId=E.ItemId;Filter=-1;}
 Refresh();
}
