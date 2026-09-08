#pragma once
#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/CoreStyle.h"

/** Shared, compact game-facing controls for settings and the field lab. */
namespace AltaiPanel
{
 inline FLinearColor Ink(.018f,.025f,.025f,1),Card(.036f,.049f,.047f,1),Gold(.78f,.68f,.46f,1),Text(.88f,.89f,.84f,1),Muted(.53f,.62f,.59f,1);
 inline TSharedRef<STextBlock> Label(const FString& S,int Size=14,FLinearColor Color=Text){return SNew(STextBlock).Text(FText::FromString(S)).Font(FCoreStyle::GetDefaultFontStyle("Regular",Size)).ColorAndOpacity(Color).AutoWrapText(true);}
 inline TSharedRef<SButton> Button(const FString& S,TFunction<void()> Action){return SNew(SButton).ContentPadding(FMargin(12,9)).ButtonColorAndOpacity(Card).OnClicked_Lambda([Action]{Action();return FReply::Handled();})[Label(S)];}
 inline TSharedRef<SWidget> Section(const FString& S){return SNew(SBox).Padding(FMargin(0,18,0,8))[Label(S,12,Gold)];}
 inline TSharedRef<SWidget> Row(const FString& S,TSharedRef<SWidget> Control){return SNew(SHorizontalBox)+SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center).Padding(0,7,12,7)[Label(S)]+SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center).Padding(0,3)[Control];}
 inline TSharedRef<SWidget> Choice(TFunction<FString()> Current,const TArray<FString>& Options,TFunction<void(int)> Select){return SNew(SComboButton).ContentPadding(FMargin(10,7)).ButtonContent()[SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Regular",14)).ColorAndOpacity(Text).Text_Lambda([Current]{return FText::FromString(Current());})].OnGetMenuContent_Lambda([Options,Select]{auto V=SNew(SVerticalBox);for(int I=0;I<Options.Num();++I)V->AddSlot().AutoHeight()[Button(Options[I],[Select,I]{FSlateApplication::Get().DismissAllMenus();Select(I);})];return SNew(SBox).MinDesiredWidth(190)[V];});}
 inline TSharedRef<SWidget> Toggle(const FString& S,TFunction<bool()> Get,TFunction<void(bool)> Set){return SNew(SCheckBox).IsChecked_Lambda([Get]{return Get()?ECheckBoxState::Checked:ECheckBoxState::Unchecked;}).OnCheckStateChanged_Lambda([Set](ECheckBoxState V){Set(V==ECheckBoxState::Checked);})[SNew(SBox).Padding(FMargin(8,5))[Label(S)]];}
}
