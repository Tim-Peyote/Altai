#pragma once
#include "CoreMinimal.h"
#include "AltaiSession.h"
#include "GameFramework/SaveGame.h"
#include "AltaiProfileStore.generated.h"

// The original class name is retained so v1 saves remain readable.
UCLASS()
class ALTAIGAMEPLAY_API UAltaiSave : public USaveGame
{
 GENERATED_BODY()
public:
 // Keep the v1 default: legacy archives omitted this default-valued property.
 UPROPERTY() int32 Version = 1;
 UPROPERTY() FString ProfileId;
 UPROPERTY() FString ProfileName;
 UPROPERTY() FDateTime SavedAt;
 UPROPERTY() double PlaySeconds = 0;
 UPROPERTY() bool ImportedLegacy = false;
 UPROPERTY() FTransform PlayerTransform;
 UPROPERTY() FRotator ViewRotation;
 UPROPERTY() TArray<FAltaiItem> Items;
 UPROPERTY() TArray<FAltaiLoadoutEntry> Loadout;
};

UCLASS()
class ALTAIGAMEPLAY_API UAltaiProfileIndex : public USaveGame
{
 GENERATED_BODY()
public:
 UPROPERTY() FString LastPlayedId;
};

// Storage is independent of worlds, UI and the active session. Tests use an isolated directory.
class ALTAIGAMEPLAY_API FAltaiProfileStore
{
public:
 explicit FAltaiProfileStore(FString InDirectory) : Directory(MoveTemp(InDirectory)) {}
 TArray<FAltaiProfileInfo> List() const;
 UAltaiSave* Load(const FString& Id, bool* Recovered = nullptr) const;
 bool Write(UAltaiSave* Save, bool bCreate = false) const;
 FString LastPlayed() const;
 bool MarkPlayed(const FString& Id) const;
 bool ImportLegacy(const FString& LegacyFile) const;
 static bool ValidId(const FString& Id);
private:
 FString Directory;
 FString Path(const FString& Id) const;
 static USaveGame* ReadFile(const FString& File);
 static bool WriteFile(USaveGame* Data, const FString& File);
 static bool ValidSave(const UAltaiSave* Save, const FString& Id);
};
