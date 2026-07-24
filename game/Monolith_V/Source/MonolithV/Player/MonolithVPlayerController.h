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

protected:
	virtual void BeginPlay() override;

public:
	// P2.10: Console command to pick role "MALE" or "FEMALE"
	UFUNCTION(Exec)
	void DebugSelectRole(FString ChosenRole);

	// Server-side RPC to submit the role choice to backend
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerSubmitRoleChoice(const FString& ChosenRole);

private:
	bool bHasRoleAssigned = false;
};
