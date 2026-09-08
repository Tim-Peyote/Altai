#include "AltaiWetSurface.h"
#include "AltaiWeather.h"
#include "Components/MeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
UAltaiWetSurface::UAltaiWetSurface(){PrimaryComponentTick.bCanEverTick=true;PrimaryComponentTick.TickInterval=.25f;}
void UAltaiWetSurface::BeginPlay()
{
 Super::BeginPlay();Mesh=GetOwner()->FindComponentByClass<UMeshComponent>();
 if(TActorIterator<AAltaiWeatherRig> I(GetWorld());I)Weather=*I;
 if(GetOwner()->ActorHasTag(TEXT("SmoothRock"))){DryGrip=.65f;WetGrip=.25f;}
 if(GetOwner()->ActorHasTag(TEXT("MossyRock"))){DryGrip=.8f;WetGrip=.18f;}
 if(Mesh.IsValid()){
  Material=Mesh->CreateDynamicMaterialInstance(0);
  if(Material){Material->SetScalarParameterValue(TEXT("DryRoughness"),GetOwner()->ActorHasTag(TEXT("SmoothRock"))?.4f:.85f);
}
 }
}
void UAltaiWetSurface::TickComponent(float Dt,ELevelTick T,FActorComponentTickFunction* F)
{
 Super::TickComponent(Dt,T,F);if(!Mesh.IsValid() || !Weather.IsValid())return;
 const FVector P=Mesh->Bounds.Origin+(GetOwner()->ActorHasTag(TEXT("AltaiClimbable"))?GetOwner()->GetActorForwardVector()*(Mesh->Bounds.BoxExtent.X+15):FVector(0,0,Mesh->Bounds.BoxExtent.Z+5));
 FHitResult H;FCollisionQueryParams Q(SCENE_QUERY_STAT(AltaiLocalWetness),false,GetOwner());
 const bool Covered=GetWorld()->LineTraceSingleByChannel(H,P,P+FVector(0,0,2500),ECC_Visibility,Q);
 Wetness=FMath::Clamp(Wetness+Dt*((Covered?0:Weather->Rain)*.09f-(Weather->Rain<.1f?.025f:0)),0.f,1.f);
 if(Material)Material->SetScalarParameterValue(TEXT("Wetness"),Wetness);
}
