#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AltaiInventoryPreview.generated.h"
class USkeletalMeshComponent;
class USceneCaptureComponent2D;
class UPointLightComponent;

// An editable studio rig. Runtime instances never own gameplay inventory or input.
UCLASS()
class ALTAIPRESENTATION_API AAltaiInventoryPreview : public AActor
{
 GENERATED_BODY()
public:
 AAltaiInventoryPreview();
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) TObjectPtr<USkeletalMeshComponent> Model;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) TObjectPtr<USceneCaptureComponent2D> Capture;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) TObjectPtr<UPointLightComponent> KeyLight;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) TObjectPtr<UPointLightComponent> RimLight;
 UFUNCTION(BlueprintCallable) void RotateModel(float Degrees);
 UFUNCTION(BlueprintCallable) void RefreshPreview();
 virtual void BeginPlay() override;
};
