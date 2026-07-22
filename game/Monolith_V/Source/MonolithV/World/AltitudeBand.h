#pragma once

#include "CoreMinimal.h"
#include "AltitudeBand.generated.h"

/**
 * FAltitudeBand
 * Represents a single altitude tier / band in the vertical chunk streaming system.
 * Each band corresponds to a Z-range and an associated sublevel package name.
 */
USTRUCT(BlueprintType)
struct MONOLITHV_API FAltitudeBand
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Streaming")
	int32 BandIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Streaming")
	float MinZ = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Streaming")
	float MaxZ = 2000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Streaming")
	FString LevelName = TEXT("/Game/Maps/Band_00");

	FAltitudeBand() = default;

	FAltitudeBand(int32 InBandIndex, float InMinZ, float InMaxZ, const FString& InLevelName)
		: BandIndex(InBandIndex), MinZ(InMinZ), MaxZ(InMaxZ), LevelName(InLevelName)
	{
	}
};
