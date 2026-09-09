#pragma once
#include "CoreMinimal.h"
#include "Engine/AssetUserData.h"
#include "AltaiGripProfile.generated.h"

/** Authored joint flexion in degrees (proximal, middle, distal), never bone scaling. */
USTRUCT(BlueprintType)
struct ALTAIENVIRONMENT_API FAltaiFingerGrasp
{
 GENERATED_BODY()
 UPROPERTY(EditAnywhere,BlueprintReadWrite) FVector Index=FVector(35,55,30);
 UPROPERTY(EditAnywhere,BlueprintReadWrite) FVector Middle=FVector(40,60,35);
 UPROPERTY(EditAnywhere,BlueprintReadWrite) FVector Ring=FVector(45,65,35);
 UPROPERTY(EditAnywhere,BlueprintReadWrite) FVector Little=FVector(50,65,40);
 UPROPERTY(EditAnywhere,BlueprintReadWrite) FVector Thumb=FVector(15,25,15);
 UPROPERTY(EditAnywhere,BlueprintReadWrite) float ThumbOpposition=15;
 FVector Digit(int32 I) const {return I==0?Index:I==1?Middle:I==2?Ring:I==3?Little:Thumb;}
};

/** Saved on a static mesh alongside Grip_One/L/R sockets; editable independently of code. */
UCLASS(BlueprintType,EditInlineNew)
class ALTAIENVIRONMENT_API UAltaiGripProfile : public UAssetUserData
{
 GENERATED_BODY()
public:
 UPROPERTY(EditAnywhere,BlueprintReadWrite) FName GraspName=TEXT("Power");
 UPROPERTY(EditAnywhere,BlueprintReadWrite) FAltaiFingerGrasp OneHand;
 UPROPERTY(EditAnywhere,BlueprintReadWrite) FAltaiFingerGrasp TwoHands;
 UPROPERTY(EditAnywhere,BlueprintReadWrite) FVector OneHandPositiveLimits=FVector(45,20,12);
 UPROPERTY(EditAnywhere,BlueprintReadWrite) FVector OneHandNegativeLimits=FVector(35,20,12);
 UPROPERTY(EditAnywhere,BlueprintReadWrite) FVector TwoHandLimits=FVector(20,20,10);
 UPROPERTY(EditAnywhere,BlueprintReadWrite) float CarryHeightOffset=0;
};
