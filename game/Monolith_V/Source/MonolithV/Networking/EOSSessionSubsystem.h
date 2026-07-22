#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSessionSettings.h"
#include "EOSSessionSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEOSSessionCreatedDelegate, bool, bWasSuccessful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnEOSSessionFoundDelegate, bool, bWasSuccessful, int32, ResultsCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnEOSSessionJoinedDelegate, bool, bWasSuccessful, const FString&, ConnectString);

/**
 * UGameInstanceSubsystem wrapping EOS session advertising, discovery, and joining.
 * Handles both Dedicated Server (bIsDedicated=true) and Listen Server / PIE session flows.
 */
UCLASS()
class MONOLITHV_API UEOSSessionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/**
	 * Create and advertise an EOS session.
	 * @param MaxPlayers Maximum allowed concurrent connections
	 * @param bIsDedicated True if hosted by an authoritative dedicated server instance
	 */
	UFUNCTION(BlueprintCallable, Category = "EOS|Session")
	void CreateSession(int32 MaxPlayers = 16, bool bIsDedicated = false);

	/**
	 * Search for advertised public EOS sessions.
	 * @param MaxSearchResults Maximum sessions to return in search query
	 */
	UFUNCTION(BlueprintCallable, Category = "EOS|Session")
	void FindSessions(int32 MaxSearchResults = 20);

	/**
	 * Join an advertised EOS session by search result index and travel to its resolved connect string.
	 * @param SessionIndex Index within the last FindSessions search results array
	 */
	UFUNCTION(BlueprintCallable, Category = "EOS|Session")
	void JoinSession(int32 SessionIndex);

	UPROPERTY(BlueprintAssignable, Category = "EOS|Session")
	FOnEOSSessionCreatedDelegate OnSessionCreated;

	UPROPERTY(BlueprintAssignable, Category = "EOS|Session")
	FOnEOSSessionFoundDelegate OnSessionsFound;

	UPROPERTY(BlueprintAssignable, Category = "EOS|Session")
	FOnEOSSessionJoinedDelegate OnSessionJoined;

private:
	void OnCreateSessionCompleteCallback(FName SessionName, bool bWasSuccessful);
	void OnFindSessionsCompleteCallback(bool bWasSuccessful);
	void OnJoinSessionCompleteCallback(FName SessionName, EOnJoinSessionCompleteResult::Type Result);

	FDelegateHandle CreateSessionDelegateHandle;
	FDelegateHandle FindSessionsDelegateHandle;
	FDelegateHandle JoinSessionDelegateHandle;

	TSharedPtr<FOnlineSessionSearch> LastSessionSearch;
};
