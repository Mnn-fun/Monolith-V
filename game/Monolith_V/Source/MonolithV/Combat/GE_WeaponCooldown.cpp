#include "GE_WeaponCooldown.h"

UGE_WeaponCooldown::UGE_WeaponCooldown()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;

	// This is overridden dynamically, but we'll put a default fallback
	DurationMagnitude = FScalableFloat(0.5f);

	FGameplayTag CooldownTag = FGameplayTag::RequestGameplayTag(TEXT("Ability.Cooldown.Weapon"));
	
	FGameplayTagContainer TagContainer;
	TagContainer.AddTag(CooldownTag);
	
	InheritableOwnedTagsContainer.AddTag(CooldownTag);
}
