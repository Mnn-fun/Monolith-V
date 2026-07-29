#include "GA_Jetpack.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "MonolithVAttributeSet.h"
#include "GE_FuelDrain.h"

UGA_Jetpack::UGA_Jetpack()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	
	FGameplayTag JetpackTag = FGameplayTag::RequestGameplayTag(TEXT("State.Jetpacking"));
	ActivationOwnedTags.AddTag(JetpackTag);
}

void UGA_Jetpack::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (HasAuthorityOrPredictionKey(ActorInfo, &ActivationInfo))
	{
		CommitAbility(Handle, ActorInfo, ActivationInfo);

		// Start ticking the jetpack every 0.05 seconds
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(JetpackTimerHandle, this, &UGA_Jetpack::OnJetpackTick, 0.05f, true);
		}
	}
}

void UGA_Jetpack::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(JetpackTimerHandle);
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_Jetpack::InputReleased(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	if (ActorInfo && ActorInfo->AvatarActor != nullptr)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
	}
}

void UGA_Jetpack::OnJetpackTick()
{
	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
	if (!ActorInfo || !ActorInfo->AvatarActor.IsValid() || !ActorInfo->AbilitySystemComponent.IsValid())
	{
		EndAbility(CurrentSpecHandle, ActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	
	// Check Fuel
	bool bFound = false;
	float CurrentFuel = ASC->GetGameplayAttributeValue(UMonolithVAttributeSet::GetFuelAttribute(), bFound);
	
	if (!bFound || CurrentFuel <= 0.0f)
	{
		// Out of fuel, end ability
		EndAbility(CurrentSpecHandle, ActorInfo, CurrentActivationInfo, true, false);
		return;
	}

	// Drain Fuel
	if (HasAuthorityOrPredictionKey(ActorInfo, &CurrentActivationInfo))
	{
		const UGameplayEffect* DrainEffectCDO = UGE_FuelDrain::StaticClass()->GetDefaultObject<UGE_FuelDrain>();
		ASC->ApplyGameplayEffectToSelf(DrainEffectCDO, 1.0f, ASC->MakeEffectContext());
	}

	// Apply Thrust
	if (ACharacter* Character = Cast<ACharacter>(ActorInfo->AvatarActor.Get()))
	{
		// Constant upward velocity of 500 units/sec while held
		Character->LaunchCharacter(FVector(0.0f, 0.0f, 500.0f), false, true);
	}
}
