#include "GE_FuelDrain.h"
#include "MonolithVAttributeSet.h"

UGE_FuelDrain::UGE_FuelDrain() {
  DurationPolicy = EGameplayEffectDurationType::Instant;

  FGameplayModifierInfo ModInfo;
  ModInfo.Attribute = UMonolithVAttributeSet::GetFuelAttribute();
  ModInfo.ModifierOp = EGameplayModOp::Additive;
  // Scaled up to overcome regen easily: drains 5.0 unit per tick
  ModInfo.ModifierMagnitude = FScalableFloat(-2.0f);

  Modifiers.Add(ModInfo);
}
