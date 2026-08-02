#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CheckpointActor.generated.h"

class UBoxComponent;

UCLASS()
class MONOLITHV_API ACheckpointActor : public AActor
{
	GENERATED_BODY()
	
public:	
	ACheckpointActor();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Checkpoint")
	int32 CheckpointIndex;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UStaticMeshComponent* BaseMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UBoxComponent* TriggerBox;

	UFUNCTION()
	void OnTriggerOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
};
