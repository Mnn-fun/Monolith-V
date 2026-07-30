#include "SoldierEnemyCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "../Combat/WeaponComponent.h"
#include "../Combat/MeleeHitboxComponent.h"

ASoldierEnemyCharacter::ASoldierEnemyCharacter()
{
	EnemyTier = EEnemyTier::Soldier;
	BaseHealth = 50.0f; // Lower health for drones/grunts

	// Adjust collision and movement for a standard fast soldier
	if (GetCapsuleComponent())
	{
		GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
	}

	if (GetCharacterMovement())
	{
		GetCharacterMovement()->MaxWalkSpeed = 450.0f; // Somewhat fast
		GetCharacterMovement()->GravityScale = 1.0f;
	}

	// Soldier-tier hitbox: fast, weak, short range
	if (MeleeHitboxComponent)
	{
		MeleeHitboxComponent->AttackRange = 150.0f;
		MeleeHitboxComponent->AttackRadius = 60.0f;
		MeleeHitboxComponent->DamageAmount = 10.0f;
		MeleeHitboxComponent->KnockbackForce = 600.0f;
		MeleeHitboxComponent->AttackCooldown = 1.5f;
	}

	// Give the soldier a weapon component so it can use GA_FireWeapon
	UWeaponComponent* WeaponComp = CreateDefaultSubobject<UWeaponComponent>(TEXT("WeaponComponent"));
	// Infinite ammo for enemies, or very high
	WeaponComp->MaxAmmo = 9999;
	WeaponComp->Ammo = 9999;
}
