#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AltaiArticulatedProp.generated.h"

/** A single physical hinge/rail. Several rails can share an independently placed cabinet. */
UCLASS()
class ALTAIENVIRONMENT_API AAltaiArticulatedProp : public AActor
{
 GENERATED_BODY()
public:
 AAltaiArticulatedProp();
 virtual void BeginPlay() override;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) TObjectPtr<class UStaticMeshComponent> Base;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) TObjectPtr<class UStaticMeshComponent> Part;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) TObjectPtr<class UPhysicsConstraintComponent> Joint;
 UPROPERTY(EditAnywhere,BlueprintReadWrite) TObjectPtr<AActor> AnchorActor;
 UPROPERTY(EditAnywhere,BlueprintReadWrite) bool Sliding=false;
 UPROPERTY(EditAnywhere,BlueprintReadWrite) FVector Axis=FVector::UpVector;
 UPROPERTY(EditAnywhere,BlueprintReadWrite) float Travel=105;
 UPROPERTY(EditAnywhere,BlueprintReadWrite) float PartMass=12;
 UPROPERTY(EditAnywhere,BlueprintReadWrite) float Resistance=4;
 UFUNCTION(BlueprintPure) float GetOpening() const;
 FVector GripAt(float Opening) const;
 UFUNCTION(BlueprintCallable) void DriveGrip(float Opening,float Strength=12000);
private:
 FTransform Closed;
 FVector GripLocal=FVector::ZeroVector;
};
