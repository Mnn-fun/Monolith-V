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

	/**
	 * Async call to GET /seasons/{SeasonId}/players/{PlayerId}/role
	 */
	void GetSeasonRole(const FString& SeasonId, const FString& PlayerId, TFunction<void(bool bSuccess, const FString& Role)> Callback);

	/**
	 * Async call to POST /seasons/{SeasonId}/players/{PlayerId}/role
	 */
	void PostSeasonRole(const FString& SeasonId, const FString& PlayerId, const FString& Role, TFunction<void(bool bSuccess, bool bAlreadyAssigned)> Callback);

	/**
	 * Async call to GET /seasons/{SeasonId}/players/{PlayerId}/has-shared
	 */
	void GetHasShared(const FString& SeasonId, const FString& PlayerId, TFunction<void(bool bSuccess, bool bHasShared)> Callback);

private:
	// Hardcoded for now. In a real project, read from an INI file.
	FString BackendBaseUrl = TEXT("http://127.0.0.1:5054");

	void OnShareEventResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully, TFunction<void(bool, bool)> Callback);
	void OnCheckpointClaimResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully, TFunction<void(bool, bool)> Callback);
	void OnGetSeasonRoleResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully, TFunction<void(bool, const FString&)> Callback);
	void OnPostSeasonRoleResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully, TFunction<void(bool, bool)> Callback);
	void OnGetHasSharedResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully, TFunction<void(bool, bool)> Callback);
};
