#include "MonolithVHUD.h"
#include "MonolithVHUDWidget.h"
#include "Blueprint/UserWidget.h"
#include "Player/MonolithVCharacter.h"

void AMonolithVHUD::BeginPlay()
{
	Super::BeginPlay();

	APlayerController* PC = GetOwningPlayerController();
	if (!PC || !PC->IsLocalController())
	{
		return; // Don't create HUD on dedicated server or for non-local controllers
	}

	if (HUDWidgetClass)
	{
		HUDWidgetInstance = CreateWidget<UMonolithVHUDWidget>(PC, HUDWidgetClass);
		if (HUDWidgetInstance)
		{
			HUDWidgetInstance->AddToViewport();
			
			// Try to bind immediately if character exists
			if (APawn* PlayerPawn = PC->GetPawn())
			{
				HUDWidgetInstance->BindToPlayer(PlayerPawn);
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("AMonolithVHUD: HUDWidgetClass is not set! Assign it in your HUD blueprint."));
	}
}
