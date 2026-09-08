#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "AltaiProfileStore.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "HAL/FileManager.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAltaiProfileTest,"Altai.Saves.ProfileIsolationAndRecovery",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FAltaiProfileTest::RunTest(const FString& Parameters)
{
 const FString Root=FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("Automation/ProfileTests"),FGuid::NewGuid().ToString(EGuidFormats::Digits));
 struct FCleanup{FString Dir;~FCleanup(){IFileManager::Get().DeleteDirectory(*Dir,false,true);}} Cleanup{Root};
 FAltaiProfileStore Store(Root);
 auto Make=[](const TCHAR* Name,int32 Count){auto* S=NewObject<UAltaiSave>();S->Version=2;S->ProfileId=FGuid::NewGuid().ToString(EGuidFormats::Digits);S->ProfileName=Name;S->SavedAt=FDateTime::UtcNow();FAltaiItem I;I.Id=TEXT("Stone");I.Quantity=Count;S->Items.Add(I);return S;};
 auto File=[&](const FString& Id){return FPaths::Combine(Root,TEXT("Profile_")+Id+TEXT(".sav"));};
 auto Bytes=[](const FString& Path){TArray<uint8> B;FFileHelper::LoadFileToArray(B,*Path);return B;};
 auto RawWrite=[](USaveGame* S,const FString& Path){TArray<uint8> B;return UGameplayStatics::SaveGameToMemory(S,B)&&FFileHelper::SaveArrayToFile(B,*Path);};
 auto* A=Make(TEXT("Первый"),3);auto* B=Make(TEXT("Второй"),8);
 TestTrue(TEXT("create A"),Store.Write(A,true));TestTrue(TEXT("create B"),Store.Write(B,true));
 TestFalse(TEXT("new game cannot replace existing ID"),Store.Write(A,true));
 const auto BeforeB=Bytes(File(B->ProfileId));
 A->Items[0].Quantity=4;A->PlayerTransform.SetLocation(FVector(100,200,110));A->SavedAt+=FTimespan::FromMinutes(1);
 A->Items[0].QuickAccess=true;FAltaiLoadoutEntry E;E.Slot=EAltaiEquipSlot::Quick1;E.ItemId=A->Items[0].Id;A->Loadout.Add(E);
 TestTrue(TEXT("update A"),Store.Write(A));TestTrue(TEXT("B byte-for-byte unchanged"),BeforeB==Bytes(File(B->ProfileId)));
 TestTrue(TEXT("select older B"),Store.MarkPlayed(B->ProfileId));
 TestEqual(TEXT("Continue uses last played, not newest timestamp"),Store.LastPlayed(),B->ProfileId);
 TestEqual(TEXT("two independent profiles"),Store.List().Num(),2);
 TestEqual(TEXT("loadout round trip"),Store.Load(A->ProfileId)->Loadout.Num(),1);
 TestEqual(TEXT("A quantity"),Store.Load(A->ProfileId)->Items[0].Quantity,4);
 TestEqual(TEXT("B quantity"),Store.Load(B->ProfileId)->Items[0].Quantity,8);
 TestFalse(TEXT("path traversal rejected"),Store.Load(TEXT("../LastPlayed"))!=nullptr);
 const auto BeforeA=Bytes(File(A->ProfileId));A->Items[0].Quantity=-1;
 TestFalse(TEXT("invalid data refused"),Store.Write(A));TestTrue(TEXT("invalid write preserves prior bytes"),BeforeA==Bytes(File(A->ProfileId)));A->Items[0].Quantity=4;
 TestTrue(TEXT("replace primary with unreadable profile class"),RawWrite(NewObject<UAltaiProfileIndex>(),File(A->ProfileId)));
 bool Recovered=false;auto* Restored=Store.Load(A->ProfileId,&Recovered);
 TestTrue(TEXT("fallback to backup"),Restored&&Recovered);
 if(Restored){TestEqual(TEXT("previous valid snapshot"),Restored->Items[0].Quantity,3);TestTrue(TEXT("save after recovery"),Store.Write(A));}
 auto* Legacy=Make(TEXT("unused"),11);Legacy->Version=1;
 const FString LegacyPath=FPaths::Combine(Root,TEXT("Legacy.sav"));TestTrue(TEXT("legacy fixture"),RawWrite(Legacy,LegacyPath));const auto Original=Bytes(LegacyPath);
 TestTrue(TEXT("import v1"),Store.ImportLegacy(LegacyPath));TestTrue(TEXT("repeat import"),Store.ImportLegacy(LegacyPath));
 TestEqual(TEXT("import exactly once"),Store.List().Num(),3);TestTrue(TEXT("legacy original retained unchanged"),Original==Bytes(LegacyPath));
 TestEqual(TEXT("migration does not steal Continue"),Store.LastPlayed(),B->ProfileId);
 for(int32 N=1;N<100;++N){FAltaiItem Item;Item.Id=FName(*FString::Printf(TEXT("LargeBag%d"),N));Item.Quantity=N;B->Items.Add(Item);}
 TestTrue(TEXT("save 100 distinct stacks"),Store.Write(B));
 auto* LargeBag=Store.Load(B->ProfileId);TestNotNull(TEXT("load large inventory"),LargeBag);
 if(LargeBag){TestEqual(TEXT("all stacks survive save/load"),LargeBag->Items.Num(),100);if(LargeBag->Items.Num()==100){TestEqual(TEXT("last item identity"),LargeBag->Items.Last().Id,B->Items.Last().Id);TestEqual(TEXT("last item quantity"),LargeBag->Items.Last().Quantity,99);}}
 TestFalse(TEXT("unknown profile cannot be updated"),Store.Write(Make(TEXT("Unknown"),1)));
 return true;
}
#endif
