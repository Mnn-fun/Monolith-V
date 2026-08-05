#include "MonolithVPlayerController.h"
#include "MonolithVCharacter.h"
#include "../Combat/MonolithVAttributeSet.h"
#include "../Networking/BackendApiClient.h"
#include "Engine/GameInstance.h"
#include "Blueprint/UserWidget.h"
#include "PlayerTypes.h"

AMonolithVPlayerController::AMonolithVPlayerController()
{
}

void AMonolithVPlayerController::TestDamage(float DamageAmount)
{
	if (HasAuthority())
	{
		ServerTestDamage_Implementation(DamageAmount);
	}
	else
	{
		ServerTestDamage(DamageAmount);
	}
}

void AMonolithVPlayerController::ServerTestDamage_Implementation(float DamageAmount)
{
	if (AMonolithVCharacter* MyChar = Cast<AMonolithVCharacter>(GetPawn()))
	{
		if (MyChar->AttributeSet)
		{
			float NewHealth = FMath::Clamp(MyChar->AttributeSet->GetHealth() - DamageAmount, 0.0f, MyChar->AttributeSet->GetMaxHealth());
			MyChar->AttributeSet->SetHealth(NewHealth);
			UE_LOG(LogTemp, Display, TEXT("AMonolithVPlayerController: Server decreased Health by %f. New Health: %f"), DamageAmount, NewHealth);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("AMonolithVPlayerController: TestDamage called but AttributeSet is null on character %s"), *MyChar->GetName());
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("AMonolithVPlayerController: TestDamage called but GetPawn() is not AMonolithVCharacter"));
	}
}

void AMonolithVPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		// Check if this player already has a role assigned for the season
		if (UGameInstance* GameInstance = GetGameInstance())
		{
			if (UBackendApiClient* BackendClient = GameInstance->GetSubsystem<UBackendApiClient>())
			{
				FString SeasonId = TEXT("season_1");
				FString PlayerId = FString::Printf(TEXT("%s_%d"), *GetName(), GetUniqueID());

				UE_LOG(LogTemp, Log, TEXT("[Server] AMonolithVPlayerController::BeginPlay - Fetching role for %s"), *PlayerId);
				BackendClient->GetSeasonRole(SeasonId, PlayerId, [this](bool bSuccess, const FString& FetchedRole)
				{
					if (bSuccess && !FetchedRole.IsEmpty())
					{
						UE_LOG(LogTemp, Log, TEXT("[Server] Player already has role: %s"), *FetchedRole);
						bHasRoleAssigned = true;
						
						if (AMonolithVCharacter* MyChar = Cast<AMonolithVCharacter>(GetPawn()))
						{
							MyChar->CurrentRole = (FetchedRole == "MALE") ? EPlayerRole::Male : EPlayerRole::Female;
						}
					}
					else
					{
						UE_LOG(LogTemp, Warning, TEXT("[Server] Player has NO role. Must call DebugSelectRole MALE or FEMALE."));
						bHasRoleAssigned = false;
						ClientShowRoleSelection();
					}
				});
			}
		}
	}
}

void AMonolithVPlayerController::DebugSelectRole(FString ChosenRole)
{
	if (IsLocalController())
	{
		UE_LOG(LogTemp, Log, TEXT("[Client] Selected role: %s. Submitting to server..."), *ChosenRole);
		ServerSubmitRoleChoice(ChosenRole);
	}
}

void AMonolithVPlayerController::ServerSubmitRoleChoice_Implementation(const FString& ChosenRole)
{
	if (bHasRoleAssigned)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Server] Player already has a role assigned this season!"));
		return;
	}

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UBackendApiClient* BackendClient = GameInstance->GetSubsystem<UBackendApiClient>())
		{
			FString SeasonId = TEXT("season_1");
			FString PlayerId = FString::Printf(TEXT("%s_%d"), *GetName(), GetUniqueID());

			UE_LOG(LogTemp, Log, TEXT("[Server] Submitting role %s for %s"), *ChosenRole, *PlayerId);
			BackendClient->PostSeasonRole(SeasonId, PlayerId, ChosenRole, [this, ChosenRole](bool bSuccess, bool bAlreadyAssigned)
			{
				if (bSuccess)
				{
					bHasRoleAssigned = true;
					UE_LOG(LogTemp, Log, TEXT("[Server] Role successfully assigned in backend!"));
					
					if (AMonolithVCharacter* MyChar = Cast<AMonolithVCharacter>(GetPawn()))
					{
						MyChar->CurrentRole = (ChosenRole == "MALE") ? EPlayerRole::Male : EPlayerRole::Female;
					}
					ClientHideRoleSelection();
				}
				else
				{
					UE_LOG(LogTemp, Error, TEXT("[Server] Failed to assign role in backend."));
				}
			});
		}
	}
}

bool AMonolithVPlayerController::ServerSubmitRoleChoice_Validate(const FString& ChosenRole)
{
	return true;
}

void AMonolithVPlayerController::ClientShowRoleSelection_Implementation()
{
	if (RoleSelectWidgetClass)
	{
		if (!RoleSelectWidgetInstance)
		{
			RoleSelectWidgetInstance = CreateWidget<UUserWidget>(this, RoleSelectWidgetClass);
		}

		if (RoleSelectWidgetInstance && !RoleSelectWidgetInstance->IsInViewport())
		{
			RoleSelectWidgetInstance->AddToViewport();
			bShowMouseCursor = true;
			SetInputMode(FInputModeUIOnly());
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("ClientShowRoleSelection called, but RoleSelectWidgetClass is not assigned in the PlayerController blueprint!"));
	}
}

void AMonolithVPlayerController::ClientHideRoleSelection_Implementation()
{
	if (RoleSelectWidgetInstance && RoleSelectWidgetInstance->IsInViewport())
	{
		RoleSelectWidgetInstance->RemoveFromParent();
	}

	// Always reset input mode to game, even if widget was already removed
	bShowMouseCursor = false;
	SetInputMode(FInputModeGameOnly());

	UE_LOG(LogTemp, Warning, TEXT("[Client] ClientHideRoleSelection - Input mode set to GameOnly"));
}
