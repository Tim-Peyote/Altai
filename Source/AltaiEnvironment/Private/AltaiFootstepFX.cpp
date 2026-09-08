#include "AltaiFootstepFX.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "Engine/StaticMesh.h"
AAltaiFootstepFX::AAltaiFootstepFX()
{
 PrimaryActorTick.bCanEverTick=true;
 Ring=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Ring"));RootComponent=Ring;Ring->SetMobility(EComponentMobility::Movable);Ring->SetCollisionEnabled(ECollisionEnabled::NoCollision);Ring->SetCastShadow(false);
 Ring->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Plane.Plane")));
 Flecks=CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Flecks"));Flecks->SetupAttachment(RootComponent);Flecks->SetCollisionEnabled(ECollisionEnabled::NoCollision);Flecks->SetCastShadow(false);Flecks->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Sphere.Sphere")));
}
void AAltaiFootstepFX::Configure(bool Water,bool Snow,UMaterialInterface* Material)
{
 WaterStep=Water;Ring->SetVisibility(Water);if(Material)Ring->SetMaterial(0,Material);
 Flecks->SetVisibility(!Water);Flecks->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,Snow?TEXT("/Game/Altai/Environment/Materials/M_SnowStepParticle.M_SnowStepParticle"):TEXT("/Game/Altai/Environment/Materials/M_MudStepParticle.M_MudStepParticle")));
 for(int32 I=0;I<8;++I)Flecks->AddInstance(FTransform(FQuat::Identity,FVector::ZeroVector,FVector(.025)),false);
 SetLifeSpan(1.2f);
}
void AAltaiFootstepFX::Tick(float Dt)
{
 Super::Tick(Dt);Age+=Dt;
 if(WaterStep){Ring->SetWorldScale3D(FVector(.18f+Age, .18f+Age,1));return;}
 for(int32 I=0;I<8;++I){const float Angle=I*2.39996f,Speed=16+I*3;const float Z=FMath::Max(0.f,(48+I*4)*Age-100*Age*Age);
  Flecks->UpdateInstanceTransform(I,FTransform(FRotator(Age*60,I*43,0),FVector(FMath::Cos(Angle)*Speed*Age,FMath::Sin(Angle)*Speed*Age,Z),FVector(.035f*FMath::Clamp(1.2f-Age,0.f,1.f))),false,I==7,true);}
}
