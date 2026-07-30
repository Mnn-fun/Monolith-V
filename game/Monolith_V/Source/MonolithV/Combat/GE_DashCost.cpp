#include "GE_DashCost.h"
#include "MonolithVAttributeSet.h"

UGE_DashCost::UGE_DashCost()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FGameplayModifierInfo ModInfo;
	ModInfo.Attribute = UMonolithVAttributeSet::GetFuelAttribute();
	ModInfo.ModifierOp = EGameplayModOp::Additive;
	ModInfo.ModifierMagnitude = FScalableFloat(-25.0f); // Dash costs 25 Fuel
	
	Modifiers.Add(ModInfo);
}
