#include "GE_TestDamage.h"
#include "MonolithVAttributeSet.h"

UGE_TestDamage::UGE_TestDamage()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FGameplayModifierInfo ModifierInfo;
	ModifierInfo.Attribute = UMonolithVAttributeSet::GetHealthAttribute();
	ModifierInfo.ModifierOp = EGameplayModOp::Additive;
	ModifierInfo.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(-10.0f));

	Modifiers.Add(ModifierInfo);
}
