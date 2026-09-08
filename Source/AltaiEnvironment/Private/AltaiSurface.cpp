#include "AltaiSurface.h"
#include "AltaiHands.h"
#include "AltaiFootstepFX.h"
#include "CollisionShape.h"
#include "AltaiWeather.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/DecalComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"

AAltaiSurfaceZone::AAltaiSurfaceZone()
{
 Bounds=CreateDefaultSubobject<UBoxComponent>(TEXT("SurfaceVolume")); RootComponent=Bounds;
 Bounds->SetBoxExtent(FVector(300,300,100));Bounds->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 Bounds->SetHiddenInGame(true);
}
bool AAltaiSurfaceZone::ContainsFeet(const FVector& Feet) const
{
 const FVector P=Bounds->GetComponentTransform().InverseTransformPosition(Feet),E=Bounds->GetUnscaledBoxExtent();
 return FMath::Square(P.X/E.X)+FMath::Square(P.Y/E.Y)<=1.f && FMath::Abs(P.Z)<=E.Z && Feet.Z<=SurfaceHeight+2;
}
float AAltaiSurfaceZone::ResistanceAt(const FVector& Feet) const
{
 if(!Surface)return 1;
 const float Depth=FMath::Clamp((SurfaceHeight-Feet.Z)/FMath::Max(1.f,FullResistanceDepth),0.f,1.f);
 return FMath::Lerp(1.f,Surface->SpeedMultiplier,Depth);
}
UAltaiSurfaceResponse::UAltaiSurfaceResponse()
{
 PrimaryComponentTick.bCanEverTick=true;PrimaryComponentTick.TickInterval=.05f;
}
void UAltaiSurfaceResponse::BeginPlay()
{
 Super::BeginPlay();Character=Cast<ACharacter>(GetOwner());
 if(!Character.IsValid()){SetComponentTickEnabled(false);return;}
 auto* Movement=Character->GetCharacterMovement();BaseSpeed=Movement->MaxWalkSpeed;BaseAcceleration=Movement->MaxAcceleration;BaseBraking=Movement->BrakingDecelerationWalking;BaseBrakingFriction=Movement->BrakingFrictionFactor;LastPosition=Character->GetActorLocation();
 for(TActorIterator<AAltaiSurfaceZone> It(GetWorld());It;++It)Zones.Add(*It);
 if(TActorIterator<AAltaiWeatherRig> It(GetWorld());It){Weather=*It;}
}
void UAltaiSurfaceResponse::EndPlay(const EEndPlayReason::Type Reason)
{
 if(Character.IsValid()){auto* M=Character->GetCharacterMovement();M->MaxWalkSpeed=BaseSpeed;M->MaxAcceleration=BaseAcceleration;M->BrakingDecelerationWalking=BaseBraking;M->BrakingFrictionFactor=BaseBrakingFriction;}
 ClearFootprints();Super::EndPlay(Reason);
}
void UAltaiSurfaceResponse::ClearFootprints(){for(auto Mark:Marks)if(Mark.IsValid())Mark->DestroyComponent();for(auto R:Ripples)if(R.IsValid())R->Destroy();Ripples.Reset();Marks.Reset();DistanceSinceStep=0;WetSteps=0;}
void UAltaiSurfaceResponse::TickComponent(float Dt,ELevelTick Type,FActorComponentTickFunction* F)
{
 Super::TickComponent(Dt,Type,F);if(!Character.IsValid())return;
 auto* C=Character.Get();auto* Move=C->GetCharacterMovement();
 const FVector Position=C->GetActorLocation(),Feet=Position-FVector(0,0,C->GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
 const FVector PreviousPosition=LastPosition;
 const float Moved=FVector::Dist2D(Position,LastPosition);LastPosition=Position;
 if(Moved>FMath::Max(500.f,BaseSpeed*Dt*2.f)){DistanceSinceStep=0;return;}
 FHitResult Floor;FCollisionQueryParams Query(SCENE_QUERY_STAT(AltaiSurface),false,C);Query.bReturnPhysicalMaterial=true;
 GetWorld()->LineTraceSingleByChannel(Floor,Feet+FVector(0,0,30),Feet-FVector(0,0,50),ECC_Visibility,Query);
 UAltaiPhysicalMaterial* Surface=Cast<UAltaiPhysicalMaterial>(Floor.PhysMaterial.Get());
 AAltaiSurfaceZone* Zone=nullptr;
 for(auto Z:Zones)if(Z.IsValid() && Z->Surface && Z->ContainsFeet(Feet) && (!Zone || Z->Priority>Zone->Priority))Zone=Z.Get();
 if(Zone)Surface=Zone->Surface;
 CurrentSurface=Surface?Surface->Kind:EAltaiSurface::Soil;
 float Desired=Zone?Zone->ResistanceAt(Feet):(Surface?Surface->SpeedMultiplier:1.f);
 if(!Move->IsMovingOnGround())Desired=1;
 StumbleTimer=FMath::Max(0.f,StumbleTimer-Dt);StumbleCooldown=FMath::Max(0.f,StumbleCooldown-Dt);
 if(EnableStumble && Move->IsMovingOnGround() && C->GetVelocity().Size2D()>260 && StumbleCooldown<=0)
 {
  const FVector Direction=C->GetVelocity().GetSafeNormal2D();
  FVector Start=PreviousPosition-FVector(0,0,C->GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
  Start.Z=FMath::Min(Start.Z,Feet.Z)+18;
  FVector End=Feet+Direction*140;End.Z=Start.Z;
  FHitResult Low,High;
  if(GetWorld()->SweepSingleByChannel(Low,Start,End,FQuat::Identity,ECC_Visibility,FCollisionShape::MakeSphere(8),Query) && Low.ImpactNormal.Z<.45f
   && !GetWorld()->LineTraceSingleByChannel(High,Start+FVector(0,0,70),End+FVector(0,0,70),ECC_Visibility,Query))
  {StumbleTimer=.45f;StumbleCooldown=1.5f;Move->Velocity*=.4f;++StumbleCount;}
 }
 auto* Hands=C->FindComponentByClass<UAltaiHands>();
 const float LoadSpeed=Hands?Hands->CarrySpeedScale:1.f,LoadAcceleration=Hands?Hands->CarryAccelerationScale:1.f;
 Move->MaxAcceleration=BaseAcceleration*LoadAcceleration;Move->BrakingDecelerationWalking=BaseBraking*LoadAcceleration;Move->BrakingFrictionFactor=BaseBrakingFriction*LoadAcceleration;
 Desired*=LoadSpeed;
 if(StumbleTimer>0)Desired=FMath::Min(Desired,.4f);
 SpeedScale=FMath::FInterpTo(SpeedScale,Desired,Dt,9.f);Move->MaxWalkSpeed=BaseSpeed*SpeedScale;
 if(Move->IsMovingOnGround() && Moved>.3f)
 {
  DistanceSinceStep+=Moved;
  if(DistanceSinceStep>=FMath::Max(25.f,StepDistance*FMath::Lerp(.65f,1.f,SpeedScale)))
  {DistanceSinceStep=0;MakeStep(Feet,Surface,Zone);}
 }
 else DistanceSinceStep=0;
}
void UAltaiSurfaceResponse::MakeStep(const FVector& Feet,UAltaiPhysicalMaterial* Surface,AAltaiSurfaceZone* Zone)
{
 ++StepCount;
 auto* C=Character.Get();const bool Right=(StepCount%2)==0;
 const FName Bone=Right?TEXT("foot_r"):TEXT("foot_l");
 FVector Point=Feet+C->GetActorRightVector()*(Right?12.f:-12.f);
 if(C->GetMesh() && C->GetMesh()->DoesSocketExist(Bone))Point=C->GetMesh()->GetSocketLocation(Bone);
 FHitResult Hit;FCollisionQueryParams Params(SCENE_QUERY_STAT(AltaiFootprint),false,C);Params.bReturnPhysicalMaterial=true;
 if(!GetWorld()->LineTraceSingleByChannel(Hit,Point+FVector(0,0,50),Point-FVector(0,0,120),ECC_Visibility,Params) || Hit.ImpactNormal.Z<.5f)return;
 UMaterialInterface* Material=Surface?Surface->Footprint.Get():nullptr;
 const bool Water=CurrentSurface==EAltaiSurface::Water;
 if(Water || CurrentSurface==EAltaiSurface::Mud)WetSteps=10;
 if(Water)Material=RippleMaterial;
 else if(CurrentSurface==EAltaiSurface::Snow || (Weather.IsValid() && Weather->SnowCover>.45f))Material=SnowFootprint;
 else if(!Material && WetSteps>0){Material=WetFootprint;--WetSteps;}
 if(!Material)return;
 const bool Snow=CurrentSurface==EAltaiSurface::Snow || (Weather.IsValid() && Weather->SnowCover>.45f);
 if(Water || Snow || CurrentSurface==EAltaiSurface::Mud)
 {
  Ripples.RemoveAll([](const auto& R){return !R.IsValid();});
  if(Ripples.Num()>=12){Ripples[0]->Destroy();Ripples.RemoveAt(0);}
  FVector FXLocation=Hit.ImpactPoint+FVector(0,0,3);
  if(Water && Zone)FXLocation.Z=Zone->SurfaceHeight+2;
  if(auto* FX=GetWorld()->SpawnActor<AAltaiFootstepFX>(FXLocation,FRotator::ZeroRotator)){FX->Configure(Water,Snow,RippleMaterial);Ripples.Add(FX);}
 }
 if(Water)return;
 Marks.RemoveAll([](const auto& M){return !M.IsValid();});
 const int32 Limit=FMath::Clamp(MaxFootprints,1,192);
 while(Marks.Num()>=Limit){if(Marks[0].IsValid())Marks[0]->DestroyComponent();Marks.RemoveAt(0);}
 FVector Location=Hit.ImpactPoint+Hit.ImpactNormal*1.5f;
 if(Water && Zone)Location.Z=Zone->SurfaceHeight+1;
 const FVector Forward=FVector::VectorPlaneProject(C->GetActorForwardVector(),Hit.ImpactNormal).GetSafeNormal();
 const FRotator Rotation=FRotationMatrix::MakeFromXZ(-Hit.ImpactNormal,Forward).Rotator();
 auto* D=UGameplayStatics::SpawnDecalAtLocation(this,Material,Water?FVector(10,38,38):FVector(7,8,16),Location,Rotation,Water?2.f:FootprintLifetime);
 if(D){D->SetFadeScreenSize(.0001f);D->SetFadeOut(Water?.2f:FootprintLifetime*.7f,Water?1.8f:FootprintLifetime*.3f,false);Marks.Add(D);}
}
