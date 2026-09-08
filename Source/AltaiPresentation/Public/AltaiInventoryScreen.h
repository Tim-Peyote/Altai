#pragma once
#include "CoreMinimal.h"
#include "AltaiScreens.h"
#include "AltaiSession.h"
#include "Blueprint/DragDropOperation.h"
#include "AltaiInventoryScreen.generated.h"
class UWidgetSwitcher;
class UScrollBox;
class UAltaiInventoryScreen;
class AAltaiInventoryPreview;
class UBorder;
UCLASS()
class ALTAIPRESENTATION_API UAltaiItemDrag : public UDragDropOperation
{
 GENERATED_BODY()
public:
 UPROPERTY() FName ItemId;
 UPROPERTY() EAltaiEquipSlot SourceSlot=EAltaiEquipSlot::None;
 UPROPERTY() TObjectPtr<UAltaiInventoryScreen> Inventory;
};

UCLASS()
class ALTAIPRESENTATION_API UAltaiPreviewSurface : public UUserWidget
{
 GENERATED_BODY()
public:
 UPROPERTY(meta=(BindWidget)) TObjectPtr<UImage> PreviewImage;
 UPROPERTY() TObjectPtr<UAltaiInventoryScreen> OwnerInventory;
 virtual FReply NativeOnMouseButtonDown(const FGeometry& G,const FPointerEvent& E) override;
 virtual FReply NativeOnMouseButtonUp(const FGeometry& G,const FPointerEvent& E) override;
 virtual FReply NativeOnMouseMove(const FGeometry& G,const FPointerEvent& E) override;
private:
 bool Rotating=false;
};

UCLASS()
class ALTAIPRESENTATION_API UAltaiEquipmentCell : public UUserWidget
{
 GENERATED_BODY()
public:
 UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton> SelectButton;
 UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> SlotName;
 UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> EquippedName;
 UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UImage> EquippedIcon;
 UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> EquippedQuantity;
 UPROPERTY(EditAnywhere,BlueprintReadWrite) EAltaiEquipSlot EquipmentSlot=EAltaiEquipSlot::None;
 UPROPERTY() TObjectPtr<UAltaiInventoryScreen> OwnerInventory;
 virtual void NativeOnInitialized() override;
 virtual FReply NativeOnMouseButtonDown(const FGeometry& G,const FPointerEvent& E) override;
 virtual FReply NativeOnPreviewMouseButtonDown(const FGeometry& G,const FPointerEvent& E) override;
 virtual FReply NativeOnMouseButtonUp(const FGeometry& G,const FPointerEvent& E) override;
 virtual void NativeOnDragDetected(const FGeometry& G,const FPointerEvent& E,UDragDropOperation*& Operation) override;
 virtual void NativeOnDragCancelled(const FDragDropEvent& E,UDragDropOperation* Operation) override;
 virtual bool NativeOnDragOver(const FGeometry& G,const FDragDropEvent& E,UDragDropOperation* Operation) override;
 virtual void NativeOnDragLeave(const FDragDropEvent& E,UDragDropOperation* Operation) override;
 virtual bool NativeOnDrop(const FGeometry& G,const FDragDropEvent& E,UDragDropOperation* Operation) override;
 UFUNCTION() void SelectSlot();
};

UCLASS()
class ALTAIPRESENTATION_API UAltaiInventoryScreen : public UAltaiScreen
{
 GENERATED_BODY()
public:
 UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UScrollBox> ItemScroll;
 UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UButton> SortButton;
 UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> SortLabel;
 UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> InteractionHint;
 UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UBorder> ContextPanel;
 UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> ContextTitle;
 UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UButton> ContextEquip;
 UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UButton> ContextQuick1;
 UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UButton> ContextQuick2;
 UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UButton> ContextRemove;
 UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UButton> ContextClose;
 UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UButton> ContextDismiss;
 UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> ContextEmpty;

