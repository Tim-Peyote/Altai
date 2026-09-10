#include "AltaiDeveloperPanel.h"
#include "AltaiBodyDynamics.h"
#include "AltaiLabController.h"
#include "AltaiWeather.h"
#include "AltaiHands.h"
#include "AltaiSurface.h"
#include "AltaiWallClimbing.h"
#include "AltaiTraversal.h"
#include "AltaiCharacter.h"
#include "AltaiGraphicsPanel.h"
#include "AltaiPanelStyle.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/SOverlay.h"
using namespace AltaiPanel;
UAltaiDeveloperPanel::UAltaiDeveloperPanel(const FObjectInitializer& I):Super(I){SetIsFocusable(true);}
FReply UAltaiDeveloperPanel::NativeOnPreviewKeyDown(const FGeometry& G,const FKeyEvent& E)
{
 if(E.GetKey()==EKeys::Escape || (E.IsControlDown() && E.GetKey()==EKeys::D)){if(Lab.IsValid())Lab->CloseDeveloperPanel();return FReply::Handled();}return Super::NativeOnPreviewKeyDown(G,E);
}
TSharedRef<SWidget> UAltaiDeveloperPanel::Stat(const FString& S,TFunction<FString()> Value)
{return Row(S,SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Regular",14)).ColorAndOpacity(Gold).AutoWrapText(true).Text_Lambda([Value]{return FText::FromString(Value());}));}
TSharedRef<SWidget> UAltaiDeveloperPanel::Environment()
{
 auto* W=Lab->Weather.Get();auto V=SNew(SVerticalBox);
 if(!W){V->AddSlot().AutoHeight()[Label(TEXT("В этой сцене нет системы погоды."))];return V;}
 V->AddSlot().AutoHeight()[Label(TEXT("Изменения видны в мире сразу. Переход погоды занимает несколько секунд."),13,Muted)];
 V->AddSlot().AutoHeight()[Section(TEXT("ПОГОДА"))];
 const TArray<FString> Names={TEXT("Ясно"),TEXT("Облачно"),TEXT("Дождь"),TEXT("Туман"),TEXT("Гроза"),TEXT("Снег"),TEXT("Дождь со снегом")};
 V->AddSlot().AutoHeight()[Row(TEXT("Сценарий"),Choice([W,Names]{return Names.IsValidIndex(W->PresetIndex)?Names[W->PresetIndex]:FString(TEXT("Особый"));},Names,[W](int I){W->AutomaticWeather=false;W->SelectPreset(I);} ))];
 V->AddSlot().AutoHeight()[Toggle(TEXT("Автоматически менять погоду"),[W]{return W->AutomaticWeather;},[W](bool B){W->AutomaticWeather=B;})];
 V->AddSlot().AutoHeight()[Row(TEXT("Держать сценарий, с"),SNew(SSpinBox<float>).Font(FCoreStyle::GetDefaultFontStyle("Regular",14)).MinValue(5).MaxValue(600).MinSliderValue(5).MaxSliderValue(180).Delta(5).Value_Lambda([W]{return W->WeatherHoldSeconds;}).OnValueChanged_Lambda([W](float F){W->WeatherHoldSeconds=F;}))];
 V->AddSlot().AutoHeight()[Row(TEXT("Переход между сценариями, с"),SNew(SSpinBox<float>).Font(FCoreStyle::GetDefaultFontStyle("Regular",14)).MinValue(.1f).MaxValue(30).Delta(.5f).Value_Lambda([W]{return W->TransitionSeconds;}).OnValueChanged_Lambda([W](float F){W->TransitionSeconds=F;}))];
 V->AddSlot().AutoHeight()[Toggle(TEXT("Молнии во время грозы"),[W]{return W->LightningEnabled;},[W](bool B){W->LightningEnabled=B;})];
 V->AddSlot().AutoHeight()[Section(TEXT("ВРЕМЯ СУТОК"))];
 V->AddSlot().AutoHeight()[Stat(TEXT("Сейчас"),[W]{int M=FMath::FloorToInt(W->Hour*60)%1440;return FString::Printf(TEXT("%02d:%02d"),M/60,M%60);})];
 V->AddSlot().AutoHeight().Padding(0,6,0,12)[SNew(SSlider).MinValue(0).MaxValue(23.99f).StepSize(.25f).Value_Lambda([W]{return W->Hour;}).OnValueChanged_Lambda([W](float H){W->CycleTime=false;W->SetHour(H);})];
 V->AddSlot().AutoHeight()[SNew(SHorizontalBox)+SHorizontalBox::Slot().FillWidth(1).Padding(0,0,8,0)[Button(TEXT("День · 14:00"),[W]{W->CycleTime=false;W->SetHour(14);})]+SHorizontalBox::Slot().FillWidth(1)[Button(TEXT("Ночь · 23:00"),[W]{W->CycleTime=false;W->SetHour(23);})]];
 V->AddSlot().AutoHeight().Padding(0,8)[Toggle(TEXT("Смена дня и ночи"),[W]{return W->CycleTime;},[W](bool B){W->CycleTime=B;})];
 V->AddSlot().AutoHeight()[Row(TEXT("Длительность суток, с"),SNew(SSpinBox<float>).Font(FCoreStyle::GetDefaultFontStyle("Regular",14)).MinValue(30).MaxValue(7200).Delta(30).Value_Lambda([W]{return W->DayDurationSeconds;}).OnValueChanged_Lambda([W](float F){W->DayDurationSeconds=F;}))];
 V->AddSlot().AutoHeight()[Section(TEXT("ФАКТИЧЕСКОЕ СОСТОЯНИЕ"))];
 V->AddSlot().AutoHeight()[Stat(TEXT("Дождь / снег / туман"),[W]{return FString::Printf(TEXT("%.0f%% / %.0f%% / %.0f%%"),W->Rain*100,W->Snow*100,W->Mist*100);})];
 V->AddSlot().AutoHeight()[Stat(TEXT("Влажность / снежный покров"),[W]{return FString::Printf(TEXT("%.0f%% / %.0f%%"),W->Wetness*100,W->SnowCover*100);})];return V;
}
TSharedRef<SWidget> UAltaiDeveloperPanel::Character()
{
 auto* C=Cast<AAltaiCharacter>(Lab->GetPawn());auto* H=Lab->Hands.Get();auto* W=Lab->WallClimbing.Get();auto* S=Lab->SurfaceResponse.Get();auto V=SNew(SVerticalBox);
 V->AddSlot().AutoHeight()[Section(TEXT("ПЕРСОНАЖ"))];
 if(C){V->AddSlot().AutoHeight()[Row(TEXT("Камера"),Choice([C]{return C->FirstPerson?FString(TEXT("Первое лицо")):FString(TEXT("Третье лицо"));},{TEXT("Первое лицо"),TEXT("Третье лицо")},[C](int I){C->SetFirstPerson(I==0);}))];
 V->AddSlot().AutoHeight()[Row(TEXT("Масса тела, кг"),SNew(SSpinBox<float>).Font(FCoreStyle::GetDefaultFontStyle("Regular",14)).MinValue(40).MaxValue(160).Delta(5).Value_Lambda([C]{return C->GetCharacterMovement()->Mass;}).OnValueChanged_Lambda([C](float F){C->GetCharacterMovement()->Mass=F;}))];}
 V->AddSlot().AutoHeight()[Label(TEXT("Масса влияет на нагрузку при лазании и переноске. Это настройка текущего теста."),12,Muted)];
 if(H && W){V->AddSlot().AutoHeight().Padding(0,12)[Button(TEXT("Восстановить выносливость"),[H,W]{H->Stamina=W->Stamina=1;for(float& St:W->LimbStamina)St=1;})];
 V->AddSlot().AutoHeight()[Toggle(TEXT("Автоматически переставлять опоры на скале"),[W]{return W->AssistedStepping;},[W](bool B){W->AssistedStepping=B;})];}
 if(S){V->AddSlot().AutoHeight()[Section(TEXT("ПОВЕРХНОСТИ И СЛЕДЫ"))];V->AddSlot().AutoHeight()[Toggle(TEXT("Спотыкаться о препятствия"),[S]{return S->EnableStumble;},[S](bool B){S->EnableStumble=B;})];
 V->AddSlot().AutoHeight()[Row(TEXT("Время жизни следа, с"),SNew(SSpinBox<float>).Font(FCoreStyle::GetDefaultFontStyle("Regular",14)).MinValue(5).MaxValue(300).Delta(5).Value_Lambda([S]{return S->FootprintLifetime;}).OnValueChanged_Lambda([S](float F){S->FootprintLifetime=F;}))];
 V->AddSlot().AutoHeight().Padding(0,8)[Button(TEXT("Очистить следы и круги на воде"),[S]{S->ClearFootprints();})];}
 if(auto* B=Lab->BodyDynamics.Get()){
 V->AddSlot().AutoHeight()[Section(TEXT("РАВНОВЕСИЕ И ПАДЕНИЯ"))];
 V->AddSlot().AutoHeight()[Toggle(TEXT("Физические реакции тела"),[B]{return B->Enabled;},[B](bool On){B->Enabled=On;})];
 V->AddSlot().AutoHeight()[Stat(TEXT("Состояние тела"),[B]{const TCHAR* States[]={TEXT("Равновесие"),TEXT("Споткнулся"),TEXT("Падает"),TEXT("Лежит"),TEXT("Встаёт")};return FString(States[int(B->State)]);})];
 V->AddSlot().AutoHeight()[Stat(TEXT("Падения / вставания"),[B]{return FString::Printf(TEXT("%d / %d"),B->Falls,B->Recoveries);})];
 V->AddSlot().AutoHeight()[Stat(TEXT("Опоры при вставании"),[B]{return FString::Printf(TEXT("%d · %s"),B->RecoverySupportCount,B->RecoveryBlocked?TEXT("ожидание безопасной опоры"):TEXT("свободно"));})];
 V->AddSlot().AutoHeight()[Stat(TEXT("Удар / физическая масса"),[B]{return FString::Printf(TEXT("%.1f м/с · %.0f кг"),B->LastImpactSpeed/100,B->PhysicalMass);})];
 V->AddSlot().AutoHeight()[Button(TEXT("Проверить спотыкание"),[B]{B->TestStumble();})];
 V->AddSlot().AutoHeight().Padding(0,6)[Button(TEXT("Проверить падение от толчка"),[B]{B->TestFall();})];
 V->AddSlot().AutoHeight()[Label(TEXT("Лёгкое препятствие — потеря шага. Сильный удар или падение с высоты — физическое тело и постепенное вставание. Для наблюдения закройте панель."),13,Muted)];
 }
 V->AddSlot().AutoHeight()[Section(TEXT("УПРАВЛЕНИЕ В ПОЛИГОНЕ"))];
 V->AddSlot().AutoHeight()[Label(TEXT("V — сменить вид · Ctrl — присесть\nF — взять / отпустить предмет\nЛКМ + мышь — двигать рукой; отпускание — бросок по инерции\nПКМ удерживать / отпустить — замах / бросок\nR + мышь — наклонять кисть в пределах хвата\nКолесо — ближе / дальше; с R — поворачивать предплечье\nT — одна / две руки (с учётом массы)\nЛКМ + движение мыши — тянуть ручку мебели\nE — начать лазание / выйти на уступ\nC — свеситься с края / отпустить скалу · Space — отпрыгнуть\nПКМ — выбрать конечность · ЛКМ — поставить опору\nQ — освободить выбранную конечность"),14,Muted)];
 return V;
}
TSharedRef<SWidget> UAltaiDeveloperPanel::Diagnostics()
{
 auto* H=Lab->Hands.Get();auto* W=Lab->WallClimbing.Get();auto* S=Lab->SurfaceResponse.Get();auto* T=Lab->Traversal.Get();auto V=SNew(SVerticalBox);
 V->AddSlot().AutoHeight()[Label(TEXT("Живые показатели. Наблюдайте изменения после закрытия панели через компактный монитор."),13,Muted)];
 V->AddSlot().AutoHeight()[Toggle(TEXT("Компактный монитор во время игры"),[this]{return Lab->ShowDiagnostics;},[this](bool B){Lab->ShowDiagnostics=B;})];
 V->AddSlot().AutoHeight()[Section(TEXT("ХВАТ И НАГРУЗКА"))];
 if(H){V->AddSlot().AutoHeight()[Stat(TEXT("Предмет в руке"),[H]{return H->Held?H->Held->GetOwner()->GetName():FString(TEXT("Нет"));})];
 V->AddSlot().AutoHeight()[Stat(TEXT("Масса / скорость с грузом"),[H]{return FString::Printf(TEXT("%.1f кг / %.0f%%"),H->HeldMass,H->CarrySpeedScale*100);})];
 V->AddSlot().AutoHeight()[Stat(TEXT("Выносливость / влажность рук"),[H]{return FString::Printf(TEXT("%.0f%% / %.0f%%"),H->Stamina*100,H->Wetness*100);})];
 V->AddSlot().AutoHeight()[Stat(TEXT("Отклонение хвата / баланс"),[H]{return FString::Printf(TEXT("%.1f см / %.0f%%"),H->PositionError,H->BalanceDemand*100);})];}
 if(H){V->AddSlot().AutoHeight()[Stat(TEXT("Режим рук"),[H]{return H->RotatingHeld?FString(TEXT("Вращение")):(H->ChargingThrow?FString(TEXT("Замах")):(H->MovingHand?FString(TEXT("Движение рукой")):(H->TwoHands?FString(TEXT("Две руки")):FString(TEXT("Одна рука")))));})];
 V->AddSlot().AutoHeight()[Stat(TEXT("Последний бросок"),[H]{return FString::Printf(TEXT("%.1f м/с · %.0f Дж · %.2f кг"),H->LastThrowSpeed/100.f,H->LastThrowEnergy,H->LastThrowMass);})];}
 if(H){V->AddSlot().AutoHeight()[Stat(TEXT("Поворот руки / предел"),[H]{return FString::Printf(TEXT("%.0f / %.0f / %.0f° · %.0f%%"),H->GripAngles.X,H->GripAngles.Y,H->GripAngles.Z,H->GripRotationEffort*100);})];
 V->AddSlot().AutoHeight()[Stat(TEXT("Предплечье / ошибка кисти"),[H]{return FString::Printf(TEXT("%.0f° / %.1f°"),H->ForearmRoll,H->WristTrackingError);})];}
 V->AddSlot().AutoHeight()[Section(TEXT("ЛАЗАНИЕ"))];
 if(W){V->AddSlot().AutoHeight()[Stat(TEXT("Состояние"),[W,T]{return W->Attached?FString(TEXT("На скале")):(T && T->Climbing?FString(T->Descending?TEXT("Свешивание с края"):TEXT("Выход на уступ")):FString(TEXT("Свободное движение")));})];
 V->AddSlot().AutoHeight()[Stat(TEXT("Сцепление / выносливость"),[W]{return FString::Printf(TEXT("%.0f%% / %.0f%%"),W->Grip*100,W->Stamina*100);})];
 V->AddSlot().AutoHeight()[Stat(TEXT("Влажность скалы / срывы"),[W]{return FString::Printf(TEXT("%.0f%% / %d"),W->Wetness*100,W->Slips);})];
 const TCHAR* Limbs[]={TEXT("Левая рука"),TEXT("Правая рука"),TEXT("Левая нога"),TEXT("Правая нога")};
 for(int I=0;I<4;++I)V->AddSlot().AutoHeight()[Stat(Limbs[I],[W,I]{const bool A=W->Attached && W->ContactActive.IsValidIndex(I) && W->ContactActive[I];return FString::Printf(TEXT("%s%s · нагрузка %.0f%%"),W->SelectedLimb==I?TEXT("▸ "):TEXT(""),W->MovingLimb==I?TEXT("Тянется"):(A?TEXT("Опора"):TEXT("Свободна")),W->SupportLoad.IsValidIndex(I)?W->SupportLoad[I]*100:0);})];}
 if(T)V->AddSlot().AutoHeight()[Stat(TEXT("Подъёмы / спуски с края"),[T]{return FString::Printf(TEXT("%d / %d"),T->CompletedClimbs,T->CompletedDescents);})];
 V->AddSlot().AutoHeight()[Section(TEXT("ПОВЕРХНОСТЬ"))];
 if(S){V->AddSlot().AutoHeight()[Stat(TEXT("Под ногами / скорость"),[S]{const TCHAR* N[]={TEXT("Земля"),TEXT("Камень"),TEXT("Грязь"),TEXT("Вода"),TEXT("Снег"),TEXT("Дерево")};return FString::Printf(TEXT("%s / %.0f%%"),N[FMath::Clamp(int(S->CurrentSurface),0,5)],S->SpeedScale*100);})];
 V->AddSlot().AutoHeight()[Stat(TEXT("Шаги / спотыкания"),[S]{return FString::Printf(TEXT("%d / %d"),S->StepCount,S->StumbleCount);})];}
 V->AddSlot().AutoHeight()[Section(TEXT("ТЕСТОВАЯ СЦЕНА"))];
 V->AddSlot().AutoHeight()[Label(TEXT("Сброс возвращает персонажа, погоду и предметы к исходному состоянию полигона. Сохранённая графика остаётся."),12,Muted)];
 V->AddSlot().AutoHeight().Padding(0,10)[Button(TEXT("Сбросить полигон"),[this]{if(Lab.IsValid())Lab->ResetLab();})];return V;
}
TSharedRef<SWidget> UAltaiDeveloperPanel::RebuildWidget()
{
 Lab=Cast<AAltaiLabController>(GetOwningPlayer());if(!Lab.IsValid())return SNew(SBox);
 auto Tabs=SNew(SHorizontalBox);const TCHAR* Names[]={TEXT("Окружение"),TEXT("Персонаж"),TEXT("Диагностика"),TEXT("Графика")};
 for(int I=0;I<4;++I)Tabs->AddSlot().FillWidth(1).Padding(0,0,4,0)[SNew(SButton).ContentPadding(FMargin(6,10)).ButtonColorAndOpacity_Lambda([this,I]{return Page==I?Gold:Card;}).OnClicked_Lambda([this,I]{Page=I;return FReply::Handled();})[Label(Names[I],13)]];
 auto Pages=SNew(SWidgetSwitcher).WidgetIndex_Lambda([this]{return Page;});
 for(auto P:{Environment(),Character(),Diagnostics()})Pages->AddSlot()[SNew(SScrollBox)+SScrollBox::Slot().Padding(0,0,12,0)[P]];
 Pages->AddSlot()[SNew(SAltaiGraphicsPanel)];
 return SNew(SOverlay)+SOverlay::Slot()[SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(FLinearColor(0,0,0,.22f))]
 +SOverlay::Slot().HAlign(HAlign_Right).Padding(20)[SNew(SBox).WidthOverride(660)[SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(Ink).Padding(22)
 [SNew(SVerticalBox)+SVerticalBox::Slot().AutoHeight()[SNew(SHorizontalBox)+SHorizontalBox::Slot().FillWidth(1)[Label(TEXT("АЛТАЙ / ПОЛИГОН"),12,Gold)]+SHorizontalBox::Slot().AutoWidth()[Button(TEXT("Закрыть · Esc"),[this]{if(Lab.IsValid())Lab->CloseDeveloperPanel();})]]
 +SVerticalBox::Slot().AutoHeight().Padding(0,8)[Label(TEXT("Панель разработчика"),25)]
 +SVerticalBox::Slot().AutoHeight().Padding(0,0,0,14)[SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Regular",14)).ColorAndOpacity(Muted).Text_Lambda([this]{return FText::FromString(FString::Printf(TEXT("Ctrl+D   ·   %.0f FPS   ·   %.1f мс"),Lab->FrameSeconds>0?1.f/Lab->FrameSeconds:0,Lab->FrameSeconds*1000));})]
 +SVerticalBox::Slot().AutoHeight().Padding(0,0,0,14)[Tabs]
 +SVerticalBox::Slot().FillHeight(1)[Pages]
 +SVerticalBox::Slot().AutoHeight().Padding(0,14,0,0)[Toggle(TEXT("Приостановить симуляцию"),[this]{return UGameplayStatics::IsGamePaused(Lab.Get());},[this](bool B){UGameplayStatics::SetGamePaused(Lab.Get(),B);})]
 ]]];
}
