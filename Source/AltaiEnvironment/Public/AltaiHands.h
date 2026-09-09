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
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) FVector FurnitureBodyOffset=FVector::ZeroVector;
 UFUNCTION(BlueprintCallable) void Release();
 UFUNCTION(BlueprintCallable) void ToggleGrab();
 UFUNCTION(BlueprintCallable) bool BeginHandMotion();
 UFUNCTION(BlueprintCallable) void MoveHeldHand(FVector2D ScreenDelta);
 UFUNCTION(BlueprintCallable) void SetHeldRotationMode(bool Enabled);
 /** Anatomical degrees: X forearm roll, Y wrist flexion, Z deviation. Bounded across R presses. */
 UFUNCTION(BlueprintCallable) void RotateHeld(FVector LocalDegrees);
 UFUNCTION(BlueprintCallable) void AdjustHoldDistance(float Delta);
 UFUNCTION(BlueprintCallable) bool SetTwoHandGrip(bool Enabled);
 UFUNCTION(BlueprintCallable) bool BeginChargeThrow();
 UFUNCTION(BlueprintCallable) bool ReleaseChargedThrow();
 /** Cancels mouse/charge modes without dropping or throwing the held prop. */
 UFUNCTION(BlueprintCallable) void CancelManipulation();
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) bool MovingHand=false;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) bool RotatingHeld=false;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) bool ChargingThrow=false;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) bool ThrowPending=false;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) FVector GripAngles=FVector::ZeroVector;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) FVector GripAngleLimits=FVector(65,40,20);
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) bool GripRotationLimited=false;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) float GripRotationEffort=0;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) float WristTrackingError=0;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) float ForearmRoll=0;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) float SignedForearmRoll=0;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) float ThrowCharge=0;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) float ThrowPose=0;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) float LastThrowSpeed=0;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) float LastReleaseSpeed=0;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) float LastThrowMass=0;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) float LastThrowEnergy=0;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) FVector HandMotionOffset=FVector::ZeroVector;
 UPROPERTY(EditAnywhere,BlueprintReadWrite) float OneHandMassLimit=6.f;
 UPROPERTY(EditAnywhere,BlueprintReadWrite) float OneHandThrowEnergy=60.f;
 UPROPERTY(EditAnywhere,BlueprintReadWrite) float TwoHandThrowEnergy=160.f;
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
 FQuat GripOrientation(int32 Hand) const;
private:
 TWeakObjectPtr<class ACharacter> Character;
 FVector LocalContact,LeftLocal,RightLocal;
 FQuat LocalGripFrames[2]={FQuat::Identity,FQuat::Identity};
 int8 PendingGripMode=-1;
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
 bool OldCCD=false;
 FQuat RelativeHoldRotation=FQuat::Identity;
 FQuat NeutralHoldRotation=FQuat::Identity;
 FQuat AnatomicalFrame=FQuat::Identity;
 bool AnatomicalFrameReady=false;
 float NeutralForearmRoll=0;
 bool GripReturningToNeutral=false;
 float TrackingLossTime=0;
 void UpdateAnatomicalRotation();
 FVector SmoothedHandOffset=FVector::ZeroVector;
 float HoldDistanceOffset=0;
 float GripHeightOffset=0;
 float ThrowElapsed=0;
 float FollowThroughRemaining=0;
 bool FollowThroughTwoHands=false;
 FVector FollowThroughDirection=FVector::ForwardVector;
 FVector ThrowDirection=FVector::ForwardVector;
 void ConfigureGrip();
 void UpdateLookLock();
 void TickManipulation(float DeltaSeconds);
 void FinishThrow();

 TEnumAsByte<ETickingGroup> OldMeshTickGroup=TG_PrePhysics;
 uint8 OldMeshVisibilityTick=0;
 bool OldUpdateRateOptimizations=false;
 struct FPendingCollision { TWeakObjectPtr<class UPrimitiveComponent> Component; ECollisionResponse Response; bool OldCCD=false; };
 TArray<FPendingCollision> PendingCollision;
 void RestoreReleasedCollision(bool Force);
};
