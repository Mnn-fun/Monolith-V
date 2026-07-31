#include "MonolithVGameMode.h"
#include "Player/MonolithVCharacter.h"
#include "Player/MonolithVPlayerController.h"
#include "World/AltitudeStreamingManager.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Networking/EOSSessionSubsystem.h"
#include "Kismet/GameplayStatics.h"

AMonolithVGameMode::AMonolithVGameMode()
{
	DefaultPawnClass = AMonolithVCharacter::StaticClass();
	PlayerControllerClass = AMonolithVPlayerController::StaticClass();
}

void AMonolithVGameMode::StartPlay()
{
	Super::StartPlay();

	if (HasAuthority())
	{
		if (!AltitudeStreamingManager)
		{
			// Check if one was already placed in the level (like BP_AltitudeStreamingManager)
			TArray<AActor*> FoundManagers;
			UGameplayStatics::GetAllActorsOfClass(GetWorld(), AAltitudeStreamingManager::StaticClass(), FoundManagers);
			if (FoundManagers.Num() > 0)
			{
				AltitudeStreamingManager = Cast<AAltitudeStreamingManager>(FoundManagers[0]);
			}
			else
			{
				FActorSpawnParameters SpawnParams;
				SpawnParams.Owner = this;
				SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
				AltitudeStreamingManager = GetWorld()->SpawnActor<AAltitudeStreamingManager>(AAltitudeStreamingManager::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
			}
		}

		if (UGameInstance* GameInstance = GetGameInstance())
		{
			if (UEOSSessionSubsystem* SessionSubsystem = GameInstance->GetSubsystem<UEOSSessionSubsystem>())
			{
				const bool bIsDedicated = IsRunningDedicatedServer();
				SessionSubsystem->CreateSession(16, bIsDedicated);
			}
		}
	}
}
