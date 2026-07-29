#include "GA_FireWeapon.h"
#include "WeaponComponent.h"
#include "GE_WeaponDamage.h"
#include "GE_WeaponCooldown.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Character.h"
#include "Camera/CameraComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "AbilitySystemInterface.h"

UGA_FireWeapon::UGA_FireWeapon()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	// Assign the cooldown effect class
	CooldownGameplayEffectClass = UGE_WeaponCooldown::StaticClass();
}

bool UGA_FireWeapon::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	if (AActor* Avatar = ActorInfo->AvatarActor.Get())
	{
		if (UWeaponComponent* WeaponComp = Avatar->FindComponentByClass<UWeaponComponent>())
		{
			return WeaponComp->CanFire();
		}
	}
	return false;
}

void UGA_FireWeapon::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	AActor* Avatar = ActorInfo->AvatarActor.Get();
	if (!Avatar)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UWeaponComponent* WeaponComp = Avatar->FindComponentByClass<UWeaponComponent>();
	if (!WeaponComp)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// Consume Ammo
	if (HasAuthorityOrPredictionKey(ActorInfo, &ActivationInfo))
	{
		WeaponComp->ConsumeAmmo();
	}

	// We only want the Server to deal damage, but we can do the trace on both for VFX.
	// Actually, doing the trace on Server is required for authority. Doing it on Client is for VFX/prediction.
	// For simplicity, we'll trace on both, but only apply damage on Authority.

	FVector StartLoc;
	FRotator CameraRot;

	if (ACharacter* Character = Cast<ACharacter>(Avatar))
	{
		// Get camera location for trace start
		if (UCameraComponent* Camera = Character->FindComponentByClass<UCameraComponent>())
		{
			StartLoc = Camera->GetComponentLocation();
			CameraRot = Camera->GetComponentRotation();
		}
		else
		{
			Character->GetActorEyesViewPoint(StartLoc, CameraRot);
		}
	}
	else
	{
		Avatar->GetActorEyesViewPoint(StartLoc, CameraRot);
	}

	FVector EndLoc = StartLoc + (CameraRot.Vector() * WeaponComp->Range);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(Avatar);

	FHitResult HitResult;
	bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, StartLoc, EndLoc, ECC_Visibility, QueryParams);

	if (bHit)
	{
		EndLoc = HitResult.ImpactPoint;

		if (HasAuthority(&ActivationInfo))
		{
			// Apply damage if target has Ability System Component
			if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(HitResult.GetActor()))
			{
				UAbilitySystemComponent* TargetASC = ASI->GetAbilitySystemComponent();
				if (TargetASC)
				{
					const UGameplayEffect* DamageEffectCDO = UGE_WeaponDamage::StaticClass()->GetDefaultObject<UGE_WeaponDamage>();
					FGameplayEffectContextHandle Context = ActorInfo->AbilitySystemComponent->MakeEffectContext();
					Context.AddHitResult(HitResult);
					
					FGameplayEffectSpecHandle SpecHandle = ActorInfo->AbilitySystemComponent->MakeOutgoingSpec(UGE_WeaponDamage::StaticClass(), 1.0f, Context);
					if (SpecHandle.IsValid())
					{
						SpecHandle.Data.Get()->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(TEXT("Data.Damage")), WeaponComp->Damage);
						// But since we use static float in UGE_WeaponDamage, we don't strictly need SetByCaller unless configured.
						// We'll just apply it directly.
						TargetASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
					}
				}
			}
		}
	}

	// Debug Line for VFX stub (drawn on both client and server)
	DrawDebugLine(GetWorld(), StartLoc, EndLoc, FColor::Red, false, 1.0f, 0, 2.0f);

	// End ability immediately as this is an instant fire
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
