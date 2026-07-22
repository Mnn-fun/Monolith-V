#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GA_TestAbility.generated.h"

UCLASS()
class MONOLITHV_API UGA_TestAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_TestAbility();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
};
