#include "AltaiProfileStore.h"
#include "Kismet/GameplayStatics.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

bool FAltaiProfileStore::ValidId(const FString& Id)
{ FGuid Guid; return Id.Len()==32 && FGuid::ParseExact(Id,EGuidFormats::Digits,Guid) && Guid.IsValid(); }
FString FAltaiProfileStore::Path(const FString& Id) const
{ return FPaths::Combine(Directory,TEXT("Profile_")+Id+TEXT(".sav")); }
USaveGame* FAltaiProfileStore::ReadFile(const FString& File)
{
 if(!IFileManager::Get().FileExists(*File))return nullptr;
 TArray<uint8> Bytes;
 if(!FFileHelper::LoadFileToArray(Bytes,*File) || Bytes.Num()==0)return nullptr;
 return UGameplayStatics::LoadGameFromMemory(Bytes);
}
bool FAltaiProfileStore::ValidSave(const UAltaiSave* S,const FString& Id)
{
 if(!S || S->Version!=2 || S->ProfileId!=Id || S->ProfileName.IsEmpty() || !FMath::IsFinite(S->PlaySeconds) || S->PlaySeconds<0 || S->PlayerTransform.ContainsNaN() || S->ViewRotation.ContainsNaN())return false;
 TSet<FName> Seen;
 for(const FAltaiItem& Item:S->Items)
 {if(Item.Id.IsNone() || Item.Quantity<=0 || Seen.Contains(Item.Id))return false;Seen.Add(Item.Id);}
 TSet<EAltaiEquipSlot> Slots;TSet<FName> Assigned;
 for(const auto& E:S->Loadout)
 {
  const auto* I=S->Items.FindByPredicate([&](const FAltaiItem& Item){return Item.Id==E.ItemId;});
  if(!I || !I->CanAssign(E.Slot) || Slots.Contains(E.Slot) || Assigned.Contains(E.ItemId))return false;
  Slots.Add(E.Slot);Assigned.Add(E.ItemId);
 }
 return true;
}
UAltaiSave* FAltaiProfileStore::Load(const FString& Id,bool* Recovered) const
{
 if(Recovered)*Recovered=false;
 if(!ValidId(Id))return nullptr;
 UAltaiSave* S=Cast<UAltaiSave>(ReadFile(Path(Id)));
 if(ValidSave(S,Id))return S;
 S=Cast<UAltaiSave>(ReadFile(Path(Id)+TEXT(".bak")));
 if(ValidSave(S,Id)){if(Recovered)*Recovered=true;return S;}
 return nullptr;
}
bool FAltaiProfileStore::WriteFile(USaveGame* Data,const FString& File)
{
 TArray<uint8> Bytes;
 if(!Data || !UGameplayStatics::SaveGameToMemory(Data,Bytes))return false;
 IFileManager& FM=IFileManager::Get();
 if(!FM.MakeDirectory(*FPaths::GetPath(File),true))return false;
 const FString Temp=File+TEXT(".tmp");
 if(!FFileHelper::SaveArrayToFile(Bytes,*Temp))return false;
 const bool bOK=FM.Move(*File,*Temp,true,false);
 if(!bOK)FM.Delete(*Temp);
 return bOK;
}
bool FAltaiProfileStore::Write(UAltaiSave* Save,bool bCreate) const
{
 if(!Save || !ValidId(Save->ProfileId) || !ValidSave(Save,Save->ProfileId))return false;
 const FString File=Path(Save->ProfileId);
 IFileManager& FM=IFileManager::Get();
 if(bCreate && (FM.FileExists(*File) || FM.FileExists(*(File+TEXT(".bak")))))return false;
 if(!bCreate)
 {
  UAltaiSave* Previous=Load(Save->ProfileId);
  if(!Previous)return false; // Never turn an unknown/corrupt profile into a new game.
  if(!WriteFile(Previous,File+TEXT(".bak")))return false;
 }
 if(!WriteFile(Save,File))return false;
 const UAltaiSave* Verify=Cast<UAltaiSave>(ReadFile(File));
 return ValidSave(Verify,Save->ProfileId) && Verify->SavedAt==Save->SavedAt;
}
TArray<FAltaiProfileInfo> FAltaiProfileStore::List() const
{
 TArray<FString> Files;IFileManager::Get().FindFiles(Files,*FPaths::Combine(Directory,TEXT("Profile_*.sav*")),true,false);
 TSet<FString> Ids;
 for(FString File:Files)
 {
  File.RemoveFromStart(TEXT("Profile_"));
  if(File.EndsWith(TEXT(".sav.bak")))File.LeftChopInline(8);
  else if(File.EndsWith(TEXT(".sav")))File.LeftChopInline(4);
  else continue;
  if(ValidId(File))Ids.Add(File);
 }
 TArray<FAltaiProfileInfo> Result;
 for(const FString& Id:Ids)
 {
  bool Recovered=false;
  if(const UAltaiSave* Save=Load(Id,&Recovered))
  { FAltaiProfileInfo Info;Info.Id=Id;Info.Name=Save->ProfileName;Info.SavedAt=Save->SavedAt;Info.PlaySeconds=Save->PlaySeconds;Info.Recovered=Recovered;Result.Add(Info); }
 }
 Result.Sort([](const FAltaiProfileInfo& A,const FAltaiProfileInfo& B){return A.SavedAt==B.SavedAt?A.Id<B.Id:A.SavedAt>B.SavedAt;});
 return Result;
}
FString FAltaiProfileStore::LastPlayed() const
{
 const FString File=FPaths::Combine(Directory,TEXT("LastPlayed.sav"));
 auto* Index=Cast<UAltaiProfileIndex>(ReadFile(File));
 if(!Index)Index=Cast<UAltaiProfileIndex>(ReadFile(File+TEXT(".bak")));
 if(Index && Load(Index->LastPlayedId))return Index->LastPlayedId;
 const auto Profiles=List();return Profiles.IsEmpty()?FString():Profiles[0].Id;
}
bool FAltaiProfileStore::MarkPlayed(const FString& Id) const
{
 if(!Load(Id))return false;
 const FString File=FPaths::Combine(Directory,TEXT("LastPlayed.sav"));
 if(auto* Old=Cast<UAltaiProfileIndex>(ReadFile(File)))
  if(!WriteFile(Old,File+TEXT(".bak")))return false;
 auto* Index=NewObject<UAltaiProfileIndex>();Index->LastPlayedId=Id;
 return WriteFile(Index,File);
}
bool FAltaiProfileStore::ImportLegacy(const FString& LegacyFile) const
{
 for(const auto& Info:List())if(const UAltaiSave* S=Load(Info.Id))if(S->ImportedLegacy)return true;
 if(!IFileManager::Get().FileExists(*LegacyFile))return true;
 auto* Old=Cast<UAltaiSave>(ReadFile(LegacyFile));
 if(!Old || Old->Version!=1)return false;
 const bool HadProfile=!LastPlayed().IsEmpty();
 Old->Version=2;Old->ProfileId=FGuid::NewGuid().ToString(EGuidFormats::Digits);Old->ProfileName=TEXT("Первое прохождение");Old->ImportedLegacy=true;
 Old->SavedAt=IFileManager::Get().GetTimeStamp(*LegacyFile);
 if(!Write(Old,true))return false;
 if(!HadProfile)return MarkPlayed(Old->ProfileId);
 return true; // The original file is retained unchanged.
}
