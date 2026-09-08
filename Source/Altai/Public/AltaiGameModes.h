#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "AltaiGameModes.generated.h"
UCLASS()
class ALTAI_API AAltaiWorldMode : public AGameModeBase
{
 GENERATED_BODY()
};
UCLASS()
class ALTAI_API AAltaiMenuMode : public AGameModeBase
{
 GENERATED_BODY()
public:
 AAltaiMenuMode(){DefaultPawnClass=nullptr;}
};
