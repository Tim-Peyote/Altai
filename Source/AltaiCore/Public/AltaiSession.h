#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Engine/DataAsset.h"
#include "AltaiSession.generated.h"

UENUM(BlueprintType)
enum class EAltaiItemCategory : uint8 { Material, Equipment, Tool };
UENUM(BlueprintType)
enum class EAltaiEquipSlot : uint8 { None, Head, Body, Hand, Quick1, Quick2 };

USTRUCT(BlueprintType)
struct ALTAICORE_API FAltaiLoadoutEntry
{
 GENERATED_BODY()
 UPROPERTY(EditAnywhere, BlueprintReadWrite) EAltaiEquipSlot Slot=EAltaiEquipSlot::None;
 UPROPERTY(EditAnywhere, BlueprintReadWrite) FName ItemId;
};

USTRUCT(BlueprintType)
struct ALTAICORE_API FAltaiItem
{
 GENERATED_BODY()
 UPROPERTY(EditAnywhere, BlueprintReadWrite) FName Id;
 UPROPERTY(EditAnywhere, BlueprintReadWrite) FText Name;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(MultiLine=true)) FText Description;
 UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 Quantity = 1;
 UPROPERTY(EditAnywhere, BlueprintReadWrite) EAltaiItemCategory Category=EAltaiItemCategory::Material;
 UPROPERTY(EditAnywhere, BlueprintReadWrite) EAltaiEquipSlot WearSlot=EAltaiEquipSlot::None;
 UPROPERTY(EditAnywhere, BlueprintReadWrite) bool QuickAccess=false;
 UPROPERTY(EditAnywhere, BlueprintReadWrite) TSoftObjectPtr<UTexture2D> Icon;
 bool CanAssign(EAltaiEquipSlot Slot) const
 {return Slot!=EAltaiEquipSlot::None && (Slot==EAltaiEquipSlot::Quick1 || Slot==EAltaiEquipSlot::Quick2 ? QuickAccess : Slot==WearSlot);}

};

USTRUCT(BlueprintType)
struct ALTAICORE_API FAltaiProfileInfo
{
 GENERATED_BODY()
 UPROPERTY(BlueprintReadOnly) FString Id;
 UPROPERTY(BlueprintReadOnly) FString Name;
 UPROPERTY(BlueprintReadOnly) FDateTime SavedAt;
 UPROPERTY(BlueprintReadOnly) double PlaySeconds = 0;
 UPROPERTY(BlueprintReadOnly) bool Recovered = false;
};

UCLASS(BlueprintType)
class ALTAICORE_API UAltaiInventoryPreset : public UDataAsset
{
 GENERATED_BODY()
public:
 UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FAltaiItem> Items;
};

// UI depends on this contract; the gameplay module owns its implementation.
UCLASS(Abstract)
class ALTAICORE_API UAltaiSession : public UGameInstanceSubsystem
{
 GENERATED_BODY()
public:
 static UAltaiSession* Find(const UObject* Context);
 UFUNCTION(BlueprintPure, Category="Altai")
 virtual bool HasSave() const { return false; }
 UFUNCTION(BlueprintCallable, Category="Altai")
 virtual bool NewExpedition() { return false; }
 UFUNCTION(BlueprintCallable, Category="Altai")
 virtual bool CreateProfile(const FString& Name) { return false; }
 UFUNCTION(BlueprintCallable, Category="Altai")
 virtual bool LoadProfile(const FString& Id) { return false; }
 UFUNCTION(BlueprintCallable, Category="Altai")
 virtual TArray<FAltaiProfileInfo> GetProfiles() { return {}; }
 UFUNCTION(BlueprintPure, Category="Altai")
 virtual FString GetActiveProfileName() const { return {}; }
 UFUNCTION(BlueprintPure, Category="Altai")
 virtual FString GetSaveStatus() const { return {}; }
 virtual uint64 GetSaveRevision() const { return 0; }
 virtual bool HasSaveError() const { return false; }
 UFUNCTION(BlueprintCallable, Category="Altai")
 virtual bool ContinueExpedition() { return false; }
 UFUNCTION(BlueprintCallable, Category="Altai")
 virtual bool SaveExpedition() { return false; }
 virtual void WorldReady() {}
 UFUNCTION(BlueprintCallable, Category="Altai")
 virtual void ReturnToMenu() {}
 UFUNCTION(BlueprintPure, Category="Altai")
 virtual const TArray<FAltaiItem>& GetItems() const { return Empty; }
 UFUNCTION(BlueprintCallable, Category="Altai")
 virtual bool RemoveOne(int32 Index) { return false; }
 UFUNCTION(BlueprintCallable, Category="Altai")
 virtual bool AddItem(const FAltaiItem& Item) { return false; }
 UFUNCTION(BlueprintPure, Category="Altai")
 virtual bool IsExpedition() const { return false; }
 UFUNCTION(BlueprintCallable, Category="Altai")
 virtual bool AssignItem(FName ItemId,EAltaiEquipSlot Slot) { return false; }
 UFUNCTION(BlueprintCallable, Category="Altai")
 virtual bool ClearSlot(EAltaiEquipSlot Slot) { return false; }
 UFUNCTION(BlueprintCallable, Category="Altai")
 virtual bool MoveAssignment(FName ItemId,EAltaiEquipSlot Source,EAltaiEquipSlot Target) { return false; }
 UFUNCTION(BlueprintPure, Category="Altai")
 virtual TArray<FAltaiLoadoutEntry> GetLoadout() const { return {}; }

private:
 TArray<FAltaiItem> Empty;
};
