#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Animation/PoseSnapshot.h"
#include "AltaiBodyDynamics.generated.h"

USTRUCT(BlueprintType)
struct FAltaiRecoveryContact
{
 GENERATED_BODY()
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) FVector Goal=FVector::ZeroVector;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) FVector Normal=FVector::UpVector;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) float Weight=0;
};

UENUM(BlueprintType)
enum class EAltaiBodyState : uint8 { Balanced, Stumbling, Falling, Settling, GettingUp };

/** Lab body reactions. Chaos owns the body during a fall; locomotion owns it only after recovery. */
UCLASS(ClassGroup=(Altai),meta=(BlueprintSpawnableComponent))
class ALTAIENVIRONMENT_API UAltaiBodyDynamics : public UActorComponent
{
 GENERATED_BODY()
public:
 UAltaiBodyDynamics();
 virtual void BeginPlay() override;
 virtual void EndPlay(const EEndPlayReason::Type Reason) override;
 virtual void TickComponent(float Dt,ELevelTick Type,FActorComponentTickFunction* F) override;
 UFUNCTION(BlueprintCallable) void Trip(const FHitResult& Hit);
 /** Impulse uses UE units kg cm/s, applied at the actual point, never an artificial random spin. */
 UFUNCTION(BlueprintCallable) void ApplyBodyImpulse(FVector Impulse,FVector WorldPoint,FName Bone=TEXT("spine_03"));
 UFUNCTION(BlueprintCallable) void TestStumble();
 UFUNCTION(BlueprintCallable) void TestFall();
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Body") bool Enabled=true;
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Body",meta=(ClampMin="650",ClampMax="1800")) float FallSpeedThreshold=850;
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Body",meta=(ClampMin="0.5",ClampMax="2")) float TripSensitivity=1;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) EAltaiBodyState State=EAltaiBodyState::Balanced;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) float Reaction=0;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) float Balance=1;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) float RecoveryProgress=0;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) float RecoveryAcquire=.45f;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) float LastImpactSpeed=0;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) float PhysicalMass=0;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) int32 Falls=0;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) int32 Recoveries=0;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) FString Hint;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) FVector ReactionDirection=FVector::ForwardVector;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) FVector AngularVelocity=FVector::ZeroVector;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) bool FaceUp=false;
 bool OwnsBody() const {return State>=EAltaiBodyState::Falling;}
 UPROPERTY() TObjectPtr<class UAnimSequence> ProneMotion;
 UPROPERTY() TObjectPtr<class UAnimSequence> SupineMotion;
 class UAnimSequence* RecoveryMotion() const;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) TArray<FAltaiRecoveryContact> RecoveryContacts;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) int32 RecoverySupportCount=0;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) FString RecoveryTerrainIssue;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) bool RecoveryBlocked=false;
 FVector RecoveryGroundOffset=FVector::ZeroVector;
 FQuat RecoveryGroundRotation=FQuat::Identity;
 FPoseSnapshot RecoverySnapshot;
 FVector RecoveryFloor=FVector::ZeroVector;
private:
 UPROPERTY() TObjectPtr<class UPhysicalAnimationComponent> Muscles;
 TWeakObjectPtr<class ACharacter> Character;
 FTransform MeshHome;
 FName CollisionHome;
 ECollisionEnabled::Type CapsuleCollisionHome=ECollisionEnabled::QueryAndPhysics;
 FVector PreviousVelocity=FVector::ZeroVector;
 float PreviousYaw=0,Elapsed=0,QuietTime=0,Cooldown=0,ReactionAge=0,ReactionPeak=0;
 bool LockedInput=false,OldYaw=false,OldOrient=false,WasFalling=false;
 bool Available() const;
 void BeginFall(FVector Velocity,FVector Spin);
 void BeginStumble(float Severity,FVector Direction);
 bool TryGetUp();
 void FinishGetUp();
 TArray<FTransform> RigReference;
 void ProjectJointSafety();
 void ConfigureBodies();
 void StopSimulation();
 TWeakObjectPtr<UAnimSequence> CachedRecovery;
 TArray<TArray<FTransform>> RecoveryPoseCache;
 float RecoveryRetry=0,RecoveryBlockedTime=0;
 void CacheRecoveryPose();
 void SampleRecoveryPose(float Time,TArray<FTransform>& Pose) const;
 bool EvaluateRecoveryTerrain(float Time,const FTransform& MeshWorld,FVector& Offset,FQuat& Rotation,TArray<FAltaiRecoveryContact>& Contacts,bool Clearance);
 bool RecoveryPathClear(const FTransform& MeshWorld);
 bool UpdateRecoveryTerrain(float Time,float Dt);
 void ResumePhysicalFall();
 UFUNCTION() void CapsuleHit(UPrimitiveComponent* HitComponent,AActor* Other,UPrimitiveComponent* OtherComp,FVector NormalImpulse,const FHitResult& Hit);
};
