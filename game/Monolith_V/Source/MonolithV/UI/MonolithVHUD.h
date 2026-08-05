#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "MonolithVHUD.generated.h"

class UMonolithVHUDWidget;

UCLASS()
class MONOLITHV_API AMonolithVHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UMonolithVHUDWidget> HUDWidgetClass;

private:
	UPROPERTY()
	UMonolithVHUDWidget* HUDWidgetInstance;
};
