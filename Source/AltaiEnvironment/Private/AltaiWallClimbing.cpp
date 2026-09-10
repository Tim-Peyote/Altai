#include "AltaiWallClimbing.h"
#include "AltaiHands.h"
#include "AltaiTraversal.h"
#include "AltaiGripZone.h"
#include "AltaiWetSurface.h"
#include "AltaiWeather.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Components/PrimitiveComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "InputCoreTypes.h"

UAltaiWallClimbing::UAltaiWallClimbing(){PrimaryComponentTick.bCanEverTick=true;LimbStamina.Init(1,4);LimbGrip.Init(1,4);SupportLoad.Init(0,4);}
void UAltaiWallClimbing::BeginPlay()
{
 Super::BeginPlay();Character=Cast<ACharacter>(GetOwner());Hands=GetOwner()->FindComponentByClass<UAltaiHands>();
 if(TActorIterator<AAltaiWeatherRig> I(GetWorld());I)Weather=*I;
 for(TActorIterator<AAltaiGripZone> I(GetWorld());I;++I)GripZones.Add(*I);
}
void UAltaiWallClimbing::EndPlay(const EEndPlayReason::Type R){ReleaseWall();Super::EndPlay(R);}
bool UAltaiWallClimbing::Probe(const FVector& Desired,FVector& Point) const
{
 if(!Character.IsValid() || !Wall.IsValid())return false;
 FHitResult H;FCollisionQueryParams Q(SCENE_QUERY_STAT(AltaiWallContact),false,Character.Get());
 if(!GetWorld()->LineTraceSingleByChannel(H,Desired+Normal*90,Desired-Normal*100,ECC_Visibility,Q) || H.GetComponent()!=Wall.Get())return false;
 if(FVector::DotProduct(H.ImpactNormal,Normal)<.6)return false;
 Point=H.ImpactPoint+Normal*7;return true;
}
bool UAltaiWallClimbing::AttachWall()
{
 if(GetOwner()->ActorHasTag(TEXT("AltaiBodyUnbalanced")))return false;
 if(Attached)return true;
 if(auto* Traversal=GetOwner()->FindComponentByClass<UAltaiTraversal>();Traversal && Traversal->Climbing)return false;
 if(!Character.IsValid() || !Hands.IsValid() || Hands->Held)return false;
 auto* C=Character.Get();FHitResult H;FCollisionQueryParams Q(SCENE_QUERY_STAT(AltaiWallStart),false,C);
 const FVector D=FRotationMatrix(FRotator(0,C->GetControlRotation().Yaw,0)).GetUnitAxis(EAxis::X);
 const FVector P=C->GetActorLocation();
 if(!GetWorld()->LineTraceSingleByChannel(H,P,P+D*150,ECC_Visibility,Q) || !H.GetActor()->ActorHasTag(TEXT("AltaiClimbable")) || FMath::Abs(H.ImpactNormal.Z)>.35f)return false;
 Wall=H.GetComponent();Normal=H.ImpactNormal.GetSafeNormal2D();Right=FVector::CrossProduct(FVector::UpVector,-Normal).GetSafeNormal();
 const FVector Center=H.ImpactPoint+Normal*44;
 TArray<FVector> Goals;
 for(int I=0;I<4;++I){FVector G;
  const FVector Offset=Right*((I%2?1.f:-1.f)*(I<2?28:22))+FVector(0,0,I<2?45:-60);
  if(!Probe(Center+Offset,G)){Hint=TEXT("Need four reachable contacts");Wall.Reset();return false;}
  if(I>=2)G+=Normal*9;Goals.Add(G);
 }
 FHitResult Hit;C->SetActorLocation(Center,true,&Hit);if(Hit.bBlockingHit){Wall.Reset();return false;}
 return StartAttachment(Goals,true);
}
bool UAltaiWallClimbing::AttachFromLedge(UPrimitiveComponent* Surface,const FVector& Outward,const TArray<FVector>& Contacts)
{
 if(Attached || !Character.IsValid() || !Hands.IsValid() || !Surface || Contacts.Num()!=4)return false;
 Wall=Surface;Normal=Outward.GetSafeNormal2D();Right=FVector::CrossProduct(FVector::UpVector,-Normal).GetSafeNormal();
 return StartAttachment(Contacts,false);
}
bool UAltaiWallClimbing::StartAttachment(const TArray<FVector>& Goals,bool Sequential)
{
 auto* C=Character.Get();
 LocalContacts.Reset();for(auto G:Goals)LocalContacts.Add(Wall->GetComponentTransform().InverseTransformPosition(G));
 OldYaw=C->bUseControllerRotationYaw;OldOrient=C->GetCharacterMovement()->bOrientRotationToMovement;
 C->bUseControllerRotationYaw=false;C->GetCharacterMovement()->bOrientRotationToMovement=false;
 C->SetActorRotation((-Normal).Rotation());C->GetCharacterMovement()->StopMovementImmediately();C->GetCharacterMovement()->DisableMovement();
 if(auto* PC=Cast<APlayerController>(C->GetController())){PC->SetIgnoreMoveInput(true);LockedInput=true;}
 DisplayContacts=Goals;MovingLimb=INDEX_NONE;ContactProgress=1;SmoothedIntent=FVector2D::ZeroVector;BodyOffset=FVector::ZeroVector;ClimbSpeed=0;BodyTwist=0;
 Attached=true;C->Tags.AddUnique(TEXT("AltaiWallAttached"));ContactActive.Init(true,4);SlipTime=0;StepTime=0;SettleTime=0;
 LimbStamina[0]=FMath::Min(LimbStamina[0],Hands->Stamina);LimbStamina[1]=FMath::Min(LimbStamina[1],Hands->Stamina);
 // Catch with one hand, then reach with the other while feet carry the load.
 if(Sequential){ContactActive[0]=false;BeginContactMove(0,Goals[0]);}
 Hands->SetWallContacts(DisplayContacts,true);Hint=TEXT("WASD up/down/traverse | RMB select limb | LMB reach | Q rest limb | Space jump | E top");
 return true;
}

