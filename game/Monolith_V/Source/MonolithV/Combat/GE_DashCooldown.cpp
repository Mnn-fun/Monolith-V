#include "GE_DashCooldown.h"

UGE_DashCooldown::UGE_DashCooldown()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FScalableFloat(2.0f); // 2 second cooldown

	FGameplayTag CooldownTag = FGameplayTag::RequestGameplayTag(TEXT("Ability.Cooldown.Dash"));
	
	FGameplayTagContainer TagContainer;
	TagContainer.AddTag(CooldownTag);
	
	InheritableOwnedTagsContainer.AddTag(CooldownTag);
}
