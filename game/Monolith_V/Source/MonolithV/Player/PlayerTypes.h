#pragma once

#include "CoreMinimal.h"
#include "PlayerTypes.generated.h"

UENUM(BlueprintType)
enum class EPlayerRole : uint8
{
    None   UMETA(DisplayName = "None"),
    Male   UMETA(DisplayName = "Male"),
    Female UMETA(DisplayName = "Female")
};
