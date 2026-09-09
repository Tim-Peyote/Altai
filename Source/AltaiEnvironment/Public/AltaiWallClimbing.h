#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AltaiWallClimbing.generated.h"
UCLASS(ClassGroup=(Altai),meta=(BlueprintSpawnableComponent))
class ALTAIENVIRONMENT_API UAltaiWallClimbing : public UActorComponent
{
 GENERATED_BODY()
public:
 UAltaiWallClimbing();
 virtual void BeginPlay() override;
 virtual void EndPlay(const EEndPlayReason::Type R) override;
 virtual void TickComponent(float Dt,ELevelTick T,FActorComponentTickFunction* F) override;
 UFUNCTION(BlueprintCallable) bool AttachWall();
 UFUNCTION(BlueprintCallable) void ReleaseWall();
 bool AttachFromLedge(class UPrimitiveComponent* Surface,const FVector& Outward,const TArray<FVector>& Contacts);
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) float BodyTwist=0;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) float ClimbSpeed=0;
 UFUNCTION(BlueprintCallable) bool PlaceContact();
 UFUNCTION(BlueprintCallable) void SelectNextLimb();
 UFUNCTION(BlueprintCallable) void ReleaseSelectedContact();
 UFUNCTION(BlueprintCallable) bool JumpOff();
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) TArray<bool> ContactActive;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) TArray<float> SupportLoad;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) int32 MovingLimb=INDEX_NONE;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) float ContactProgress=1;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) FVector BodyOffset=FVector::ZeroVector;
 UFUNCTION(BlueprintCallable) void SetMoveIntent(FVector2D Value){TestIntent=Value;IntentExpiry=.15f;}
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) bool Attached=false;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) int32 SelectedLimb=0;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) float Wetness=0;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) float Grip=1;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) float Stamina=1;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) TArray<float> LimbGrip;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) TArray<float> LimbStamina;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) int32 Slips=0;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) FString Hint;
 UPROPERTY(EditAnywhere,BlueprintReadWrite) bool AssistedStepping=true;
 UPROPERTY(EditAnywhere,BlueprintReadWrite) bool InputFromPlayer=true;
private:
 bool StartAttachment(const TArray<FVector>& Goals,bool Sequential);
 bool Probe(const FVector& Desired,FVector& Hit) const;
 void UpdateContacts(float Dt);
 bool BeginContactMove(int32 Limb,const FVector& Goal);
 void LoseContact(int32 Limb);
 float Reach(int32 Limb) const;
 TWeakObjectPtr<class ACharacter> Character;
 TWeakObjectPtr<class UAltaiHands> Hands;
 TWeakObjectPtr<class UPrimitiveComponent> Wall;
 TWeakObjectPtr<class AAltaiWeatherRig> Weather;
 FVector Normal,Right;
 TArray<FVector> LocalContacts;
 TArray<TWeakObjectPtr<class AAltaiGripZone>> GripZones;
 FVector MoveStart=FVector::ZeroVector;
 TArray<FVector> DisplayContacts;
 float ContactElapsed=0;
 float ContactDuration=.38f;
 float SettleTime=0;
 int32 LastStep=3;
 FVector2D SmoothedIntent=FVector2D::ZeroVector;
 float SlipTime=0,StepTime=0,IntentExpiry=0;
 FVector2D TestIntent=FVector2D::ZeroVector;
 bool OldYaw=false,OldOrient=false,LockedInput=false;
};
