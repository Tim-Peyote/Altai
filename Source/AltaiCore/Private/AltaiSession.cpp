#include "AltaiSession.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
UAltaiSession* UAltaiSession::Find(const UObject* Context)
{
 if (!Context || !Context->GetWorld()) return nullptr;
 if (UGameInstance* GI = Context->GetWorld()->GetGameInstance())
  { const auto Systems=GI->GetSubsystemArrayCopy<UAltaiSession>(); return Systems.Num() ? Systems[0] : nullptr; }
 return nullptr;
}
