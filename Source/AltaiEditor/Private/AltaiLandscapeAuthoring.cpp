#include "AltaiEditorLibrary.h"
#include "Landscape.h"
#include "LandscapeInfo.h"
#include "Editor.h"
#include "Misc/FileHelper.h"
#include "Materials/MaterialInterface.h"

ALandscape* UAltaiEditorLibrary::ImportLabLandscape(const FString& File,UMaterialInterface* Material)
{
 constexpr int32 Size=253;
 TArray<uint8> Bytes;
 if(!GEditor || !FFileHelper::LoadFileToArray(Bytes,*File) || Bytes.Num()!=Size*Size*2)return nullptr;
 UWorld* World=GEditor->GetEditorWorldContext().World();if(!World)return nullptr;
 auto* L=World->SpawnActor<ALandscape>();
 L->SetActorLabel(TEXT("Altai_Valley_Landscape"));
 L->SetActorLocation(FVector(-7560,-7560,0));L->SetActorScale3D(FVector(60,60,100));
 L->LandscapeMaterial=Material;
 TArray<uint16> Heights;Heights.SetNumUninitialized(Size*Size);
 FMemory::Memcpy(Heights.GetData(),Bytes.GetData(),Bytes.Num());
 TMap<FGuid,TArray<uint16>> HeightData;HeightData.Add(FGuid(),MoveTemp(Heights));
 TMap<FGuid,TArray<FLandscapeImportLayerInfo>> Layers;Layers.Add(FGuid(),{});
 L->Import(FGuid::NewGuid(),0,0,Size-1,Size-1,1,63,HeightData,*File,Layers,ELandscapeImportAlphamapType::Additive,{});
 L->CreateLandscapeInfo();L->PostEditChange();L->MarkPackageDirty();return L;
}
