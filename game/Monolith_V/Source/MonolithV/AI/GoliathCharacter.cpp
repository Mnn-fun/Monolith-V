#include "GoliathCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "../Combat/MeleeHitboxComponent.h"
#include "../Combat/MonolithVAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "Engine/OverlapResult.h"

AGoliathCharacter::AGoliathCharacter()
{
	EnemyTier = EEnemyTier::Goliath;
	BaseHealth = 1000.0f;

	// Massive capsule for the final boss
	if (GetCapsuleComponent())
	{
		GetCapsuleComponent()->InitCapsuleSize(100.f, 200.0f);
	}

	if (GetCharacterMovement())
	{
		GetCharacterMovement()->MaxWalkSpeed = 250.0f; // Slow, unstoppable
		GetCharacterMovement()->GravityScale = 1.0f;
	}

	// Default melee hitbox (used as fallback, but Goliath uses its own multi-attack system)
	if (MeleeHitboxComponent)
	{
		MeleeHitboxComponent->AttackRange = 500.0f;
		MeleeHitboxComponent->AttackRadius = 250.0f;
		MeleeHitboxComponent->DamageAmount = 60.0f;
		MeleeHitboxComponent->KnockbackForce = 2000.0f;
		MeleeHitboxComponent->AttackCooldown = 3.0f;
	}

	// Multi-attack configuration
	GroundPoundRadius = 400.0f;
	GroundPoundDamage = 60.0f;

	ChargeRange = 800.0f;
	ChargeDamage = 45.0f;

	SweepRadius = 350.0f;
	SweepDamage = 35.0f;

	GoliathKnockbackForce = 2000.0f;

	CurrentAttack = EGoliathAttackType::GroundPound;
}

bool AGoliathCharacter::ExecuteGoliathAttack()
{
	if (!HasAuthority()) return false;

	bool bResult = false;

	switch (CurrentAttack)
	{
	case EGoliathAttackType::GroundPound:
		bResult = ExecuteGroundPound();
		CurrentAttack = EGoliathAttackType::Charge;
		break;
	case EGoliathAttackType::Charge:
		bResult = ExecuteCharge();
		CurrentAttack = EGoliathAttackType::Sweep;
		break;
	case EGoliathAttackType::Sweep:
		bResult = ExecuteSweep();
		CurrentAttack = EGoliathAttackType::GroundPound;
		break;
	}

	return bResult;
}

bool AGoliathCharacter::ExecuteGroundPound()
{
	// Ground Pound: massive AoE centered on the Goliath
	FVector Center = GetActorLocation();

	// Debug: show the pound radius
	DrawDebugSphere(GetWorld(), Center, GroundPoundRadius, 16, FColor::Purple, false, 2.0f);

	UE_LOG(LogTemp, Warning, TEXT("Goliath: GROUND POUND! Radius=%.0f, Damage=%.0f"), GroundPoundRadius, GroundPoundDamage);

	return DamageActorsInSphere(Center, GroundPoundRadius, GroundPoundDamage, GoliathKnockbackForce * 1.5f);
}

