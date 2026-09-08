#pragma once
#include "CoreMinimal.h"
#include "AltaiSession.h"
#include "AltaiProfileStore.h"
#include "AltaiExpedition.generated.h"

UCLASS()
class ALTAIGAMEPLAY_API UAltaiExpedition : public UAltaiSession
{
 GENERATED_BODY()
public:
 UAltaiExpedition();
 UPROPERTY(EditDefaultsOnly, Category="Inventory") TSoftObjectPtr<UAltaiInventoryPreset> StartingInventory;
 virtual bool HasSave() const override;
 virtual bool NewExpedition() override;
 virtual bool CreateProfile(const FString& Name) override;
 virtual bool LoadProfile(const FString& Id) override;
 virtual TArray<FAltaiProfileInfo> GetProfiles() override;
 virtual bool ContinueExpedition() override;
 virtual bool SaveExpedition() override;
 virtual void WorldReady() override;
 virtual void ReturnToMenu() override;
 virtual const TArray<FAltaiItem>& GetItems() const override { return Items; }
 virtual bool RemoveOne(int32 Index) override;
 virtual bool AddItem(const FAltaiItem& Item) override;
 virtual bool IsExpedition() const override;
 virtual FString GetActiveProfileName() const override { return ActiveName; }
 virtual FString GetSaveStatus() const override { return SaveStatus; }
 virtual uint64 GetSaveRevision() const override { return SaveRevision; }
 virtual bool HasSaveError() const override { return bSaveError; }
 virtual bool AssignItem(FName ItemId,EAltaiEquipSlot Slot) override;
 virtual bool ClearSlot(EAltaiEquipSlot Slot) override;
 virtual bool MoveAssignment(FName ItemId,EAltaiEquipSlot Source,EAltaiEquipSlot Target) override;
 virtual TArray<FAltaiLoadoutEntry> GetLoadout() const override { return Loadout; }
private:
 UPROPERTY() TArray<FAltaiLoadoutEntry> Loadout;
 uint64 SaveRevision=0;
 bool bSaveError=false;
 UPROPERTY() TArray<FAltaiItem> Items;
 UPROPERTY() TObjectPtr<UAltaiSave> PendingSave;
 FString ActiveId;
 FString ActiveName;
 FString SaveStatus;
 double BasePlaySeconds=0;
 double SessionStartSeconds=0;
 bool bSessionReady=false;
 FTimerHandle AutosaveTimer;
 FAltaiProfileStore Store() const;
 void ImportLegacy() const;
 void Autosave();
};