void UAltaiWallClimbing::ReleaseWall()
{
 if(!Attached)return;
 Attached=false;MovingLimb=INDEX_NONE;BodyOffset=FVector::ZeroVector;BodyTwist=0;ClimbSpeed=0;ContactActive.Init(false,4);SupportLoad.Init(0,4);if(Hands.IsValid()){Hands->Stamina=(LimbStamina[0]+LimbStamina[1])*.5f;Hands->SetWallContacts({},false);}
 if(Character.IsValid()){
  auto* C=Character.Get();C->Tags.Remove(TEXT("AltaiWallAttached"));if(C->ActorHasTag(TEXT("AltaiViewConfigured"))){OldYaw=C->ActorHasTag(TEXT("AltaiFirstPerson"));OldOrient=!OldYaw;}C->bUseControllerRotationYaw=OldYaw;C->GetCharacterMovement()->bOrientRotationToMovement=OldOrient;
  C->GetCharacterMovement()->SetMovementMode(MOVE_Falling);
  if(LockedInput)if(auto* PC=Cast<APlayerController>(C->GetController()))PC->SetIgnoreMoveInput(false);
 }
 LockedInput=false;Wall.Reset();Hint=TEXT("Released wall");TestIntent=FVector2D::ZeroVector;
}
float UAltaiWallClimbing::Reach(int32 Limb) const
{
 return Hands.IsValid()?Hands->LimbReach[Limb]:(Limb<2?53.f:85.f);
}
bool UAltaiWallClimbing::BeginContactMove(int32 Limb,const FVector& Goal)
{
 if(MovingLimb!=INDEX_NONE || !Wall.IsValid())return false;
 // Do not lift the last supporting hand. Two-point support can hang, but cannot shuffle freely.
 int32 Others=0;bool OtherHand=false;
 for(int32 I=0;I<4;++I)if(I!=Limb && ContactActive[I]){++Others;if(I<2)OtherHand=true;}
 if(ContactActive[Limb] && (Others<2 || !OtherHand)){Hint=TEXT("Restore another support before moving this limb");return false;}
 const FName Bone=Limb==0?TEXT("hand_l"):Limb==1?TEXT("hand_r"):Limb==2?TEXT("foot_l"):TEXT("foot_r");
 MoveStart=Character->GetMesh()->GetSocketLocation(Bone);LocalContacts[Limb]=Wall->GetComponentTransform().InverseTransformPosition(Goal);
 ContactActive[Limb]=false;MovingLimb=Limb;ContactElapsed=0;ContactProgress=0;
 ContactDuration=FMath::Clamp(FVector::Dist(MoveStart,Goal)/100.f,.4f,.8f);LastStep=Limb;SelectedLimb=Limb;
 return true;
}
void UAltaiWallClimbing::LoseContact(int32 Limb)
{
 if(MovingLimb==Limb)MovingLimb=INDEX_NONE;
 ContactActive[Limb]=false;SelectedLimb=Limb;
}
void UAltaiWallClimbing::ReleaseSelectedContact(){if(Attached)LoseContact(SelectedLimb);}
bool UAltaiWallClimbing::JumpOff()
{
 if(!Attached || !Character.IsValid())return false;
 const FVector Velocity=Normal*260+FVector::UpVector*210;
 ReleaseWall();Character->LaunchCharacter(Velocity,true,true);Hint=TEXT("Pushed away from wall");return true;
}
void UAltaiWallClimbing::UpdateContacts(float Dt)
{
 if(!Wall.IsValid() || !Hands.IsValid())return;
 if(MovingLimb!=INDEX_NONE){
  ContactElapsed+=Dt;ContactProgress=FMath::Clamp(ContactElapsed/ContactDuration,0.f,1.f);
  if(ContactProgress>=1){ContactActive[MovingLimb]=true;MovingLimb=INDEX_NONE;SettleTime=.12f;}
 }
 for(int I=0;I<4;++I){
  const FVector Goal=Wall->GetComponentTransform().TransformPosition(LocalContacts[I]);
  if(I==MovingLimb){const float A=FMath::SmoothStep(0.f,1.f,FMath::Clamp((ContactProgress-.18f)/.82f,0.f,1.f));
   DisplayContacts[I]=FMath::Lerp(MoveStart,Goal,A)+Normal*(FMath::Sin(PI*A)*(I<2?9.f:13.f));
  }else if(ContactActive[I])DisplayContacts[I]=Goal;
  else {
   // A freed limb relaxes down/outward instead of snapping to the walking pose.
   const FVector Rest=Character->GetActorLocation()+Right*((I%2?1.f:-1.f)*(I<2?32:22))+FVector(0,0,I<2?5:-85);
   DisplayContacts[I]=FMath::VInterpTo(DisplayContacts[I],Rest,Dt,5.f);
  }
 }
 Hands->SetWallContacts(DisplayContacts,true);
 for(int I=0;I<4;++I)if(!ContactActive[I] && MovingLimb!=I)Hands->ContactWeights[I]=1;
}
void UAltaiWallClimbing::SelectNextLimb(){SelectedLimb=(SelectedLimb+1)%4;}
bool UAltaiWallClimbing::PlaceContact()
{
 if(!Attached || !Wall.IsValid() || !Character.IsValid())return false;
 auto* C=Character.Get();FVector Eye=C->GetActorLocation()+FVector(0,0,50);FRotator View=C->GetControlRotation();
 if(auto* PC=Cast<APlayerController>(C->GetController()))PC->GetPlayerViewPoint(Eye,View);
 // The camera origin and current look direction select the visible hold in either view.
 View=C->GetControlRotation();
 FHitResult H;FCollisionQueryParams Q(SCENE_QUERY_STAT(AltaiManualContact),false,C);
 if(!GetWorld()->LineTraceSingleByChannel(H,Eye,Eye+View.Vector()*900,ECC_Visibility,Q) || H.GetComponent()!=Wall.Get())return false;
 const FName RootBone=SelectedLimb==0?TEXT("upperarm_l"):SelectedLimb==1?TEXT("upperarm_r"):SelectedLimb==2?TEXT("thigh_l"):TEXT("thigh_r");
 const FVector LimbRoot=C->GetMesh()->GetSocketLocation(RootBone);
 if(FVector::Dist(LimbRoot,H.ImpactPoint+Normal*(SelectedLimb<2?7:16))>Reach(SelectedLimb)){Hint=TEXT("Contact outside limb reach");return false;}
 if(LimbStamina[SelectedLimb]<.12f){Hint=TEXT("Rest this limb before regripping");return false;}
 return BeginContactMove(SelectedLimb,H.ImpactPoint+Normal*(SelectedLimb<2?7:16));
}
void UAltaiWallClimbing::TickComponent(float Dt,ELevelTick T,FActorComponentTickFunction* F)
{
 Super::TickComponent(Dt,T,F);
 if(!Attached){for(float& S:LimbStamina)S=FMath::Min(1.f,S+Dt*.08f);return;}
 if(!Character.IsValid() || !Wall.IsValid() || !Hands.IsValid()){ReleaseWall();return;}
 auto* C=Character.Get();auto* PC=Cast<APlayerController>(C->GetController());
 if(PC && InputFromPlayer && PC->WasInputKeyJustPressed(EKeys::SpaceBar)){JumpOff();return;}
 if(PC && InputFromPlayer && PC->WasInputKeyJustPressed(EKeys::Q))ReleaseSelectedContact();
 if(C->bUseControllerRotationYaw || C->GetCharacterMovement()->bOrientRotationToMovement){OldYaw=C->bUseControllerRotationYaw;OldOrient=C->GetCharacterMovement()->bOrientRotationToMovement;}
 C->bUseControllerRotationYaw=false;C->GetCharacterMovement()->bOrientRotationToMovement=false;
 C->SetActorRotation((-Normal).Rotation());
 FVector2D Intent=FVector2D::ZeroVector;bool Strong=false;
 if(PC && InputFromPlayer){Intent.X=(PC->IsInputKeyDown(EKeys::D)?1:0)-(PC->IsInputKeyDown(EKeys::A)?1:0);Intent.Y=(PC->IsInputKeyDown(EKeys::W)?1:0)-(PC->IsInputKeyDown(EKeys::S)?1:0);Strong=PC->IsInputKeyDown(EKeys::LeftShift);}
 IntentExpiry-=Dt;if(IntentExpiry>0)Intent=TestIntent;Intent=Intent.GetClampedToMaxSize(1);
 if(auto* Surface=Wall->GetOwner()->FindComponentByClass<UAltaiWetSurface>()){Wetness=Surface->Wetness;Grip=Surface->Grip();}
 else {Wetness=0;Grip=.95f;}
 float GripTotal=0;int ActiveCount=0;
 for(int I=0;I<4;++I){
  LimbGrip[I]=Grip;const FVector P=Wall->GetComponentTransform().TransformPosition(LocalContacts[I]);
  for(const auto& Zone:GripZones)if(Zone.IsValid() && Zone->Contains(P,Wall->GetOwner())){LimbGrip[I]=Zone->Grip(Wetness);break;}
  if(ContactActive[I]){GripTotal+=LimbGrip[I];++ActiveCount;}
 }
 Grip=ActiveCount?GripTotal/ActiveCount:0;
 SmoothedIntent=FMath::Vector2DInterpTo(SmoothedIntent,Intent,Dt,6.f);
 const FVector Direction=Right*SmoothedIntent.X+FVector::UpVector*SmoothedIntent.Y;
 StepTime+=Dt;SettleTime=FMath::Max(0.f,SettleTime-Dt);
 // Re-seat the most trailing support before the rig reaches full extension.
 // Moving one limb never freezes the whole body; the other contacts constrain its motion.
 if(AssistedStepping && !Intent.IsNearlyZero() && MovingLimb==INDEX_NONE && SettleTime<=0 && ActiveCount>=2){
  int32 Best=INDEX_NONE;float Score=9.f;FVector BestGoal;
  for(int32 I=0;I<4;++I){
   FVector Target=C->GetActorLocation()+Right*((I%2?1.f:-1.f)*(I<2?28:22))+FVector(0,0,I<2?50:-60);
   const FVector Current=Wall->GetComponentTransform().TransformPosition(LocalContacts[I]);
   float Lag=FVector::DotProduct(Target-Current,Direction.GetSafeNormal());
   if(!ContactActive[I])Lag+=80;
   if(I==LastStep)Lag-=6;
   // On descent, lower a foot first; hands follow the lowered body.
   if(Intent.Y<-.1f && I>=2)Lag+=5;
   if(Lag<=Score)continue;
   Target+=Direction.GetSafeNormal()*(I<2?16.f:20.f);
   FVector G;if(!Probe(Target,G))continue;if(I>=2)G+=Normal*9;
   const FName RootBone=I==0?TEXT("upperarm_l"):I==1?TEXT("upperarm_r"):I==2?TEXT("thigh_l"):TEXT("thigh_r");
   if(FVector::Dist(C->GetMesh()->GetSocketLocation(RootBone),G)>Reach(I)*.98f)continue;
   int32 Other=0;bool OtherHand=false;for(int32 J=0;J<4;++J)if(J!=I && ContactActive[J]){++Other;if(J<2)OtherHand=true;}
   if(ContactActive[I] && (Other<2 || !OtherHand))continue;
   Best=I;BestGoal=G;Score=Lag;
  }
  if(Best!=INDEX_NONE)BeginContactMove(Best,BestGoal);
 }
 const float DesiredSpeed=ActiveCount>=2?(MovingLimb==INDEX_NONE?34.f:12.f)*FMath::Lerp(.65f,1.f,Grip):0;
 ClimbSpeed=FMath::FInterpTo(ClimbSpeed,Intent.IsNearlyZero()?0.f:DesiredSpeed,Dt,5.f);
 FVector Move=Direction*ClimbSpeed*Dt;
 // Project the requested displacement into the intersection of supporting limb reach spheres.
 for(int32 Pass=0;Pass<3;++Pass)for(int32 I=0;I<4;++I)if(ContactActive[I]){
  const FName RootBone=I==0?TEXT("upperarm_l"):I==1?TEXT("upperarm_r"):I==2?TEXT("thigh_l"):TEXT("thigh_r");
  const FVector Root=C->GetMesh()->GetSocketLocation(RootBone),G=Wall->GetComponentTransform().TransformPosition(LocalContacts[I]);
  const float Limit=FMath::Max(Reach(I)*.98f,FVector::Dist(Root,G));
  const FVector Delta=Root+Move-G;if(Delta.Size()>Limit)Move+=Delta.GetSafeNormal()*Limit-Delta;
 }
 if(!Move.IsNearlyZero()){
  FHitResult Hit;C->SetActorLocation(C->GetActorLocation()+Move,true,&Hit);
  if(Hit.bBlockingHit && Intent.Y<0 && Hit.ImpactNormal.Z>.7f){ReleaseWall();C->GetCharacterMovement()->SetMovementMode(MOVE_Walking);Hint=TEXT("Feet on ground");return;}
 }
 // Relative load is distributed across actual supports, including unilateral hanging.
 float Capacity=0;int32 Feet=0,HandCount=0;
 for(int I=0;I<4;++I)if(ContactActive[I]){Capacity+=(I<2?.55f:1.2f)*FMath::Max(.15f,LimbGrip[I]);if(I>=2)++Feet;else ++HandCount;}
 const float MassScale=FMath::Clamp(C->GetCharacterMovement()->Mass/80.f,.5f,2.f);
 for(int I=0;I<4;++I)SupportLoad[I]=ContactActive[I]?(I<2?.55f:1.2f)*FMath::Max(.15f,LimbGrip[I])/FMath::Max(Capacity,.01f):0;
 const float Side=(SupportLoad[1]+SupportLoad[3]-SupportLoad[0]-SupportLoad[2]);
 const float Transfer=MovingLimb==INDEX_NONE?0.f:(MovingLimb%2?-1.f:1.f);
 BodyOffset=FMath::VInterpTo(BodyOffset,Right*(Side*5+Transfer*3)+FVector(0,0,Feet==0?-16:HandCount==1?-5:0),Dt,4.f);
 BodyTwist=FMath::FInterpTo(BodyTwist,Transfer*7.f+Side*5.f,Dt,4.f);
 float Total=0;
 for(int I=0;I<4;++I){
  if(!ContactActive[I]){LimbStamina[I]=FMath::Min(1.f,LimbStamina[I]+Dt*.06f);Total+=LimbStamina[I];continue;}
  const FVector G=Wall->GetComponentTransform().TransformPosition(LocalContacts[I]);
  const FName RootBone=I==0?TEXT("upperarm_l"):I==1?TEXT("upperarm_r"):I==2?TEXT("thigh_l"):TEXT("thigh_r");
  const float Extension=FVector::Dist(C->GetMesh()->GetSocketLocation(RootBone),G)/(I<2?55.f:90.f);
  const float Effort=((I<2?.013f:.007f)+FMath::Max(0.f,Extension-.7f)*.05f+(1-LimbGrip[I])*.04f+(Strong?.025f:0))*MassScale*FMath::Max(1.f,SupportLoad[I]*4.f);
  const bool Rest=ActiveCount>=3 && MovingLimb==INDEX_NONE && Intent.IsNearlyZero() && LimbGrip[I]>.6f && Extension<.85f;
  LimbStamina[I]=FMath::Clamp(LimbStamina[I]+Dt*(Rest?.018f:-Effort),0.f,1.f);Total+=LimbStamina[I];
 }
 for(int I=0;I<4;++I)if(ContactActive[I] && LimbStamina[I]<=.01f){LoseContact(I);++Slips;Hint=TEXT("Limb exhausted: rest then regrip");}
 Stamina=Total/4;
 const float Reserve=Grip*(.4f+.6f*Stamina)*(Strong?1.3f:1.f);
 SlipTime=Reserve<.32f?SlipTime+Dt:FMath::Max(0.f,SlipTime-Dt*2);
 if(SlipTime>0){Hint=TEXT("Grip slipping - move to better support");FHitResult H;C->SetActorLocation(C->GetActorLocation()-FVector(0,0,Dt*12),true,&H);}
 if(SlipTime>2){const int I=!ContactActive[0]?1:!ContactActive[1]?0:LimbStamina[0]<LimbStamina[1]?0:1;LoseContact(I);++Slips;SlipTime=0;}
 int Support=0;for(bool A:ContactActive)if(A)++Support;
 if(Support==0 || (!ContactActive[0] && !ContactActive[1])){ReleaseWall();Hint=TEXT("Lost support - falling");return;}
 UpdateContacts(Dt);
}
