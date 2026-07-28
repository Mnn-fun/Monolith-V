#include "RoleSelectWidget.h"
#include "../Player/MonolithVPlayerController.h"

void URoleSelectWidget::SelectRole(EPlayerRole Role)
{
	FString RoleString = TEXT("NONE");
	if (Role == EPlayerRole::Male)
	{
		RoleString = TEXT("MALE");
	}
	else if (Role == EPlayerRole::Female)
	{
		RoleString = TEXT("FEMALE");
	}

	if (AMonolithVPlayerController* PC = Cast<AMonolithVPlayerController>(GetOwningPlayer()))
	{
		PC->ServerSubmitRoleChoice(RoleString);
	}
}
