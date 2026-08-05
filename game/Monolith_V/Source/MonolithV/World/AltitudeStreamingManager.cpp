#include "AltitudeStreamingManager.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/LevelStreamingDynamic.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "TimerManager.h"
#include "Engine/Level.h"
#include "GameFramework/WorldSettings.h"
#include "../Player/MonolithVCharacter.h"

AAltitudeStreamingManager::AAltitudeStreamingManager()
{
	PrimaryActorTick.bCanEverTick = false; // Using ~1Hz timer instead of Tick
	bReplicates = true; // Replicate to clients so they also run local level streaming
	bAlwaysRelevant = true; // Ensure manager is always relevant to all connected clients

	// Default 5 bands (each 2000 Unreal units of Z-height, Band_00 to Band_04)
	Bands.Add(FAltitudeBand(0, 0.0f, 2000.0f, TEXT("/Game/Maps/Band_00")));
	Bands.Add(FAltitudeBand(1, 2000.0f, 4000.0f, TEXT("/Game/Maps/Band_01")));
	Bands.Add(FAltitudeBand(2, 4000.0f, 6000.0f, TEXT("/Game/Maps/Band_02")));
	Bands.Add(FAltitudeBand(3, 6000.0f, 8000.0f, TEXT("/Game/Maps/Band_03")));
	Bands.Add(FAltitudeBand(4, 8000.0f, 10000.0f, TEXT("/Game/Maps/Band_04")));
}

void AAltitudeStreamingManager::BeginPlay()
{
	Super::BeginPlay();

	if (GetWorld())
	{
		// Throttled ~1Hz check for player altitudes (runs on both Server and Clients)
		GetWorldTimerManager().SetTimer(StreamingCheckTimerHandle, this, &AAltitudeStreamingManager::CheckPlayerAltitudes, 1.0f, true);
	}
}

void AAltitudeStreamingManager::CheckPlayerAltitudes()
{
	if (!GetWorld())
	{
		return;
	}

	TSet<int32> NeededBands;

	if (HasAuthority())
	{
		// Server: collect all valid player states across all connections and compute their current band
		CurrentBandPerPlayer.Empty();

		for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
		{
			APlayerController* PC = Iterator->Get();
			if (!PC || !PC->PlayerState)
			{
				continue;
			}

			float PawnZ = 0.0f;
			APawn* Pawn = PC->GetPawn();
			if (Pawn)
			{
				PawnZ = Pawn->GetActorLocation().Z;
			}

			int32 FoundBandIndex = 0;
			bool bBandFound = false;

			for (const FAltitudeBand& Band : Bands)
			{
				if (PawnZ >= Band.MinZ && PawnZ < Band.MaxZ)
				{
					FoundBandIndex = Band.BandIndex;
					bBandFound = true;
					break;
				}
			}

			if (!bBandFound && Bands.Num() > 0)
			{
				if (PawnZ < Bands[0].MinZ)
				{
					FoundBandIndex = Bands[0].BandIndex;
				}
				else
				{
					FoundBandIndex = Bands.Last().BandIndex;
				}
			}

			CurrentBandPerPlayer.Add(PC->PlayerState, FoundBandIndex);

			if (AMonolithVCharacter* Character = Cast<AMonolithVCharacter>(Pawn))
			{
				Character->CurrentBandIndex = FoundBandIndex;
			}


			for (int32 Offset = -1; Offset <= 1; ++Offset)
			{
				const int32 BufferBandIndex = FoundBandIndex + Offset;
				if (BufferBandIndex >= 0 && BufferBandIndex < Bands.Num())
				{
					NeededBands.Add(BufferBandIndex);
				}
			}
		}
	}
	else
	{
		// Client: collect needed bands based solely on the local player's altitude
		APlayerController* LocalPC = GetWorld()->GetFirstPlayerController();
		if (LocalPC)
		{
			float PawnZ = 0.0f;
			if (APawn* Pawn = LocalPC->GetPawn())
			{
				PawnZ = Pawn->GetActorLocation().Z;
			}

			int32 FoundBandIndex = 0;
			bool bBandFound = false;

			for (const FAltitudeBand& Band : Bands)
			{
				if (PawnZ >= Band.MinZ && PawnZ < Band.MaxZ)
				{
					FoundBandIndex = Band.BandIndex;
					bBandFound = true;
					break;
				}
			}

			if (!bBandFound && Bands.Num() > 0)
			{
				if (PawnZ < Bands[0].MinZ)
				{
					FoundBandIndex = Bands[0].BandIndex;
				}
				else
				{
					FoundBandIndex = Bands.Last().BandIndex;
				}
			}

			for (int32 Offset = -1; Offset <= 1; ++Offset)
			{
				const int32 BufferBandIndex = FoundBandIndex + Offset;
				if (BufferBandIndex >= 0 && BufferBandIndex < Bands.Num())
				{
					NeededBands.Add(BufferBandIndex);
				}
			}

			if (AMonolithVCharacter* Character = Cast<AMonolithVCharacter>(LocalPC->GetPawn()))
			{
				Character->CurrentBandIndex = FoundBandIndex;
			}
		}
	}

	ActiveBands = NeededBands;

	// 2. Trigger load for any needed band that isn't loaded yet
	for (int32 NeededBandIndex : NeededBands)
	{
		if (!LoadedBands.Contains(NeededBandIndex) || !LoadedBands[NeededBandIndex])
		{
			LoadBand(NeededBandIndex);
		}
	}

	// 3. Trigger unload for any band that is currently loaded but no longer needed by any player within ±1 buffer
	TArray<int32> BandsToUnload;
	for (const auto& Pair : LoadedBands)
	{
		if (!NeededBands.Contains(Pair.Key))
		{
			BandsToUnload.Add(Pair.Key);
		}
	}

	for (int32 UnloadBandIndex : BandsToUnload)
	{
		UnloadBand(UnloadBandIndex);
	}

	// 4. Debug on-screen display listing currently loaded bands and each player's current band
	if (GEngine)
	{
		FString LoadedBandsStr;
		for (const auto& Pair : LoadedBands)
		{
			if (Pair.Value)
			{
				LoadedBandsStr += FString::Printf(TEXT("Band_%02d "), Pair.Key);
			}
		}
		if (LoadedBandsStr.IsEmpty())
		{
			LoadedBandsStr = TEXT("None");
		}

		GEngine->AddOnScreenDebugMessage(1001, 1.5f, FColor::Cyan, FString::Printf(TEXT("[AltitudeStreaming] Loaded Bands (±1 buffer): %s"), *LoadedBandsStr));

		int32 DebugMsgID = 1002;
		for (const auto& Pair : CurrentBandPerPlayer)
		{
			if (APlayerState* PS = Pair.Key.Get())
			{
				APlayerController* PC = PS->GetPlayerController();
				float ZPos = (PC && PC->GetPawn()) ? PC->GetPawn()->GetActorLocation().Z : 0.0f;
				GEngine->AddOnScreenDebugMessage(DebugMsgID++, 1.5f, FColor::Green, FString::Printf(TEXT("[AltitudeStreaming] Player %s | Z: %.1f | Current Band: %d"), *PS->GetPlayerName(), ZPos, Pair.Value));
			}
		}
	}
}

