#include "GE_WeaponDamage.h"
#include "MonolithVAttributeSet.h"

UGE_WeaponDamage::UGE_WeaponDamage()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FGameplayModifierInfo ModInfo;
	ModInfo.Attribute = UMonolithVAttributeSet::GetHealthAttribute();
	ModInfo.ModifierOp = EGameplayModOp::Additive;
	
	// We will override this magnitude in GA_FireWeapon with SetSetByCallerMagnitude,
	// but we can set a fallback default here.
	ModInfo.ModifierMagnitude = FScalableFloat(-20.0f);
	
	Modifiers.Add(ModInfo);
}
