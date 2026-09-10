#include "AltaiBodyDynamics.h"
#include "AltaiPhysicalAnimation.h"
#include "AltaiHands.h"
#include "AltaiSurface.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "PhysicsEngine/PhysicalAnimationComponent.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "PhysicsEngine/SkeletalBodySetup.h"
#include "PhysicsEngine/BodyInstance.h"
#include "Engine/World.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/AnimSequence.h"
#include "Animation/Skeleton.h"
#include "UObject/ConstructorHelpers.h"

UAltaiBodyDynamics::UAltaiBodyDynamics(){
 PrimaryComponentTick.bCanEverTick=true;PrimaryComponentTick.TickGroup=TG_PostPhysics;
 static ConstructorHelpers::FObjectFinder<UAnimSequence> Prone(TEXT("/Game/Altai/Player/Animations/A_GetUpProne.A_GetUpProne"));
 static ConstructorHelpers::FObjectFinder<UAnimSequence> Supine(TEXT("/Game/Altai/Player/Animations/A_GetUpSupine.A_GetUpSupine"));
 ProneMotion=Prone.Object;SupineMotion=Supine.Object;
}
void UAltaiBodyDynamics::BeginPlay(){
 Super::BeginPlay();Character=Cast<ACharacter>(GetOwner());if(!Character.IsValid())return;
 auto* C=Character.Get();C->GetMesh()->AddTickPrerequisiteComponent(this);MeshHome=C->GetMesh()->GetRelativeTransform();CollisionHome=C->GetMesh()->GetCollisionProfileName();
 CapsuleCollisionHome=C->GetCapsuleComponent()->GetCollisionEnabled();PreviousYaw=C->GetActorRotation().Yaw;
 Muscles=NewObject<UAltaiPhysicalAnimation>(C,TEXT("BodyMuscles"));Muscles->RegisterComponent();Muscles->SetComponentTickEnabled(false);
 C->GetCapsuleComponent()->OnComponentHit.AddDynamic(this,&UAltaiBodyDynamics::CapsuleHit);
}
bool UAltaiBodyDynamics::Available() const{
 return Enabled && Character.IsValid() && !GetOwner()->ActorHasTag(TEXT("AltaiWallAttached")) && !GetOwner()->ActorHasTag(TEXT("AltaiMantling"));
}
void UAltaiBodyDynamics::ConfigureBodies(){
 auto* C=Character.Get();auto* Mesh=C->GetMesh();auto* Asset=Mesh->GetPhysicsAsset();if(!Asset)return;
 // Fractions are an approximation for a human, normalized to the actual physics bodies.
 auto Fraction=[](FName Bone){const FString N=Bone.ToString();
  if(N==TEXT("pelvis"))return .16f;if(N.StartsWith(TEXT("spine")))return .115f;
  if(N==TEXT("head"))return .075f;if(N.StartsWith(TEXT("neck")))return .015f;
  if(N.StartsWith(TEXT("thigh")))return .10f;if(N.StartsWith(TEXT("calf")))return .0465f;
  if(N.StartsWith(TEXT("foot")))return .0145f;if(N.StartsWith(TEXT("upperarm")))return .028f;
  if(N.StartsWith(TEXT("lowerarm")))return .016f;if(N.StartsWith(TEXT("hand")))return .006f;return .004f;};
 float Sum=0;for(auto B:Asset->SkeletalBodySetups)if(B)Sum+=Fraction(B->BoneName);
 PhysicalMass=0;
 for(auto B:Asset->SkeletalBodySetups)if(B)if(auto* Body=Mesh->GetBodyInstance(B->BoneName)){
  const float Mass=FMath::Clamp(C->GetCharacterMovement()->Mass,40.f,160.f)*Fraction(B->BoneName)/FMath::Max(Sum,.01f);
  Body->SetMassOverride(Mass,true);PhysicalMass+=Mass;Body->SetUseCCD(true);Body->SetMaxAngularVelocityInRadians(10,false);Body->SetOverrideIterationCounts(true);Body->SetPositionSolverIterationCount(24);Body->SetVelocitySolverIterationCount(8);Body->SetProjectionSolverIterationCount(3);
 }
 const auto& Ref=Mesh->GetSkeletalMeshAsset()->GetRefSkeleton();TArray<FTransform> Reference;RigReference=Ref.GetRefBonePose();
 for(int32 I=0;I<Ref.GetNum();++I){FTransform T=Ref.GetRefBonePose()[I];const int32 P=Ref.GetParentIndex(I);if(P>=0)T=T*Reference[P];Reference.Add(T);}
 for(auto* Joint:Mesh->Constraints)if(Joint){
  Joint->SetProjectionParams(true,.6f,1.f,2.f,1.f);Joint->EnableMassConditioning();
  const FString Name=Joint->ConstraintBone1.ToString();
  if(Name.StartsWith(TEXT("calf")) && Joint->ConstraintBone2==TEXT("pelvis")){Joint->TermConstraint();continue;}
  if(Name.StartsWith(TEXT("calf")) || Name.StartsWith(TEXT("lowerarm")) || Name.StartsWith(TEXT("foot")) || Name.StartsWith(TEXT("hand"))){
   const int32 ChildIndex=Ref.FindBoneIndex(Joint->ConstraintBone1),ParentIndex=Ref.FindBoneIndex(Joint->ConstraintBone2);
   if(ChildIndex>=0 && ParentIndex>=0){Joint->SetRefPosition(EConstraintFrame::Frame1,FVector::ZeroVector);Joint->SetRefPosition(EConstraintFrame::Frame2,Reference[ChildIndex].GetRelativeTransform(Reference[ParentIndex]).GetLocation());}
  }
  if(!Name.StartsWith(TEXT("calf")) && !Name.StartsWith(TEXT("lowerarm")))continue;
  const int32 Child=Ref.FindBoneIndex(Joint->ConstraintBone1),Parent=Ref.FindBoneIndex(Joint->ConstraintBone2);
  const FName EndName(*(FString(Name.StartsWith(TEXT("calf"))?TEXT("foot_"):TEXT("hand_"))+Name.Right(1)));
  const int32 End=Ref.FindBoneIndex(EndName);if(Child<0 || Parent<0 || End<0)continue;
  const FVector A=(Reference[Child].GetLocation()-Reference[Parent].GetLocation()).GetSafeNormal();
  const FVector B=(Reference[End].GetLocation()-Reference[Child].GetLocation()).GetSafeNormal();
  const float RefFlex=FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(float(FVector::DotProduct(A,B)),-1.f,1.f)));
  const FQuat Frame=FRotationMatrix::MakeFromXY(FVector::UpVector,FVector::ForwardVector).ToQuat();
  const FQuat Center=Reference[Parent].GetRotation().Inverse()*Reference[Child].GetRotation()*FQuat(FVector::UpVector,FMath::DegreesToRadians(68.5f-RefFlex))*Frame;
  Joint->SetRefOrientation(EConstraintFrame::Frame1,Frame.GetAxisX(),Frame.GetAxisY());
  Joint->SetRefOrientation(EConstraintFrame::Frame2,Center.GetAxisX(),Center.GetAxisY());
  Joint->SetAngularSwing1Limit(ACM_Locked,0);Joint->SetAngularSwing2Limit(ACM_Locked,0);Joint->SetAngularTwistLimit(ACM_Limited,56.5f);
 }
 // Local orientation drives act as bounded muscle tension; no position motor pins a falling pelvis in air.
 FPhysicalAnimationData Drive;Drive.bIsLocalSimulation=true;Drive.OrientationStrength=220;Drive.AngularVelocityStrength=28;Drive.MaxAngularForce=1800;
 Muscles->SetSkeletalMeshComponent(Mesh);Muscles->SetComponentTickEnabled(true);
 Muscles->ApplyPhysicalAnimationSettingsBelow(TEXT("pelvis"),Drive,false);
 Muscles->SetStrengthMultiplyer(.18f);
}
void UAltaiBodyDynamics::ProjectJointSafety(){
 auto* Mesh=Character->GetMesh();auto* Asset=Mesh->GetPhysicsAsset();const auto& Ref=Mesh->GetSkeletalMeshAsset()->GetRefSkeleton();
 if(RigReference.Num()!=Ref.GetNum() || !Asset)return;
 // Chaos contact projection may temporarily violate an angular limit under a hard impact.
 // Project the PHYSICAL chain, before the mesh evaluates, rather than hiding it in the rendered pose.
 for(const TCHAR* Name:{TEXT("calf_l"),TEXT("calf_r"),TEXT("lowerarm_l"),TEXT("lowerarm_r")}){
  const int32 Index=Ref.FindBoneIndex(Name),Parent=Ref.GetParentIndex(Index);
  auto* ChildBody=Mesh->GetBodyInstance(Name);auto* ParentBody=Mesh->GetBodyInstance(Ref.GetBoneName(Parent));if(!ChildBody || !ParentBody)continue;
  const FTransform Child=ChildBody->GetUnrealWorldTransform(),Root=ParentBody->GetUnrealWorldTransform();
  const FRotator Delta=(Root.GetRotation().Inverse()*Child.GetRotation()*RigReference[Index].GetRotation().Inverse()).Rotator();
  const float ReferenceFlex=FString(Name).StartsWith(TEXT("calf"))?5.f:39.f;
  const FQuat Local=FQuat(FVector::UpVector,FMath::DegreesToRadians(FMath::Clamp(float(Delta.Yaw),12.f-ReferenceFlex,125.f-ReferenceFlex)))*RigReference[Index].GetRotation();
  FTransform Limited=Child;Limited.SetRotation((Root.GetRotation()*Local).GetNormalized());
  Limited.SetLocation(Root.TransformPosition(RigReference[Index].GetLocation()));
  if(Child.GetRotation().AngularDistance(Limited.GetRotation())<.001 && FVector::DistSquared(Child.GetLocation(),Limited.GetLocation())<.01)continue;
  const FQuat Correction=Limited.GetRotation()*Child.GetRotation().Inverse();
  for(auto Setup:Asset->SkeletalBodySetups)if(Setup){
   int32 Bone=Ref.FindBoneIndex(Setup->BoneName);while(Bone>=0 && Bone!=Index)Bone=Ref.GetParentIndex(Bone);
   if(Bone!=Index)continue;if(auto* Body=Mesh->GetBodyInstance(Setup->BoneName)){
    const FTransform Before=Body->GetUnrealWorldTransform();Body->SetBodyTransform(Before.GetRelativeTransform(Child)*Limited,ETeleportType::TeleportPhysics,false);
    Body->SetAngularVelocityInRadians(Correction.RotateVector(Body->GetUnrealWorldAngularVelocityInRadians()).GetClampedToMaxSize(10),false,false);
   }
  }
 }
}
void UAltaiBodyDynamics::BeginStumble(float Severity,FVector Direction){
 if(!Available() || OwnsBody() || Cooldown>0)return;
 State=EAltaiBodyState::Stumbling;ReactionAge=0;ReactionPeak=FMath::Clamp(Severity,.15f,1.f);ReactionDirection=Direction.GetSafeNormal2D();
 Balance=1-ReactionPeak*.7f;Cooldown=1.1f;Hint=TEXT("Recovering balance");
 Character->GetCharacterMovement()->Velocity*=FMath::Lerp(.8f,.45f,ReactionPeak);
}
void UAltaiBodyDynamics::Trip(const FHitResult& Hit){
 if(!Available() || OwnsBody() || Cooldown>0)return;auto* C=Character.Get();const FVector V=C->GetVelocity();
 auto* H=C->FindComponentByClass<UAltaiHands>();const float Load=H?H->BalanceDemand:0;
 const float BodyMass=FMath::Max(40.f,C->GetCharacterMovement()->Mass);
 auto* Obstacle=Hit.GetComponent();const float Resistance=Obstacle && Obstacle->IsSimulatingPhysics()?FMath::Clamp(Obstacle->GetMass()/(BodyMass*.12f),.12f,1.f):1.f;
 const float FeetZ=C->GetActorLocation().Z-C->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
 const float Height=Obstacle?FMath::Max(5.f,float(Obstacle->Bounds.GetBox().Max.Z-FeetZ)):25.f;
 const float Severity=TripSensitivity*(V.Size2D()/500.f)*Resistance*FMath::Clamp(Height/35.f,.25f,1.2f)*(1+Load*.7f);
 // Stop the caught foot at the contact: momentum continues through the upper body.
 if(Severity>.87f){BeginFall(V,FVector(0,0,AngularVelocity.Z));C->GetMesh()->AddImpulseAtLocation(-V*BodyMass*.25f,Hit.ImpactPoint,FVector::DotProduct(Hit.ImpactPoint-C->GetActorLocation(),C->GetActorRightVector())>0?TEXT("foot_r"):TEXT("foot_l"));}
 else BeginStumble(Severity,V);
}
void UAltaiBodyDynamics::BeginFall(FVector Velocity,FVector Spin){
 if(!Available() || OwnsBody())return;auto* C=Character.Get();auto* Mesh=C->GetMesh();
 if(!Mesh->GetPhysicsAsset() || !Mesh->GetBodyInstance(TEXT("pelvis"))){Hint=TEXT("Missing physical body");return;}
 if(auto* H=C->FindComponentByClass<UAltaiHands>())H->Release();
 C->UnCrouch();MeshHome=Mesh->GetRelativeTransform();CollisionHome=Mesh->GetCollisionProfileName();CapsuleCollisionHome=C->GetCapsuleComponent()->GetCollisionEnabled();
 C->UnCrouch();OldYaw=C->bUseControllerRotationYaw;OldOrient=C->GetCharacterMovement()->bOrientRotationToMovement;
 C->bUseControllerRotationYaw=false;C->GetCharacterMovement()->bOrientRotationToMovement=false;C->StopJumping();C->GetCharacterMovement()->DisableMovement();
 if(auto* PC=Cast<APlayerController>(C->GetController())){PC->SetIgnoreMoveInput(true);LockedInput=true;}
 C->Tags.AddUnique(TEXT("AltaiBodyUnbalanced"));C->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 Mesh->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
 Mesh->SetCollisionProfileName(TEXT("Ragdoll"));Mesh->SetCollisionResponseToChannel(ECC_Pawn,ECR_Ignore);
 Mesh->SetAllBodiesBelowSimulatePhysics(TEXT("pelvis"),true,true);Mesh->SetAllBodiesBelowPhysicsBlendWeight(TEXT("pelvis"),1,false,true);
 ConfigureBodies();Mesh->SetAllPhysicsLinearVelocity(Velocity);Mesh->SetAllPhysicsAngularVelocityInRadians(Spin.GetClampedToMaxSize(8));
 RecoveryContacts.Reset();RecoverySupportCount=0;RecoveryBlocked=false;RecoveryRetry=0;
 State=EAltaiBodyState::Falling;Elapsed=QuietTime=RecoveryProgress=0;Reaction=0;Balance=0;++Falls;Hint=TEXT("Physical fall");
}
void UAltaiBodyDynamics::ApplyBodyImpulse(FVector Impulse,FVector Point,FName Bone){
 if(!Available())return;
 if(!OwnsBody()){
  const float DeltaV=Impulse.Size()/FMath::Max(40.f,Character->GetCharacterMovement()->Mass);
  if(DeltaV<180){BeginStumble(DeltaV/180.f,Impulse);return;}
  BeginFall(Character->GetVelocity(),AngularVelocity);
 }
 if(State==EAltaiBodyState::GettingUp)ResumePhysicalFall();
 if(State==EAltaiBodyState::Falling || State==EAltaiBodyState::Settling){auto* Mesh=Character->GetMesh();
  // Animation bones need not have a Chaos body (Manny spine_03 is such a bone).
  // Resolve to an actual ancestor body while keeping the original contact point.
  const auto& Ref=Mesh->GetSkeletalMeshAsset()->GetRefSkeleton();int32 Index=Ref.FindBoneIndex(Bone);
  while(!Mesh->GetBodyInstance(Bone) && Index>=0){Index=Ref.GetParentIndex(Index);Bone=Index>=0?Ref.GetBoneName(Index):FName(TEXT("pelvis"));}
  if(!Mesh->GetBodyInstance(Bone))Bone=TEXT("pelvis");
  Mesh->AddImpulseAtLocation(Impulse,Point,Bone);QuietTime=0;State=EAltaiBodyState::Falling;}
}
void UAltaiBodyDynamics::TestStumble(){if(Character.IsValid())BeginStumble(.65f,Character->GetActorForwardVector());}
void UAltaiBodyDynamics::TestFall(){if(Character.IsValid())ApplyBodyImpulse(Character->GetActorRightVector()*Character->GetCharacterMovement()->Mass*230,Character->GetMesh()->GetSocketLocation(TEXT("spine_03"))+FVector(0,0,15));}
void UAltaiBodyDynamics::CapsuleHit(UPrimitiveComponent*,AActor*,UPrimitiveComponent* Other,FVector Impulse,const FHitResult& Hit){
 if(!Available() || OwnsBody() || !Other)return;
 if(Other->IsSimulatingPhysics()){
  const FVector Relative=Other->GetPhysicsLinearVelocityAtPoint(Hit.ImpactPoint)-PreviousVelocity;
  const float Closing=FMath::Max(0.f,float(FVector::DotProduct(Relative,Hit.ImpactNormal)));
  const float Mass=Other->GetMass(),BodyMass=FMath::Max(40.f,Character->GetCharacterMovement()->Mass);
  if(Closing>100)ApplyBodyImpulse(Hit.ImpactNormal*(Closing*Mass*BodyMass/(Mass+BodyMass)),Hit.ImpactPoint);
 }
}
UAnimSequence* UAltaiBodyDynamics::RecoveryMotion() const{return FaceUp?SupineMotion.Get():ProneMotion.Get();}
bool UAltaiBodyDynamics::TryGetUp(){
 RecoveryRetry=.35f;
 auto* C=Character.Get();auto* Mesh=C->GetMesh();const FVector Hip=Mesh->GetSocketLocation(TEXT("pelvis"));
 FCollisionQueryParams Q(SCENE_QUERY_STAT(AltaiGetUp),false,C);FHitResult Floor;
 if(!GetWorld()->LineTraceSingleByChannel(Floor,Hip+FVector(0,0,25),Hip-FVector(0,0,90),ECC_Visibility,Q) || Floor.ImpactNormal.Z<C->GetCharacterMovement()->GetWalkableFloorZ())return false;
 const float Radius=C->GetCapsuleComponent()->GetScaledCapsuleRadius(),Height=C->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
 RecoveryFloor=Floor.ImpactPoint+FVector(0,0,2);

 // Fit the clip's initial pelvis to the settled body. Feet may be folded behind
 // the torso; their direction is not a reliable heading for recovery.
 const FQuat PhysicalHip=Mesh->GetSocketQuaternion(TEXT("pelvis"));
 FaceUp=(PhysicalHip.GetAxisY().Z+Mesh->GetSocketQuaternion(TEXT("spine_05")).GetAxisY().Z)>0;
 if(!RecoveryMotion()){Hint=TEXT("Missing recovery motion");return false;}
 FTransform FirstHip;RecoveryMotion()->GetBoneTransform(FirstHip,FSkeletonPoseBoneIndex(RecoveryMotion()->GetSkeleton()->GetReferenceSkeleton().FindBoneIndex(TEXT("pelvis"))),FAnimExtractContext(0.,false),false);
 const FQuat ClipHip=MeshHome.GetRotation()*FirstHip.GetRotation();
 float Dot=0,Cross=0;
 for(const FVector Axis:{FVector::ForwardVector,FVector::RightVector,FVector::UpVector}){
  const FVector From=ClipHip.RotateVector(Axis),To=PhysicalHip.RotateVector(Axis);
  Dot+=From.X*To.X+From.Y*To.Y;Cross+=From.X*To.Y-From.Y*To.X;
 }
 const float PreferredYaw=FMath::RadiansToDegrees(FMath::Atan2(Cross,Dot));
 FVector Forward;bool Found=false;CacheRecoveryPose();
 for(const float Turn:{0.f,30.f,-30.f,60.f,-60.f,90.f,-90.f}){
  Forward=FRotator(0,PreferredYaw+Turn,0).Vector();
  const FVector InitialOffset=Forward.Rotation().RotateVector(MeshHome.TransformPosition(FirstHip.GetLocation()));
  RecoveryFloor=Floor.ImpactPoint+FVector(0,0,2)-FVector(InitialOffset.X,InitialOffset.Y,0);
  FHitResult Destination;
  if(!GetWorld()->LineTraceSingleByChannel(Destination,RecoveryFloor+FVector(0,0,65),RecoveryFloor-FVector(0,0,65),ECC_Visibility,Q) || Destination.ImpactNormal.Z<.866f)continue;
  RecoveryFloor.Z=Destination.ImpactPoint.Z+2;
  if(GetWorld()->OverlapBlockingTestByChannel(RecoveryFloor+FVector(0,0,Height+2),FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(Radius,Height),Q))continue;
  const FTransform CandidateMesh=MeshHome*FTransform(Forward.Rotation(),RecoveryFloor+FVector(0,0,Height));
  if(RecoveryPathClear(CandidateMesh)){Found=true;break;}
 }
 if(!Found){RecoveryBlocked=true;Hint=TEXT("Waiting for stable supports / clear recovery path: ")+RecoveryTerrainIssue;return false;}
 RecoveryBlocked=false;RecoveryBlockedTime=0;RecoveryContacts.Reset();
 RecoveryAcquire=FMath::Clamp(.45f+(Forward.Rotation().Quaternion()*ClipHip).AngularDistance(PhysicalHip)*.35f,.45f,1.05f);

 Muscles->SetStrengthMultiplyer(0);Muscles->SetComponentTickEnabled(false);Muscles->SetSkeletalMeshComponent(nullptr);
 Mesh->SnapshotPose(RecoverySnapshot);
 const FTransform MeshWorld=Mesh->GetComponentTransform();
 const FVector CameraAnchor=C->GetActorLocation();
 C->SetActorLocationAndRotation(RecoveryFloor+FVector(0,0,Height),Forward.Rotation(),false,nullptr,ETeleportType::TeleportPhysics);
 if(auto* Boom=C->FindComponentByClass<USpringArmComponent>())Boom->TargetOffset+=CameraAnchor-C->GetActorLocation();
 // Preserve the snapshot's world-space root when the capsule returns upright.
 const FTransform NewMeshWorld=MeshHome*C->GetActorTransform();
 if(!RecoverySnapshot.LocalTransforms.IsEmpty()){
  const FTransform Root=RecoverySnapshot.LocalTransforms[0]*MeshWorld.GetRelativeTransform(NewMeshWorld);
  const auto& Ref=Mesh->GetSkeletalMeshAsset()->GetRefSkeleton();
  for(int32 I=1;I<RecoverySnapshot.BoneNames.Num();++I)if(Ref.GetParentIndex(Ref.FindBoneIndex(RecoverySnapshot.BoneNames[I]))==0)RecoverySnapshot.LocalTransforms[I]=RecoverySnapshot.LocalTransforms[I]*Root;
  RecoverySnapshot.LocalTransforms[0]=FTransform::Identity;
 }
 Muscles->SetStrengthMultiplyer(0);Mesh->SetAllBodiesSimulatePhysics(false);Mesh->SetAllBodiesPhysicsBlendWeight(0);
 Mesh->AttachToComponent(C->GetCapsuleComponent(),FAttachmentTransformRules::KeepWorldTransform);Mesh->SetRelativeTransform(MeshHome);Mesh->SetCollisionProfileName(CollisionHome);
 State=EAltaiBodyState::GettingUp;Elapsed=0;RecoveryProgress=0;UpdateRecoveryTerrain(0,0);Hint=TEXT("Rolling to support / kneeling / standing");
 Mesh->TickAnimation(0,false);Mesh->RefreshBoneTransforms();return true;
}
void UAltaiBodyDynamics::FinishGetUp(){
 auto* C=Character.Get();C->GetCapsuleComponent()->SetCollisionEnabled(CapsuleCollisionHome);
 C->bUseControllerRotationYaw=C->ActorHasTag(TEXT("AltaiViewConfigured"))?C->ActorHasTag(TEXT("AltaiFirstPerson")):OldYaw;
 C->GetCharacterMovement()->bOrientRotationToMovement=C->ActorHasTag(TEXT("AltaiViewConfigured"))?!C->ActorHasTag(TEXT("AltaiFirstPerson")):OldOrient;
 C->GetCharacterMovement()->SetMovementMode(MOVE_Walking);C->Tags.Remove(TEXT("AltaiBodyUnbalanced"));
 if(LockedInput)if(auto* PC=Cast<APlayerController>(C->GetController()))PC->SetIgnoreMoveInput(false);
 LockedInput=false;RecoveryBlocked=false;RecoverySupportCount=0;RecoveryContacts.Reset();State=EAltaiBodyState::Balanced;Cooldown=1.2f;Balance=1;RecoverySnapshot=FPoseSnapshot();PreviousVelocity=FVector::ZeroVector;WasFalling=false;PreviousYaw=C->GetActorRotation().Yaw;AngularVelocity=FVector::ZeroVector;++Recoveries;Hint=TEXT("Balanced");
}
void UAltaiBodyDynamics::StopSimulation(){
 if(!Character.IsValid() || !OwnsBody())return;auto* C=Character.Get();if(Muscles){Muscles->SetStrengthMultiplyer(0);Muscles->SetComponentTickEnabled(false);Muscles->SetSkeletalMeshComponent(nullptr);}
 C->GetMesh()->SetAllBodiesSimulatePhysics(false);C->GetMesh()->SetAllBodiesPhysicsBlendWeight(0);C->GetMesh()->AttachToComponent(C->GetCapsuleComponent(),FAttachmentTransformRules::KeepWorldTransform);C->GetMesh()->SetRelativeTransform(MeshHome);C->GetMesh()->SetCollisionProfileName(CollisionHome);FinishGetUp();
}
void UAltaiBodyDynamics::EndPlay(const EEndPlayReason::Type Reason){StopSimulation();Super::EndPlay(Reason);}
void UAltaiBodyDynamics::TickComponent(float Dt,ELevelTick Type,FActorComponentTickFunction* F){
 Super::TickComponent(Dt,Type,F);if(!Character.IsValid() || Dt<=0)return;auto* C=Character.Get();auto* M=C->GetCharacterMovement();auto* Mesh=C->GetMesh();Cooldown=FMath::Max(0.f,Cooldown-Dt);
 if(State==EAltaiBodyState::GettingUp){
  const float Next=Elapsed+Dt;
  if(!UpdateRecoveryTerrain(FMath::Max(0.f,Next-RecoveryAcquire),Dt)){
   RecoveryBlocked=true;RecoveryBlockedTime+=Dt;Hint=TEXT("Recovery interrupted by obstruction / missing support");
   if(RecoveryBlockedTime>.15f)ResumePhysicalFall();return;
  }
  RecoveryBlocked=false;RecoveryBlockedTime=0;Elapsed=Next;
  RecoveryProgress=FMath::Clamp(Elapsed/(RecoveryMotion()?RecoveryMotion()->GetPlayLength()+RecoveryAcquire+.2f:4.f),0.f,1.f);
  if(RecoveryProgress>=1)FinishGetUp();return;
 }
 if(OwnsBody()){
  ProjectJointSafety();Elapsed+=Dt;const FVector Hip=Mesh->GetSocketLocation(TEXT("pelvis"));
  // Move only the camera/capsule proxy. Never teleport the simulated bodies with it.
  C->SetActorLocation(Hip,false,nullptr,ETeleportType::None);
  const float Speed=Mesh->GetPhysicsLinearVelocity(TEXT("pelvis")).Size();AngularVelocity=Mesh->GetPhysicsAngularVelocityInRadians(TEXT("pelvis"));
  Muscles->SetStrengthMultiplyer(Elapsed<.6f?.18f:.04f);
  const bool Quiet=Speed<35 && AngularVelocity.Size()<.8f;QuietTime=Quiet?QuietTime+Dt:0;
  State=Quiet?EAltaiBodyState::Settling:EAltaiBodyState::Falling;
  RecoveryRetry=FMath::Max(0.f,RecoveryRetry-Dt);if(QuietTime>.65f && Elapsed>1.1f && RecoveryRetry<=0)TryGetUp();return;
 }
 const FVector V=C->GetVelocity();const float Yaw=C->GetActorRotation().Yaw;
 const float Spin=FMath::DegreesToRadians(FMath::FindDeltaAngleDegrees(PreviousYaw,Yaw))/Dt;
 AngularVelocity=FMath::VInterpTo(AngularVelocity,FVector(0,0,FMath::Clamp(Spin,-8.f,8.f)),Dt,10);PreviousYaw=Yaw;
 if(!Available()){Reaction=0;State=EAltaiBodyState::Balanced;WasFalling=false;PreviousVelocity=V;return;}
 const bool Air=M->IsFalling();
 if(Air && V.Z<-FallSpeedThreshold){LastImpactSpeed=-V.Z;BeginFall(V,AngularVelocity);return;}
 if(WasFalling && M->IsMovingOnGround() && PreviousVelocity.Z<-450){LastImpactSpeed=-PreviousVelocity.Z;BeginStumble(FMath::Clamp((-PreviousVelocity.Z-400)/500.f,.1f,.9f),C->GetActorForwardVector());}
 if(State==EAltaiBodyState::Stumbling){ReactionAge+=Dt;Reaction=ReactionPeak*FMath::SmoothStep(0.f,.12f,ReactionAge)*(1-FMath::SmoothStep(.18f,.95f,ReactionAge));Balance=1-Reaction*.7f;if(ReactionAge>=1){State=EAltaiBodyState::Balanced;Reaction=0;Balance=1;}}
 PreviousVelocity=V;WasFalling=Air;
}
