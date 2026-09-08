#include "AltaiInventoryScreen.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/Border.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ScrollBox.h"
#include "Components/UniformGridPanel.h"
#include "InputCoreTypes.h"

FReply UAltaiItemCell::NativeOnPreviewMouseButtonDown(const FGeometry& G,const FPointerEvent& E)
{
 if(auto* Screen=Cast<UAltaiInventoryScreen>(OwnerScreen))
 {
  if(E.GetEffectingButton()==EKeys::RightMouseButton){Screen->OpenContext(ItemId,E.GetScreenSpacePosition());return FReply::Handled();}
  if(E.GetEffectingButton()==EKeys::LeftMouseButton)return FReply::Handled().CaptureMouse(TakeWidget()).DetectDrag(TakeWidget(),EKeys::LeftMouseButton);
 }
 return Super::NativeOnPreviewMouseButtonDown(G,E);
}
FReply UAltaiItemCell::NativeOnMouseButtonUp(const FGeometry& G,const FPointerEvent& E)
{
 if(E.GetEffectingButton()==EKeys::LeftMouseButton && Cast<UAltaiInventoryScreen>(OwnerScreen))
 {if(G.IsUnderLocation(E.GetScreenSpacePosition()))Cast<UAltaiInventoryScreen>(OwnerScreen)->SelectId(ItemId);return FReply::Handled().ReleaseMouseCapture();}
 return Super::NativeOnMouseButtonUp(G,E);
}
void UAltaiItemCell::NativeOnDragDetected(const FGeometry& G,const FPointerEvent& E,UDragDropOperation*& Operation)
{if(auto* Screen=Cast<UAltaiInventoryScreen>(OwnerScreen))Operation=Screen->BeginItemDrag(ItemId);}
void UAltaiItemCell::NativeOnDragCancelled(const FDragDropEvent& E,UDragDropOperation* Operation)
{if(auto* Screen=Cast<UAltaiInventoryScreen>(OwnerScreen))Screen->EndItemDrag();}
FReply UAltaiEquipmentCell::NativeOnPreviewMouseButtonDown(const FGeometry& G,const FPointerEvent& E)
{
 if(OwnerInventory)
 {
  if(E.GetEffectingButton()==EKeys::RightMouseButton){OwnerInventory->OpenContext(OwnerInventory->EquippedId(EquipmentSlot),E.GetScreenSpacePosition());return FReply::Handled();}
  if(E.GetEffectingButton()==EKeys::LeftMouseButton)return FReply::Handled().CaptureMouse(TakeWidget()).DetectDrag(TakeWidget(),EKeys::LeftMouseButton);
 }
 return Super::NativeOnPreviewMouseButtonDown(G,E);
}
FReply UAltaiEquipmentCell::NativeOnMouseButtonUp(const FGeometry& G,const FPointerEvent& E)
{
 if(E.GetEffectingButton()==EKeys::LeftMouseButton){if(G.IsUnderLocation(E.GetScreenSpacePosition()))SelectSlot();return FReply::Handled().ReleaseMouseCapture();}
 return Super::NativeOnMouseButtonUp(G,E);
}
void UAltaiEquipmentCell::NativeOnDragDetected(const FGeometry& G,const FPointerEvent& E,UDragDropOperation*& Operation)
{if(OwnerInventory)Operation=OwnerInventory->BeginItemDrag(OwnerInventory->EquippedId(EquipmentSlot),EquipmentSlot);}
void UAltaiEquipmentCell::NativeOnDragCancelled(const FDragDropEvent& E,UDragDropOperation* Operation)
{if(OwnerInventory)OwnerInventory->EndItemDrag();}
bool UAltaiEquipmentCell::NativeOnDragOver(const FGeometry& G,const FDragDropEvent& E,UDragDropOperation* Operation)
{
 auto* Drag=Cast<UAltaiItemDrag>(Operation);if(!OwnerInventory || !Drag || Drag->Inventory!=OwnerInventory)return false;
 OwnerInventory->SetDragTarget(EquipmentSlot,OwnerInventory->CanDropItem(Drag,EquipmentSlot));return true;
}
void UAltaiEquipmentCell::NativeOnDragLeave(const FDragDropEvent& E,UDragDropOperation* Operation)
{if(OwnerInventory)OwnerInventory->SetDragTarget(EAltaiEquipSlot::None,true);}
bool UAltaiEquipmentCell::NativeOnDrop(const FGeometry& G,const FDragDropEvent& E,UDragDropOperation* Operation)
{auto* Drag=Cast<UAltaiItemDrag>(Operation);if(!OwnerInventory || !Drag || Drag->Inventory!=OwnerInventory)return false;OwnerInventory->DropItem(Drag,EquipmentSlot);return true;}

