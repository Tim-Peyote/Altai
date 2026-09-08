// Editor-only black-box access to native Slate panels that the widget-tree inspector cannot see.
#include "AltaiEditorLibrary.h"
#include "Blueprint/UserWidget.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/SWindow.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/GameUserSettings.h"
#include "HAL/IConsoleManager.h"
static bool LabelIn(TSharedRef<SWidget> W,const FString& Label)
{
 if(!W->GetVisibility().IsVisible())return false;
 if(W->GetType()==TEXT("STextBlock") && StaticCastSharedRef<STextBlock>(W)->GetText().ToString()==Label)return true;
 auto* Children=W->GetChildren();for(int I=0;I<Children->Num();++I)if(LabelIn(Children->GetChildAt(I),Label))return true;return false;
}
static TSharedPtr<SWidget> FindControl(TSharedRef<SWidget> W,const FString& Label)
{
 if(!W->GetVisibility().IsVisible() || !W->IsEnabled())return nullptr;
 if((W->GetType()==TEXT("SButton") || W->GetType()==TEXT("SCheckBox")) && LabelIn(W,Label))return W;
 auto* Children=W->GetChildren();for(int I=0;I<Children->Num();++I)if(auto Found=FindControl(Children->GetChildAt(I),Label))return Found;return nullptr;
}
FString UAltaiEditorLibrary::InspectGamePanel(UUserWidget* Panel)
{
 if(!Panel)return TEXT("No panel");FString Result;
 TFunction<void(TSharedRef<SWidget>,int)> Walk=[&](TSharedRef<SWidget> W,int Depth){if(Depth>80 || !W->GetVisibility().IsVisible())return;
 if(W->GetType()==TEXT("STextBlock"))Result+=StaticCastSharedRef<STextBlock>(W)->GetText().ToString()+TEXT("\n");
 auto* C=W->GetChildren();for(int I=0;I<C->Num();++I)Walk(C->GetChildAt(I),Depth+1);};Walk(Panel->TakeWidget(),0);return Result;
}
bool UAltaiEditorLibrary::ActivateGameControl(UUserWidget* Panel,const FString& Label)
{
 if(!Panel)return false;auto Found=FindControl(Panel->TakeWidget(),Label);
 if(!Found)for(const auto& Window:FSlateApplication::Get().GetTopLevelWindows())if(auto W=FindControl(Window,Label)){Found=W;break;}
 if(!Found)return false;
 if(Found->GetType()==TEXT("SButton")){StaticCastSharedPtr<SButton>(Found)->SimulateClick();return true;}
 FSlateApplication::Get().SetKeyboardFocus(Found);FKeyEvent Down(EKeys::SpaceBar,FModifierKeysState(),0,false,0,0);FSlateApplication::Get().ProcessKeyDownEvent(Down);FSlateApplication::Get().ProcessKeyUpEvent(Down);return true;
}
bool UAltaiEditorLibrary::SendGamePanelKey(UUserWidget* Panel,FName Key,bool Control)
{
 if(!Panel)return false;FSlateApplication::Get().SetKeyboardFocus(Panel->TakeWidget());
 FModifierKeysState M(false,false,Control,false,false,false,false,false,false);FKeyEvent E(FKey(Key),M,0,false,0,0);
 const bool Handled=FSlateApplication::Get().ProcessKeyDownEvent(E);FSlateApplication::Get().ProcessKeyUpEvent(E);return Handled;
}
void UAltaiEditorLibrary::RestoreAutomaticResolution(float ScreenPercentage)
{
 auto* G=UGameUserSettings::GetGameUserSettings();G->ScalabilityQuality.ResolutionQuality=0;G->SaveSettings();
 if(auto* V=IConsoleManager::Get().FindConsoleVariable(TEXT("r.ScreenPercentage")))V->Set(ScreenPercentage,ECVF_SetByScalability);
}
