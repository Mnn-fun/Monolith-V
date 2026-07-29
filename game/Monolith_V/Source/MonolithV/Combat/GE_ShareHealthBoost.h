#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GE_ShareHealthBoost.generated.h"

/**
 * UGE_ShareHealthBoost
 * A GameplayEffect that temporarily increases MaxHealth when players share their role item.
 */
UCLASS()
class MONOLITHV_API UGE_ShareHealthBoost : public UGameplayEffect
{
	GENERATED_BODY()
	
public:
	UGE_ShareHealthBoost();
};
