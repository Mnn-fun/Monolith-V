#include "MonolithVPlayerController.h"
#include "MonolithVCharacter.h"

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
		MyChar->Health -= DamageAmount;
		UE_LOG(LogTemp, Display, TEXT("AMonolithVPlayerController: Server decreased Health by %f. New Health: %f"), DamageAmount, MyChar->Health);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("AMonolithVPlayerController: TestDamage called but GetPawn() is not AMonolithVCharacter"));
	}
}
