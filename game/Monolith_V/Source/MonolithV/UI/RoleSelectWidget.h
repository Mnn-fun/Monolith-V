#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "../Player/PlayerTypes.h"
#include "RoleSelectWidget.generated.h"

UCLASS()
class MONOLITHV_API URoleSelectWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// Called by your Blueprint buttons to select a role
	UFUNCTION(BlueprintCallable, Category = "Role Selection")
	void SelectRole(EPlayerRole Role);
};