 UPROPERTY(EditDefaultsOnly) TSubclassOf<AAltaiInventoryPreview> PreviewClass;
 UPROPERTY(meta=(BindWidget)) TObjectPtr<UAltaiPreviewSurface> PreviewSurface;
 UPROPERTY(meta=(BindWidget)) TObjectPtr<UWidgetSwitcher> Pages;
 UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton> InventoryTab;
 UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton> MapTab;
 UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton> FilterAll;
 UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton> FilterMaterials;
 UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton> FilterEquipment;
 UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton> FilterTools;
 UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton> AssignButton;
 UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton> UnassignButton;
 UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> AssignLabel;
 UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> SelectedName;
 UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> SelectedCategory;
 UPROPERTY(meta=(BindWidget)) TObjectPtr<UImage> SelectedIcon;
 UPROPERTY(meta=(BindWidget)) TObjectPtr<UAltaiEquipmentCell> HeadSlot;
 UPROPERTY(meta=(BindWidget)) TObjectPtr<UAltaiEquipmentCell> BodySlot;
 UPROPERTY(meta=(BindWidget)) TObjectPtr<UAltaiEquipmentCell> HandSlot;
 UPROPERTY(meta=(BindWidget)) TObjectPtr<UAltaiEquipmentCell> QuickSlot1;
 UPROPERTY(meta=(BindWidget)) TObjectPtr<UAltaiEquipmentCell> QuickSlot2;
 virtual void NativeOnInitialized() override;
 virtual void NativeConstruct() override;
 virtual void NativeDestruct() override;
 virtual void NativeTick(const FGeometry& Geometry,float Delta) override;
 virtual FReply NativeOnKeyDown(const FGeometry& G,const FKeyEvent& E) override;
 virtual void Refresh() override;
 virtual void SelectItem(int32 Index) override;
 virtual bool NativeOnDrop(const FGeometry& G,const FDragDropEvent& E,UDragDropOperation* Operation) override;
 virtual bool NativeOnDragOver(const FGeometry& G,const FDragDropEvent& E,UDragDropOperation* Operation) override;
 UDragDropOperation* BeginItemDrag(FName Id,EAltaiEquipSlot Source=EAltaiEquipSlot::None);
 bool CanDropItem(const UAltaiItemDrag* Drag,EAltaiEquipSlot Target) const;
 bool DropItem(UAltaiItemDrag* Drag,EAltaiEquipSlot Target);
 FName EquippedId(EAltaiEquipSlot Slot) const;
 void SetDragTarget(EAltaiEquipSlot Slot,bool Allowed);
 void EndItemDrag();
 void SelectId(FName Id);
 void OpenContext(FName Id,FVector2D Position);
 UFUNCTION() void CloseContext();
 UFUNCTION() void CycleSort();
 UFUNCTION() void ContextAssignWear();
 UFUNCTION() void ContextAssignQuick1();
 UFUNCTION() void ContextAssignQuick2();
 UFUNCTION() void ContextUnassign();
 void RotatePreview(float Degrees);
 void ClickEquipment(EAltaiEquipSlot Slot,bool Clear=false);
 UFUNCTION() void ShowInventory();
 UFUNCTION() void ShowMap();
 UFUNCTION() void All();
 UFUNCTION() void Materials();
 UFUNCTION() void Equipment();
 UFUNCTION() void Tools();
 UFUNCTION() void AssignSelected();
 UFUNCTION() void UnassignSelected();
private:
 UPROPERTY() TObjectPtr<AAltaiInventoryPreview> Preview;
 FName SelectedId;
 int32 Filter=-1;
 int32 SortMode=0;
 bool ContextOpen=false;
 bool DraggingItem=false;
 FName ContextId;
 void UpdateSelection();
 void ContextAssign(EAltaiEquipSlot Slot);
 bool InitialCapture=false;
 void SetFilter(int32 Value);
 void UpdateDetails();
 EAltaiEquipSlot SuggestedSlot(const FAltaiItem& Item) const;
};