bool AGoliathCharacter::ExecuteCharge()
{
	// Charge: sweep a long capsule forward
	FVector StartLoc = GetActorLocation();
	FVector ForwardDir = GetActorForwardVector();
	FVector EndLoc = StartLoc + (ForwardDir * ChargeRange);

	// Launch the Goliath forward too
	LaunchCharacter(ForwardDir * 1500.0f, true, true);

	// Debug: show the charge path
	DrawDebugLine(GetWorld(), StartLoc, EndLoc, FColor::Yellow, false, 2.0f, 0, 10.0f);
	DrawDebugSphere(GetWorld(), EndLoc, 100.0f, 12, FColor::Yellow, false, 2.0f);

	UE_LOG(LogTemp, Warning, TEXT("Goliath: CHARGE! Range=%.0f, Damage=%.0f"), ChargeRange, ChargeDamage);

	// Sphere sweep along the charge path
	FCollisionShape SweepShape = FCollisionShape::MakeSphere(120.0f);
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	TArray<FHitResult> HitResults;
	bool bHit = GetWorld()->SweepMultiByChannel(HitResults, StartLoc, EndLoc, FQuat::Identity, ECC_Pawn, SweepShape, QueryParams);

	if (!bHit) return false;

	bool bDamagedAnyone = false;
	TSet<AActor*> AlreadyHit;

	IAbilitySystemInterface* SourceASI = Cast<IAbilitySystemInterface>(this);
	if (!SourceASI) return false;
	UAbilitySystemComponent* SourceASC = SourceASI->GetAbilitySystemComponent();
	if (!SourceASC) return false;

	for (const FHitResult& Hit : HitResults)
	{
		AActor* HitActor = Hit.GetActor();
		if (!HitActor || AlreadyHit.Contains(HitActor)) continue;
		AlreadyHit.Add(HitActor);

		IAbilitySystemInterface* TargetASI = Cast<IAbilitySystemInterface>(HitActor);
		if (!TargetASI) continue;
		UAbilitySystemComponent* TargetASC = TargetASI->GetAbilitySystemComponent();
		if (!TargetASC) continue;

		float CurrentHealth = TargetASC->GetNumericAttribute(UMonolithVAttributeSet::GetHealthAttribute());
		float NewHealth = FMath::Max(0.0f, CurrentHealth - ChargeDamage);
		TargetASC->SetNumericAttributeBase(UMonolithVAttributeSet::GetHealthAttribute(), NewHealth);

		if (ACharacter* HitChar = Cast<ACharacter>(HitActor))
		{
			FVector KnockDir = ForwardDir;
			KnockDir.Z = 0.4f;
			KnockDir.Normalize();
			HitChar->LaunchCharacter(KnockDir * GoliathKnockbackForce, true, true);
		}

		bDamagedAnyone = true;
	}

	return bDamagedAnyone;
}

bool AGoliathCharacter::ExecuteSweep()
{
	// Sweep: 360-degree AoE around the Goliath (like a spin attack)
	FVector Center = GetActorLocation();

	// Debug: show the sweep ring
	DrawDebugSphere(GetWorld(), Center, SweepRadius, 16, FColor::Cyan, false, 2.0f);

	UE_LOG(LogTemp, Warning, TEXT("Goliath: SWEEP! Radius=%.0f, Damage=%.0f"), SweepRadius, SweepDamage);

	return DamageActorsInSphere(Center, SweepRadius, SweepDamage, GoliathKnockbackForce);
}

bool AGoliathCharacter::DamageActorsInSphere(FVector Center, float Radius, float Damage, float Knockback)
{
	// Find all actors within the sphere
	FCollisionShape Shape = FCollisionShape::MakeSphere(Radius);
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	TArray<FOverlapResult> Overlaps;
	bool bFound = GetWorld()->OverlapMultiByChannel(Overlaps, Center, FQuat::Identity, ECC_Pawn, Shape, QueryParams);

	if (!bFound) return false;

	bool bDamagedAnyone = false;
	TSet<AActor*> AlreadyHit;

	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* HitActor = Overlap.GetActor();
		if (!HitActor || AlreadyHit.Contains(HitActor)) continue;
		AlreadyHit.Add(HitActor);

		IAbilitySystemInterface* TargetASI = Cast<IAbilitySystemInterface>(HitActor);
		if (!TargetASI) continue;
		UAbilitySystemComponent* TargetASC = TargetASI->GetAbilitySystemComponent();
		if (!TargetASC) continue;

		float CurrentHealth = TargetASC->GetNumericAttribute(UMonolithVAttributeSet::GetHealthAttribute());
		float NewHealth = FMath::Max(0.0f, CurrentHealth - Damage);
		TargetASC->SetNumericAttributeBase(UMonolithVAttributeSet::GetHealthAttribute(), NewHealth);

		UE_LOG(LogTemp, Warning, TEXT("Goliath AoE: dealt %.0f to %s (%.0f -> %.0f)"),
			Damage, *HitActor->GetName(), CurrentHealth, NewHealth);

		if (ACharacter* HitChar = Cast<ACharacter>(HitActor))
		{
			FVector KnockDir = (HitActor->GetActorLocation() - Center).GetSafeNormal();
			KnockDir.Z = 0.3f;
			KnockDir.Normalize();
			HitChar->LaunchCharacter(KnockDir * Knockback, true, true);
		}

		bDamagedAnyone = true;
	}

	return bDamagedAnyone;
}
