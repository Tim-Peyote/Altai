#include "AltaiGripZone.h"
#include "Components/BoxComponent.h"
AAltaiGripZone::AAltaiGripZone()
{
 Bounds=CreateDefaultSubobject<UBoxComponent>(TEXT("ContactRegion"));RootComponent=Bounds;
 Bounds->SetBoxExtent(FVector(25,250,150));Bounds->SetCollisionEnabled(ECollisionEnabled::NoCollision);Bounds->SetHiddenInGame(true);
}
bool AAltaiGripZone::Contains(const FVector& Point,const AActor* WallActor) const
{
 if(SurfaceActor!=WallActor)return false;
 const FVector P=Bounds->GetComponentTransform().InverseTransformPosition(Point),E=Bounds->GetUnscaledBoxExtent();
 const float Edge=FMath::Max(FMath::Abs(P.Y)/E.Y,FMath::Abs(P.Z)/E.Z);
 const float Noise=FMath::Sin(Point.Y*.045f+FMath::Sin(Point.Z*.07f)*2)*.08f+FMath::Sin(Point.Z*.12f)*.025f;
 // Matches M_LabCliffPatch opacity: the gameplay boundary follows the visible stratum.
 return FMath::Abs(P.X)<=E.X && (.93f-Edge+Noise)*12>=.333f;
}
