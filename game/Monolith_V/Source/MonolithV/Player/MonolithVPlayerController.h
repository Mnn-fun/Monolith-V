#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MonolithVPlayerController.generated.h"

UCLASS()
class MONOLITHV_API AMonolithVPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AMonolithVPlayerController();

	// Console command to test Health replication via OnRep_Health (Usage: TestDamage 10)
	UFUNCTION(Exec)
	void TestDamage(float DamageAmount = 10.0f);

	UFUNCTION(Server, Reliable)
	void ServerTestDamage(float DamageAmount);
};
