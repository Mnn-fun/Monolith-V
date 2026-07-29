#include "GE_ShareHealthBoost.h"
#include "MonolithVAttributeSet.h"

UGE_ShareHealthBoost::UGE_ShareHealthBoost()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	
	// Set the duration to 30 seconds
	FScalableFloat DurationScalableFloat;
	DurationScalableFloat.SetValue(30.0f);
	
	this->DurationMagnitude = FGameplayEffectModifierMagnitude(DurationScalableFloat);
	// Add the MaxHealth Modifier
	FGameplayModifierInfo ModInfo;
	ModInfo.Attribute = UMonolithVAttributeSet::GetMaxHealthAttribute();
	ModInfo.ModifierOp = EGameplayModOp::Additive;
	
	FScalableFloat ModValue;
	ModValue.SetValue(50.0f);
	
	FGameplayEffectModifierMagnitude ModValueMag(ModValue);
	ModInfo.ModifierMagnitude = ModValueMag;
	
	Modifiers.Add(ModInfo);
}