void AAltitudeStreamingManager::LoadBand(int32 BandIndex)
{
	if (BandIndex < 0 || BandIndex >= Bands.Num() || !GetWorld())
	{
		return;
	}

	const FAltitudeBand& Band = Bands[BandIndex];
	bool bFound = false;

	ULevelStreamingDynamic* StreamingLevel = ULevelStreamingDynamic::LoadLevelInstance(GetWorld(), Band.LevelName, FVector::ZeroVector, FRotator::ZeroRotator, bFound);
	if (bFound && StreamingLevel)
	{
		LoadedBands.Add(BandIndex, StreamingLevel);
		StreamingLevel->OnLevelLoaded.AddDynamic(this, &AAltitudeStreamingManager::OnBandLevelLoaded);
		UE_LOG(LogTemp, Log, TEXT("[AltitudeStreaming] Loaded Band %d (%s)"), BandIndex, *Band.LevelName);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[AltitudeStreaming] Failed to load level instance for Band %d (%s). Verify level package exists in Content/Maps/."), BandIndex, *Band.LevelName);
	}
}

void AAltitudeStreamingManager::UnloadBand(int32 BandIndex)
{
	if (LoadedBands.Contains(BandIndex))
	{
		if (ULevelStreamingDynamic* StreamingLevel = LoadedBands[BandIndex])
		{
			StreamingLevel->SetShouldBeLoaded(false);
			StreamingLevel->SetShouldBeVisible(false);
			UE_LOG(LogTemp, Log, TEXT("[AltitudeStreaming] Unloaded Band %d"), BandIndex);
		}
		LoadedBands.Remove(BandIndex);
		
		OnBandUnloaded.Broadcast(BandIndex);
	}
}

void AAltitudeStreamingManager::OnBandLevelLoaded()
{
	if (!HasAuthority())
	{
		return;
	}

	for (const auto& Pair : LoadedBands)
	{
		if (ULevelStreamingDynamic* StreamingLevel = Pair.Value)
		{
			if (ULevel* LoadedLevel = StreamingLevel->GetLoadedLevel())
			{
				for (AActor* Actor : LoadedLevel->Actors)
				{
					if (Actor && !Actor->IsA<AWorldSettings>())
					{
						Actor->SetReplicates(true);
					}
				}
			}
		}
	}
}

bool AAltitudeStreamingManager::GetBandConfig(int32 BandIndex, FAltitudeBand& OutBand) const
{
	if (BandIndex >= 0 && BandIndex < Bands.Num())
	{
		OutBand = Bands[BandIndex];
		return true;
	}
	return false;
}
