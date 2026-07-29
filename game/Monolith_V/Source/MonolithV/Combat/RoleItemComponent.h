#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "../Player/PlayerTypes.h"
#include "RoleItemComponent.generated.h"

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class MONOLITHV_API URoleItemComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	URoleItemComponent();

protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:	
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Role Item")
	bool bItemAvailable;

	UFUNCTION(BlueprintCallable, Category = "Role Item")
	FString GetItemType(EPlayerRole Role) const;
};
