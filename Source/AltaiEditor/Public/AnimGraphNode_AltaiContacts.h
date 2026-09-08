#pragma once
#include "CoreMinimal.h"
#include "AnimGraphNode_SkeletalControlBase.h"
#include "AnimNode_AltaiContacts.h"
#include "AnimGraphNode_AltaiContacts.generated.h"
UCLASS()
class ALTAIEDITOR_API UAnimGraphNode_AltaiContacts : public UAnimGraphNode_SkeletalControlBase
{
 GENERATED_BODY()
public:
 UPROPERTY(EditAnywhere,Category=Settings) FAnimNode_AltaiContacts Node;
 virtual FText GetNodeTitle(ENodeTitleType::Type) const override{return FText::FromString(TEXT("Altai Hand and Foot Contacts"));}
 virtual FText GetControllerDescription() const override{return GetNodeTitle(ENodeTitleType::FullTitle);}
 virtual const FAnimNode_SkeletalControlBase* GetNode() const override{return &Node;}
};
