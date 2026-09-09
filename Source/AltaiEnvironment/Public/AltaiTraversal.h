#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AltaiTraversal.generated.h"

/** Contact-driven mantle and its reverse, using a swept compact capsule while crouched. */
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
 UFUNCTION(BlueprintCallable) bool TryDescend();
 UFUNCTION(BlueprintCallable) void CancelClimb();
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) bool Climbing=false;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) bool Descending=false;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) bool CanClimb=false;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) int32 CompletedClimbs=0;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) int32 CompletedDescents=0;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) float Progress=0;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) FVector BodyOffset=FVector::ZeroVector;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) float BodyLean=0;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) float PalmDown=0;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) FVector LedgePoint=FVector::ZeroVector;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) FString Hint;
private:
 bool FindLedge(FVector& Destination);
 bool BeginClimb();
 bool ValidatePath() const;
 void ReleaseControl(bool Grounded=false);
 TWeakObjectPtr<class ACharacter> Character;
 TWeakObjectPtr<class UPrimitiveComponent> Surface;
 FVector Start,Above,Finish,Normal,Right;
 FQuat TurnStartRotation=FQuat::Identity;
 TArray<FVector> StartContacts;
 bool FromWall=false;
 float Elapsed=0,FullHeight=96,CompactHeight=44;
 float ViewTurn=0,AppliedViewTurn=0;
 bool LockedInput=false,OldYaw=false,OldOrient=false;
};
