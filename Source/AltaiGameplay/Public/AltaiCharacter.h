#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "AltaiCharacter.generated.h"
class UInputAction;
class UInputMappingContext;
UCLASS()
class ALTAIGAMEPLAY_API AAltaiCharacter : public ACharacter
{
 GENERATED_BODY()
public:
 AAltaiCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
 virtual void Tick(float DeltaSeconds) override;
 UFUNCTION(BlueprintCallable) void SetFirstPerson(bool Enabled);
 UFUNCTION(BlueprintCallable) void ToggleView();
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Camera") bool FirstPerson=false;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera") float FirstPersonForward=8;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Camera") FVector InteractionEyeOffset=FVector::ZeroVector;
 virtual void SetupPlayerInputComponent(UInputComponent* Input) override;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<class USpringArmComponent> CameraBoom;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<class UCameraComponent> FollowCamera;
 UPROPERTY(EditDefaultsOnly, Category="Input") TObjectPtr<UInputMappingContext> MappingContext;
 UPROPERTY(EditDefaultsOnly, Category="Input") TObjectPtr<UInputAction> MoveAction;
 UPROPERTY(EditDefaultsOnly, Category="Input") TObjectPtr<UInputAction> LookAction;
 UPROPERTY(EditDefaultsOnly, Category="Input") TObjectPtr<UInputAction> JumpAction;
private:
 float OriginalArmLength=350;
 bool CameraInitialized=false;
 void Move(const FInputActionValue& Value);
 void Look(const FInputActionValue& Value);
};
