#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"
#include "AltaiScreens.generated.h"
class UButton;
class UImage;
class UTextBlock;
class UUniformGridPanel;
class UScrollBox;
class UEditableTextBox;
class UAltaiScreen;
class UDragDropOperation;

UENUM(BlueprintType)
enum class EAltaiScreenKind : uint8 { None, MainMenu, Pause, Inventory, Settings, ConfirmNew, LoadGame, NewGame, ConfirmExit };

UCLASS()
class ALTAIPRESENTATION_API UAltaiItemCell : public UUserWidget
{
 GENERATED_BODY()
public:
 UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton> SelectButton;
 UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> ItemName;
 UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> ItemQuantity;
 UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UImage> ItemIcon;
 UPROPERTY() TObjectPtr<UAltaiScreen> OwnerScreen;
 int32 ItemIndex=INDEX_NONE;
 FName ItemId;
 virtual FReply NativeOnPreviewMouseButtonDown(const FGeometry& G,const FPointerEvent& E) override;
 virtual FReply NativeOnMouseButtonUp(const FGeometry& G,const FPointerEvent& E) override;
 virtual void NativeOnDragDetected(const FGeometry& G,const FPointerEvent& E,UDragDropOperation*& Operation) override;
 virtual void NativeOnDragCancelled(const FDragDropEvent& E,UDragDropOperation* Operation) override;

 virtual void NativeOnInitialized() override;
 UFUNCTION() void Select();
};

UCLASS()
class ALTAIPRESENTATION_API UAltaiProfileRow : public UUserWidget
{
 GENERATED_BODY()
public:
 UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton> SelectButton;
 UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> ProfileName;
 UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> ProfileDetails;
 UPROPERTY() TObjectPtr<UAltaiScreen> OwnerScreen;
 FString ProfileId;
 virtual void NativeOnInitialized() override;
 UFUNCTION() void Select();
};

UCLASS()
class ALTAIPRESENTATION_API UAltaiHUD : public UUserWidget
{
 GENERATED_BODY()
public:
 UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> SaveNotice;
 virtual void NativeTick(const FGeometry& Geometry,float DeltaTime) override;
private:
 uint64 PreviousRevision=0;
 float NoticeAge=0;
};

UCLASS()
class ALTAIPRESENTATION_API UAltaiScreen : public UUserWidget
{
 GENERATED_BODY()
public:
 UPROPERTY(EditDefaultsOnly) EAltaiScreenKind Kind=EAltaiScreenKind::MainMenu;
 UPROPERTY(EditDefaultsOnly) TSubclassOf<UAltaiItemCell> ItemCellClass;
 UPROPERTY(EditDefaultsOnly) TSubclassOf<UAltaiProfileRow> ProfileRowClass;
 UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UScrollBox> ProfileList;
 UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UEditableTextBox> ProfileNameInput;
 UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UButton> LoadButton;
 UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UButton> PrimaryButton;
 UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UButton> SecondaryButton;
 UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UButton> SettingsButton;
 UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UButton> QuitButton;
 UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UButton> BackButton;
 UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> DetailText;
 UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> StatusText;
 UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UUniformGridPanel> ItemGrid;
 virtual void NativeOnInitialized() override;
 virtual void NativeConstruct() override;
 virtual FReply NativeOnKeyDown(const FGeometry& Geometry,const FKeyEvent& Event) override;
 virtual void SelectItem(int32 Index);
 void SelectProfile(const FString& Id);
 UFUNCTION() void OpenLoad();
 virtual void Refresh();
 UFUNCTION() void Primary();
 UFUNCTION() void Secondary();
 UFUNCTION() void Settings();
 UFUNCTION() void Quit();
 UFUNCTION() void Back();
};

UCLASS()
class ALTAIPRESENTATION_API AAltaiPlayerController : public APlayerController
{
 GENERATED_BODY()
public:
 UPROPERTY(EditDefaultsOnly, Category="Screens") TSubclassOf<UAltaiScreen> MainMenuClass;
 UPROPERTY(EditDefaultsOnly, Category="Screens") TSubclassOf<UAltaiScreen> PauseClass;
 UPROPERTY(EditDefaultsOnly, Category="Screens") TSubclassOf<UAltaiScreen> InventoryClass;
 UPROPERTY(EditDefaultsOnly, Category="Screens") TSubclassOf<UAltaiScreen> SettingsClass;
 UPROPERTY(EditDefaultsOnly, Category="Screens") TSubclassOf<UAltaiScreen> ConfirmNewClass;
 UPROPERTY(EditDefaultsOnly, Category="Screens") TSubclassOf<UAltaiScreen> LoadGameClass;
 UPROPERTY(EditDefaultsOnly, Category="Screens") TSubclassOf<UAltaiScreen> NewGameClass;
 UPROPERTY(EditDefaultsOnly, Category="Screens") TSubclassOf<UAltaiScreen> ConfirmExitClass;
 UPROPERTY(EditDefaultsOnly, Category="Screens") TSubclassOf<UUserWidget> HUDClass;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly) EAltaiScreenKind CurrentScreen=EAltaiScreenKind::None;
 UPROPERTY() TObjectPtr<UAltaiScreen> ActiveScreen;
 UPROPERTY() TObjectPtr<UUserWidget> HUDWidget;
 EAltaiScreenKind ReturnScreen=EAltaiScreenKind::MainMenu;
 virtual void BeginPlay() override;
 virtual void SetupInputComponent() override;
 UFUNCTION(BlueprintCallable, Category="Screens")
 virtual void ShowScreen(EAltaiScreenKind Kind);
 UFUNCTION(BlueprintCallable, Category="Screens")
 void GoBack();
 UFUNCTION(BlueprintCallable, Category="Screens")
 void ToggleInventory();
 UFUNCTION(BlueprintCallable, Category="Screens")
 void TogglePause();
 UFUNCTION() void ToggleMap();
protected:
 bool AutoInitializeScreens=true;
};
