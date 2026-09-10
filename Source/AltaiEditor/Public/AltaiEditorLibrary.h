#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "AltaiEditorLibrary.generated.h"
UCLASS()
class ALTAIEDITOR_API UAltaiEditorLibrary : public UBlueprintFunctionLibrary
{
 GENERATED_BODY()
public:
 UFUNCTION(BlueprintCallable, Category="Altai|Editor") static FString InspectGamePanel(class UUserWidget* Panel);
 UFUNCTION(BlueprintCallable, Category="Altai|Editor") static bool ActivateGameControl(class UUserWidget* Panel,const FString& Label);
 UFUNCTION(BlueprintCallable, Category="Altai|Editor") static bool SendGamePanelKey(class UUserWidget* Panel,FName Key,bool Control=false);
 UFUNCTION(BlueprintCallable, Category="Altai|Editor") static void RestoreAutomaticResolution(float ScreenPercentage);
 UFUNCTION(BlueprintCallable, Category="Altai|Editor") static void SetPhysicsTestMode(bool Enabled);
 UFUNCTION(BlueprintCallable, Category="Altai|Editor") static bool CreateContactAnimation(class USkeleton* Skeleton);
 UFUNCTION(BlueprintCallable, Category="Altai|Editor") static class ALandscape* ImportLabLandscape(const FString& HeightFile, class UMaterialInterface* Material);
 UFUNCTION(BlueprintCallable, Category="Altai|Editor") static bool SculptSwimmingPond(class ALandscape* Landscape);
 UFUNCTION(BlueprintCallable, Category="Altai|Editor") static bool CreateHeadingFont();
 // Authoring only. Existing assets are never overwritten.
 UFUNCTION(BlueprintCallable, Category="Altai|Editor") static void CreateUIAssets();
 UFUNCTION(BlueprintCallable, Category="Altai|Editor") static void UpgradeSaveUI();
 UFUNCTION(BlueprintCallable, Category="Altai|Editor") static void CreateInventoryAssets();
 UFUNCTION(BlueprintCallable, Category="Altai|Editor") static void PolishInventoryAssets();
};
