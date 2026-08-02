#include "MonolithActor.h"
#include "NavigationSystem.h"
#include "AltitudeStreamingManager.h"
#include "Kismet/GameplayStatics.h"
#include "../AI/EnemyBaseCharacter.h"
#include "../AI/GeneralEnemyCharacter.h"
#include "../AI/SoldierEnemyCharacter.h"

AMonolithActor::AMonolithActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
}

void AMonolithActor::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogTemp, Warning, TEXT("[Monolith] BeginPlay entered! (Authority: %d)"), HasAuthority() ? 1 : 0);

	if (HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("[Monolith] BeginPlay executing on server..."));
		
		// Find the streaming manager
		TArray<AActor*> FoundManagers;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), AAltitudeStreamingManager::StaticClass(), FoundManagers);
		if (FoundManagers.Num() > 0)
		{
			StreamingManager = Cast<AAltitudeStreamingManager>(FoundManagers[0]);
			if (StreamingManager)
			{
				StreamingManager->OnBandUnloaded.AddDynamic(this, &AMonolithActor::HandleBandUnloaded);
				UE_LOG(LogTemp, Warning, TEXT("[Monolith] Found AltitudeStreamingManager!"));
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("[Monolith] AltitudeStreamingManager NOT FOUND in world!"));
		}

		GetWorld()->GetTimerManager().SetTimer(SpawnTimerHandle, this, &AMonolithActor::SpawnTick, SpawnInterval, true, 5.0f);
	}
}

void AMonolithActor::SpawnTick()
{
	if (!HasAuthority()) return;

	if (!StreamingManager)
	{
		// Try to find it again (GameMode might have spawned it after Monolith's BeginPlay)
		TArray<AActor*> FoundManagers;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), AAltitudeStreamingManager::StaticClass(), FoundManagers);
		if (FoundManagers.Num() > 0)
		{
			StreamingManager = Cast<AAltitudeStreamingManager>(FoundManagers[0]);
			if (StreamingManager)
			{
				StreamingManager->OnBandUnloaded.AddDynamic(this, &AMonolithActor::HandleBandUnloaded);
			}
		}

		if (!StreamingManager)
		{
			GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, TEXT("[Monolith] Spawn Failed: StreamingManager is NULL!"));
			return;
		}
	}

	const TSet<int32>& ActiveBands = StreamingManager->GetActiveBands();

	for (int32 BandIndex : ActiveBands)
	{
		SpawnForBand(BandIndex);
	}
}

