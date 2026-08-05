#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayEffectTypes.h"
#include "MonolithVHUDWidget.generated.h"

class UAbilitySystemComponent;
class AMonolithVCharacter;

/**
 * C++ base class for the Monolith-V HUD.
 * Handles event-driven updates from the Gameplay Ability System.
 */
UCLASS()
class MONOLITHV_API UMonolithVHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// Setup the widget and bind delegates
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void BindToPlayer(APawn* PlayerPawn);

	protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// -- Blueprint accessible properties for the UI to read --

	UPROPERTY(BlueprintReadOnly, Category = "HUD|Stats")
	float Health = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "HUD|Stats")
	float MaxHealth = 100.f;

	UPROPERTY(BlueprintReadOnly, Category = "HUD|Stats")
	float Fuel = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "HUD|Stats")
	float MaxFuel = 100.f;

	UPROPERTY(BlueprintReadOnly, Category = "HUD|Stats")
	float CompassAngle = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "HUD|Role")
	bool bItemAvailable = false;

	// -- Blueprint Events for Custom Animations/FX --

	UFUNCTION(BlueprintImplementableEvent, Category = "HUD")
	void OnHealthChanged();

	UFUNCTION(BlueprintImplementableEvent, Category = "HUD")
	void OnFuelChanged();

private:
	// Handlers for GAS attribute changes
	void HealthChanged(const FOnAttributeChangeData& Data);
	void MaxHealthChanged(const FOnAttributeChangeData& Data);
	void FuelChanged(const FOnAttributeChangeData& Data);
	void MaxFuelChanged(const FOnAttributeChangeData& Data);

	UPROPERTY()
	UAbilitySystemComponent* BoundASC;
};
