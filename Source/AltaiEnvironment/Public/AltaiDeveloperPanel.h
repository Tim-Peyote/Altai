#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AltaiDeveloperPanel.generated.h"
UCLASS()
class ALTAIENVIRONMENT_API UAltaiDeveloperPanel : public UUserWidget
{
 GENERATED_BODY()
public:
 UAltaiDeveloperPanel(const FObjectInitializer& I);
 virtual TSharedRef<SWidget> RebuildWidget() override;
 virtual FReply NativeOnPreviewKeyDown(const FGeometry& G,const FKeyEvent& E) override;
private:
 int Page=0;
 TWeakObjectPtr<class AAltaiLabController> Lab;
 TSharedRef<SWidget> Environment();
 TSharedRef<SWidget> Character();
 TSharedRef<SWidget> Diagnostics();
 TSharedRef<SWidget> Stat(const FString& Label,TFunction<FString()> Value);
};
