#include "AltaiGraphicsPanel.h"
#include "AltaiPanelStyle.h"
#include "GameFramework/GameUserSettings.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/SOverlay.h"
using namespace AltaiPanel;
static const TArray<FString> Levels={TEXT("Низкое"),TEXT("Среднее"),TEXT("Высокое"),TEXT("Очень высокое"),TEXT("Кино")};
void SAltaiGraphicsPanel::Read()
{
 auto* G=UGameUserSettings::GetGameUserSettings();
 const int Values[]={G->GetViewDistanceQuality(),G->GetShadowQuality(),G->GetGlobalIlluminationQuality(),G->GetReflectionQuality(),G->GetAntiAliasingQuality(),G->GetTextureQuality(),G->GetVisualEffectQuality(),G->GetPostProcessingQuality(),G->GetFoliageQuality(),G->GetShadingQuality()};
 for(int I=0;I<10;++I)Quality[I]=FMath::Clamp(Values[I],0,4);
 float N,Min,Max;G->GetResolutionScaleInformationEx(N,Resolution,Min,Max);AutoResolution=Resolution<=0;if(AutoResolution)Resolution=100;FPSLimit=G->GetFrameRateLimit();VSync=G->IsVSyncEnabled();Dirty=false;
}
int SAltaiGraphicsPanel::Overall() const{for(int I=1;I<10;++I)if(Quality[I]!=Quality[0])return -1;return Quality[0];}
void SAltaiGraphicsPanel::Preset(int L){AutoResolution=false;for(int& Q:Quality)Q=L;Resolution=L==0?67:L==1?85:100;Dirty=true;Notice=TEXT("Выбран профиль. Нажмите «Применить».");}
void SAltaiGraphicsPanel::Apply()
{
 auto* G=UGameUserSettings::GetGameUserSettings();
 G->SetViewDistanceQuality(Quality[0]);G->SetShadowQuality(Quality[1]);G->SetGlobalIlluminationQuality(Quality[2]);G->SetReflectionQuality(Quality[3]);G->SetAntiAliasingQuality(Quality[4]);G->SetTextureQuality(Quality[5]);G->SetVisualEffectQuality(Quality[6]);G->SetPostProcessingQuality(Quality[7]);G->SetFoliageQuality(Quality[8]);G->SetShadingQuality(Quality[9]);
 if(!AutoResolution)G->SetResolutionScaleValueEx(Resolution);G->SetFrameRateLimit(FPSLimit);G->SetVSyncEnabled(VSync);
 // No display-mode switch: safe in PIE as well as the standalone game.
 G->ApplyNonResolutionSettings();G->SaveSettings();Read();Notice=TEXT("Применено и сохранено. Настройки общие для игры и полигона.");
}
void SAltaiGraphicsPanel::Construct(const FArguments& Args)
{
 Read();auto Body=SNew(SVerticalBox);
 Body->AddSlot().AutoHeight()[Label(TEXT("Один набор настроек для игры и полигона. Изменения вступают в силу после применения."),13,Muted)];
 auto Quick=SNew(SHorizontalBox);const TCHAR* Names[]={TEXT("Быстрее"),TEXT("Баланс"),TEXT("Красивее")};const int L[]={0,1,3};
 for(int I=0;I<3;++I)Quick->AddSlot().FillWidth(1).Padding(0,10,6,4)[Button(Names[I],[this,I,L]{Preset(L[I]);})];
 Body->AddSlot().AutoHeight()[Quick];Body->AddSlot().AutoHeight()[Section(TEXT("КАЧЕСТВО ИЗОБРАЖЕНИЯ"))];
 Body->AddSlot().AutoHeight()[Row(TEXT("Общее качество"),Choice([this]{int Q=Overall();return Q<0?FString(TEXT("Пользовательское")):Levels[Q];},Levels,[this](int I){Preset(I);}))];
 auto Res=SNew(SVerticalBox);Res->AddSlot().AutoHeight()[SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Regular",14)).ColorAndOpacity(Gold).Text_Lambda([this]{return FText::FromString(AutoResolution?FString(TEXT("Автоматически")):FString::Printf(TEXT("%.0f%%"),Resolution));})];
 Res->AddSlot().AutoHeight().Padding(0,6)[SNew(SSlider).MinValue(50).MaxValue(100).StepSize(5).Value_Lambda([this]{return Resolution;}).OnValueChanged_Lambda([this](float V){AutoResolution=false;Resolution=FMath::RoundToFloat(V/5)*5;Dirty=true;})];
 Body->AddSlot().AutoHeight()[Row(TEXT("Масштаб рендера"),Res)];
 Body->AddSlot().AutoHeight()[Label(TEXT("67–85% обычно снижает нагрузку на GPU. Интерфейс сохраняет чёткость."),12,Muted)];
 const TCHAR* Labels[]={TEXT("Дальность видимости"),TEXT("Тени"),TEXT("Глобальное освещение"),TEXT("Отражения"),TEXT("Сглаживание"),TEXT("Текстуры"),TEXT("Эффекты и частицы"),TEXT("Постобработка"),TEXT("Растительность"),TEXT("Затенение материалов")};
 for(int I=0;I<10;++I)Body->AddSlot().AutoHeight()[Row(Labels[I],Choice([this,I]{return Levels[Quality[I]];},Levels,[this,I](int Q){Quality[I]=Q;Dirty=true;}))];
 Body->AddSlot().AutoHeight()[Section(TEXT("ПЛАВНОСТЬ"))];
 Body->AddSlot().AutoHeight()[Row(TEXT("Ограничение FPS"),Choice([this]{return FPSLimit>0?FString::Printf(TEXT("%.0f FPS"),FPSLimit):FString(TEXT("Без ограничения"));},{TEXT("30 FPS"),TEXT("60 FPS"),TEXT("90 FPS"),TEXT("120 FPS"),TEXT("Без ограничения")},[this](int I){const float Values[]={30,60,90,120,0};FPSLimit=Values[I];Dirty=true;}))];
 Body->AddSlot().AutoHeight().Padding(0,5)[Toggle(TEXT("Вертикальная синхронизация (VSync)"),[this]{return VSync;},[this](bool V){VSync=V;Dirty=true;})];
 ChildSlot[SNew(SVerticalBox)
 +SVerticalBox::Slot().FillHeight(1)[SNew(SScrollBox)+SScrollBox::Slot().Padding(0,0,14,0)[Body]]
 +SVerticalBox::Slot().AutoHeight().Padding(0,12,0,8)[SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Regular",14)).AutoWrapText(true).ColorAndOpacity(Muted).Text_Lambda([this]{return FText::FromString(Dirty?TEXT("Есть неприменённые изменения."):Notice);})]
 +SVerticalBox::Slot().AutoHeight()[SNew(SHorizontalBox)+SHorizontalBox::Slot().FillWidth(1).Padding(0,0,8,0)[SNew(SButton).ContentPadding(FMargin(12,10)).ButtonColorAndOpacity(Gold).IsEnabled_Lambda([this]{return Dirty;}).OnClicked_Lambda([this]{Apply();return FReply::Handled();})[Label(TEXT("Применить"),14,Ink)]]+SHorizontalBox::Slot().AutoWidth()[Button(TEXT("Отменить изменения"),[this]{Read();Notice=TEXT("Восстановлены применённые значения.");})]]];
}
UAltaiGraphicsScreen::UAltaiGraphicsScreen(const FObjectInitializer& I):Super(I){Kind=EAltaiScreenKind::Settings;SetIsFocusable(true);}
TSharedRef<SWidget> UAltaiGraphicsScreen::RebuildWidget()
{
 return SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(Ink).Padding(32)
 [SNew(SHorizontalBox)+SHorizontalBox::Slot().FillWidth(.25f)+SHorizontalBox::Slot().FillWidth(1)
 [SNew(SVerticalBox)+SVerticalBox::Slot().AutoHeight()[Label(TEXT("АЛТАЙ / НАСТРОЙКИ"),12,Gold)]+SVerticalBox::Slot().AutoHeight().Padding(0,8,0,16)[Label(TEXT("Графика"),30)]
 +SVerticalBox::Slot().FillHeight(1)[SNew(SAltaiGraphicsPanel)]
 +SVerticalBox::Slot().AutoHeight().Padding(0,16,0,0)[Button(TEXT("Назад  ·  Esc"),[this]{Back();})]]+SHorizontalBox::Slot().FillWidth(.25f)];
}
