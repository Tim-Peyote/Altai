#include "AltaiWeather.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/VolumetricCloudComponent.h"
#include "Components/PostProcessComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "Camera/PlayerCameraManager.h"
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TimerManager.h"
#include "Engine/World.h"

AAltaiWeatherRig::AAltaiWeatherRig()
{
 PrimaryActorTick.bCanEverTick = true;
 PrimaryActorTick.TickInterval = .05f;
 RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("WeatherRoot"));
 Sun = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("Sun")); Sun->SetupAttachment(RootComponent);
 Sun->SetMobility(EComponentMobility::Movable); Sun->bAtmosphereSunLight = true; Sun->AtmosphereSunLightIndex = 0;
 Moon = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("Moon")); Moon->SetupAttachment(RootComponent);
 Moon->SetMobility(EComponentMobility::Movable); Moon->bAtmosphereSunLight = true; Moon->AtmosphereSunLightIndex = 1;
 Moon->SetLightColor(FLinearColor(.48f,.62f,.8f));
 Lightning=CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("Lightning"));Lightning->SetupAttachment(RootComponent);
 Lightning->SetMobility(EComponentMobility::Movable);Lightning->SetRelativeRotation(FRotator(-65,30,0));Lightning->SetIntensity(0);Lightning->SetCastShadows(false);
 Atmosphere = CreateDefaultSubobject<USkyAtmosphereComponent>(TEXT("Atmosphere")); Atmosphere->SetupAttachment(RootComponent);
 Sky = CreateDefaultSubobject<USkyLightComponent>(TEXT("SkyFill")); Sky->SetupAttachment(RootComponent);
 Sky->SetMobility(EComponentMobility::Movable); Sky->bRealTimeCapture = true;
 Fog = CreateDefaultSubobject<UExponentialHeightFogComponent>(TEXT("ValleyFog")); Fog->SetupAttachment(RootComponent);
 Fog->SetVolumetricFog(true);
 Clouds = CreateDefaultSubobject<UVolumetricCloudComponent>(TEXT("CloudLayer")); Clouds->SetupAttachment(RootComponent);
 Clouds->SetMaterial(LoadObject<UMaterialInterface>(nullptr,TEXT("/Engine/EngineSky/VolumetricClouds/m_SimpleVolumetricCloud_Inst.m_SimpleVolumetricCloud_Inst")));
 Grade = CreateDefaultSubobject<UPostProcessComponent>(TEXT("AtmosphericGrade")); Grade->SetupAttachment(RootComponent); Grade->bUnbound = true;
 auto& S = Grade->Settings;
 S.bOverride_AutoExposureMethod = true; S.AutoExposureMethod = EAutoExposureMethod::AEM_Manual;
 S.bOverride_AutoExposureApplyPhysicalCameraExposure = true; S.AutoExposureApplyPhysicalCameraExposure = true;
 S.bOverride_CameraISO = true; S.CameraISO = 100;
 S.bOverride_CameraShutterSpeed = true; S.CameraShutterSpeed = 125;
 S.bOverride_DepthOfFieldFstop = true; S.DepthOfFieldFstop = 8;
 S.bOverride_AutoExposureBias = true; S.AutoExposureBias = 0;
 S.bOverride_ColorSaturation = true; S.ColorSaturation = FVector4(.76,.78,.73,1);
 S.bOverride_ColorGain = true; S.ColorGain = FVector4(.96,1.0,1.025,1);
 S.bOverride_VignetteIntensity = true; S.VignetteIntensity = .22f;
 S.bOverride_MotionBlurAmount = true; S.MotionBlurAmount = 0;
 RainFX = CreateDefaultSubobject<UNiagaraComponent>(TEXT("Rain")); RainFX->SetupAttachment(RootComponent);
 SnowFX = CreateDefaultSubobject<UNiagaraComponent>(TEXT("Snow")); SnowFX->SetupAttachment(RootComponent);
 RainFX->SetAsset(LoadObject<UNiagaraSystem>(nullptr,TEXT("/Game/Altai/Environment/Weather/NS_LabRain.NS_LabRain")));
 SnowFX->SetAsset(LoadObject<UNiagaraSystem>(nullptr,TEXT("/Game/Altai/Environment/Weather/NS_LabSnow.NS_LabSnow")));
}
void AAltaiWeatherRig::OnConstruction(const FTransform& T) { Super::OnConstruction(T); PreviewWeather(); }
void AAltaiWeatherRig::BeginPlay() { Super::BeginPlay(); Random.Initialize(RandomSeed); SelectPreset(PresetIndex,true); }
void AAltaiWeatherRig::SelectPreset(int32 Index, bool Immediate)
{
 if (!Presets.IsValidIndex(Index) || !Presets[Index]) return;
 PresetIndex = Index; WeatherElapsed = 0;
 if (Immediate) { Rain=Presets[Index]->Rain; Snow=Presets[Index]->Snow; Mist=Presets[Index]->Mist; Cloud=Presets[Index]->Cloud; }
 ApplyLighting();
}
void AAltaiWeatherRig::PreviewWeather() { SelectPreset(PresetIndex,true); ApplyLighting(); }
void AAltaiWeatherRig::SetHour(float V) { Hour=FMath::Fmod(V+24.f,24.f); ApplyLighting(); }
void AAltaiWeatherRig::ApplyLighting()
{
 if(!CloudMaterial)
 {
  CloudMaterial=UMaterialInstanceDynamic::Create(LoadObject<UMaterialInterface>(nullptr,TEXT("/Engine/EngineSky/VolumetricClouds/m_SimpleVolumetricCloud_Inst.m_SimpleVolumetricCloud_Inst")),this);
  Clouds->SetMaterial(CloudMaterial);
 }
 if(CloudMaterial){CloudMaterial->SetScalarParameterValue(TEXT("Cloud_GlobalCoverage"),FMath::Lerp(.18f,.85f,Cloud));CloudMaterial->SetScalarParameterValue(TEXT("Cloud_GlobalDensity"),FMath::Lerp(.3f,1.1f,Cloud));}
 const float Altitude=FMath::Sin((Hour-6.f)/24.f*2.f*PI);
 const float Day=FMath::Clamp(Altitude*4.f,0.f,1.f);
 Sun->SetWorldRotation(FRotator(-Altitude*65.f,(Hour-12.f)*15.f-35.f,0));
 Sun->SetIntensity(FMath::Max(0.f,Altitude)*65000.f*FMath::Lerp(1.f,.32f,Cloud));
 Lightning->SetIntensity(FlashRemaining>0?95000.f:0.f);
 Sun->SetLightColor(FLinearColor::LerpUsingHSV(FLinearColor(1,.66,.4),FLinearColor(.93,.96,1),Day));
 Moon->SetWorldRotation(FRotator(-35,Hour*15.f+180,0)); Moon->SetIntensity((1.f-Day)*240.f);
 Sky->SetIntensity(FMath::Lerp(.25f,1.1f,Day));
 Fog->SetFogDensity(FMath::Lerp(.018f,.18f,Mist));
 Fog->SetFogHeightFalloff(.14f);
 Fog->SetFogInscatteringColor(FLinearColor(.27f,.34f,.36f)*FMath::Lerp(14.f,4200.f,Day));
 Fog->SetStartDistance(FMath::Lerp(1200.f,80.f,Mist));
 Grade->Settings.AutoExposureBias=FMath::Lerp(5.3f,.6f,Day);
 if (GetWorld() && SurfaceParameters)
 {
  UKismetMaterialLibrary::SetScalarParameterValue(this,SurfaceParameters,TEXT("Wetness"),Wetness);
  UKismetMaterialLibrary::SetScalarParameterValue(this,SurfaceParameters,TEXT("SnowCover"),SnowCover);
 }
}
void AAltaiWeatherRig::Tick(float Dt)
{
 Super::Tick(Dt);
 if(CycleTime)Hour=FMath::Fmod(Hour+24.f*Dt/FMath::Max(30.f,DayDurationSeconds),24.f);
 WeatherElapsed+=Dt;
 if(AutomaticWeather && WeatherElapsed>FMath::Max(5.f,WeatherHoldSeconds) && Presets.Num()>1)
  SelectPreset((PresetIndex+Random.RandRange(1,Presets.Num()-1))%Presets.Num());
 if(Presets.IsValidIndex(PresetIndex) && Presets[PresetIndex])
 {
  auto* P=Presets[PresetIndex].Get(); const float Speed=3.f/FMath::Max(.1f,TransitionSeconds);
  Rain=FMath::FInterpTo(Rain,P->Rain,Dt,Speed);Snow=FMath::FInterpTo(Snow,P->Snow,Dt,Speed);
  Mist=FMath::FInterpTo(Mist,P->Mist,Dt,Speed);Cloud=FMath::FInterpTo(Cloud,P->Cloud,Dt,Speed);
  if(P->Thunder && LightningEnabled && (LightningCountdown-=Dt)<=0)
  {
   FlashRemaining=.18f;LightningCountdown=Random.FRandRange(28.f,42.f);
   if(ThunderSound)
   {
    FTimerHandle Handle;
    GetWorldTimerManager().SetTimer(Handle,FTimerDelegate::CreateWeakLambda(this,[this](){UGameplayStatics::PlaySound2D(this,ThunderSound,.45f);}),Random.FRandRange(1.2f,3.5f),false);
   }
  }
 }
 FlashRemaining=FMath::Max(0.f,FlashRemaining-Dt);
 Wetness=FMath::FInterpTo(Wetness,FMath::Max(Rain,Snow*.4f),Dt,Rain>.1f?.12f:.015f);
 SnowCover=FMath::FInterpTo(SnowCover,Snow,Dt,Snow>.1f?.025f:.008f);
 if(auto* Camera=UGameplayStatics::GetPlayerCameraManager(this,0))
 {
  const FVector Pos=Camera->GetCameraLocation()+FVector(0,0,1500);
  RainFX->SetWorldLocation(Pos);SnowFX->SetWorldLocation(Pos);
 }
 RainFX->SetVariableFloat(TEXT("User.SpawnRate"),Rain*700.f);
 SnowFX->SetVariableFloat(TEXT("User.SpawnRate"),Snow*100.f);
 ApplyLighting();
}
