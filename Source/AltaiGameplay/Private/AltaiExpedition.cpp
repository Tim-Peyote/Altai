#include "AltaiExpedition.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "Misc/Paths.h"
#include "TimerManager.h"

UAltaiExpedition::UAltaiExpedition()
{
 StartingInventory=TSoftObjectPtr<UAltaiInventoryPreset>(FSoftObjectPath(TEXT("/Game/Altai/Items/DA_StartingInventory.DA_StartingInventory")));
}
FAltaiProfileStore UAltaiExpedition::Store() const
{
 const bool PIE=GetWorld() && GetWorld()->WorldType==EWorldType::PIE;
 return FAltaiProfileStore(FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("SaveGames"),PIE?TEXT("EditorProfiles"):TEXT("Profiles")));
}
void UAltaiExpedition::ImportLegacy() const
{
 const bool PIE=GetWorld() && GetWorld()->WorldType==EWorldType::PIE;
 Store().ImportLegacy(FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("SaveGames"),PIE?TEXT("Altai_EditorExpedition_v1.sav"):TEXT("Altai_Expedition_v1.sav")));
}
bool UAltaiExpedition::HasSave() const
{ ImportLegacy();return !Store().LastPlayed().IsEmpty(); }
TArray<FAltaiProfileInfo> UAltaiExpedition::GetProfiles()
{ ImportLegacy();return Store().List(); }
bool UAltaiExpedition::IsExpedition() const
{ return UGameplayStatics::GetCurrentLevelName(this,true)==TEXT("L_World"); }
bool UAltaiExpedition::NewExpedition()
{ return CreateProfile(TEXT("Новое прохождение")); }
bool UAltaiExpedition::CreateProfile(const FString& Name)
{
 const FString CleanName=Name.TrimStartAndEnd().Left(32);
 if(CleanName.IsEmpty()){SaveStatus=TEXT("Введите имя прохождения.");return false;}
 ImportLegacy();
 auto* Save=NewObject<UAltaiSave>();Save->Version=2;
 Save->ProfileId=FGuid::NewGuid().ToString(EGuidFormats::Digits);Save->ProfileName=CleanName;Save->SavedAt=FDateTime::UtcNow();
 Save->PlayerTransform=FTransform(FRotator::ZeroRotator,FVector(0,0,110));
 if(const auto* Preset=StartingInventory.LoadSynchronous())Save->Items=Preset->Items;
 if(!Store().Write(Save,true)){SaveStatus=TEXT("Не удалось создать сохранение. Проверьте место на диске и доступ к папке.");return false;}
 return LoadProfile(Save->ProfileId);
}
bool UAltaiExpedition::ContinueExpedition()
{ ImportLegacy();return LoadProfile(Store().LastPlayed()); }
bool UAltaiExpedition::LoadProfile(const FString& Id)
{
 auto* Save=Store().Load(Id);
 if(!Save){SaveStatus=TEXT("Прохождение недоступно: сохранение не удалось прочитать.");return false;}
 if(!Store().MarkPlayed(Id)){SaveStatus=TEXT("Не удалось записать выбор прохождения.");return false;}
 bSessionReady=false;PendingSave=Save;ActiveId=Id;ActiveName=Save->ProfileName;
 SaveStatus.Empty();bSaveError=false;
 UGameplayStatics::SetGamePaused(this,false);
 UGameplayStatics::OpenLevel(this,TEXT("/Game/Altai/Maps/L_World"));return true;
}
void UAltaiExpedition::WorldReady()
{
 auto* PC=UGameplayStatics::GetPlayerController(this,0);
 if(!IsExpedition() || !PC || !PC->GetPawn())return;
 if(!PendingSave)
 { bSessionReady=false;ActiveId.Empty();ActiveName.Empty();Items.Reset();Loadout.Reset();return; }
 Items=PendingSave->Items;Loadout=PendingSave->Loadout;BasePlaySeconds=PendingSave->PlaySeconds;
 // Definitions may gain presentation/category metadata without replacing saved quantities.
 if(const auto* Preset=StartingInventory.LoadSynchronous())for(auto& Item:Items)
  if(const auto* Definition=Preset->Items.FindByPredicate([&](const FAltaiItem& D){return D.Id==Item.Id;}))
  {Item.Category=Definition->Category;Item.WearSlot=Definition->WearSlot;Item.QuickAccess=Definition->QuickAccess;Item.Icon=Definition->Icon;}
 Loadout.RemoveAll([&](const FAltaiLoadoutEntry& E){const auto* I=Items.FindByPredicate([&](const FAltaiItem& Item){return Item.Id==E.ItemId;});return !I || !I->CanAssign(E.Slot);});
 PC->GetPawn()->SetActorTransform(PendingSave->PlayerTransform,false,nullptr,ETeleportType::TeleportPhysics);
 PC->SetControlRotation(PendingSave->ViewRotation);PendingSave=nullptr;
 SessionStartSeconds=GetWorld()->GetTimeSeconds();bSessionReady=true;
 GetWorld()->GetTimerManager().SetTimer(AutosaveTimer,this,&UAltaiExpedition::Autosave,30.0f,true);
}
bool UAltaiExpedition::SaveExpedition()
{
 ++SaveRevision;bSaveError=true;
 auto* PC=UGameplayStatics::GetPlayerController(this,0);
 if(!bSessionReady || ActiveId.IsEmpty() || !IsExpedition() || !PC || !PC->GetPawn())
 {SaveStatus=TEXT("Сначала начните или загрузите прохождение из главного меню.");return false;}
 auto* Previous=Store().Load(ActiveId);
 if(!Previous){SaveStatus=TEXT("Активное сохранение недоступно. Другие прохождения не изменены.");return false;}
 auto* Save=NewObject<UAltaiSave>();Save->Version=2;
 Save->ProfileId=ActiveId;Save->ProfileName=ActiveName;Save->ImportedLegacy=Previous->ImportedLegacy;
 Save->SavedAt=FDateTime::UtcNow();Save->PlaySeconds=BasePlaySeconds+FMath::Max(0.0,GetWorld()->GetTimeSeconds()-SessionStartSeconds);
 Save->Items=Items;Save->Loadout=Loadout;Save->PlayerTransform=PC->GetPawn()->GetActorTransform();Save->ViewRotation=PC->GetControlRotation();
 if(!Store().Write(Save)) {SaveStatus=TEXT("Не удалось сохранить. Проверьте место на диске и доступ к папке.");return false;}
 bSaveError=false;SaveStatus=TEXT("Сохранено: ")+ActiveName;return true;
}
void UAltaiExpedition::Autosave()
{ if(bSessionReady && IsExpedition())SaveExpedition(); }
void UAltaiExpedition::ReturnToMenu()
{
 if(GetWorld())GetWorld()->GetTimerManager().ClearTimer(AutosaveTimer);
 bSessionReady=false;PendingSave=nullptr;Items.Reset();Loadout.Reset();ActiveId.Empty();ActiveName.Empty();
 UGameplayStatics::SetGamePaused(this,false);
 UGameplayStatics::OpenLevel(this,TEXT("/Game/Altai/Maps/L_MainMenu"));
}
bool UAltaiExpedition::RemoveOne(int32 Index)
{
 if(!Items.IsValidIndex(Index))return false;
 if(--Items[Index].Quantity<=0){const FName Id=Items[Index].Id;Items.RemoveAt(Index);Loadout.RemoveAll([&](const FAltaiLoadoutEntry& E){return E.ItemId==Id;});}
 Autosave();return true;
}
bool UAltaiExpedition::AddItem(const FAltaiItem& Item)
{
 if(Item.Id.IsNone() || Item.Quantity<=0)return false;
 for(auto& Existing:Items)if(Existing.Id==Item.Id)
 {if(Existing.Quantity>MAX_int32-Item.Quantity)return false;Existing.Quantity+=Item.Quantity;Autosave();return true;}
 Items.Add(Item);Autosave();return true;
}

