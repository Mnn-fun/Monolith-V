#include "EOSSessionSubsystem.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "OnlineSessionSettings.h"
#include "Online/OnlineSessionNames.h"
#include "GameFramework/PlayerController.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

void UEOSSessionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UEOSSessionSubsystem::Deinitialize()
{
	if (IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld()))
	{
		if (IOnlineSessionPtr SessionInterface = Subsystem->GetSessionInterface())
		{
			SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionDelegateHandle);
			SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsDelegateHandle);
			SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionDelegateHandle);
		}
	}
	Super::Deinitialize();
}

void UEOSSessionSubsystem::CreateSession(int32 MaxPlayers, bool bIsDedicated)
{
	IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
	if (!Subsystem)
	{
		UE_LOG(LogTemp, Warning, TEXT("[EOSSession] OnlineSubsystem not available."));
		OnSessionCreated.Broadcast(false);
		return;
	}

	IOnlineSessionPtr SessionInterface = Subsystem->GetSessionInterface();
	if (!SessionInterface.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[EOSSession] Session interface not available."));
		OnSessionCreated.Broadcast(false);
		return;
	}

	CreateSessionDelegateHandle = SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(
		FOnCreateSessionCompleteDelegate::CreateUObject(this, &UEOSSessionSubsystem::OnCreateSessionCompleteCallback));

	FOnlineSessionSettings* SessionSettings = new FOnlineSessionSettings();
	SessionSettings->bIsDedicated = bIsDedicated;
	SessionSettings->bShouldAdvertise = true;
	SessionSettings->bAllowJoinInProgress = true;
	SessionSettings->bAllowJoinViaPresence = !bIsDedicated;
	SessionSettings->bUsesPresence = !bIsDedicated;
	SessionSettings->NumPublicConnections = MaxPlayers;

	SessionSettings->Set(FName("SESSION_NAME"), FString("MonolithVSession"), EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	SessionSettings->Set(SEARCH_KEYWORDS, FString("MonolithV"), EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

	UE_LOG(LogTemp, Log, TEXT("[EOSSession] Creating session (MaxPlayers: %d, Dedicated: %s)..."), MaxPlayers, bIsDedicated ? TEXT("True") : TEXT("False"));
	SessionInterface->CreateSession(0, NAME_GameSession, *SessionSettings);
}

void UEOSSessionSubsystem::OnCreateSessionCompleteCallback(FName SessionName, bool bWasSuccessful)
{
	IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
	if (Subsystem)
	{
		if (IOnlineSessionPtr SessionInterface = Subsystem->GetSessionInterface())
		{
			SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionDelegateHandle);
		}
	}

	if (bWasSuccessful)
	{
		UE_LOG(LogTemp, Log, TEXT("[EOSSession] Successfully created session: %s"), *SessionName.ToString());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[EOSSession] Failed to create session: %s"), *SessionName.ToString());
	}

	OnSessionCreated.Broadcast(bWasSuccessful);
}

void UEOSSessionSubsystem::FindSessions(int32 MaxSearchResults)
{
	IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
	if (!Subsystem)
	{
		UE_LOG(LogTemp, Warning, TEXT("[EOSSession] OnlineSubsystem not available."));
		OnSessionsFound.Broadcast(false, 0);
		return;
	}

	IOnlineSessionPtr SessionInterface = Subsystem->GetSessionInterface();
	if (!SessionInterface.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[EOSSession] Session interface not available."));
		OnSessionsFound.Broadcast(false, 0);
		return;
	}

	LastSessionSearch = MakeShareable(new FOnlineSessionSearch());
	LastSessionSearch->MaxSearchResults = MaxSearchResults;
	LastSessionSearch->QuerySettings.Set(SEARCH_KEYWORDS, FString("MonolithV"), EOnlineComparisonOp::Equals);

	FindSessionsDelegateHandle = SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(
		FOnFindSessionsCompleteDelegate::CreateUObject(this, &UEOSSessionSubsystem::OnFindSessionsCompleteCallback));

	UE_LOG(LogTemp, Log, TEXT("[EOSSession] Searching for MonolithV sessions..."));
	SessionInterface->FindSessions(0, LastSessionSearch.ToSharedRef());
}

void UEOSSessionSubsystem::OnFindSessionsCompleteCallback(bool bWasSuccessful)
{
	IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
	if (Subsystem)
	{
		if (IOnlineSessionPtr SessionInterface = Subsystem->GetSessionInterface())
		{
			SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsDelegateHandle);
		}
	}

	int32 ResultsCount = (bWasSuccessful && LastSessionSearch.IsValid()) ? LastSessionSearch->SearchResults.Num() : 0;
	UE_LOG(LogTemp, Log, TEXT("[EOSSession] FindSessions completed (Success: %s, Results: %d)"), bWasSuccessful ? TEXT("True") : TEXT("False"), ResultsCount);

	OnSessionsFound.Broadcast(bWasSuccessful, ResultsCount);
}

void UEOSSessionSubsystem::JoinSession(int32 SessionIndex)
{
	if (!LastSessionSearch.IsValid() || !LastSessionSearch->SearchResults.IsValidIndex(SessionIndex))
	{
		UE_LOG(LogTemp, Warning, TEXT("[EOSSession] Invalid session search index: %d"), SessionIndex);
		OnSessionJoined.Broadcast(false, FString());
		return;
	}

	IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
	if (!Subsystem)
	{
		OnSessionJoined.Broadcast(false, FString());
		return;
	}

	IOnlineSessionPtr SessionInterface = Subsystem->GetSessionInterface();
	if (!SessionInterface.IsValid())
	{
		OnSessionJoined.Broadcast(false, FString());
		return;
	}

	JoinSessionDelegateHandle = SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(
		FOnJoinSessionCompleteDelegate::CreateUObject(this, &UEOSSessionSubsystem::OnJoinSessionCompleteCallback));

	UE_LOG(LogTemp, Log, TEXT("[EOSSession] Joining session at index %d..."), SessionIndex);
	SessionInterface->JoinSession(0, NAME_GameSession, LastSessionSearch->SearchResults[SessionIndex]);
}

void UEOSSessionSubsystem::OnJoinSessionCompleteCallback(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
	if (Subsystem)
	{
		if (IOnlineSessionPtr SessionInterface = Subsystem->GetSessionInterface())
		{
			SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionDelegateHandle);
		}
	}

	bool bWasSuccessful = (Result == EOnJoinSessionCompleteResult::Success);
	FString ConnectString;

	if (bWasSuccessful && Subsystem)
	{
		if (IOnlineSessionPtr SessionInterface = Subsystem->GetSessionInterface())
		{
			SessionInterface->GetResolvedConnectString(NAME_GameSession, ConnectString);
			UE_LOG(LogTemp, Log, TEXT("[EOSSession] Successfully joined session! Resolved ConnectString: %s"), *ConnectString);

			if (APlayerController* PC = GetGameInstance()->GetFirstLocalPlayerController())
			{
				UE_LOG(LogTemp, Log, TEXT("[EOSSession] Traveling to server: %s"), *ConnectString);
				PC->ClientTravel(ConnectString, TRAVEL_Absolute);
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[EOSSession] Failed to join session. Result code: %d"), (int32)Result);
	}

	OnSessionJoined.Broadcast(bWasSuccessful, ConnectString);
}
