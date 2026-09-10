#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AltaiSurface.h"
#include "AltaiSwimming.generated.h"
UCLASS()
class ALTAIENVIRONMENT_API AAltaiPond : public AAltaiSurfaceZone
{
 GENERATED_BODY()
public:
 AAltaiPond();
 UPROPERTY(EditAnywhere,BlueprintReadWrite) float MaximumDepth=440;
 bool ContainsPoint(const FVector& Point) const;
};
UENUM(BlueprintType)
enum class EAltaiSwimState:uint8 { Dry,Wading,Surface,Diving };
UCLASS(ClassGroup=(Altai),meta=(BlueprintSpawnableComponent))
class ALTAIENVIRONMENT_API UAltaiSwimming : public UActorComponent
{
 GENERATED_BODY()
public:
 UAltaiSwimming();
 virtual void BeginPlay() override;
 virtual void EndPlay(const EEndPlayReason::Type Reason) override;
 virtual void TickComponent(float Dt,ELevelTick Type,FActorComponentTickFunction* F) override;
 UPROPERTY(BlueprintReadOnly) EAltaiSwimState State=EAltaiSwimState::Dry;
 UPROPERTY(BlueprintReadOnly) float Depth=0;
 UPROPERTY(BlueprintReadOnly) float Stamina=1;
 UPROPERTY(BlueprintReadOnly) float Oxygen=1;
 UPROPERTY(BlueprintReadOnly) float DrowningHealth=1;
 UPROPERTY(BlueprintReadOnly) float Blackout=0;
 UPROPERTY(BlueprintReadOnly) bool Dead=false;
 UPROPERTY(BlueprintReadOnly) bool AirwayUnderwater=false;
 UPROPERTY(BlueprintReadOnly) float DeathProgress=0;
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Breathing",meta=(ClampMin="1")) float OxygenSeconds=40;
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Breathing",meta=(ClampMin="1")) float DrowningSeconds=6;
 UPROPERTY(BlueprintReadOnly) float SwimAlpha=0;
 UPROPERTY(BlueprintReadOnly) float WadeAlpha=0;
 UPROPERTY(BlueprintReadOnly) float Phase=0;
 UPROPERTY(BlueprintReadOnly) float TravelBlend=0;
 UPROPERTY(BlueprintReadOnly) float Fatigue=0;
 UPROPERTY(BlueprintReadOnly) float Speed=0;
 UPROPERTY(BlueprintReadOnly) float CameraUnderwater=0;
 UPROPERTY(EditAnywhere,BlueprintReadWrite) bool InputFromPlayer=true;
 UPROPERTY(EditAnywhere,BlueprintReadWrite) bool Sprint=false;
 UPROPERTY(EditAnywhere,BlueprintReadWrite) bool Dive=false;
 UPROPERTY(EditAnywhere,BlueprintReadWrite) bool Ascend=false;
 UPROPERTY(BlueprintReadOnly) FString Hint;
 UPROPERTY() TObjectPtr<class UAnimSequence> Breaststroke;
 UPROPERTY() TObjectPtr<class UAnimSequence> EasyStroke;
 UPROPERTY() TObjectPtr<class UAnimSequence> TreadMotion;
 UPROPERTY() TObjectPtr<class UAnimSequence> DrownMotion;
 UFUNCTION(BlueprintCallable) void SetTestOxygen(float Value){if(!Dead)Oxygen=FMath::Clamp(Value,0.f,1.f);}
 UFUNCTION(BlueprintCallable) void ResetAtShore();
 UFUNCTION(BlueprintCallable) void SetTestStamina(float Value){Stamina=FMath::Clamp(Value,0.f,1.f);}
 bool Swimming() const{return State==EAltaiSwimState::Surface || State==EAltaiSwimState::Diving;}
private:
 TWeakObjectPtr<class ACharacter> Character;
 TArray<TWeakObjectPtr<AAltaiPond>> Ponds;
 UPROPERTY() TObjectPtr<class UPostProcessComponent> Underwater;
 float RippleTime=0,OriginalRate=1,StrokeRate=1,WaterBlend=0;
 bool Exhausted=false;
 void UpdateBreathing(float Dt,bool UnderwaterAirway);
 void ApplyBlackout();
};
