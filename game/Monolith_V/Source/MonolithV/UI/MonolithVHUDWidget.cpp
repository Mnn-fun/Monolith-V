#include "MonolithVHUDWidget.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Combat/MonolithVAttributeSet.h"
#include "Player/MonolithVCharacter.h"

void UMonolithVHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// If we are spawned by the HUD, BindToPlayer will be called soon.
	// But just in case, try to bind to the owning player pawn now.
	if (APawn* OwningPawn = GetOwningPlayerPawn())
	{
		BindToPlayer(OwningPawn);
	}
}

void UMonolithVHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// Update compass angle from C++ - no Blueprint cast needed
	if (AMonolithVCharacter* Character = Cast<AMonolithVCharacter>(GetOwningPlayerPawn()))
	{
		CompassAngle = Character->GetDirectionToMonolith();
	}
}

void UMonolithVHUDWidget::NativeDestruct()
{
	if (BoundASC)
	{
		BoundASC->GetGameplayAttributeValueChangeDelegate(UMonolithVAttributeSet::GetHealthAttribute()).RemoveAll(this);
		BoundASC->GetGameplayAttributeValueChangeDelegate(UMonolithVAttributeSet::GetMaxHealthAttribute()).RemoveAll(this);
		BoundASC->GetGameplayAttributeValueChangeDelegate(UMonolithVAttributeSet::GetFuelAttribute()).RemoveAll(this);
		BoundASC->GetGameplayAttributeValueChangeDelegate(UMonolithVAttributeSet::GetMaxFuelAttribute()).RemoveAll(this);
	}

	Super::NativeDestruct();
}

void UMonolithVHUDWidget::BindToPlayer(APawn* PlayerPawn)
{
	if (!PlayerPawn) return;

	UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(PlayerPawn);
	if (ASC == BoundASC) return; // Already bound

	// Cleanup old bindings if we are rebinding
	if (BoundASC)
	{
		BoundASC->GetGameplayAttributeValueChangeDelegate(UMonolithVAttributeSet::GetHealthAttribute()).RemoveAll(this);
		BoundASC->GetGameplayAttributeValueChangeDelegate(UMonolithVAttributeSet::GetMaxHealthAttribute()).RemoveAll(this);
		BoundASC->GetGameplayAttributeValueChangeDelegate(UMonolithVAttributeSet::GetFuelAttribute()).RemoveAll(this);
		BoundASC->GetGameplayAttributeValueChangeDelegate(UMonolithVAttributeSet::GetMaxFuelAttribute()).RemoveAll(this);
	}

	BoundASC = ASC;

	if (BoundASC)
	{
		// Bind delegates
		BoundASC->GetGameplayAttributeValueChangeDelegate(UMonolithVAttributeSet::GetHealthAttribute()).AddUObject(this, &UMonolithVHUDWidget::HealthChanged);
		BoundASC->GetGameplayAttributeValueChangeDelegate(UMonolithVAttributeSet::GetMaxHealthAttribute()).AddUObject(this, &UMonolithVHUDWidget::MaxHealthChanged);
		BoundASC->GetGameplayAttributeValueChangeDelegate(UMonolithVAttributeSet::GetFuelAttribute()).AddUObject(this, &UMonolithVHUDWidget::FuelChanged);
		BoundASC->GetGameplayAttributeValueChangeDelegate(UMonolithVAttributeSet::GetMaxFuelAttribute()).AddUObject(this, &UMonolithVHUDWidget::MaxFuelChanged);

		// Initialize starting values
		bool bFound = false;
		Health = BoundASC->GetGameplayAttributeValue(UMonolithVAttributeSet::GetHealthAttribute(), bFound);
		MaxHealth = BoundASC->GetGameplayAttributeValue(UMonolithVAttributeSet::GetMaxHealthAttribute(), bFound);
		Fuel = BoundASC->GetGameplayAttributeValue(UMonolithVAttributeSet::GetFuelAttribute(), bFound);
		MaxFuel = BoundASC->GetGameplayAttributeValue(UMonolithVAttributeSet::GetMaxFuelAttribute(), bFound);

		// Trigger visual updates immediately
		OnHealthChanged();
		OnFuelChanged();
	}
}

void UMonolithVHUDWidget::HealthChanged(const FOnAttributeChangeData& Data)
{
	Health = Data.NewValue;
	OnHealthChanged();
}

void UMonolithVHUDWidget::MaxHealthChanged(const FOnAttributeChangeData& Data)
{
	MaxHealth = Data.NewValue;
	OnHealthChanged();
}

void UMonolithVHUDWidget::FuelChanged(const FOnAttributeChangeData& Data)
{
	Fuel = Data.NewValue;
	OnFuelChanged();
}

void UMonolithVHUDWidget::MaxFuelChanged(const FOnAttributeChangeData& Data)
{
	MaxFuel = Data.NewValue;
	OnFuelChanged();
}
