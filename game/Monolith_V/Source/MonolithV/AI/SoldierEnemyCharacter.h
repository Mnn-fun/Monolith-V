#pragma once

#include "CoreMinimal.h"
#include "EnemyBaseCharacter.h"
#include "SoldierEnemyCharacter.generated.h"

UCLASS()
class MONOLITHV_API ASoldierEnemyCharacter : public AEnemyBaseCharacter
{
	GENERATED_BODY()

public:
	ASoldierEnemyCharacter();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	TWeakObjectPtr<class AGeneralEnemyCharacter> AssignedGeneral;
};
