#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "GameplayEffectTypes.h"
#include "EnemyBaseCharacter.generated.h"

class UAbilitySystemComponent;
class UMonolithVAttributeSet;
class UGameplayAbility;
class UMeleeHitboxComponent;

UENUM(BlueprintType)
enum class EEnemyTier : uint8
{
	Soldier UMETA(DisplayName = "Soldier"),
	General UMETA(DisplayName = "General"),
	LevelBoss UMETA(DisplayName = "Level Boss"),
	Goliath UMETA(DisplayName = "Goliath")
};

UCLASS()
class MONOLITHV_API AEnemyBaseCharacter : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AEnemyBaseCharacter();

	// IAbilitySystemInterface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy Framework")
	EEnemyTier EnemyTier;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy Framework")
	float BaseHealth;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	UMeleeHitboxComponent* MeleeHitboxComponent;

protected:
	virtual void BeginPlay() override;
	virtual void PossessedBy(AController* NewController) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities", meta = (AllowPrivateAccess = "true"))
	UAbilitySystemComponent* AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities", meta = (AllowPrivateAccess = "true"))
	UMonolithVAttributeSet* AttributeSet;

	// Abilities to grant on spawn
	UPROPERTY(EditDefaultsOnly, Category = "Abilities")
	TArray<TSubclassOf<UGameplayAbility>> DefaultAbilities;

	// Death handling
	virtual void OnHealthChanged(const FOnAttributeChangeData& Data);
	virtual void Die();

	bool bIsDead;
};
