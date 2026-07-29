#include "ShareGateActor.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Player/MonolithVCharacter.h"
#include "Networking/BackendApiClient.h"
#include "Engine/GameInstance.h"
#include "GameFramework/PlayerController.h"
#include "Components/CapsuleComponent.h"

AShareGateActor::AShareGateActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	GateMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GateMesh"));
	RootComponent = GateMesh;
	GateMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));

	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	CollisionBox->SetupAttachment(RootComponent);
	// Make the box slightly larger than the gate to catch overlaps before they hit the wall
	CollisionBox->SetBoxExtent(FVector(100.0f, 200.0f, 200.0f));
	CollisionBox->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
}

void AShareGateActor::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		CollisionBox->OnComponentBeginOverlap.AddDynamic(this, &AShareGateActor::OnOverlapBegin);
	}
}

void AShareGateActor::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, 
									 UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, 
									 bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority() || !OtherActor || OtherActor == this) return;

	AMonolithVCharacter* PlayerChar = Cast<AMonolithVCharacter>(OtherActor);
	if (!PlayerChar) return;

	APlayerController* PC = Cast<APlayerController>(PlayerChar->GetController());
	if (!PC) return;

	// Construct Player ID
	FString PlayerId = FString::Printf(TEXT("%s_%d"), *PC->GetName(), PC->GetUniqueID());

	// If already pending a check, ignore
	if (PendingChecks.Contains(PlayerId)) return;
	PendingChecks.Add(PlayerId);

	if (UGameInstance* GI = GetGameInstance())
	{
		if (UBackendApiClient* ApiClient = GI->GetSubsystem<UBackendApiClient>())
		{
			// Season 1 hardcoded for now
			FString SeasonId = TEXT("season_1");
			
			ApiClient->GetHasShared(SeasonId, PlayerId, [this, PlayerId, PlayerChar](bool bSuccess, bool bHasShared)
			{
				PendingChecks.Remove(PlayerId);

				if (bSuccess && bHasShared)
				{
					// Player has shared! Let them pass through by ignoring collision between their capsule and our gate mesh
					if (PlayerChar && PlayerChar->GetCapsuleComponent() && GateMesh)
					{
						GateMesh->IgnoreComponentWhenMoving(PlayerChar->GetCapsuleComponent(), true);
						PlayerChar->GetCapsuleComponent()->IgnoreComponentWhenMoving(GateMesh, true);
						
						UE_LOG(LogTemp, Log, TEXT("[Server] Gate unlocked for %s"), *PlayerId);
					}
				}
				else
				{
					// Player has not shared, or the request failed
					UE_LOG(LogTemp, Warning, TEXT("[Server] Gate blocked for %s (HasShared: %d)"), *PlayerId, bHasShared);
					if (PlayerChar)
					{
						PlayerChar->ClientShowShareGateWarning();
					}
				}
			});
		}
	}
}
