#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GA_Jetpack.generated.h"

UCLASS()
class MONOLITHV_API UGA_Jetpack : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Jetpack();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	virtual void InputReleased(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) override;

protected:
	UFUNCTION()
	void OnJetpackTick();

	FTimerHandle JetpackTimerHandle;
	
	// Fuel drain per tick (0.05s)
	float FuelDrainRate = 1.0f; 

	// Thrust applied per tick (0.05s)
	float ThrustStrength = 100.0f;
};
