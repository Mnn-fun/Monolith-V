#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "HttpFwd.h"
#include "BackendApiClient.generated.h"

/**
 * UBackendApiClient
 * A subsystem that acts as the C++ HTTP client wrapping calls to the ASP.NET Backend API.
 * This should ONLY be invoked by the Dedicated Server.
 */
UCLASS()
class MONOLITHV_API UBackendApiClient : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/**
	 * Async call to POST /seasons/{SeasonId}/share-events
	 */
	void PostShareEvent(const FString& SeasonId, const FString& GiverId, const FString& ReceiverId, const FString& ItemType, TFunction<void(bool bSuccess, bool bAlreadyShared)> Callback);

	/**
	 * Async call to POST /seasons/{SeasonId}/checkpoints/{CheckpointIndex}/claim
	 */
	void PostCheckpointClaim(const FString& SeasonId, const FString& PlayerId, int32 CheckpointIndex, TFunction<void(bool bSuccess, bool bAlreadyClaimed)> Callback);

private:
	// Hardcoded for now. In a real project, read from an INI file.
	FString BackendBaseUrl = TEXT("http://127.0.0.1:5054");

	void OnShareEventResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully, TFunction<void(bool, bool)> Callback);
	void OnCheckpointClaimResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully, TFunction<void(bool, bool)> Callback);
};
