#include "AltaiEditorLibrary.h"
#include "AnimGraphNode_AltaiContacts.h"
#include "Animation/AnimBlueprint.h"
#include "Animation/AnimInstance.h"
#include "Animation/Skeleton.h"
#include "Factories/AnimBlueprintFactory.h"
#include "AnimGraphNode_LinkedInputPose.h"
#include "AnimGraphNode_LocalToComponentSpace.h"
#include "AnimGraphNode_ComponentToLocalSpace.h"
#include "AnimGraphNode_Root.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphSchema.h"
#include "AssetToolsModule.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "EditorAssetLibrary.h"
#include "Editor/EditorPerformanceSettings.h"
namespace
{
template<class T> T* Add(UEdGraph* G,int32 X)
{
 auto* N=NewObject<T>(G);G->AddNode(N,false,false);N->CreateNewGuid();N->PostPlacedNewNode();N->AllocateDefaultPins();N->NodePosX=X;return N;
}
bool Connect(UEdGraph* G,UEdGraphNode* A,UEdGraphNode* B)
{
 for(auto* P:A->Pins)if(P->Direction==EGPD_Output)
  for(auto* Q:B->Pins)if(Q->Direction==EGPD_Input && G->GetSchema()->TryCreateConnection(P,Q))return true;
 return false;
}
}
bool UAltaiEditorLibrary::CreateContactAnimation(USkeleton* Skeleton)
{
 if(!Skeleton)return false;
 const FString Path=TEXT("/Game/Altai/Player/ABP_AltaiContacts");
 if(UEditorAssetLibrary::DoesAssetExist(Path))return true;
 auto* Factory=NewObject<UAnimBlueprintFactory>();Factory->TargetSkeleton=Skeleton;Factory->ParentClass=UAnimInstance::StaticClass();
 auto* BP=Cast<UAnimBlueprint>(FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get().CreateAsset(TEXT("ABP_AltaiContacts"),TEXT("/Game/Altai/Player"),UAnimBlueprint::StaticClass(),Factory));
 if(!BP)return false;
 UEdGraph* Graph=nullptr;UAnimGraphNode_Root* Root=nullptr;
 for(auto G:BP->FunctionGraphs)for(auto N:G->Nodes)if(auto* R=Cast<UAnimGraphNode_Root>(N)){Graph=G;Root=R;break;}
 if(!Graph)return false;
 auto* Input=Add<UAnimGraphNode_LinkedInputPose>(Graph,-600);
 auto* ToCS=Add<UAnimGraphNode_LocalToComponentSpace>(Graph,-400);
 auto* Contacts=Add<UAnimGraphNode_AltaiContacts>(Graph,-200);
 auto* ToLocal=Add<UAnimGraphNode_ComponentToLocalSpace>(Graph,0);
 Root->NodePosX=220;
 if(!Connect(Graph,Input,ToCS)||!Connect(Graph,ToCS,Contacts)||!Connect(Graph,Contacts,ToLocal)||!Connect(Graph,ToLocal,Root))return false;
 FKismetEditorUtilities::CompileBlueprint(BP);
 return BP->Status!=BS_Error && UEditorAssetLibrary::SaveLoadedAsset(BP);
}

void UAltaiEditorLibrary::SetPhysicsTestMode(bool Enabled)
{
 static bool Saved=false,Original=true;
 auto* Settings=GetMutableDefault<UEditorPerformanceSettings>();
 if(Enabled && !Saved){Original=Settings->bThrottleCPUWhenNotForeground;Saved=true;Settings->bThrottleCPUWhenNotForeground=false;}
 else if(!Enabled && Saved){Settings->bThrottleCPUWhenNotForeground=Original;Saved=false;}
}
