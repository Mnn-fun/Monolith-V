#include "GE_RespawnRestore.h"
#include "MonolithVAttributeSet.h"

UGE_RespawnRestore::UGE_RespawnRestore()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	// Restore Health
	FGameplayModifierInfo HealthModifier;
	HealthModifier.Attribute = UMonolithVAttributeSet::GetHealthAttribute();
	HealthModifier.ModifierOp = EGameplayModOp::Additive;
	HealthModifier.ModifierMagnitude = FScalableFloat(10000.0f); // Will be clamped to MaxHealth by AttributeSet

	// Restore Fuel
	FGameplayModifierInfo FuelModifier;
	FuelModifier.Attribute = UMonolithVAttributeSet::GetFuelAttribute();
	FuelModifier.ModifierOp = EGameplayModOp::Additive;
	FuelModifier.ModifierMagnitude = FScalableFloat(10000.0f); // Will be clamped to MaxFuel by AttributeSet

	Modifiers.Add(HealthModifier);
	Modifiers.Add(FuelModifier);
}
