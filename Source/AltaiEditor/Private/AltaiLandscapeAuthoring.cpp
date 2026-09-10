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

#include "LandscapeEdit.h"
#include "LandscapeEditLayer.h"
bool UAltaiEditorLibrary::SculptSwimmingPond(ALandscape* L)
{
 if(!L || !L->GetLandscapeInfo())return false;
 auto* Info=L->GetLandscapeInfo();L->Modify();const auto* Layer=L->GetEditLayerConst(0);
 FLandscapeEditDataInterface Edit(Info,Layer?Layer->GetGuid():FGuid());
 int32 X1,Y1,X2,Y2;if(!Info->GetLandscapeExtent(X1,Y1,X2,Y2))return false;
 TArray<uint16> Data;Data.SetNumZeroed((X2-X1+1)*(Y2-Y1+1));Edit.GetHeightDataFast(X1,Y1,X2,Y2,Data.GetData(),0);
 const FTransform T=L->GetActorTransform();int32 Changed=0;
 for(int32 Y=Y1;Y<=Y2;++Y)for(int32 X=X1;X<=X2;++X){
  const FVector W=T.TransformPosition(FVector(X,Y,0));const float R=FMath::Sqrt(FMath::Square((W.X-1800)/1500)+FMath::Square((W.Y-1700)/1100));
  if(R>=1)continue;const float Z=28-420*FMath::Square(1-R*R);
  const float LocalZ=T.InverseTransformPosition(FVector(W.X,W.Y,Z)).Z;
  auto& H=Data[(Y-Y1)*(X2-X1+1)+X-X1];++Changed;H=FMath::Min(H,uint16(FMath::Clamp(FMath::RoundToInt(32768+LocalZ*128),0,65535)));
 }
 Edit.SetHeightData(X1,Y1,X2,Y2,Data.GetData(),0,true);Edit.Flush();L->RequestLayersContentUpdateForceAll();L->ForceLayersFullUpdate();Info->RecreateCollisionComponents();L->MarkPackageDirty();
 UE_LOG(LogTemp,Display,TEXT("Pond sculpt: %d vertices, extent %d,%d %d,%d transform %s"),Changed,X1,Y1,X2,Y2,*T.ToString());return Changed>0;
}
