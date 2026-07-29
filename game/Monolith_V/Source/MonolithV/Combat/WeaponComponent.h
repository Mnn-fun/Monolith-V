#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WeaponComponent.generated.h"

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class MONOLITHV_API UWeaponComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UWeaponComponent();

protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "Weapon Stats")
	int32 Ammo;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "Weapon Stats")
	int32 MaxAmmo;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "Weapon Stats")
	float FireRate; // Shots per second or cooldown duration

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "Weapon Stats")
	float Range;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "Weapon Stats")
	float Damage;

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	bool CanFire() const;

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void ConsumeAmmo();

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void Reload();
};
