#include "EnemyBaseCharacter.h"
#include "EnemyAIController.h"
#include "AbilitySystemComponent.h"
#include "../Combat/MonolithVAttributeSet.h"
#include "../Combat/MeleeHitboxComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "../Player/MonolithVCharacter.h"

AEnemyBaseCharacter::AEnemyBaseCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	AIControllerClass = AEnemyAIController::StaticClass();

	// Enable server-authoritative movement for enemies too
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->bOrientRotationToMovement = true;
		GetCharacterMovement()->NetworkSmoothingMode = ENetworkSmoothingMode::Exponential;
		GetCharacterMovement()->bUseRVOAvoidance = true;
		GetCharacterMovement()->AvoidanceConsiderationRadius = 150.0f;
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

void AEnemyBaseCharacter::InitializeForBand(int32 BandIndex)
{
	SpawnedBandIndex = BandIndex;

	// Band 2+ enemies can fly to chase the player through the air
	if (BandIndex >= 2 && GetCharacterMovement())
	{
		GetCharacterMovement()->SetMovementMode(MOVE_Flying);
		GetCharacterMovement()->MaxFlySpeed = 600.0f;
		GetCharacterMovement()->BrakingDecelerationFlying = 1500.0f;
		GetCharacterMovement()->GravityScale = 0.0f;
		bCanFly = true;
		UE_LOG(LogTemp, Warning, TEXT("[Enemy] %s initialized as FLYING enemy for Band %d"), *GetName(), BandIndex);
	}
}

void AEnemyBaseCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!HasAuthority() || bIsDead) return;

	// Fallback direct chase: find nearest player and walk toward them
	// This ensures enemies work even without NavMesh on upper bands
	APawn* ClosestPlayer = nullptr;
	float ClosestDist = MAX_flt;

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (PC && PC->GetPawn())
		{
			float Dist = FVector::Dist(GetActorLocation(), PC->GetPawn()->GetActorLocation());
			if (Dist < ClosestDist)
			{
				ClosestDist = Dist;
				ClosestPlayer = PC->GetPawn();
			}
		}
	}

	if (ClosestPlayer && ClosestDist < 5000.0f)
	{
		FVector Direction;
		if (bCanFly)
		{
			// Flying enemies: chase in full 3D space
			Direction = (ClosestPlayer->GetActorLocation() - GetActorLocation()).GetSafeNormal();
		}
		else
		{
			// Ground enemies: chase on the horizontal plane only
			Direction = (ClosestPlayer->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
		}

		if (!Direction.IsNearlyZero())
		{
			AddMovementInput(Direction, 1.0f);
		}

		// Attack if close enough
		if (ClosestDist < 250.0f && MeleeHitboxComponent)
		{
			MeleeHitboxComponent->ExecuteAttack();
		}
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
