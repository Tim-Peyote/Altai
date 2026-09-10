#include "AltaiSwimming.h"
#include "AltaiWaterMovement.h"
#include "AltaiCharacter.h"
#include "AltaiHands.h"
#include "AltaiBodyDynamics.h"
#include "AltaiFootstepFX.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/PlayerController.h"
#include "Animation/AnimSequence.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "InputCoreTypes.h"
AAltaiPond::AAltaiPond(){Priority=30;FullResistanceDepth=130;}
bool AAltaiPond::ContainsPoint(const FVector& Point) const
{
 const FVector P=Bounds->GetComponentTransform().InverseTransformPosition(Point),E=Bounds->GetUnscaledBoxExtent();
 return FMath::Square(P.X/E.X)+FMath::Square(P.Y/E.Y)<1.f && Point.Z<SurfaceHeight+210 && Point.Z>SurfaceHeight-MaximumDepth-150;
}
UAltaiSwimming::UAltaiSwimming(){PrimaryComponentTick.bCanEverTick=true;PrimaryComponentTick.TickGroup=TG_PrePhysics;}
void UAltaiSwimming::BeginPlay()
{
 Super::BeginPlay();Character=Cast<ACharacter>(GetOwner());if(!Character.IsValid())return;
 auto* C=Character.Get();C->GetCharacterMovement()->AddTickPrerequisiteComponent(this);OriginalRate=C->GetMesh()->GlobalAnimRateScale;
 for(TActorIterator<AAltaiPond> It(GetWorld());It;++It)Ponds.Add(*It);
 Breaststroke=LoadObject<UAnimSequence>(nullptr,TEXT("/Game/Altai/Player/Animations/A_SwimBreaststroke.A_SwimBreaststroke"));
 EasyStroke=LoadObject<UAnimSequence>(nullptr,TEXT("/Game/Altai/Player/Animations/A_SwimEasy.A_SwimEasy"));
 TreadMotion=LoadObject<UAnimSequence>(nullptr,TEXT("/Game/Altai/Player/Animations/A_SwimTread.A_SwimTread"));
 DrownMotion=LoadObject<UAnimSequence>(nullptr,TEXT("/Game/Altai/Player/Animations/A_SwimDrown.A_SwimDrown"));
 Underwater=NewObject<UPostProcessComponent>(C,TEXT("SwimmingUnderwater"));Underwater->bUnbound=true;Underwater->Priority=60;Underwater->BlendWeight=0;
 auto& S=Underwater->Settings;S.bOverride_SceneColorTint=true;S.SceneColorTint=FLinearColor(.30,.65,.60);S.bOverride_ColorSaturation=true;S.ColorSaturation=FVector4(.7,.8,.75,1);S.bOverride_VignetteIntensity=true;S.VignetteIntensity=.45;S.bOverride_DepthOfFieldFocalDistance=true;S.DepthOfFieldFocalDistance=350;S.bOverride_DepthOfFieldFstop=true;S.DepthOfFieldFstop=2;
 Underwater->Settings.WeightedBlendables.Array.Add(FWeightedBlendable(1.f,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Altai/Environment/Materials/M_UnderwaterDepth.M_UnderwaterDepth"))));
 Underwater->RegisterComponent();
}
void UAltaiSwimming::EndPlay(const EEndPlayReason::Type Reason)
{
 if(Character.IsValid()){Character->GetMesh()->GlobalAnimRateScale=OriginalRate;if(auto* M=Cast<UAltaiWaterMovement>(Character->GetCharacterMovement()))M->WaterActive=false;}
 if(Underwater)Underwater->DestroyComponent();Super::EndPlay(Reason);
}
void UAltaiSwimming::TickComponent(float Dt,ELevelTick Type,FActorComponentTickFunction* F)
{
 Super::TickComponent(Dt,Type,F);auto* C=Character.Get();if(!C)return;
 auto* M=Cast<UAltaiWaterMovement>(C->GetCharacterMovement());auto* PC=Cast<APlayerController>(C->GetController());if(!M || !PC)return;
 if(Dead){
  Sprint=Dive=Ascend=false;C->ConsumeMovementInputVector();
  M->WaterActive=true;M->Submerged=true;M->PassiveSink=true;
  SwimAlpha=FMath::FInterpTo(SwimAlpha,1.f,Dt,4);DeathProgress=FMath::Min(1.f,DeathProgress+Dt/4.f);
  Blackout=FMath::FInterpConstantTo(Blackout,1.f,Dt,.12f);Speed=C->GetVelocity().Size();ApplyBlackout();return;
 }
 const bool UI=C->ActorHasTag(TEXT("AltaiDeveloperPanelOpen"));
 if(InputFromPlayer){Sprint=PC->IsInputKeyDown(EKeys::LeftShift);Dive=PC->IsInputKeyDown(EKeys::C);Ascend=PC->IsInputKeyDown(EKeys::SpaceBar);}
 if(UI){Dive=Ascend=Sprint=false;}
 AAltaiPond* Pond=nullptr;for(auto P:Ponds)if(P.IsValid() && P->ContainsPoint(C->GetActorLocation())){Pond=P.Get();break;}
 const bool WasSwimming=Swimming();float BottomDepth=0;Depth=0;
 if(Pond){
  FHitResult Hit;FCollisionQueryParams Q(SCENE_QUERY_STAT(AltaiSwimDepth),false,C);const FVector P=C->GetActorLocation();
  if(GetWorld()->LineTraceSingleByChannel(Hit,FVector(P.X,P.Y,Pond->SurfaceHeight+20),FVector(P.X,P.Y,Pond->SurfaceHeight-Pond->MaximumDepth-100),ECC_Visibility,Q))BottomDepth=FMath::Max(0.f,Pond->SurfaceHeight-float(Hit.ImpactPoint.Z));
  Depth=FMath::Clamp(Pond->SurfaceHeight-float(P.Z-C->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()),0.f,BottomDepth);
 }
 const bool Busy=C->ActorHasTag(TEXT("AltaiWallAttached")) || C->ActorHasTag(TEXT("AltaiMantling")) || C->ActorHasTag(TEXT("AltaiBodyUnbalanced"));
 const bool CanSwim=Pond && !Busy && BottomDepth>(WasSwimming?125:150) && Depth>(WasSwimming?80:125);
 if(CanSwim){
  if(!WasSwimming){if(auto* H=C->FindComponentByClass<UAltaiHands>())H->Release();C->UnCrouch();M->WaterActive=true;M->SetMovementMode(MOVE_Custom,1);}
  State=(State==EAltaiSwimState::Diving || (Dive && BottomDepth>230))?EAltaiSwimState::Diving:EAltaiSwimState::Surface;
  if(State==EAltaiSwimState::Diving && C->GetActorLocation().Z>Pond->SurfaceHeight-40 && !Dive)State=EAltaiSwimState::Surface;
  if(Stamina<.15f)Exhausted=true;else if(Stamina>.4f)Exhausted=false;
  const bool Moving=C->GetVelocity().Size()>20 || C->GetPendingMovementInputVector().SizeSquared()>.01;
  Stamina=FMath::Clamp(Stamina+Dt*(Sprint && Moving && !Exhausted?-.055f:Moving && !Exhausted?-.009f:.028f),0.f,1.f);
  Fatigue=FMath::FInterpTo(Fatigue,Exhausted?1.f:0.f,Dt,2);
  M->WaterHeight=Pond->SurfaceHeight;M->Submerged=State==EAltaiSwimState::Diving;
  M->WaterSpeed=FMath::Lerp(Sprint?255.f:175.f,90.f,Fatigue);M->VerticalIntent=Ascend?1.f:(Dive?-1.f:0.f);
  Hint=TEXT("Shift — темп · C — глубже · Space — всплыть");
  if(!M->Submerged)M->VerticalIntent=0;
  C->Tags.AddUnique(TEXT("AltaiSwimming"));if(M->Submerged)C->Tags.AddUnique(TEXT("AltaiDiving"));else C->Tags.Remove(TEXT("AltaiDiving"));
 }else{
  State=Depth>4?EAltaiSwimState::Wading:EAltaiSwimState::Dry;
  if(WasSwimming){M->WaterActive=false;M->Submerged=false;M->SetMovementMode(MOVE_Falling);}
  C->Tags.Remove(TEXT("AltaiSwimming"));C->Tags.Remove(TEXT("AltaiDiving"));Stamina=FMath::Min(1.f,Stamina+Dt*.065f);Fatigue=FMath::FInterpTo(Fatigue,0.f,Dt,2);
  Hint=State==EAltaiSwimState::Wading?TEXT("Брод · глубже — тяжелее шаг · Shift — ускориться"):TEXT("");
 }
 // Prepare the upright sculling pose before losing the bottom. Reverse this
 // blend on the shore; forward extension fades before returning to stepping.
 const float TargetWater=Swimming()?1.f:(State==EAltaiSwimState::Wading?FMath::SmoothStep(85.f,125.f,Depth)*.65f:0.f);
 WaterBlend=FMath::FInterpConstantTo(WaterBlend,TargetWater,Dt,Swimming()?1.25f:1.65f);
 SwimAlpha=FMath::SmoothStep(0.f,1.f,WaterBlend);
 const float TravelTarget=Swimming()?FMath::SmoothStep(25.f,125.f,float(State==EAltaiSwimState::Diving?C->GetVelocity().Size():C->GetVelocity().Size2D()))*FMath::SmoothStep(125.f,200.f,BottomDepth):0.f;
 TravelBlend=FMath::FInterpTo(TravelBlend,TravelTarget,Dt,Swimming()?2.4f:4.f);
 WadeAlpha=FMath::FInterpTo(WadeAlpha,State==EAltaiSwimState::Wading?FMath::Clamp(Depth/130,0.f,1.f):0.f,Dt,5);
 Speed=C->GetVelocity().Size();
 StrokeRate=FMath::FInterpTo(StrokeRate,Swimming()?FMath::Lerp(Sprint?1.4f:1.f,.6f,Fatigue):1.f,Dt,2.f);
 Phase=FMath::Fmod(Phase+Dt*StrokeRate/2.4f,1.f);
 C->GetMesh()->GlobalAnimRateScale=OriginalRate*FMath::Lerp(1.f,Sprint?.95f:.7f,WadeAlpha);
 const auto* Camera=C->FindComponentByClass<UCameraComponent>();const float EyeZ=Camera?Camera->GetComponentLocation().Z:C->GetActorLocation().Z+60;
 const float Under=Pond?FMath::Clamp((Pond->SurfaceHeight-EyeZ)/20.f,0.f,1.f):0.f;
 CameraUnderwater=FMath::FInterpTo(CameraUnderwater,Under,Dt,6);if(Underwater)Underwater->BlendWeight=CameraUnderwater;
 // Breathing belongs to the character, never to the third-person camera.
 // Surface swimming includes the breathing phase; animation bob must not refill/drain per frame.
 const float AirwayZ=C->GetActorLocation().Z+(Swimming()?40.f:64.f);
 if(!Pond || AirwayZ>Pond->SurfaceHeight+5.f)AirwayUnderwater=false;
 else if(AirwayZ<Pond->SurfaceHeight-5.f)AirwayUnderwater=true;
 UpdateBreathing(Dt,AirwayUnderwater);
 RippleTime+=Dt;if(Swimming() && Pond && !M->Submerged && RippleTime>.65f){RippleTime=0;if(auto* FX=GetWorld()->SpawnActor<AAltaiFootstepFX>(FVector(C->GetActorLocation().X,C->GetActorLocation().Y,Pond->SurfaceHeight+2),FRotator::ZeroRotator))FX->Configure(true,false,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Altai/Environment/Materials/M_WaterRipple.M_WaterRipple")));}
}

void UAltaiSwimming::ApplyBlackout()
{
 if(Underwater)Underwater->Settings.VignetteIntensity=FMath::Lerp(.45f,1.f,FMath::Clamp((.25f-Oxygen)*4.f,0.f,1.f));
}
void UAltaiSwimming::UpdateBreathing(float Dt,bool SubmergedAirway)
{
 const float PreviousOxygen=Oxygen;
 const float Drain=(Sprint && Speed>20?1.35f:1.f)/FMath::Max(1.f,OxygenSeconds);
 Oxygen=FMath::Clamp(Oxygen+Dt*(SubmergedAirway?-Drain:.18f),0.f,1.f);
 if(SubmergedAirway && Oxygen<=0){
  // Count only the portion of this frame after oxygen ran out.
  const float SuffocationDt=FMath::Max(0.f,Dt-PreviousOxygen/Drain);
  DrowningHealth=FMath::Max(0.f,DrowningHealth-SuffocationDt/FMath::Max(1.f,DrowningSeconds));
  Stamina=FMath::Min(Stamina,DrowningHealth*.25f);
  Hint=TEXT("НЕТ ВОЗДУХА — Space: всплыть!");
 }else if(!SubmergedAirway)DrowningHealth=FMath::Min(1.f,DrowningHealth+Dt*.2f);
 if(Oxygen>0 && Oxygen<.25f && SubmergedAirway)Hint=TEXT("МАЛО ВОЗДУХА — Space: всплыть");
 const float Target=FMath::Clamp((.2f-Oxygen)*1.25f,0.f,.25f)+(1-DrowningHealth)*.55f;
 Blackout=FMath::FInterpTo(Blackout,Target,Dt,SubmergedAirway?3.f:2.f);
 if(DrowningHealth<=0){
  Dead=true;DeathProgress=0;Oxygen=0;Sprint=Dive=Ascend=false;
  if(auto* C=Character.Get()){
   C->Tags.AddUnique(TEXT("AltaiDead"));C->Tags.AddUnique(TEXT("AltaiSwimming"));C->bUseControllerRotationYaw=false;C->StopJumping();
   if(auto* H=C->FindComponentByClass<UAltaiHands>())H->Release();
   if(auto* M=Cast<UAltaiWaterMovement>(C->GetCharacterMovement())){M->PassiveSink=true;M->WaterActive=true;M->Submerged=true;M->bOrientRotationToMovement=false;M->SetMovementMode(MOVE_Custom,1);}
  }
  Hint=TEXT("Вы утонули · Панель разработчика → Вернуться на берег");
 }
 ApplyBlackout();
}
void UAltaiSwimming::ResetAtShore()
{
 auto* C=Character.Get();if(!C)return;
 Dead=false;AirwayUnderwater=false;DeathProgress=Blackout=0;Oxygen=DrowningHealth=Stamina=1;Fatigue=0;Exhausted=false;
 Sprint=Dive=Ascend=false;SwimAlpha=WadeAlpha=Depth=WaterBlend=TravelBlend=0;StrokeRate=1;State=EAltaiSwimState::Dry;Hint.Empty();
 C->Tags.Remove(TEXT("AltaiDead"));C->Tags.Remove(TEXT("AltaiSwimming"));C->Tags.Remove(TEXT("AltaiDiving"));
 C->ConsumeMovementInputVector();C->SetActorLocation(FVector(3280,1700,140));
 if(auto* M=Cast<UAltaiWaterMovement>(C->GetCharacterMovement())){M->PassiveSink=M->WaterActive=M->Submerged=false;M->StopMovementImmediately();M->SetMovementMode(MOVE_Falling);}
 if(auto* PC=Cast<APlayerController>(C->GetController()))PC->SetControlRotation(FRotator(-10,180,0));
 if(auto* AC=Cast<AAltaiCharacter>(C))AC->SetFirstPerson(AC->FirstPerson);
 C->GetMesh()->GlobalAnimRateScale=OriginalRate;CameraUnderwater=0;if(Underwater)Underwater->BlendWeight=0;ApplyBlackout();
}
