#include "AltaiTraversal.h"
#include "AltaiWallClimbing.h"
#include "AltaiHands.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "CollisionShape.h"

namespace {
float Phase(float A,float B,float T){return FMath::SmoothStep(A,B,T);}
FVector StepArc(const FVector& A,const FVector& B,float T,const FVector& Out){return FMath::Lerp(A,B,T)+Out*FMath::Sin(PI*T);}
}
UAltaiTraversal::UAltaiTraversal(){PrimaryComponentTick.bCanEverTick=true;}
void UAltaiTraversal::BeginPlay(){Super::BeginPlay();Character=Cast<ACharacter>(GetOwner());}
void UAltaiTraversal::ReleaseControl(bool Grounded)
{
 if(Character.IsValid()){
  auto* C=Character.Get();auto* Cap=C->GetCapsuleComponent();
  // Restore standing volume only where it fits. A blocked abort stays crouched.
  const float Delta=FullHeight-Cap->GetScaledCapsuleHalfHeight();
  FCollisionQueryParams Q(SCENE_QUERY_STAT(AltaiRestoreCapsule),false,C);
  const FVector Raised=C->GetActorLocation();
  if(Delta>.1f && !GetWorld()->OverlapBlockingTestByChannel(Raised,FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(Cap->GetScaledCapsuleRadius(),FullHeight-1),Q)){
   Cap->SetCapsuleHalfHeight(FullHeight/Cap->GetShapeScale(),true);
  }else if(Delta>.1f){C->bIsCrouched=true;C->GetCharacterMovement()->bWantsToCrouch=true;C->OnStartCrouch(Delta/Cap->GetShapeScale(),Delta);}
  if(C->ActorHasTag(TEXT("AltaiViewConfigured"))){OldYaw=C->ActorHasTag(TEXT("AltaiFirstPerson"));OldOrient=!OldYaw;}
  C->bUseControllerRotationYaw=OldYaw;C->GetCharacterMovement()->bOrientRotationToMovement=OldOrient;
  C->GetCharacterMovement()->SetMovementMode(Grounded?MOVE_Walking:MOVE_Falling);
  if(LockedInput)if(auto* PC=Cast<APlayerController>(C->GetController()))PC->SetIgnoreMoveInput(false);
 }
 if(auto* H=GetOwner()->FindComponentByClass<UAltaiHands>())H->SetWallContacts({},false);
 GetOwner()->Tags.Remove(TEXT("AltaiMantling"));LockedInput=false;Climbing=false;BodyOffset=FVector::ZeroVector;BodyLean=0;PalmDown=0;
}
void UAltaiTraversal::EndPlay(const EEndPlayReason::Type R){if(Climbing)ReleaseControl();Super::EndPlay(R);}
void UAltaiTraversal::CancelClimb(){if(Climbing){ReleaseControl();Hint=TEXT("Released ledge");}}
bool UAltaiTraversal::FindLedge(FVector& Destination)
{
 if(!Character.IsValid())return false;auto* C=Character.Get();auto* Cap=C->GetCapsuleComponent();
 if(C->ActorHasTag(TEXT("AltaiWallAttached")) || !C->GetCharacterMovement()->IsMovingOnGround())return false;
 const FVector Dir=FRotationMatrix(FRotator(0,C->GetControlRotation().Yaw,0)).GetUnitAxis(EAxis::X);
 const FVector Feet=C->GetActorLocation()-FVector(0,0,Cap->GetScaledCapsuleHalfHeight());
 FCollisionQueryParams Q(SCENE_QUERY_STAT(AltaiMantle),false,C);FHitResult Wall,Top;bool Found=false;
 for(float Z:{25.f,55.f,100.f,150.f})if(GetWorld()->LineTraceSingleByChannel(Wall,Feet+FVector(0,0,Z),Feet+FVector(0,0,Z)+Dir*115,ECC_Visibility,Q) && FMath::Abs(Wall.ImpactNormal.Z)<.35f){Found=true;break;}
 if(!Found){Hint=TEXT("E: face a ledge | C: lower over an edge");return false;}
 Normal=Wall.ImpactNormal.GetSafeNormal2D();Right=FVector::CrossProduct(FVector::UpVector,-Normal);Surface=Wall.GetComponent();
 const FVector Inside=Wall.ImpactPoint-Normal*(Cap->GetScaledCapsuleRadius()+18);
 if(!GetWorld()->LineTraceSingleByChannel(Top,FVector(Inside.X,Inside.Y,Feet.Z+195),FVector(Inside.X,Inside.Y,Feet.Z+35),ECC_Visibility,Q) || Top.ImpactNormal.Z<.7f)return false;
 const float Rise=Top.ImpactPoint.Z-Feet.Z;if(Rise<40 || Rise>175){Hint=TEXT("Ledge outside comfortable reach");return false;}
 Destination=Top.ImpactPoint+FVector(0,0,Cap->GetScaledCapsuleHalfHeight()+3);
 LedgePoint=FVector(Wall.ImpactPoint.X,Wall.ImpactPoint.Y,Top.ImpactPoint.Z);
 if(FVector::Dist2D(C->GetActorLocation(),LedgePoint)>65.f || FVector::Dist(C->GetMesh()->GetSocketLocation(TEXT("upperarm_r")),LedgePoint+Right*24+Normal*7)>115.f){Hint=TEXT("Move closer to the lip");return false;}
 Hint=TEXT("E: mantle");return true;
}
bool UAltaiTraversal::TryClimbFromWall()
{
 auto* W=GetOwner()->FindComponentByClass<UAltaiWallClimbing>();auto* H=GetOwner()->FindComponentByClass<UAltaiHands>();
 if(Climbing || !Character.IsValid() || !W || !W->Attached || !H || H->Held)return false;
 if(!W->ContactActive[0] || !W->ContactActive[1] || W->MovingLimb!=INDEX_NONE || W->Stamina<.15f){Hint=TEXT("Regrip both hands before the final push");return false;}
 auto* C=Character.Get();auto* Cap=C->GetCapsuleComponent();const FVector P=C->GetActorLocation(),Dir=C->GetActorForwardVector();
 FCollisionQueryParams Q(SCENE_QUERY_STAT(AltaiWallMantle),false,C);FHitResult Top,Face;
 if(!GetWorld()->LineTraceSingleByChannel(Face,P,P+Dir*100,ECC_Visibility,Q))return false;
 Normal=Face.ImpactNormal.GetSafeNormal2D();Right=FVector::CrossProduct(FVector::UpVector,-Normal);Surface=Face.GetComponent();
 const FVector Inside=Face.ImpactPoint-Normal*(Cap->GetScaledCapsuleRadius()+18);
 if(!GetWorld()->LineTraceSingleByChannel(Top,Inside+FVector(0,0,85),Inside-FVector(0,0,15),ECC_Visibility,Q) || Top.ImpactNormal.Z<.7f){Hint=TEXT("Reach the top edge before pulling up");return false;}
 Finish=Top.ImpactPoint+FVector(0,0,Cap->GetScaledCapsuleHalfHeight()+3);
 LedgePoint=FVector(Face.ImpactPoint.X,Face.ImpactPoint.Y,Top.ImpactPoint.Z);
 StartContacts=H->ContactGoals;FromWall=true;Descending=false;
 // Validate before surrendering the wall's support or input lock.
 Start=P;FullHeight=Cap->GetScaledCapsuleHalfHeight();CompactHeight=Cap->GetScaledCapsuleRadius()+2;
 Above=FVector(Start.X,Start.Y,LedgePoint.Z+CompactHeight+3);
 if(!ValidatePath()){Hint=TEXT("Top or mantle path obstructed");return false;}
 W->ReleaseWall();return BeginClimb();
}
bool UAltaiTraversal::TryClimb()
{
 if(Climbing)return false;if(auto* H=GetOwner()->FindComponentByClass<UAltaiHands>();H && H->Held)return false;
 if(!FindLedge(Finish)){CanClimb=false;return false;}FromWall=false;Descending=false;return BeginClimb();
}
bool UAltaiTraversal::TryDescend()
{
 auto* C=Character.Get();auto* H=GetOwner()->FindComponentByClass<UAltaiHands>();
 if(!C || Climbing || !H || H->Held || C->bIsCrouched || !C->GetCharacterMovement()->IsMovingOnGround())return false;
 auto* Cap=C->GetCapsuleComponent();FullHeight=Cap->GetScaledCapsuleHalfHeight();CompactHeight=Cap->GetScaledCapsuleRadius()+2;
 const FVector P=C->GetActorLocation(),Dir=FRotationMatrix(FRotator(0,C->GetControlRotation().Yaw,0)).GetUnitAxis(EAxis::X);
 const float Floor=P.Z-FullHeight;
 FCollisionQueryParams Q(SCENE_QUERY_STAT(AltaiLowerToHang),false,C);FHitResult Face,Top;
 const FVector Outside=P+Dir*125;
 // Find the outward-facing vertical surface just below the floor, not a distant wall.
 if(!GetWorld()->LineTraceSingleByChannel(Face,FVector(Outside.X,Outside.Y,Floor-28),FVector(P.X,P.Y,Floor-28),ECC_Visibility,Q) || FMath::Abs(Face.ImpactNormal.Z)>.35f || !Face.GetActor()->ActorHasTag(TEXT("AltaiClimbable"))){Hint=TEXT("Face a climbable edge and move closer");return false;}
 Normal=Face.ImpactNormal.GetSafeNormal2D();Right=FVector::CrossProduct(FVector::UpVector,-Normal);Surface=Face.GetComponent();
 const FVector Inside=Face.ImpactPoint-Normal*20;
 if(!GetWorld()->LineTraceSingleByChannel(Top,FVector(Inside.X,Inside.Y,Floor+25),FVector(Inside.X,Inside.Y,Floor-15),ECC_Visibility,Q) || Top.ImpactNormal.Z<.7f)return false;
 LedgePoint=FVector(Face.ImpactPoint.X,Face.ImpactPoint.Y,Top.ImpactPoint.Z);
 if(FVector::Dist2D(P,LedgePoint)>95){Hint=TEXT("Move nearer the edge before lowering");return false;}
 Start=LedgePoint+Normal*(Cap->GetScaledCapsuleRadius()+8)-FVector(0,0,50);
 Finish=LedgePoint-Normal*(Cap->GetScaledCapsuleRadius()+18)+FVector(0,0,FullHeight+3);
 // Begin at the player's actual standing position, preserving continuity.
 Finish=P;Above=FVector(Start.X,Start.Y,LedgePoint.Z+CompactHeight+3);
 StartContacts.Reset();
 for(int32 I=0;I<4;++I){FHitResult Contact;const FVector Desired=Start+Right*((I%2?1.f:-1.f)*(I<2?24:22))+FVector(0,0,I<2?48:-60);
  if(!GetWorld()->LineTraceSingleByChannel(Contact,Desired+Normal*25,Desired-Normal*95,ECC_Visibility,Q) || Contact.GetComponent()!=Surface.Get()){Hint=TEXT("No support below this edge");return false;}
  StartContacts.Add(Contact.ImpactPoint+Normal*(I<2?7:16));}
 if(!ValidatePath()){Hint=TEXT("Not enough clearance to lower over this edge");return false;}
 Descending=true;FromWall=true;return BeginClimb();
}
bool UAltaiTraversal::ValidatePath() const
{
 auto* C=Character.Get();const float Radius=C->GetCapsuleComponent()->GetScaledCapsuleRadius();
 FCollisionQueryParams Q(SCENE_QUERY_STAT(AltaiMantlePath),false,C);FHitResult Hit;
 if(GetWorld()->OverlapBlockingTestByChannel(Finish,FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(Radius,FullHeight-1),Q))return false;
 const FVector Over(Finish.X,Finish.Y,Above.Z);
 for(const auto& Pair:TArray<TPair<FVector,FVector>>{{Start,Above},{Above,Over}})
  if(GetWorld()->SweepSingleByChannel(Hit,Pair.Key,Pair.Value,FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(Radius,CompactHeight-1),Q))return false;
 return true;
}
bool UAltaiTraversal::BeginClimb()
{
 auto* C=Character.Get();auto* Cap=C->GetCapsuleComponent();
 if(!Descending){Start=C->GetActorLocation();FullHeight=Cap->GetScaledCapsuleHalfHeight();CompactHeight=Cap->GetScaledCapsuleRadius()+2;Above=FVector(Start.X,Start.Y,LedgePoint.Z+CompactHeight+3);}
 if(!ValidatePath()){Hint=TEXT("Mantle path obstructed");return false;}
 if(!FromWall){StartContacts.Reset();for(const FName B:{FName(TEXT("hand_l")),FName(TEXT("hand_r")),FName(TEXT("foot_l")),FName(TEXT("foot_r"))})StartContacts.Add(C->GetMesh()->GetSocketLocation(B));}
 OldYaw=C->bUseControllerRotationYaw;OldOrient=C->GetCharacterMovement()->bOrientRotationToMovement;
 ViewTurn=Descending && C->ActorHasTag(TEXT("AltaiFirstPerson"))?FMath::FindDeltaAngleDegrees(C->GetControlRotation().Yaw,(-Normal).Rotation().Yaw):0.f;AppliedViewTurn=0;
 TurnStartRotation=C->GetActorQuat();
 C->bUseControllerRotationYaw=false;C->GetCharacterMovement()->bOrientRotationToMovement=false;
 if(!Descending)C->SetActorRotation((-Normal).Rotation());Elapsed=0;Progress=0;Climbing=true;C->Tags.AddUnique(TEXT("AltaiMantling"));
 C->GetCharacterMovement()->StopMovementImmediately();C->GetCharacterMovement()->DisableMovement();
 if(auto* PC=Cast<APlayerController>(C->GetController())){PC->SetIgnoreMoveInput(true);LockedInput=true;}
 Hint=Descending?TEXT("Hands on lip - lowering to the wall"):TEXT("Grip lip - push - step onto ledge");return true;
}
void UAltaiTraversal::TickComponent(float Dt,ELevelTick T,FActorComponentTickFunction* F)
{
 Super::TickComponent(Dt,T,F);if(!Character.IsValid())return;
 if(!Climbing){FVector Unused;CanClimb=FindLedge(Unused);return;}
 auto* C=Character.Get();auto* Cap=C->GetCapsuleComponent();auto* H=GetOwner()->FindComponentByClass<UAltaiHands>();
 if(!Surface.IsValid() || !H){CancelClimb();return;}
 Elapsed+=Dt;
 C->SetActorRotation(Descending?FQuat::Slerp(TurnStartRotation,(-Normal).Rotation().Quaternion(),Phase(0.f,.5f,Elapsed)):(-Normal).Rotation().Quaternion());
 Progress=FMath::Clamp((Elapsed-(Descending?.5f:0.f))/(Descending?3.f:2.6f),0.f,1.f);const float A=Descending?1-Progress:Progress;
 // Add the deliberate turn-to-face-wall motion without discarding mouse look input.
 if(Descending && FMath::Abs(ViewTurn)>.01f)if(auto* PC=Cast<APlayerController>(C->GetController())){
  const float Turn=ViewTurn*Phase(0.f,.5f,Elapsed);auto Look=PC->GetControlRotation();Look.Yaw+=Turn-AppliedViewTurn;PC->SetControlRotation(Look);AppliedViewTurn=Turn;
 }
 const float Stand=Phase(.82f,1.f,A);const float Half=FMath::Lerp(CompactHeight,FullHeight,Stand);
 // Shrinking volume enables a real crouched rock-over; collision stays enabled throughout.
 Cap->SetCapsuleHalfHeight(Half/Cap->GetShapeScale(),false);
 const FVector Over(Finish.X,Finish.Y,Above.Z);
 const FVector P=A<.18f?Start:A<.48f?FMath::Lerp(Start,Above,Phase(.18f,.48f,A)):A<.82f?FMath::Lerp(Above,Over,Phase(.56f,.82f,A)):FMath::Lerp(Over,Finish,Stand);
 FHitResult Hit;C->SetActorLocation(P,true,&Hit);
 if(Hit.bBlockingHit){CancelClimb();Hint=TEXT("Movement blocked - released ledge");return;}
 BodyOffset=FVector(0,0,(-10.f*Phase(.18f,.44f,A)+18.f*Phase(.50f,.66f,A))*(1-Stand));
 BodyLean=75.f*Phase(.22f,.52f,A)*(1-Stand);PalmDown=Phase(.26f,.52f,A);
 TArray<FVector> Goals=StartContacts;
 for(int32 I=0;I<2;++I){const FVector Lip=LedgePoint+Right*((I?1.f:-1.f)*24)+Normal*7+FVector(0,0,3);
  Goals[I]=StepArc(StartContacts[I],Lip,Phase(I?.02f:.06f,I?.12f:.18f,A),Normal*6);
  // Re-seat one palm farther onto the top before the hips cross the lip.
  // The reverse motion walks the hands back to the edge before lowering.
  if(A>(I?.40f:.49f))Goals[I]=StepArc(Lip,Lip-Normal*32.f,Phase(I?.40f:.49f,I?.48f:.57f,A),FVector(0,0,6));}
 const FVector FootBase(Finish.X,Finish.Y,LedgePoint.Z+8);
 const FVector First=LedgePoint-Normal*16+Right*17+FVector(0,0,8);
 const FVector LeftLow=LedgePoint+Normal*16-Right*22-FVector(0,0,65);
 const FVector LeftWall=LedgePoint+Normal*16-Right*22-FVector(0,0,20);
 const FVector RightWall=LedgePoint+Normal*16+Right*22-FVector(0,0,50);
 const FVector RightClear=LedgePoint+Normal*24+Right*17+FVector(0,0,14);
 const FVector LeftClear=LedgePoint+Normal*24-Right*17+FVector(0,0,14);
 // Lift each foot outside the face before crossing the lip; a diagonal arc cuts through the rock.
 Goals[3]=A<.32f?StepArc(StartContacts[3],RightWall,Phase(.23f,.32f,A),Normal*10):A<.46f?StepArc(RightWall,RightClear,Phase(.32f,.46f,A),Normal*10):A<.8f?StepArc(RightClear,First,Phase(.46f,.56f,A),FVector(0,0,8)):StepArc(First,FootBase+Right*17,Phase(.80f,.92f,A),FVector(0,0,14));
 Goals[2]=A<.28f?StepArc(StartContacts[2],LeftLow,Phase(.12f,.23f,A),Normal*10):A<.55f?StepArc(LeftLow,LeftWall,Phase(.28f,.42f,A),Normal*10):A<.68f?StepArc(LeftWall,LeftClear,Phase(.55f,.68f,A),Normal*10):StepArc(LeftClear,FootBase-Right*17,Phase(.68f,.80f,A),FVector(0,0,8));
 if(Descending && Elapsed<.5f){
  // Turn on the top before lowering. Feet follow the turning stance rather
  // than swapping left/right targets while the torso is still facing out.
  Goals[2]=FootBase-C->GetActorRightVector()*17;Goals[3]=FootBase+C->GetActorRightVector()*17;
 }
 H->SetWallContacts(Goals,true);
 H->ContactWeights[0]=1-Phase(.58f,.78f,A);H->ContactWeights[1]=1-Phase(.50f,.70f,A);
 H->ContactWeights[2]=H->ContactWeights[3]=1;
 if(A<.18f && !FromWall){H->ContactWeights[0]=Phase(.04f,.15f,A);H->ContactWeights[1]=Phase(0.f,.12f,A);}
 if(Progress>=1){
  if(Descending){
   const auto Contacts=StartContacts;auto* SurfacePtr=Surface.Get();const FVector Out=Normal;
   // The full capsule fits outside the face at the hang destination.
   Cap->SetCapsuleHalfHeight(FullHeight/Cap->GetShapeScale(),true);
   ReleaseControl();if(auto* W=GetOwner()->FindComponentByClass<UAltaiWallClimbing>();W && W->AttachFromLedge(SurfacePtr,Out,Contacts)){++CompletedDescents;Hint=TEXT("Hanging - S down / W up / RMB limb");}
   else Hint=TEXT("Support unavailable - falling");
  }else{++CompletedClimbs;ReleaseControl(true);Hint=TEXT("On the ledge - C near edge to descend");}
 }
}
