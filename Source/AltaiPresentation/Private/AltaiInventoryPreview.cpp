#include "AltaiInventoryPreview.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/PointLightComponent.h"
#include "Animation/AnimSequence.h"
#include "UObject/ConstructorHelpers.h"

AAltaiInventoryPreview::AAltaiInventoryPreview()
{
 PrimaryActorTick.bCanEverTick=false;
 auto* Root=CreateDefaultSubobject<USceneComponent>(TEXT("StudioRoot"));SetRootComponent(Root);
 Model=CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Model"));Model->SetupAttachment(Root);
 Model->SetRelativeRotation(FRotator(0,-90,0));Model->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 Model->SetLightingChannels(false,true,false);Model->VisibilityBasedAnimTickOption=EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
 static ConstructorHelpers::FObjectFinder<USkeletalMesh> Mesh(TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"));
 if(Mesh.Succeeded())Model->SetSkeletalMesh(Mesh.Object);
 static ConstructorHelpers::FObjectFinder<UAnimSequence> Idle(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/MM_Idle.MM_Idle"));
 Model->SetAnimationMode(EAnimationMode::AnimationSingleNode);Model->AnimationData.AnimToPlay=Idle.Object;Model->AnimationData.bSavedLooping=true;Model->AnimationData.bSavedPlaying=false;
 Capture=CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("PortraitCamera"));Capture->SetupAttachment(Root);
 Capture->SetRelativeLocation(FVector(330,0,100));Capture->SetRelativeRotation(FRotator(0,180,0));Capture->FOVAngle=30;
 Capture->CaptureSource=ESceneCaptureSource::SCS_FinalColorLDR;Capture->bCaptureEveryFrame=false;Capture->bCaptureOnMovement=false;Capture->bAlwaysPersistRenderingState=true;
 Capture->PrimitiveRenderMode=ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;
 Capture->ShowFlags.SetAtmosphere(false);Capture->ShowFlags.SetFog(false);Capture->ShowFlags.SetMotionBlur(false);Capture->ShowFlags.SetSkyLighting(false);Capture->ShowFlags.SetEyeAdaptation(true);
 Capture->PostProcessSettings.bOverride_AutoExposureMethod=true;Capture->PostProcessSettings.AutoExposureMethod=AEM_Manual;
 Capture->PostProcessSettings.bOverride_AutoExposureBias=true;Capture->PostProcessSettings.AutoExposureBias=2;
 Capture->PostProcessSettings.bOverride_BloomIntensity=true;Capture->PostProcessSettings.BloomIntensity=0;
 Capture->PostProcessSettings.bOverride_AutoExposureApplyPhysicalCameraExposure=true;Capture->PostProcessSettings.AutoExposureApplyPhysicalCameraExposure=false;
 KeyLight=CreateDefaultSubobject<UPointLightComponent>(TEXT("KeyLight"));KeyLight->SetupAttachment(Root);
 KeyLight->SetRelativeLocation(FVector(160,-130,220));KeyLight->SetIntensity(6500);KeyLight->SetLightColor(FLinearColor(1,.85,.65));KeyLight->SetAttenuationRadius(900);KeyLight->LightingChannels.bChannel0=false;KeyLight->LightingChannels.bChannel1=true;
 RimLight=CreateDefaultSubobject<UPointLightComponent>(TEXT("RimLight"));RimLight->SetupAttachment(Root);
 RimLight->SetRelativeLocation(FVector(-70,120,190));RimLight->SetIntensity(8500);RimLight->SetLightColor(FLinearColor(.45,.65,1));RimLight->SetAttenuationRadius(900);RimLight->LightingChannels.bChannel0=false;RimLight->LightingChannels.bChannel1=true;
}
void AAltaiInventoryPreview::BeginPlay(){Super::BeginPlay();RefreshPreview();}
void AAltaiInventoryPreview::RefreshPreview()
{
 Model->TickAnimation(0,false);Model->RefreshBoneTransforms();Model->UpdateComponentToWorld();
 Capture->ClearShowOnlyComponents();Capture->ShowOnlyComponent(Model);Capture->CaptureScene();
}
void AAltaiInventoryPreview::RotateModel(float Degrees)
{Model->AddLocalRotation(FRotator(0,Degrees,0));RefreshPreview();}
