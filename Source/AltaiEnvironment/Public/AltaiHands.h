#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AltaiHands.generated.h"

UCLASS(ClassGroup=(Altai),meta=(BlueprintSpawnableComponent))
class ALTAIENVIRONMENT_API UAltaiHands : public UActorComponent
{
 GENERATED_BODY()
public:
 UAltaiHands();
 virtual void BeginPlay() override;
 virtual void EndPlay(const EEndPlayReason::Type Reason) override;
 virtual void TickComponent(float Dt,ELevelTick Type,FActorComponentTickFunction* F) override;
 UFUNCTION(BlueprintCallable) bool TryGrab(bool FurnitureOnly=false);
 UFUNCTION(BlueprintCallable) void DragInteraction(float Delta);
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) bool ConstrainedGrip=false;
 UFUNCTION(BlueprintCallable) void Release();
 UFUNCTION(BlueprintCallable) void ToggleGrab();
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) TObjectPtr<class UPrimitiveComponent> Held;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) float HeldMass=0;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) bool TwoHands=false;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) float Stamina=1;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) float Wetness=0;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) float AppliedForce=0;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) float PositionError=0;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) float BodyMass=100;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) TArray<float> LimbReach;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) float CarryPose=0;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) float CarrySpeedScale=1;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) float CarryAccelerationScale=1;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) float BalanceDemand=0;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) FName LeftGripSocket;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) FName RightGripSocket;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) FString Hint;
 UPROPERTY(EditAnywhere,BlueprintReadWrite) float OneHandForce=12000;
 UPROPERTY(EditAnywhere,BlueprintReadWrite) float TwoHandForce=32000;
 UPROPERTY(EditAnywhere,BlueprintReadWrite) float MaxReach=190;
 UPROPERTY(EditAnywhere,BlueprintReadWrite) float MaxMass=30;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) TArray<FVector> ContactGoals;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) TArray<float> ContactWeights;
 // Wall traversal owns contacts only when no item is held.
 void SetWallContacts(const TArray<FVector>& Goals,bool Active);
 void RefreshGripGoals();
private:
 TWeakObjectPtr<class ACharacter> Character;
 FVector LocalContact,LeftLocal,RightLocal;
 float OverreachTime=0;
 float OldLinearDamping=0,OldAngularDamping=0;
 float Blend=0;
 bool AuthoredGrip=false;
 FVector PreviousVelocity=FVector::ZeroVector;
 FVector PreviousBodyLocation=FVector::ZeroVector;
 FQuat HoldRotation=FQuat::Identity;
 bool WallContacts=false;
 ECollisionResponse OldPawnResponse=ECR_Block;
 float GrabAge=0;
 float DesiredOpening=0;
 bool LockedLook=false;
 TEnumAsByte<ETickingGroup> OldMeshTickGroup=TG_PrePhysics;
 struct FPendingCollision { TWeakObjectPtr<class UPrimitiveComponent> Component; ECollisionResponse Response; };
 TArray<FPendingCollision> PendingCollision;
 void RestoreReleasedCollision(bool Force);
};
