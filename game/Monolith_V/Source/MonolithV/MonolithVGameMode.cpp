#include "MonolithVGameMode.h"
#include "Player/MonolithVCharacter.h"
#include "Player/MonolithVPlayerController.h"

AMonolithVGameMode::AMonolithVGameMode()
{
	DefaultPawnClass = AMonolithVCharacter::StaticClass();
	PlayerControllerClass = AMonolithVPlayerController::StaticClass();
}

void AMonolithVGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	if (DefaultPawnClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("AMonolithVGameMode::PostLogin - Spawning Player %s with PawnClass: %s"), 
			*NewPlayer->GetName(), *DefaultPawnClass->GetName());
	}
}

APawn* AMonolithVGameMode::SpawnDefaultPawnFor_Implementation(AController* NewPlayer, AActor* StartSpot)
{
	if (NewPlayer == nullptr || DefaultPawnClass == nullptr)
	{
		return nullptr;
	}

	// Calculate a safe spawn location safely above the floor with an offset for each player so they never overlap or fall under
	static int32 SpawnCount = 0;
	FVector SpawnLocation = StartSpot ? StartSpot->GetActorLocation() : FVector::ZeroVector;
	SpawnLocation.X += (SpawnCount * 200.0f) - 100.0f; // Separate players side by side: -100, +100, +300...
	SpawnLocation.Z += 150.0f; // Ensure capsule (`half-height 88`) is safely above floor surface (`0`) so it never clips under the level
	SpawnCount++;

	FRotator SpawnRotation = StartSpot ? StartSpot->GetActorRotation() : FRotator::ZeroRotator;

	FActorSpawnParameters SpawnInfo;
	SpawnInfo.Instigator = GetInstigator();
	SpawnInfo.ObjectFlags |= RF_Transient; // Do not save default spawned pawns into the map file
	SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	APawn* ResultPawn = GetWorld()->SpawnActor<APawn>(DefaultPawnClass, SpawnLocation, SpawnRotation, SpawnInfo);
	if (ResultPawn)
	{
		UE_LOG(LogTemp, Warning, TEXT("AMonolithVGameMode: Successfully spawned %s for %s at (%f, %f, %f)"),
			*ResultPawn->GetName(), *NewPlayer->GetName(), SpawnLocation.X, SpawnLocation.Y, SpawnLocation.Z);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("AMonolithVGameMode: FAILED to spawn pawn for %s at (%f, %f, %f)!"),
			*NewPlayer->GetName(), SpawnLocation.X, SpawnLocation.Y, SpawnLocation.Z);
	}
	return ResultPawn;
}
