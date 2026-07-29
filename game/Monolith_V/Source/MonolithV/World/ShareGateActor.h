#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ShareGateActor.generated.h"

class UBoxComponent;
class UStaticMeshComponent;

UCLASS()
class MONOLITHV_API AShareGateActor : public AActor
{
	GENERATED_BODY()
	
public:	
	AShareGateActor();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UBoxComponent* CollisionBox;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* GateMesh;

	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, 
						UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, 
						bool bFromSweep, const FHitResult& SweepResult);

private:
	// A set to keep track of players who are currently checking the backend
	// to avoid spamming the API if they wiggle in the overlap zone.
	TSet<FString> PendingChecks;
};
