#include "MeleeHitboxComponent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "MonolithVAttributeSet.h"
#include "GameFramework/Character.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"

UMeleeHitboxComponent::UMeleeHitboxComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	// Sensible defaults (Soldier-grade)
	AttackRange = 200.0f;
	AttackRadius = 75.0f;
	DamageAmount = 10.0f;
	KnockbackForce = 800.0f;
	AttackCooldown = 1.5f;
	LastAttackTime = -999.0f;
}

bool UMeleeHitboxComponent::CanAttack() const
{
	return (GetWorld()->GetTimeSeconds() - LastAttackTime) >= AttackCooldown;
}

bool UMeleeHitboxComponent::ExecuteAttack()
{
	if (!CanAttack()) return false;

	AActor* Owner = GetOwner();
	if (!Owner) return false;

	ACharacter* OwnerCharacter = Cast<ACharacter>(Owner);
	if (!OwnerCharacter) return false;

	// Only the server should deal damage
	if (!OwnerCharacter->HasAuthority()) return false;

	LastAttackTime = GetWorld()->GetTimeSeconds();

	// Get the source ASC for applying effects
	IAbilitySystemInterface* SourceASI = Cast<IAbilitySystemInterface>(Owner);
	if (!SourceASI) return false;

	UAbilitySystemComponent* SourceASC = SourceASI->GetAbilitySystemComponent();
	if (!SourceASC) return false;

	// Perform sphere sweep from the enemy's position along its forward vector
	FVector StartLoc = Owner->GetActorLocation();
	FVector ForwardDir = Owner->GetActorForwardVector();
	FVector EndLoc = StartLoc + (ForwardDir * AttackRange);

	FCollisionShape SweepShape = FCollisionShape::MakeSphere(AttackRadius);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(Owner);

	TArray<FHitResult> HitResults;
	bool bHit = GetWorld()->SweepMultiByChannel(
		HitResults, StartLoc, EndLoc, FQuat::Identity,
		ECC_Pawn, SweepShape, QueryParams
	);

	// Debug visualization — draw the sweep sphere so you can see the hitbox
	DrawDebugSphere(GetWorld(), StartLoc + (ForwardDir * AttackRange * 0.5f), AttackRadius, 12, FColor::Red, false, 1.0f);
	DrawDebugLine(GetWorld(), StartLoc, EndLoc, FColor::Red, false, 1.0f);

	if (!bHit) return false;

	bool bDamagedAnyone = false;

	// Track which actors we already hit to avoid double-damage from multi-hits
	TSet<AActor*> AlreadyHit;

	for (const FHitResult& Hit : HitResults)
	{
		AActor* HitActor = Hit.GetActor();
		if (!HitActor || AlreadyHit.Contains(HitActor)) continue;
		AlreadyHit.Add(HitActor);

		IAbilitySystemInterface* TargetASI = Cast<IAbilitySystemInterface>(HitActor);
		if (!TargetASI) continue;

		UAbilitySystemComponent* TargetASC = TargetASI->GetAbilitySystemComponent();
		if (!TargetASC) continue;

		// Apply damage: read current health, subtract DamageAmount, clamp to 0, write back
		float CurrentHealth = TargetASC->GetNumericAttribute(UMonolithVAttributeSet::GetHealthAttribute());
		float NewHealth = FMath::Max(0.0f, CurrentHealth - DamageAmount);
		TargetASC->SetNumericAttributeBase(UMonolithVAttributeSet::GetHealthAttribute(), NewHealth);

		// Log the damage for debugging
		UE_LOG(LogTemp, Warning, TEXT("MeleeHitbox: %s dealt %.0f damage to %s (Health: %.0f -> %.0f)"),
			*Owner->GetName(), DamageAmount, *HitActor->GetName(), CurrentHealth, NewHealth);

		// Apply knockback to the hit character
		if (ACharacter* HitCharacter = Cast<ACharacter>(HitActor))
		{
			FVector KnockbackDir = (HitActor->GetActorLocation() - Owner->GetActorLocation()).GetSafeNormal();
			KnockbackDir.Z = 0.3f; // Add slight upward launch
			KnockbackDir.Normalize();
			HitCharacter->LaunchCharacter(KnockbackDir * KnockbackForce, true, true);
		}

		bDamagedAnyone = true;

		// Debug: flash green on the hit actor
		DrawDebugSphere(GetWorld(), HitActor->GetActorLocation(), 50.0f, 8, FColor::Green, false, 1.0f);
	}

	return bDamagedAnyone;
}
