#include "GeneralEnemyCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "../Combat/MeleeHitboxComponent.h"

AGeneralEnemyCharacter::AGeneralEnemyCharacter()
{
	EnemyTier = EEnemyTier::General;
	BaseHealth = 200.0f;

	// Slightly larger capsule for a beefier enemy
	if (GetCapsuleComponent())
	{
		GetCapsuleComponent()->InitCapsuleSize(55.f, 110.0f);
	}

	if (GetCharacterMovement())
	{
		GetCharacterMovement()->MaxWalkSpeed = 350.0f; // Slower but menacing
		GetCharacterMovement()->GravityScale = 1.0f;
	}

	// General-tier hitbox: wider swings, more damage, longer cooldown
	if (MeleeHitboxComponent)
	{
		MeleeHitboxComponent->AttackRange = 250.0f;
		MeleeHitboxComponent->AttackRadius = 100.0f;
		MeleeHitboxComponent->DamageAmount = 25.0f;
		MeleeHitboxComponent->KnockbackForce = 1000.0f;
		MeleeHitboxComponent->AttackCooldown = 2.0f;
	}
}
