#pragma once

#include "CoreMinimal.h"
#include "EnemyBaseCharacter.h"
#include "GoliathCharacter.generated.h"

/**
 * The Goliath — final boss of the game.
 * Has multiple attack patterns that cycle based on internal state.
 * Attacks: Ground Pound (wide AoE), Charge (long range dash), Sweep (360 degree)
 */
UENUM(BlueprintType)
enum class EGoliathAttackType : uint8
{
	GroundPound  UMETA(DisplayName = "Ground Pound"),   // Wide AoE slam
	Charge       UMETA(DisplayName = "Charge"),          // Long-range dash forward
	Sweep        UMETA(DisplayName = "Sweep")            // 360 degree spin attack
};

UCLASS()
class MONOLITHV_API AGoliathCharacter : public AEnemyBaseCharacter
{
	GENERATED_BODY()

public:
	AGoliathCharacter();

	/** Execute the current attack pattern, cycling to the next one after. */
	bool ExecuteGoliathAttack();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Goliath")
	float GroundPoundRadius;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Goliath")
	float GroundPoundDamage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Goliath")
	float ChargeRange;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Goliath")
	float ChargeDamage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Goliath")
	float SweepRadius;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Goliath")
	float SweepDamage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Goliath")
	float GoliathKnockbackForce;

protected:
	EGoliathAttackType CurrentAttack;

	bool ExecuteGroundPound();
	bool ExecuteCharge();
	bool ExecuteSweep();

	/** Apply damage and knockback to all ASC actors in a sphere at a given location */
	bool DamageActorsInSphere(FVector Center, float Radius, float Damage, float Knockback);
};
