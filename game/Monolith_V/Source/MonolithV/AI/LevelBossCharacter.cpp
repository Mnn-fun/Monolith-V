#include "LevelBossCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "../Combat/MeleeHitboxComponent.h"

ALevelBossCharacter::ALevelBossCharacter()
{
	EnemyTier = EEnemyTier::LevelBoss;
	BaseHealth = 500.0f;

	// Large capsule for a boss-sized enemy
	if (GetCapsuleComponent())
	{
		GetCapsuleComponent()->InitCapsuleSize(70.f, 140.0f);
	}

	if (GetCharacterMovement())
	{
		GetCharacterMovement()->MaxWalkSpeed = 300.0f; // Slow, imposing
		GetCharacterMovement()->GravityScale = 1.0f;
	}

	// Level Boss hitbox: massive AoE slams that punish close-range
	if (MeleeHitboxComponent)
	{
		MeleeHitboxComponent->AttackRange = 350.0f;
		MeleeHitboxComponent->AttackRadius = 150.0f;
		MeleeHitboxComponent->DamageAmount = 40.0f;
		MeleeHitboxComponent->KnockbackForce = 1500.0f;
		MeleeHitboxComponent->AttackCooldown = 2.5f;
	}
}