void AMonolithActor::SpawnForBand(int32 BandIndex)
{
	FBandSpawnState& State = BandStates.FindOrAdd(BandIndex);

	// Remove any nullptrs from destroyed enemies
	State.SpawnedEnemies.RemoveAll([](AEnemyBaseCharacter* Val) { return Val == nullptr || Val->IsPendingKillPending(); });
	State.ActiveGenerals.RemoveAll([](AGeneralEnemyCharacter* Val) { return Val == nullptr || Val->IsPendingKillPending(); });
	State.ActiveSoldiers.RemoveAll([](ASoldierEnemyCharacter* Val) { return Val == nullptr || Val->IsPendingKillPending(); });

	// Target counts
	int32 TargetSoldiers = 5 * FMath::Pow(2.0f, (float)BandIndex);
	int32 TargetGenerals = 1 + (BandIndex / 3);
	int32 TargetBosses = 1 + (BandIndex / 6);

	int32 CurrentSoldiers = State.ActiveSoldiers.Num();
	int32 CurrentGenerals = State.ActiveGenerals.Num();
	int32 CurrentBosses = 0;

	GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow, FString::Printf(TEXT("[Monolith] Band %d: Needs %d Soldiers, %d Generals"), BandIndex, TargetSoldiers, TargetGenerals));
	UE_LOG(LogTemp, Warning, TEXT("[Monolith] Band %d: Needs %d Soldiers, %d Generals, %d Bosses"), BandIndex, TargetSoldiers, TargetGenerals, TargetBosses);

	for (AEnemyBaseCharacter* Enemy : State.SpawnedEnemies)
	{
		if (Enemy && Enemy->EnemyTier == EEnemyTier::LevelBoss)
		{
			CurrentBosses++;
		}
	}

	// Helper lambda to spawn a specific class
	auto SpawnEnemy = [&](TSubclassOf<AEnemyBaseCharacter> ClassToSpawn) -> AEnemyBaseCharacter*
	{
		if (!ClassToSpawn) return nullptr;
		FVector SpawnLoc = GetRandomSpawnLocation(BandIndex);
		if (SpawnLoc != FVector::ZeroVector)
		{
			FActorSpawnParameters SpawnParams;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
			AEnemyBaseCharacter* Spawned = GetWorld()->SpawnActor<AEnemyBaseCharacter>(ClassToSpawn, SpawnLoc, FRotator::ZeroRotator, SpawnParams);
			if (Spawned)
			{
				Spawned->SpawnDefaultController();
				Spawned->InitializeForBand(BandIndex);
				Spawned->OnDestroyed.AddDynamic(this, &AMonolithActor::OnEnemyDestroyed);
				State.SpawnedEnemies.Add(Spawned);
				return Spawned;
			}
		}
		return nullptr;
	};

	// Spawn Bosses
	int32 Retries = 0;
	while (CurrentBosses < TargetBosses && BossClass && Retries < 20)
	{
		if (SpawnEnemy(BossClass)) CurrentBosses++;
		Retries++;
	}

	// Spawn Generals
	Retries = 0;
	while (CurrentGenerals < TargetGenerals && GeneralClass && Retries < 20)
	{
		if (AEnemyBaseCharacter* Spawned = SpawnEnemy(GeneralClass))
		{
			if (AGeneralEnemyCharacter* Gen = Cast<AGeneralEnemyCharacter>(Spawned))
			{
				State.ActiveGenerals.Add(Gen);
				CurrentGenerals++;
			}
		}
		Retries++;
	}

	// Spawn Soldiers
	Retries = 0;
	while (CurrentSoldiers < TargetSoldiers && SoldierClass && Retries < 20)
	{
		if (AEnemyBaseCharacter* Spawned = SpawnEnemy(SoldierClass))
		{
			if (ASoldierEnemyCharacter* Sol = Cast<ASoldierEnemyCharacter>(Spawned))
			{
				State.ActiveSoldiers.Add(Sol);
				CurrentSoldiers++;
				
				// Assign immediately to a general if available
				if (State.ActiveGenerals.Num() > 0)
				{
					int32 RandomGenIdx = FMath::RandRange(0, State.ActiveGenerals.Num() - 1);
					Sol->AssignedGeneral = State.ActiveGenerals[RandomGenIdx];
				}
			}
		}
		Retries++;
	}
}

