#include "GE_FuelRegen.h"
#include "MonolithVAttributeSet.h"

UGE_FuelRegen::UGE_FuelRegen()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;
	
	// Periodic execution every 0.1s
	Period = 0.1f;

	// Require NOT having State.Jetpacking tag to regenerate
	FGameplayTag JetpackTag = FGameplayTag::RequestGameplayTag(TEXT("State.Jetpacking"));
	FGameplayTagRequirements OngoingTagReqs;
	OngoingTagReqs.IgnoreTags.AddTag(JetpackTag);
	OngoingTagRequirements = OngoingTagReqs;

	FGameplayModifierInfo ModInfo;
	ModInfo.Attribute = UMonolithVAttributeSet::GetFuelAttribute();
	ModInfo.ModifierOp = EGameplayModOp::Additive;
	// Scaled down, so we regenerate 2.0 unit per tick
	ModInfo.ModifierMagnitude = FScalableFloat(2.0f);
	
	Modifiers.Add(ModInfo);
}
