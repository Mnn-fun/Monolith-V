#include "GE_DashCooldown.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

UGE_DashCooldown::UGE_DashCooldown()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FScalableFloat(2.0f); // 2 second cooldown

	FGameplayTag CooldownTag = FGameplayTag::RequestGameplayTag(TEXT("Ability.Cooldown.Dash"));
	
	UTargetTagsGameplayEffectComponent& TargetTagsComponent = FindOrAddComponent<UTargetTagsGameplayEffectComponent>();
	FInheritedTagContainer TagContainer;
	TagContainer.AddTag(CooldownTag);
	TargetTagsComponent.SetAndApplyTargetTagChanges(TagContainer);
}
