#include "GE_WeaponCooldown.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

UGE_WeaponCooldown::UGE_WeaponCooldown()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FScalableFloat(0.5f);

	FGameplayTag CooldownTag = FGameplayTag::RequestGameplayTag(TEXT("Ability.Cooldown.Weapon"));
	
	UTargetTagsGameplayEffectComponent& TargetTagsComponent = FindOrAddComponent<UTargetTagsGameplayEffectComponent>();
	FInheritedTagContainer TagContainer;
	TagContainer.AddTag(CooldownTag);
	TargetTagsComponent.SetAndApplyTargetTagChanges(TagContainer);
}
