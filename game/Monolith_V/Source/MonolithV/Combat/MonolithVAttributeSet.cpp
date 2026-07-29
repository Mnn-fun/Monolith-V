#include "MonolithVAttributeSet.h"
#include "GameplayEffect.h"
#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"

UMonolithVAttributeSet::UMonolithVAttributeSet()
	: Health(100.0f)
	, MaxHealth(100.0f)
	, Fuel(100.0f)
	, MaxFuel(100.0f)
{
}

void UMonolithVAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UMonolithVAttributeSet, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMonolithVAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMonolithVAttributeSet, Fuel, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMonolithVAttributeSet, MaxFuel, COND_None, REPNOTIFY_Always);
}

void UMonolithVAttributeSet::OnRep_Health(const FGameplayAttributeData& OldHealth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMonolithVAttributeSet, Health, OldHealth);
}

void UMonolithVAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMonolithVAttributeSet, MaxHealth, OldMaxHealth);
}

void UMonolithVAttributeSet::OnRep_Fuel(const FGameplayAttributeData& OldFuel)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMonolithVAttributeSet, Fuel, OldFuel);
}

void UMonolithVAttributeSet::OnRep_MaxFuel(const FGameplayAttributeData& OldMaxFuel)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMonolithVAttributeSet, MaxFuel, OldMaxFuel);
}

void UMonolithVAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxHealth());
	}
	else if (Attribute == GetFuelAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxFuel());
	}
}

void UMonolithVAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		SetHealth(FMath::Clamp(GetHealth(), 0.0f, GetMaxHealth()));
		UE_LOG(LogTemp, Warning, TEXT("UMonolithVAttributeSet::PostGameplayEffectExecute - Health changed to: %f on %s"), GetHealth(), *GetOuter()->GetName());
	}
	else if (Data.EvaluatedData.Attribute == GetFuelAttribute())
	{
		SetFuel(FMath::Clamp(GetFuel(), 0.0f, GetMaxFuel()));
	}
}