FVector AMonolithActor::GetRandomSpawnLocation(int32 BandIndex)
{
	FVector Center = GetActorLocation();

	if (StreamingManager)
	{
		FAltitudeBand BandConfig;
		if (StreamingManager->GetBandConfig(BandIndex, BandConfig))
		{
			// Try to pick a Z within the band's bounds. If infinite, just use a default offset
			float ZLevel = BandConfig.MinZ;
			if (BandConfig.MinZ == -MAX_flt) ZLevel = 0.0f;
			Center.Z = ZLevel + 100.0f; // Slightly above ground floor
		}
	}

	UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld());
	if (NavSys)
	{
		FNavLocation ResultLocation;
		if (NavSys->GetRandomReachablePointInRadius(Center, SpawnRadius, ResultLocation))
		{
			FAltitudeBand BandConfig;
			if (StreamingManager && StreamingManager->GetBandConfig(BandIndex, BandConfig))
			{
				// Ensure NavMesh didn't accidentally pick a point on the ground floor far below!
				if (ResultLocation.Location.Z >= BandConfig.MinZ - 500.0f && ResultLocation.Location.Z <= BandConfig.MaxZ + 500.0f)
				{
					GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Green, TEXT("[Monolith] Found NavMesh Spawn!"));
					return ResultLocation.Location;
				}
			}
			else
			{
				return ResultLocation.Location;
			}
		}
	}

	// Fallback to simple line trace if NavMesh isn't fully built yet
	for (int32 i = 0; i < 10; ++i)
	{
		// Try a smaller radius first, increasing if we fail (to ensure we hit the platform if it's small)
		float CurrentRadius = SpawnRadius * (0.1f + (i * 0.1f)); 
		
		FVector RandomXY = Center + FVector(FMath::RandRange(-CurrentRadius, CurrentRadius), FMath::RandRange(-CurrentRadius, CurrentRadius), 0.0f);
		FVector TraceStart = RandomXY + FVector(0, 0, 1000.0f);
		FVector TraceEnd = RandomXY - FVector(0, 0, 2000.0f);

		FHitResult HitResult;
		FCollisionQueryParams QueryParams;
		if (GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_WorldStatic, QueryParams))
		{
			FAltitudeBand BandConfig;
			if (StreamingManager && StreamingManager->GetBandConfig(BandIndex, BandConfig))
			{
				// Ensure we are actually hitting the floor of THIS specific band, not falling through to a lower band!
				if (HitResult.ImpactPoint.Z >= BandConfig.MinZ - 500.0f && HitResult.ImpactPoint.Z <= BandConfig.MaxZ + 500.0f)
				{
					return HitResult.ImpactPoint + FVector(0, 0, 150.0f);
				}
			}
			else
			{
				return HitResult.ImpactPoint + FVector(0, 0, 150.0f);
			}
		}
	}

	GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("[Monolith] Spawn Failed: No NavMesh & Trace Failed!"));
	return FVector::ZeroVector;
}

void AMonolithActor::HandleBandUnloaded(int32 BandIndex)
{
	if (BandStates.Contains(BandIndex))
	{
		FBandSpawnState& State = BandStates[BandIndex];
		for (AEnemyBaseCharacter* Enemy : State.SpawnedEnemies)
		{
			if (Enemy && !Enemy->IsPendingKillPending())
			{
				Enemy->Destroy();
			}
		}
		BandStates.Remove(BandIndex);
	}
}

void AMonolithActor::OnEnemyDestroyed(AActor* DestroyedActor)
{
	AEnemyBaseCharacter* Enemy = Cast<AEnemyBaseCharacter>(DestroyedActor);
	if (!Enemy) return;

	int32 BandIndex = Enemy->SpawnedBandIndex;
	if (BandStates.Contains(BandIndex))
	{
		if (AGeneralEnemyCharacter* DeadGeneral = Cast<AGeneralEnemyCharacter>(Enemy))
		{
			// Reassign their soldiers
			ReassignSoldiers(BandIndex);
		}
	}
}

void AMonolithActor::ReassignSoldiers(int32 BandIndex)
{
	if (!BandStates.Contains(BandIndex)) return;
	
	FBandSpawnState& State = BandStates[BandIndex];
	
	// Clean up lists first
	State.ActiveGenerals.RemoveAll([](AGeneralEnemyCharacter* Val) { return Val == nullptr || Val->IsPendingKillPending(); });
	State.ActiveSoldiers.RemoveAll([](ASoldierEnemyCharacter* Val) { return Val == nullptr || Val->IsPendingKillPending(); });

	if (State.ActiveGenerals.Num() == 0)
	{
		// No generals left in this band
		for (ASoldierEnemyCharacter* Soldier : State.ActiveSoldiers)
		{
			if (Soldier) Soldier->AssignedGeneral = nullptr;
		}
		return;
	}

	for (ASoldierEnemyCharacter* Soldier : State.ActiveSoldiers)
	{
		if (Soldier && (!Soldier->AssignedGeneral.IsValid() || Soldier->AssignedGeneral->IsPendingKillPending()))
		{
			int32 RandomGenIdx = FMath::RandRange(0, State.ActiveGenerals.Num() - 1);
			Soldier->AssignedGeneral = State.ActiveGenerals[RandomGenIdx];
		}
	}
}
