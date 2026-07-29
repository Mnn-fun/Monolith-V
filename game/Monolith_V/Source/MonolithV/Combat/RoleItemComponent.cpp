#include "RoleItemComponent.h"
#include "Net/UnrealNetwork.h"

URoleItemComponent::URoleItemComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
	bItemAvailable = true;
}

void URoleItemComponent::BeginPlay()
{
	Super::BeginPlay();
}

void URoleItemComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(URoleItemComponent, bItemAvailable);
}

FString URoleItemComponent::GetItemType(EPlayerRole Role) const
{
	if (Role == EPlayerRole::Male)
	{
		return TEXT("GOLDEN_APPLE");
	}
	else if (Role == EPlayerRole::Female)
	{
		return TEXT("COUNTERPART_ITEM");
	}
	return TEXT("UNKNOWN");
}
