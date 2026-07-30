#include "GA_DashAttack.h"
#include "GE_DashCost.h"
#include "GE_DashCooldown.h"
#include "GE_WeaponDamage.h"
#include "AbilitySystemComponent.h"
#include "MonolithVAttributeSet.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "AbilitySystemInterface.h"
#include "DrawDebugHelpers.h"

UGA_DashAttack::UGA_DashAttack()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	CostGameplayEffectClass = UGE_DashCost::StaticClass();
	CooldownGameplayEffectClass = UGE_DashCooldown::StaticClass();
}

bool UGA_DashAttack::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	// Double check we have enough fuel
	if (ActorInfo->AbilitySystemComponent.IsValid())
	{
		bool bFound = false;
		float CurrentFuel = ActorInfo->AbilitySystemComponent->GetGameplayAttributeValue(UMonolithVAttributeSet::GetFuelAttribute(), bFound);
		if (bFound && CurrentFuel < 25.0f) // Hardcoded matching the GE_DashCost
		{
			return false;
		}
	}

	return true;
}

void UGA_DashAttack::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ACharacter* Character = Cast<ACharacter>(ActorInfo->AvatarActor.Get());
	if (!Character)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 1. Launch the character forward based on camera aim direction
	FVector ForwardDir = Character->GetBaseAimRotation().Vector();
	FVector LaunchVelocity = ForwardDir * 2000.0f; // Dash speed
	
	// Add a little bit of upward velocity if aiming horizontally, but let them dash down if they want
	if (ForwardDir.Z > -0.2f && ForwardDir.Z < 0.2f)
	{
		LaunchVelocity.Z += 200.0f;
	}
	
	Character->LaunchCharacter(LaunchVelocity, true, true);

	// 2. Perform a capsule sweep on Authority to deal damage
	if (HasAuthority(&ActivationInfo))
	{
		FVector StartLoc = Character->GetActorLocation();
		FVector EndLoc = StartLoc + (ForwardDir * 1000.0f); // Sweep 1000 units ahead

		UCapsuleComponent* Capsule = Character->GetCapsuleComponent();
		float Radius = Capsule ? Capsule->GetScaledCapsuleRadius() * 1.5f : 50.0f;
		float HalfHeight = Capsule ? Capsule->GetScaledCapsuleHalfHeight() : 90.0f;

		FCollisionShape SweepShape = FCollisionShape::MakeCapsule(Radius, HalfHeight);

		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(Character); // Don't hit ourselves

		TArray<FHitResult> HitResults;
		bool bHit = GetWorld()->SweepMultiByChannel(HitResults, StartLoc, EndLoc, FQuat::Identity, ECC_Pawn, SweepShape, QueryParams);

		// Debug visualization (drawn on server)
		DrawDebugCapsule(GetWorld(), EndLoc, HalfHeight, Radius, FQuat::Identity, FColor::Orange, false, 2.0f);

		if (bHit)
		{
			UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
			const UGameplayEffect* DamageEffectCDO = UGE_WeaponDamage::StaticClass()->GetDefaultObject<UGE_WeaponDamage>();

			for (const FHitResult& Hit : HitResults)
			{
				if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(Hit.GetActor()))
				{
					UAbilitySystemComponent* TargetASC = ASI->GetAbilitySystemComponent();
					if (TargetASC)
					{
						FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
						Context.AddHitResult(Hit);

						// Apply the same 20 damage as the weapon
						FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(UGE_WeaponDamage::StaticClass(), 1.0f, Context);
						if (SpecHandle.IsValid())
						{
							TargetASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
						}
					}
				}
			}
		}
	}

	// End ability immediately (instant activation, the physics launch carries the momentum)
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
