#include "Networking/BackendApiClient.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "JsonObjectConverter.h"

void UBackendApiClient::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UBackendApiClient::Deinitialize()
{
	Super::Deinitialize();
}

void UBackendApiClient::PostShareEvent(const FString& SeasonId, const FString& GiverId, const FString& ReceiverId, const FString& ItemType, TFunction<void(bool bSuccess, bool bAlreadyShared)> Callback)
{
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	
	FString Url = FString::Printf(TEXT("%s/seasons/%s/share-events"), *BackendBaseUrl, *SeasonId);
	Request->SetURL(Url);
	Request->SetVerb(TEXT("POST"));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));

	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());
	JsonObject->SetStringField(TEXT("giverPlayerId"), GiverId);
	JsonObject->SetStringField(TEXT("receiverPlayerId"), ReceiverId);
	JsonObject->SetStringField(TEXT("itemType"), ItemType);

	FString RequestBody;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestBody);
	FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

	Request->SetContentAsString(RequestBody);

	Request->OnProcessRequestComplete().BindUObject(this, &UBackendApiClient::OnShareEventResponseReceived, Callback);
	Request->ProcessRequest();
}

void UBackendApiClient::OnShareEventResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully, TFunction<void(bool, bool)> Callback)
{
	if (!bConnectedSuccessfully || !Response.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("[UBackendApiClient] PostShareEvent failed to connect or response invalid."));
		Callback(false, false);
		return;
	}

	int32 StatusCode = Response->GetResponseCode();
	if (StatusCode == 200 || StatusCode == 201)
	{
		// Parse the response body for alreadyShared
		FString ResponseBody = Response->GetContentAsString();
		TSharedPtr<FJsonObject> JsonObject;
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseBody);

		bool bAlreadyShared = false;
		if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
		{
			JsonObject->TryGetBoolField(TEXT("alreadyShared"), bAlreadyShared);
		}

		UE_LOG(LogTemp, Log, TEXT("[UBackendApiClient] PostShareEvent SUCCESS (Code: %d, AlreadyShared: %d)"), StatusCode, bAlreadyShared);
		Callback(true, bAlreadyShared);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[UBackendApiClient] PostShareEvent failed. Code: %d, Response: %s"), StatusCode, *Response->GetContentAsString());
		Callback(false, false);
	}
}

void UBackendApiClient::PostCheckpointClaim(const FString& SeasonId, const FString& PlayerId, int32 CheckpointIndex, TFunction<void(bool bSuccess, bool bAlreadyClaimed)> Callback)
{
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	
	FString Url = FString::Printf(TEXT("%s/seasons/%s/players/%s/checkpoints"), *BackendBaseUrl, *SeasonId, *PlayerId);
	Request->SetURL(Url);
	Request->SetVerb(TEXT("POST"));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));

	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());
	JsonObject->SetNumberField(TEXT("checkpointIndex"), CheckpointIndex);

	FString RequestBody;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestBody);
	FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

	Request->SetContentAsString(RequestBody);

	Request->OnProcessRequestComplete().BindUObject(this, &UBackendApiClient::OnCheckpointClaimResponseReceived, Callback);
	Request->ProcessRequest();
}

void UBackendApiClient::OnCheckpointClaimResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully, TFunction<void(bool, bool)> Callback)
{
	if (!bConnectedSuccessfully || !Response.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("[UBackendApiClient] PostCheckpointClaim failed to connect or response invalid."));
		Callback(false, false);
		return;
	}

	int32 StatusCode = Response->GetResponseCode();
	if (StatusCode == 200 || StatusCode == 201)
	{
		FString ResponseBody = Response->GetContentAsString();
		TSharedPtr<FJsonObject> JsonObject;
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseBody);

		bool bAlreadyClaimed = false;
		if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
		{
			JsonObject->TryGetBoolField(TEXT("alreadyClaimed"), bAlreadyClaimed);
		}

		UE_LOG(LogTemp, Log, TEXT("[UBackendApiClient] PostCheckpointClaim SUCCESS (Code: %d, AlreadyClaimed: %d)"), StatusCode, bAlreadyClaimed);
		Callback(true, bAlreadyClaimed);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[UBackendApiClient] PostCheckpointClaim failed. Code: %d, Response: %s"), StatusCode, *Response->GetContentAsString());
		Callback(false, false);
	}
}

void UBackendApiClient::GetSeasonRole(const FString& SeasonId, const FString& PlayerId, TFunction<void(bool bSuccess, const FString& Role)> Callback)
{
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	
	FString Url = FString::Printf(TEXT("%s/seasons/%s/players/%s/role"), *BackendBaseUrl, *SeasonId, *PlayerId);
	Request->SetURL(Url);
	Request->SetVerb(TEXT("GET"));

	Request->OnProcessRequestComplete().BindUObject(this, &UBackendApiClient::OnGetSeasonRoleResponseReceived, Callback);
	Request->ProcessRequest();
}

