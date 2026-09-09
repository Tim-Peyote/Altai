#include "AltaiCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "InputCoreTypes.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/PlayerController.h"
#include "EnhancedInputComponent.h"
#include "InputMappingContext.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
AAltaiCharacter::AAltaiCharacter()
{
 PrimaryActorTick.bCanEverTick=true;
 GetCapsuleComponent()->InitCapsuleSize(42,96);
 bUseControllerRotationYaw=false;
 GetCharacterMovement()->bOrientRotationToMovement=true;
 GetCharacterMovement()->RotationRate=FRotator(0,500,0);
 GetCharacterMovement()->MaxWalkSpeed=500;
 GetCharacterMovement()->JumpZVelocity=500;
 GetCharacterMovement()->AirControl=0.35;
 CameraBoom=CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
 CameraBoom->SetupAttachment(RootComponent);CameraBoom->TargetArmLength=350;CameraBoom->bUsePawnControlRotation=true;
 FollowCamera=CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
 FollowCamera->SetupAttachment(CameraBoom,USpringArmComponent::SocketName);
}
void AAltaiCharacter::SetupPlayerInputComponent(UInputComponent* Input)
{
 Super::SetupPlayerInputComponent(Input);
 Input->BindKey(EKeys::V,IE_Pressed,this,&AAltaiCharacter::ToggleView);
 if (APlayerController* PC=Cast<APlayerController>(GetController()))
  if (ULocalPlayer* LP=PC->GetLocalPlayer())
   if (auto* S=LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>()) { if(MappingContext) S->AddMappingContext(MappingContext,0); if(auto* Mouse=LoadObject<UInputMappingContext>(nullptr,TEXT("/Game/Input/IMC_MouseLook.IMC_MouseLook"))) S->AddMappingContext(Mouse,1); }
 if (auto* EI=Cast<UEnhancedInputComponent>(Input))
 {
  EI->BindAction(MoveAction,ETriggerEvent::Triggered,this,&AAltaiCharacter::Move);
  EI->BindAction(LookAction,ETriggerEvent::Triggered,this,&AAltaiCharacter::Look);
  EI->BindAction(JumpAction,ETriggerEvent::Started,this,&ACharacter::Jump);
  EI->BindAction(JumpAction,ETriggerEvent::Completed,this,&ACharacter::StopJumping);
 }
}
void AAltaiCharacter::Move(const FInputActionValue& Value)
{
 const FVector2D V=Value.Get<FVector2D>();
 const FRotator R(0,GetControlRotation().Yaw,0);
 AddMovementInput(FRotationMatrix(R).GetUnitAxis(EAxis::X),V.Y);
 AddMovementInput(FRotationMatrix(R).GetUnitAxis(EAxis::Y),V.X);
}
void AAltaiCharacter::Look(const FInputActionValue& Value)
{
 const FVector2D V=Value.Get<FVector2D>();AddControllerYawInput(V.X);AddControllerPitchInput(V.Y);
}

void AAltaiCharacter::SetFirstPerson(bool Enabled)
{
 if(!CameraInitialized){OriginalArmLength=CameraBoom->TargetArmLength;CameraInitialized=true;}
 FirstPerson=Enabled;Tags.AddUnique(TEXT("AltaiViewConfigured"));if(Enabled)Tags.AddUnique(TEXT("AltaiFirstPerson"));else Tags.Remove(TEXT("AltaiFirstPerson"));
 const bool OnWall=ActorHasTag(TEXT("AltaiWallAttached")) || ActorHasTag(TEXT("AltaiMantling"));bUseControllerRotationYaw=Enabled && !OnWall;GetCharacterMovement()->bOrientRotationToMovement=!Enabled && !OnWall;
 if(Enabled){if(!OnWall)SetActorRotation(FRotator(0,GetControlRotation().Yaw,0));GetMesh()->HideBoneByName(TEXT("head"),EPhysBodyOp::PBO_None);}
 else GetMesh()->UnHideBoneByName(TEXT("head"));
}
void AAltaiCharacter::ToggleView(){SetFirstPerson(!FirstPerson);}
void AAltaiCharacter::Tick(float Dt)
{
 Super::Tick(Dt);if(!CameraInitialized)return;
 CameraBoom->TargetArmLength=FMath::FInterpTo(CameraBoom->TargetArmLength,FirstPerson?0.f:OriginalArmLength,Dt,12);
 const bool OnWall=ActorHasTag(TEXT("AltaiWallAttached")) || ActorHasTag(TEXT("AltaiMantling"));
 float EyeHeight=OnWall?82.f:(bIsCrouched?56.f:64.f);
 // During climbing the torso can move relative to the capsule in all three axes.
 // Keep the eye in front of the neck, not inside the chest when looking at footholds.
 FVector EyeOffset=GetActorForwardVector()*FMath::Clamp(FirstPersonForward,0.f,8.f)+FVector(0,0,EyeHeight);
 if(OnWall && GetMesh()->DoesSocketExist(TEXT("neck_01"))){
  const FVector Neck=GetMesh()->GetSocketLocation(TEXT("neck_01"))-GetActorLocation();
  EyeOffset=FVector(Neck.X,Neck.Y,FMath::Clamp(float(Neck.Z+14.f),10.f,90.f))+GetActorForwardVector()*12.f;
 }
 CameraBoom->TargetOffset=FMath::VInterpTo(CameraBoom->TargetOffset,FirstPerson?EyeOffset+InteractionEyeOffset:FVector::ZeroVector,Dt,OnWall?10.f:5.f);
 FollowCamera->SetRelativeLocation(FMath::VInterpTo(FollowCamera->GetRelativeLocation(),FVector::ZeroVector,Dt,12));
}
