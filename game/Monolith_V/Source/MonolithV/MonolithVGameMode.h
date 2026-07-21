#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MonolithVGameMode.generated.h"

UCLASS()
class MONOLITHV_API AMonolithVGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AMonolithVGameMode();
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual APawn* SpawnDefaultPawnFor_Implementation(AController* NewPlayer, AActor* StartSpot) override;
};
