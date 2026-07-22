#include "EOSLoginSubsystem.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"

void UEOSLoginSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UEOSLoginSubsystem::Deinitialize()
{
	if (IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld()))
	{
		if (IOnlineIdentityPtr Identity = Subsystem->GetIdentityInterface())
		{
			Identity->ClearOnLoginCompleteDelegate_Handle(0, LoginCompleteDelegateHandle);
		}
	}
	Super::Deinitialize();
}

void UEOSLoginSubsystem::Login(const FString& AuthId, const FString& AuthToken, const FString& AuthType)
{
	IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
	if (!Subsystem)
	{
		UE_LOG(LogTemp, Warning, TEXT("[EOSLogin] OnlineSubsystem not available."));
		OnLoginComplete.Broadcast(false, FString());
		return;
	}

	IOnlineIdentityPtr Identity = Subsystem->GetIdentityInterface();
	if (!Identity.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[EOSLogin] Identity interface not available on OnlineSubsystem."));
		OnLoginComplete.Broadcast(false, FString());
		return;
	}

	LoginCompleteDelegateHandle = Identity->AddOnLoginCompleteDelegate_Handle(
		0, FOnLoginCompleteDelegate::CreateUObject(this, &UEOSLoginSubsystem::OnLoginCompleteCallback));

	FOnlineAccountCredentials Credentials;
	Credentials.Id = AuthId;
	Credentials.Token = AuthToken;
	Credentials.Type = AuthType;

	UE_LOG(LogTemp, Log, TEXT("[EOSLogin] Initiating login (Type: %s, Id: %s)"), *AuthType, *AuthId);
	Identity->Login(0, Credentials);
}

void UEOSLoginSubsystem::OnLoginCompleteCallback(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& UserId, const FString& Error)
{
	IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
	if (Subsystem)
	{
		if (IOnlineIdentityPtr Identity = Subsystem->GetIdentityInterface())
		{
			Identity->ClearOnLoginCompleteDelegate_Handle(LocalUserNum, LoginCompleteDelegateHandle);
		}
	}

	FString ProductUserIdStr;
	if (bWasSuccessful && UserId.IsValid())
	{
		ProductUserIdStr = UserId.ToString();
		UE_LOG(LogTemp, Log, TEXT("[EOSLogin] Login successful! EOS_ProductUserId: %s"), *ProductUserIdStr);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[EOSLogin] Login failed. Error: %s"), *Error);
	}

	OnLoginComplete.Broadcast(bWasSuccessful, ProductUserIdStr);
}

FString UEOSLoginSubsystem::GetLoggedInProductUserId(int32 LocalUserNum) const
{
	if (IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld()))
	{
		if (IOnlineIdentityPtr Identity = Subsystem->GetIdentityInterface())
		{
			FUniqueNetIdPtr NetId = Identity->GetUniquePlayerId(LocalUserNum);
			if (NetId.IsValid())
			{
				return NetId->ToString();
			}
		}
	}
	return FString();
}
