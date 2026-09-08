#pragma once
#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "AltaiScreens.h"
#include "AltaiGraphicsPanel.generated.h"

class ALTAIPRESENTATION_API SAltaiGraphicsPanel : public SCompoundWidget
{
public:
 SLATE_BEGIN_ARGS(SAltaiGraphicsPanel){} SLATE_END_ARGS()
 void Construct(const FArguments& Args);
private:
 int Quality[10]={};float Resolution=100,FPSLimit=0;bool VSync=false,Dirty=false,AutoResolution=false;
 FString Notice;
 void Read();void Preset(int Level);void Apply();int Overall() const;
};

UCLASS()
class ALTAIPRESENTATION_API UAltaiGraphicsScreen : public UAltaiScreen
{
 GENERATED_BODY()
public:
 UAltaiGraphicsScreen(const FObjectInitializer& Initializer);
 virtual TSharedRef<SWidget> RebuildWidget() override;
};
