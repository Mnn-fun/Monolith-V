#include "MonolithVPlayerController.h"
#include "MonolithVCharacter.h"
#include "../Combat/MonolithVAttributeSet.h"

AMonolithVPlayerController::AMonolithVPlayerController()
{
}

void AMonolithVPlayerController::TestDamage(float DamageAmount)
{
	if (HasAuthority())
	{
		ServerTestDamage_Implementation(DamageAmount);
	}
	else
	{
		ServerTestDamage(DamageAmount);
	}
}

void AMonolithVPlayerController::ServerTestDamage_Implementation(float DamageAmount)
{
	if (AMonolithVCharacter* MyChar = Cast<AMonolithVCharacter>(GetPawn()))
	{
		if (MyChar->AttributeSet)
		{
			float NewHealth = FMath::Clamp(MyChar->AttributeSet->GetHealth() - DamageAmount, 0.0f, MyChar->AttributeSet->GetMaxHealth());
			MyChar->AttributeSet->SetHealth(NewHealth);
			UE_LOG(LogTemp, Display, TEXT("AMonolithVPlayerController: Server decreased Health by %f. New Health: %f"), DamageAmount, NewHealth);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("AMonolithVPlayerController: TestDamage called but AttributeSet is null on character %s"), *MyChar->GetName());
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("AMonolithVPlayerController: TestDamage called but GetPawn() is not AMonolithVCharacter"));
	}
}