FName UAltaiInventoryScreen::EquippedId(EAltaiEquipSlot Slot) const
{if(auto* S=UAltaiSession::Find(this))for(const auto& E:S->GetLoadout())if(E.Slot==Slot)return E.ItemId;return NAME_None;}
UDragDropOperation* UAltaiInventoryScreen::BeginItemDrag(FName Id,EAltaiEquipSlot Source)
{
 auto* S=UAltaiSession::Find(this);if(!S)return nullptr;
 const auto* Item=S->GetItems().FindByPredicate([&](const FAltaiItem& I){return I.Id==Id;});if(!Item || (Source!=EAltaiEquipSlot::None && EquippedId(Source)!=Id))return nullptr;
 CloseContext();SelectId(Id);DraggingItem=true;
 auto* Drag=NewObject<UAltaiItemDrag>();Drag->ItemId=Id;Drag->SourceSlot=Source;Drag->Inventory=this;Drag->Pivot=EDragPivot::CenterCenter;
 auto* Ghost=CreateWidget<UAltaiItemCell>(GetOwningPlayer(),ItemCellClass);Ghost->ItemName->SetText(Item->Name);Ghost->ItemQuantity->SetText(FText::AsNumber(Item->Quantity));
 if(Ghost->ItemIcon)Ghost->ItemIcon->SetBrushFromTexture(Item->Icon.LoadSynchronous());Ghost->SetVisibility(ESlateVisibility::HitTestInvisible);Ghost->SetRenderOpacity(.85f);Drag->DefaultDragVisual=Ghost;
 if(InteractionHint)InteractionHint->SetText(FText::FromString(TEXT("Перетащите в подсвеченную ячейку. Из ячейки в сумку — снять. Esc — отменить.")));
 return Drag;
}
bool UAltaiInventoryScreen::CanDropItem(const UAltaiItemDrag* Drag,EAltaiEquipSlot Target) const
{
 if(!Drag || Drag->Inventory!=this)return false;auto* S=UAltaiSession::Find(this);if(!S)return false;
 const auto* Item=S->GetItems().FindByPredicate([&](const FAltaiItem& I){return I.Id==Drag->ItemId;});if(!Item)return false;
 if(Drag->SourceSlot!=EAltaiEquipSlot::None && EquippedId(Drag->SourceSlot)!=Drag->ItemId)return false;
 if(Target==EAltaiEquipSlot::None)return Drag->SourceSlot!=EAltaiEquipSlot::None;
 if(!Item->CanAssign(Target))return false;
 const FName Occupant=EquippedId(Target);
 if(Drag->SourceSlot!=EAltaiEquipSlot::None && Occupant!=NAME_None && Occupant!=Drag->ItemId)
 {const auto* Other=S->GetItems().FindByPredicate([&](const FAltaiItem& I){return I.Id==Occupant;});if(!Other || !Other->CanAssign(Drag->SourceSlot))return false;}
 return true;
}
bool UAltaiInventoryScreen::DropItem(UAltaiItemDrag* Drag,EAltaiEquipSlot Target)
{
 const bool Allowed=CanDropItem(Drag,Target);bool Changed=false;
 if(Allowed)if(auto* S=UAltaiSession::Find(this))Changed=Drag->SourceSlot==EAltaiEquipSlot::None?S->AssignItem(Drag->ItemId,Target):S->MoveAssignment(Drag->ItemId,Drag->SourceSlot,Target);
 EndItemDrag();
 if(!Changed && InteractionHint)InteractionHint->SetText(FText::FromString(TEXT("Эта ячейка не подходит. Предмет остался на месте.")));
 return Changed;
}
bool UAltaiInventoryScreen::NativeOnDrop(const FGeometry& G,const FDragDropEvent& E,UDragDropOperation* Operation)
{
 if(ItemScroll && ItemScroll->GetCachedGeometry().IsUnderLocation(E.GetScreenSpacePosition()))
 {if(auto* Drag=Cast<UAltaiItemDrag>(Operation);Drag && Drag->Inventory==this){DropItem(Drag,EAltaiEquipSlot::None);return true;}}
 return false;
}
bool UAltaiInventoryScreen::NativeOnDragOver(const FGeometry& G,const FDragDropEvent& E,UDragDropOperation* Operation)
{auto* Drag=Cast<UAltaiItemDrag>(Operation);return Drag && Drag->Inventory==this && ItemScroll && ItemScroll->GetCachedGeometry().IsUnderLocation(E.GetScreenSpacePosition());}
void UAltaiInventoryScreen::SetDragTarget(EAltaiEquipSlot Slot,bool Allowed)
{
 UpdateDetails();
 for(auto* Cell:{HeadSlot.Get(),BodySlot.Get(),HandSlot.Get(),QuickSlot1.Get(),QuickSlot2.Get()})if(Cell->EquipmentSlot==Slot)
 {auto Style=Cell->SelectButton->GetStyle();Style.Normal.OutlineSettings.Color=FSlateColor(Allowed?FLinearColor(.55,.48,.27,1):FLinearColor(.45,.08,.06,1));Style.Normal.OutlineSettings.Width=1.6f;Cell->SelectButton->SetStyle(Style);}
}
void UAltaiInventoryScreen::EndItemDrag(){DraggingItem=false;UpdateSelection();UpdateDetails();}
void UAltaiInventoryScreen::OpenContext(FName Id,FVector2D Position)
{
 auto* S=UAltaiSession::Find(this);const auto* Item=S?S->GetItems().FindByPredicate([&](const FAltaiItem& I){return I.Id==Id;}):nullptr;
 if(!Item || !ContextPanel)return;
 SelectId(Id);ContextId=Id;ContextOpen=true;ContextPanel->SetVisibility(ESlateVisibility::Visible);if(ContextDismiss)ContextDismiss->SetVisibility(ESlateVisibility::Visible);
 auto Show=[](UWidget* W,bool B){if(W)W->SetVisibility(B?ESlateVisibility::Visible:ESlateVisibility::Collapsed);};
 Show(ContextEquip,Item->WearSlot!=EAltaiEquipSlot::None);Show(ContextQuick1,Item->QuickAccess);Show(ContextQuick2,Item->QuickAccess);
 bool Assigned=false;for(const auto& E:S->GetLoadout())Assigned|=E.ItemId==Id;Show(ContextRemove,Assigned);Show(ContextEmpty,!Assigned && !Item->QuickAccess && Item->WearSlot==EAltaiEquipSlot::None);
 if(ContextTitle)ContextTitle->SetText(Item->Name);
 if(auto* Slot=Cast<UCanvasPanelSlot>(ContextPanel->Slot))
 {const auto G=GetCachedGeometry();FVector2D P=G.AbsoluteToLocal(Position);P.X=FMath::Clamp(P.X,16.,FMath::Max(16.,G.GetLocalSize().X-296.));P.Y=FMath::Clamp(P.Y,16.,FMath::Max(16.,G.GetLocalSize().Y-320.));Slot->SetPosition(P);}
 if(ContextClose)ContextClose->SetKeyboardFocus();
}
void UAltaiInventoryScreen::CloseContext(){ContextOpen=false;ContextId=NAME_None;if(ContextPanel)ContextPanel->SetVisibility(ESlateVisibility::Collapsed);if(ContextDismiss)ContextDismiss->SetVisibility(ESlateVisibility::Collapsed);}
void UAltaiInventoryScreen::ContextAssign(EAltaiEquipSlot Slot){if(auto* S=UAltaiSession::Find(this))S->AssignItem(ContextId,Slot);CloseContext();UpdateSelection();UpdateDetails();SetKeyboardFocus();}
void UAltaiInventoryScreen::ContextAssignWear(){if(auto* S=UAltaiSession::Find(this))if(const auto* I=S->GetItems().FindByPredicate([&](const FAltaiItem& X){return X.Id==ContextId;}))ContextAssign(I->WearSlot);}
void UAltaiInventoryScreen::ContextAssignQuick1(){ContextAssign(EAltaiEquipSlot::Quick1);}
void UAltaiInventoryScreen::ContextAssignQuick2(){ContextAssign(EAltaiEquipSlot::Quick2);}
void UAltaiInventoryScreen::ContextUnassign(){SelectedId=ContextId;UnassignSelected();CloseContext();SetKeyboardFocus();}
