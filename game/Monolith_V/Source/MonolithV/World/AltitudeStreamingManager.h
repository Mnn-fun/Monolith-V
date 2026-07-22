#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AltitudeBand.h"
#include "AltitudeStreamingManager.generated.h"

class ULevelStreamingDynamic;
class APlayerState;

/**
 * AAltitudeStreamingManager
 * Server-authoritative manager responsible for tracking player altitudes (~1Hz check)
 * and dynamically loading/unloading sublevel chunks (FAltitudeBand) via ULevelStreamingDynamic.
 */
UCLASS()
class MONOLITHV_API AAltitudeStreamingManager : public AActor
{
	GENERATED_BODY()

public:
	AAltitudeStreamingManager();

protected:
	virtual void BeginPlay() override;

	void CheckPlayerAltitudes();

	void LoadBand(int32 BandIndex);
	void UnloadBand(int32 BandIndex);

	UFUNCTION()
	void OnBandLevelLoaded();

	UPROPERTY(EditAnywhere, Category = "Streaming")
	TArray<FAltitudeBand> Bands;

	UPROPERTY()
	TMap<int32, TObjectPtr<ULevelStreamingDynamic>> LoadedBands;

	UPROPERTY()
	TMap<TWeakObjectPtr<APlayerState>, int32> CurrentBandPerPlayer;

	FTimerHandle StreamingCheckTimerHandle;
};
