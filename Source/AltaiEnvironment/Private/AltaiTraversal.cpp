#include "AltaiTraversal.h"
#include "AltaiWallClimbing.h"
#include "AltaiHands.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "CollisionShape.h"

UAltaiTraversal::UAltaiTraversal(){PrimaryComponentTick.bCanEverTick=true;}
void UAltaiTraversal::BeginPlay(){Super::BeginPlay();Character=Cast<ACharacter>(GetOwner());}
void UAltaiTraversal::ReleaseControl()
{
 if(Character.IsValid()){
  Character->GetCharacterMovement()->SetMovementMode(MOVE_Falling);
  if(LockedInput)if(auto* PC=Cast<APlayerController>(Character->GetController()))PC->SetIgnoreMoveInput(false);
 }
 if(auto* H=GetOwner()->FindComponentByClass<UAltaiHands>())H->SetWallContacts({},false);
 GetOwner()->Tags.Remove(TEXT("AltaiMantling"));LockedInput=false;Climbing=false;
}
void UAltaiTraversal::EndPlay(const EEndPlayReason::Type R){if(Climbing)ReleaseControl();Super::EndPlay(R);}
void UAltaiTraversal::CancelClimb(){if(Climbing){ReleaseControl();Hint=TEXT("Released ledge");}}
bool UAltaiTraversal::FindLedge(FVector& Destination)
{
 if(!Character.IsValid())return false;
 auto* C=Character.Get();auto* Cap=C->GetCapsuleComponent();
 if(C->ActorHasTag(TEXT("AltaiWallAttached")))return false;
 if(!C->GetCharacterMovement()->IsMovingOnGround()){Hint=TEXT("Stand near a ledge");return false;}
 const FVector Dir=FRotationMatrix(FRotator(0,C->GetControlRotation().Yaw,0)).GetUnitAxis(EAxis::X);
 const FVector Feet=C->GetActorLocation()-FVector(0,0,Cap->GetScaledCapsuleHalfHeight());
 FCollisionQueryParams Q(SCENE_QUERY_STAT(AltaiMantle),false,C);FHitResult Wall,Top;
 bool Found=false;
 for(float Z:{55.f,100.f,150.f}){
  if(GetWorld()->LineTraceSingleByChannel(Wall,Feet+FVector(0,0,Z),Feet+FVector(0,0,Z)+Dir*145,ECC_Visibility,Q) && Wall.ImpactNormal.Z<.45f){Found=true;break;}
 }
 if(!Found){Hint=TEXT("Look at a reachable rock ledge");return false;}
 const FVector Inside=Wall.ImpactPoint+Dir*(Cap->GetScaledCapsuleRadius()+20);
 if(!GetWorld()->LineTraceSingleByChannel(Top,FVector(Inside.X,Inside.Y,Feet.Z+220),FVector(Inside.X,Inside.Y,Feet.Z+35),ECC_Visibility,Q) || Top.ImpactNormal.Z<.7f){Hint=TEXT("No walkable top within reach");return false;}
 const float Rise=Top.ImpactPoint.Z-Feet.Z;
 if(Rise<45 || Rise>205){Hint=TEXT("Ledge is outside reach");return false;}
 Destination=Top.ImpactPoint+FVector(0,0,Cap->GetScaledCapsuleHalfHeight()+4);
 if(GetWorld()->OverlapBlockingTestByChannel(Destination,FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(Cap->GetScaledCapsuleRadius(),Cap->GetScaledCapsuleHalfHeight()-1),Q)){Hint=TEXT("Not enough room above ledge");return false;}
 LedgePoint=FVector(Wall.ImpactPoint.X,Wall.ImpactPoint.Y,Top.ImpactPoint.Z);
 Hint=TEXT("E: climb ledge");return true;
}
bool UAltaiTraversal::TryClimbFromWall()
{
 auto* W=GetOwner()->FindComponentByClass<UAltaiWallClimbing>();
 auto* H=GetOwner()->FindComponentByClass<UAltaiHands>();
 if(Climbing || !Character.IsValid() || !W || !W->Attached || !H || H->Held)return false;
 if(!W->ContactActive[0] || !W->ContactActive[1] || W->MovingLimb!=INDEX_NONE || W->Stamina<.15f){Hint=TEXT("Both hands and stamina needed for mantle");return false;}
 auto* C=Character.Get();auto* Cap=C->GetCapsuleComponent();const FVector P=C->GetActorLocation(),Dir=C->GetActorForwardVector();
 FCollisionQueryParams Q(SCENE_QUERY_STAT(AltaiWallMantle),false,C);FHitResult Top;
 // Search immediately over the lip, then check the full standing capsule at the landing.
 const FVector Inside=P+Dir*(44+Cap->GetScaledCapsuleRadius()+22);
 if(!GetWorld()->LineTraceSingleByChannel(Top,Inside+FVector(0,0,95),Inside-FVector(0,0,25),ECC_Visibility,Q) || Top.ImpactNormal.Z<.7f){Hint=TEXT("Reach the top edge before pulling up");return false;}
 Finish=Top.ImpactPoint+FVector(0,0,Cap->GetScaledCapsuleHalfHeight()+4);
 if(GetWorld()->OverlapBlockingTestByChannel(Finish,FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(Cap->GetScaledCapsuleRadius(),Cap->GetScaledCapsuleHalfHeight()-1),Q)){Hint=TEXT("Landing occupied");return false;}
 StartContacts=H->ContactGoals;LedgePoint=P+Dir*44;LedgePoint.Z=Top.ImpactPoint.Z;
 W->ReleaseWall();FromWall=true;return BeginClimb();
}
bool UAltaiTraversal::TryClimb()
{
 if(Climbing)return false;
 if(auto* H=GetOwner()->FindComponentByClass<UAltaiHands>();H && H->Held)return false;
 if(!FindLedge(Finish)){CanClimb=false;return false;}
 FromWall=false;return BeginClimb();
}
bool UAltaiTraversal::BeginClimb()
{
 auto* C=Character.Get();Start=C->GetActorLocation();Above=FVector(Start.X,Start.Y,Finish.Z+8);Elapsed=0;Climbing=true;
 C->Tags.AddUnique(TEXT("AltaiMantling"));
 if(!FromWall)C->SetActorRotation(FRotator(0,C->GetControlRotation().Yaw,0));
 C->GetCharacterMovement()->StopMovementImmediately();C->GetCharacterMovement()->DisableMovement();
 if(auto* PC=Cast<APlayerController>(C->GetController())){PC->SetIgnoreMoveInput(true);LockedInput=true;}
 Hint=TEXT("Pulling up - C to release");return true;
}
void UAltaiTraversal::TickComponent(float Dt,ELevelTick T,FActorComponentTickFunction* F)
{
 Super::TickComponent(Dt,T,F);if(!Character.IsValid())return;
 if(!Climbing){FVector Unused;CanClimb=FindLedge(Unused);return;}
 Elapsed+=Dt;
 const float A=FMath::Clamp(Elapsed/(FromWall?1.65f:1.15f),0.f,1.f);
 if(FromWall)if(auto* Hands=GetOwner()->FindComponentByClass<UAltaiHands>()){
  Hands->SetWallContacts(StartContacts,true);
  const float Release=1.f-FMath::SmoothStep(.12f,.55f,A);
  Hands->ContactWeights[0]=Hands->ContactWeights[1]=Release;
  Hands->ContactWeights[2]=Hands->ContactWeights[3]=1.f-FMath::SmoothStep(0.f,.22f,A);
 }
 const float Phase=A<.45f?A/.45f:(A<.8f?(A-.45f)/.35f:(A-.8f)/.2f);
 const float Smooth=FMath::SmoothStep(0.f,1.f,Phase);
 const FVector OverTop(Finish.X,Finish.Y,Above.Z);
 const FVector P=A<.45f?FMath::Lerp(Start,Above,Smooth):(A<.8f?FMath::Lerp(Above,OverTop,Smooth):FMath::Lerp(OverTop,Finish,Smooth));
 FHitResult Hit;Character->SetActorLocation(P,true,&Hit);
 if(Hit.bBlockingHit){
  if(A>=.8f && Hit.ImpactNormal.Z>=.7f){++CompletedClimbs;ReleaseControl();Hint=TEXT("Ledge reached");return;}
  CancelClimb();Hint=TEXT("Climb blocked - reposition");return;}
 if(A>=1){++CompletedClimbs;ReleaseControl();Hint=TEXT("Ledge reached");}
}
