#include "MonolithVGameMode.h"
#include "Player/MonolithVCharacter.h"
#include "Player/MonolithVPlayerController.h"
#include "World/AltitudeStreamingManager.h"
#include "Engine/World.h"

AMonolithVGameMode::AMonolithVGameMode()
{
	DefaultPawnClass = AMonolithVCharacter::StaticClass();
	PlayerControllerClass = AMonolithVPlayerController::StaticClass();
}

void AMonolithVGameMode::StartPlay()
{
	Super::StartPlay();

	if (HasAuthority() && !AltitudeStreamingManager)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AltitudeStreamingManager = GetWorld()->SpawnActor<AAltitudeStreamingManager>(AAltitudeStreamingManager::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
	}
}
