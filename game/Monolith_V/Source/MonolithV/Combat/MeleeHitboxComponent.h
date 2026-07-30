#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "MeleeHitboxComponent.generated.h"

class UAbilitySystemComponent;

UCLASS(ClassGroup=(Combat), meta=(BlueprintSpawnableComponent))
class MONOLITHV_API UMeleeHitboxComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMeleeHitboxComponent();

	// Configurable per enemy tier
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Melee Hitbox")
	float AttackRange;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Melee Hitbox")
	float AttackRadius;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Melee Hitbox")
	float DamageAmount;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Melee Hitbox")
	float KnockbackForce;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Melee Hitbox")
	float AttackCooldown;

	/**
	 * Performs a server-authoritative sphere sweep in front of the owning character.
	 * Damages and knocks back every IAbilitySystemInterface actor hit.
	 * @return true if at least one target was hit.
	 */
	bool ExecuteAttack();

	/** Returns true if the cooldown has elapsed since the last attack. */
	bool CanAttack() const;

private:
	float LastAttackTime;
};