bool UAltaiExpedition::AssignItem(FName Id,EAltaiEquipSlot Slot)
{
 const auto* Item=Items.FindByPredicate([&](const FAltaiItem& I){return I.Id==Id;});
 if(!Item || !Item->CanAssign(Slot))return false;
 Loadout.RemoveAll([&](const FAltaiLoadoutEntry& E){return E.Slot==Slot || E.ItemId==Id;});
 FAltaiLoadoutEntry E;E.Slot=Slot;E.ItemId=Id;Loadout.Add(E);Autosave();return true;
}
bool UAltaiExpedition::ClearSlot(EAltaiEquipSlot Slot)
{
 if(!Loadout.RemoveAll([&](const FAltaiLoadoutEntry& E){return E.Slot==Slot;}))return false;
 Autosave();return true;
}

bool UAltaiExpedition::MoveAssignment(FName Id,EAltaiEquipSlot Source,EAltaiEquipSlot Target)
{
 const auto* From=Loadout.FindByPredicate([&](const FAltaiLoadoutEntry& E){return E.Slot==Source && E.ItemId==Id;});
 if(!From)return false;
 if(Source==Target)return true;
 if(Target==EAltaiEquipSlot::None)return ClearSlot(Source);
 const auto* Item=Items.FindByPredicate([&](const FAltaiItem& I){return I.Id==Id;});
 if(!Item || !Item->CanAssign(Target))return false;
 const auto* To=Loadout.FindByPredicate([&](const FAltaiLoadoutEntry& E){return E.Slot==Target;});
 const FName OtherId=To?To->ItemId:NAME_None;
 if(To)
 {
  const auto* Other=Items.FindByPredicate([&](const FAltaiItem& I){return I.Id==OtherId;});
  if(!Other || !Other->CanAssign(Source))return false;
 }
 // Validate both sides before changing either slot. One save contains the entire swap.
 for(auto& E:Loadout){if(E.ItemId==Id)E.Slot=Target;else if(E.ItemId==OtherId)E.Slot=Source;}
 Autosave();return true;
}