void UBackendApiClient::OnGetSeasonRoleResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully, TFunction<void(bool, const FString&)> Callback)
{
	if (!bConnectedSuccessfully || !Response.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("[UBackendApiClient] GetSeasonRole failed to connect or response invalid."));
		Callback(false, TEXT(""));
		return;
	}

	int32 StatusCode = Response->GetResponseCode();
	if (StatusCode == 200)
	{
		FString ResponseBody = Response->GetContentAsString();
		TSharedPtr<FJsonObject> JsonObject;
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseBody);

		FString Role = TEXT("");
		if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
		{
			JsonObject->TryGetStringField(TEXT("role"), Role);
		}

		UE_LOG(LogTemp, Log, TEXT("[UBackendApiClient] GetSeasonRole SUCCESS (Role: %s)"), *Role);
		Callback(true, Role);
	}
	else if (StatusCode == 404)
	{
		UE_LOG(LogTemp, Log, TEXT("[UBackendApiClient] GetSeasonRole returned 404 Not Found (Player has no role)."));
		Callback(true, TEXT("")); // success=true, empty string means not found
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[UBackendApiClient] GetSeasonRole failed. Code: %d, Response: %s"), StatusCode, *Response->GetContentAsString());
		Callback(false, TEXT(""));
	}
}

void UBackendApiClient::PostSeasonRole(const FString& SeasonId, const FString& PlayerId, const FString& Role, TFunction<void(bool bSuccess, bool bAlreadyAssigned)> Callback)
{
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	
	FString Url = FString::Printf(TEXT("%s/seasons/%s/players/%s/role"), *BackendBaseUrl, *SeasonId, *PlayerId);
	Request->SetURL(Url);
	Request->SetVerb(TEXT("POST"));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));

	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());
	JsonObject->SetStringField(TEXT("role"), Role);

	FString RequestBody;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestBody);
	FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

	Request->SetContentAsString(RequestBody);

	Request->OnProcessRequestComplete().BindUObject(this, &UBackendApiClient::OnPostSeasonRoleResponseReceived, Callback);
	Request->ProcessRequest();
}

void UBackendApiClient::OnPostSeasonRoleResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully, TFunction<void(bool, bool)> Callback)
{
	if (!bConnectedSuccessfully || !Response.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("[UBackendApiClient] PostSeasonRole failed to connect or response invalid."));
		Callback(false, false);
		return;
	}

	int32 StatusCode = Response->GetResponseCode();
	if (StatusCode == 201)
	{
		UE_LOG(LogTemp, Log, TEXT("[UBackendApiClient] PostSeasonRole SUCCESS (Code: 201)."));
		Callback(true, false);
	}
	else if (StatusCode == 409)
	{
		UE_LOG(LogTemp, Warning, TEXT("[UBackendApiClient] PostSeasonRole returned 409 Conflict (Role already assigned)."));
		Callback(true, true);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[UBackendApiClient] PostSeasonRole failed. Code: %d, Response: %s"), StatusCode, *Response->GetContentAsString());
		Callback(false, false);
	}
}

void UBackendApiClient::GetHasShared(const FString& SeasonId, const FString& PlayerId, TFunction<void(bool bSuccess, bool bHasShared)> Callback)
{
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	
	FString Url = FString::Printf(TEXT("%s/seasons/%s/players/%s/has-shared"), *BackendBaseUrl, *SeasonId, *PlayerId);
	Request->SetURL(Url);
	Request->SetVerb(TEXT("GET"));

	Request->OnProcessRequestComplete().BindUObject(this, &UBackendApiClient::OnGetHasSharedResponseReceived, Callback);
	Request->ProcessRequest();
}

void UBackendApiClient::OnGetHasSharedResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully, TFunction<void(bool, bool)> Callback)
{
	if (!bConnectedSuccessfully || !Response.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("[UBackendApiClient] GetHasShared failed to connect or response invalid."));
		Callback(false, false);
		return;
	}

	int32 StatusCode = Response->GetResponseCode();
	if (StatusCode == 200)
	{
		FString ResponseBody = Response->GetContentAsString();
		TSharedPtr<FJsonObject> JsonObject;
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseBody);

		bool bHasShared = false;
		if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
		{
			JsonObject->TryGetBoolField(TEXT("hasShared"), bHasShared);
		}

		UE_LOG(LogTemp, Log, TEXT("[UBackendApiClient] GetHasShared SUCCESS (Code: %d, HasShared: %d)"), StatusCode, bHasShared);
		Callback(true, bHasShared);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[UBackendApiClient] GetHasShared failed. Code: %d, Response: %s"), StatusCode, *Response->GetContentAsString());
		Callback(false, false);
	}
}
