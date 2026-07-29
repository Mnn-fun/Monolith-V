#include "WeaponComponent.h"
#include "Net/UnrealNetwork.h"

UWeaponComponent::UWeaponComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);

	MaxAmmo = 30;
	Ammo = MaxAmmo;
	FireRate = 0.5f; // 0.5s cooldown
	Range = 5000.0f;
	Damage = 20.0f;
}

void UWeaponComponent::BeginPlay()
{
	Super::BeginPlay();
	
	if (GetOwner()->HasAuthority())
	{
		Ammo = MaxAmmo;
	}
}

void UWeaponComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UWeaponComponent, Ammo);
	DOREPLIFETIME(UWeaponComponent, MaxAmmo);
	DOREPLIFETIME(UWeaponComponent, FireRate);
	DOREPLIFETIME(UWeaponComponent, Range);
	DOREPLIFETIME(UWeaponComponent, Damage);
}

bool UWeaponComponent::CanFire() const
{
	return Ammo > 0;
}

void UWeaponComponent::ConsumeAmmo()
{
	if (GetOwner()->HasAuthority())
	{
		if (Ammo > 0)
		{
			Ammo--;
		}
	}
}

void UWeaponComponent::Reload()
{
	if (GetOwner()->HasAuthority())
	{
		Ammo = MaxAmmo;
	}
}
