#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AltaiGripZone.generated.h"

/** Authored surface patch: queries actual contact positions, never blocks traces. */
UCLASS(Blueprintable)
class ALTAIENVIRONMENT_API AAltaiGripZone : public AActor
{
 GENERATED_BODY()
public:
 AAltaiGripZone();
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) TObjectPtr<class UBoxComponent> Bounds;
 UPROPERTY(EditAnywhere,BlueprintReadWrite) TObjectPtr<AActor> SurfaceActor;
 UPROPERTY(EditAnywhere,BlueprintReadWrite) float DryGrip=.65f;
 UPROPERTY(EditAnywhere,BlueprintReadWrite) float WetGrip=.25f;
 UPROPERTY(EditAnywhere,BlueprintReadWrite) FString SurfaceLabel=TEXT("Smooth rock");
 bool Contains(const FVector& Point,const AActor* WallActor) const;
 float Grip(float Wetness) const {return FMath::Lerp(DryGrip,WetGrip,Wetness);}
};
