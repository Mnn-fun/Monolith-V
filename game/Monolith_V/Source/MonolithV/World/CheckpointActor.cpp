#include "CheckpointActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "../Player/MonolithVCharacter.h"

ACheckpointActor::ACheckpointActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	CheckpointIndex = 0;

	// Visible cube mesh so players can see the checkpoint
	BaseMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BaseMesh"));
	RootComponent = BaseMesh;
	BaseMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		BaseMesh->SetStaticMesh(CubeMesh.Object);
		BaseMesh->SetWorldScale3D(FVector(1.5f, 1.5f, 0.3f)); // Flat platform shape
	}

	// Glowing green material
	static ConstructorHelpers::FObjectFinder<UMaterial> GlowMat(TEXT("/Engine/EngineMaterials/DefaultMaterial.DefaultMaterial"));
	if (GlowMat.Succeeded())
	{
		BaseMesh->SetMaterial(0, GlowMat.Object);
	}

	// Overlap trigger volume above the cube
	TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
	TriggerBox->SetupAttachment(RootComponent);
	TriggerBox->SetRelativeLocation(FVector(0.f, 0.f, 100.f));
	TriggerBox->SetBoxExtent(FVector(150.f, 150.f, 200.f));
	TriggerBox->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	TriggerBox->SetGenerateOverlapEvents(true);
}

void ACheckpointActor::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority() && TriggerBox)
	{
		TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &ACheckpointActor::OnTriggerOverlap);
	}

	UE_LOG(LogTemp, Warning, TEXT("[Checkpoint] Actor spawned: Index=%d at %s"), CheckpointIndex, *GetActorLocation().ToString());
}

void ACheckpointActor::OnTriggerOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	AMonolithVCharacter* Player = Cast<AMonolithVCharacter>(OtherActor);
	if (Player && HasAuthority())
	{
		// Register this as the player's last checkpoint
		Player->LastCheckpointLocation = GetActorLocation() + FVector(0.f, 0.f, 150.f); // Spawn slightly above the platform
		Player->LastCheckpointIndex = CheckpointIndex;

		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Cyan,
			FString::Printf(TEXT("[Checkpoint] Player reached Checkpoint %d!"), CheckpointIndex));
		UE_LOG(LogTemp, Warning, TEXT("[Checkpoint] Player %s reached Checkpoint %d"), *Player->GetName(), CheckpointIndex);
	}
}
