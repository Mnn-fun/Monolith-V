#include "EnemyBaseCharacter.h"
#include "AbilitySystemComponent.h"
#include "../Combat/MonolithVAttributeSet.h"
#include "../Combat/MeleeHitboxComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

AEnemyBaseCharacter::AEnemyBaseCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	// Enable server-authoritative movement for enemies too
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->bOrientRotationToMovement = true;
		GetCharacterMovement()->NetworkSmoothingMode = ENetworkSmoothingMode::Exponential;
	}

	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	AttributeSet = CreateDefaultSubobject<UMonolithVAttributeSet>(TEXT("AttributeSet"));

	// Create melee hitbox with base defaults (subclasses override these)
	MeleeHitboxComponent = CreateDefaultSubobject<UMeleeHitboxComponent>(TEXT("MeleeHitboxComponent"));

	EnemyTier = EEnemyTier::Soldier;
	BaseHealth = 100.0f;
	bIsDead = false;
}

UAbilitySystemComponent* AEnemyBaseCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void AEnemyBaseCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (AbilitySystemComponent)
	{
		// Listen for health changes to handle death
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AttributeSet->GetHealthAttribute()).AddUObject(this, &AEnemyBaseCharacter::OnHealthChanged);
	}
}

void AEnemyBaseCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);

		// Set base health
		AbilitySystemComponent->SetNumericAttributeBase(UMonolithVAttributeSet::GetMaxHealthAttribute(), BaseHealth);
		AbilitySystemComponent->SetNumericAttributeBase(UMonolithVAttributeSet::GetHealthAttribute(), BaseHealth);

		// Grant default abilities
		if (HasAuthority())
		{
			for (TSubclassOf<UGameplayAbility>& AbilityClass : DefaultAbilities)
			{
				if (AbilityClass)
				{
					AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(AbilityClass, 1, 0));
				}
			}
		}
	}
}

void AEnemyBaseCharacter::OnHealthChanged(const FOnAttributeChangeData& Data)
{
	if (bIsDead) return;

	if (Data.NewValue <= 0.0f)
	{
		Die();
	}
}

void AEnemyBaseCharacter::Die()
{
	bIsDead = true;

	// Disable collision and movement
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetCharacterMovement()->DisableMovement();

	// Temporary: Destroy after 3 seconds on death
	SetLifeSpan(3.0f);

	// Hide the mesh immediately or play a death animation if we had one
	if (GetMesh())
	{
		GetMesh()->SetVisibility(false, true);
	}
}
