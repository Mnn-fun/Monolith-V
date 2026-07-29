#include "GE_ShareHealthBoost.h"
#include "MonolithVAttributeSet.h"

UGE_ShareHealthBoost::UGE_ShareHealthBoost()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	
	// Set the duration to 30 seconds
	FScalableFloat DurationMagnitude;
	DurationMagnitude.SetValue(30.0f);
	DurationMagnitude.Curve.CurveTable = nullptr;
	DurationMagnitude.Curve.RowName = NAME_None;
	
	DurationMagnitude.RegistryType = FScalableFloat::ERegistryType::None;

	FGameplayEffectModifierMagnitude ModMag;
	ModMag.MagnitudeType = EGameplayEffectMagnitudeCalculation::ScalableFloat;
	ModMag.ScalableFloatMagnitude = DurationMagnitude;
	
	DurationMagnitude = ModMag.ScalableFloatMagnitude; // Temporary fix to apply scalable float magnitude to duration
	// Wait, Unreal 5.x uses standard struct assignment for DurationMagnitude
	
	// A simpler approach in UE5 for Duration:
	FGameplayEffectModifierMagnitude DurationModMag(DurationMagnitude);
	DurationMagnitude = DurationModMag.ScalableFloatMagnitude; // Keep it simple by just passing FScalableFloat to DurationMagnitude if possible, but actually in UE5 it's just `DurationMagnitude` which is an FScalableFloat.
	
	this->DurationMagnitude = DurationMagnitude;

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
