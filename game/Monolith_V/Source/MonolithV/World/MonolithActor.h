#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MonolithActor.generated.h"

class AEnemyBaseCharacter;
class AAltitudeStreamingManager;
class AGeneralEnemyCharacter;
class ASoldierEnemyCharacter;

USTRUCT(BlueprintType)
struct FBandSpawnState
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<TObjectPtr<AEnemyBaseCharacter>> SpawnedEnemies;

	UPROPERTY()
	TArray<TObjectPtr<AGeneralEnemyCharacter>> ActiveGenerals;

	UPROPERTY()
	TArray<TObjectPtr<ASoldierEnemyCharacter>> ActiveSoldiers;
};

UCLASS()
class MONOLITHV_API AMonolithActor : public AActor
{
	GENERATED_BODY()
	
public:	
	AMonolithActor();

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void SpawnTick();

	UFUNCTION()
	void HandleBandUnloaded(int32 BandIndex);

	UFUNCTION()
	void OnEnemyDestroyed(AActor* DestroyedActor);

	void SpawnForBand(int32 BandIndex);
	void ReassignSoldiers(int32 BandIndex);

	FVector GetRandomSpawnLocation(int32 BandIndex);

	UPROPERTY(EditAnywhere, Category = "Spawning")
	TSubclassOf<ASoldierEnemyCharacter> SoldierClass;

	UPROPERTY(EditAnywhere, Category = "Spawning")
	TSubclassOf<AGeneralEnemyCharacter> GeneralClass;

	UPROPERTY(EditAnywhere, Category = "Spawning")
	TSubclassOf<AEnemyBaseCharacter> BossClass;

	UPROPERTY(EditAnywhere, Category = "Spawning")
	TSubclassOf<AEnemyBaseCharacter> GoliathClass;

	UPROPERTY(EditAnywhere, Category = "Spawning")
	float SpawnInterval = 10.0f;

	UPROPERTY(EditAnywhere, Category = "Spawning")
	float SpawnRadius = 4000.0f;

	UPROPERTY()
	TMap<int32, FBandSpawnState> BandStates;

	UPROPERTY()
	AAltitudeStreamingManager* StreamingManager;

	FTimerHandle SpawnTimerHandle;
};
