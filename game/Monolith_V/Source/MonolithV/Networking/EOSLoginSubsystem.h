#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "EOSLoginSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnEOSLoginCompleteDelegate, bool, bWasSuccessful, const FString&, ProductUserIdStr);

/**
 * UGameInstanceSubsystem wrapping EOS login flows (specifically Developer Authentication Tool for sandbox testing).
 * Exposes login completion delegates and ProductUserId retrieval for downstream transaction systems.
 */
UCLASS()
class MONOLITHV_API UEOSLoginSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/**
	 * Initiate login via OnlineSubsystem EOS.
	 * @param AuthId Credential name or IP:Port (e.g. "localhost:8081" or "PlayerAlpha")
	 * @param AuthToken Password/credential name in Dev Auth Tool (e.g. "PlayerAlpha")
	 * @param AuthType Authentication type ("developer" for EOS Dev Auth Tool, "accountportal" for Epic login)
	 */
	UFUNCTION(BlueprintCallable, Category = "EOS|Login")
	void Login(const FString& AuthId, const FString& AuthToken, const FString& AuthType = TEXT("developer"));

	/** Retrieves the canonical EOS ProductUserId string for the specified local user index */
	UFUNCTION(BlueprintCallable, Category = "EOS|Login")
	FString GetLoggedInProductUserId(int32 LocalUserNum = 0) const;

	UPROPERTY(BlueprintAssignable, Category = "EOS|Login")
	FOnEOSLoginCompleteDelegate OnLoginComplete;

private:
	void OnLoginCompleteCallback(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& UserId, const FString& Error);

	FDelegateHandle LoginCompleteDelegateHandle;
};
