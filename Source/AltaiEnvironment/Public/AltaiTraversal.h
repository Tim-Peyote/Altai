#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AltaiTraversal.generated.h"

/** Collision-tested two-stage mantle. No teleport through rock or occupied landings. */
UCLASS(ClassGroup=(Altai),meta=(BlueprintSpawnableComponent))
class ALTAIENVIRONMENT_API UAltaiTraversal : public UActorComponent
{
 GENERATED_BODY()
public:
 UAltaiTraversal();
 virtual void BeginPlay() override;
 virtual void EndPlay(const EEndPlayReason::Type Reason) override;
 virtual void TickComponent(float Dt,ELevelTick Type,FActorComponentTickFunction* Function) override;
 UFUNCTION(BlueprintCallable) bool TryClimb();
 UFUNCTION(BlueprintCallable) bool TryClimbFromWall();
 UFUNCTION(BlueprintCallable) void CancelClimb();
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) bool Climbing=false;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) bool CanClimb=false;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) int32 CompletedClimbs=0;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) FVector LedgePoint=FVector::ZeroVector;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) FString Hint;
private:
 bool FindLedge(FVector& Destination);
 bool BeginClimb();
 void ReleaseControl();
 TWeakObjectPtr<class ACharacter> Character;
 FVector Start,Above,Finish;
 TArray<FVector> StartContacts;
 bool FromWall=false;
 float Elapsed=0;
 bool LockedInput=false;
};
