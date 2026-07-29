#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "AbilitySystemInterface.h"
#include "PlayerTypes.h"
#include "MonolithVCharacter.generated.h"

class UInputMappingContext;
class UInputAction;
class USpringArmComponent;
class UCameraComponent;
class UStaticMeshComponent;
class UAbilitySystemComponent;
class UMonolithVAttributeSet;
class UGameplayAbility;

UCLASS()
class MONOLITHV_API AMonolithVCharacter : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AMonolithVCharacter();

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Role Item")
	class URoleItemComponent* RoleItemComponent;

	UPROPERTY(ReplicatedUsing=OnRep_DebugShareConfirmed, BlueprintReadOnly, Category = "Networking")
	bool bDebugShareConfirmed;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Role")
	EPlayerRole CurrentRole = EPlayerRole::None;

	UFUNCTION()
	void OnRep_DebugShareConfirmed();

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerRequestShareItem();

	UFUNCTION(Exec)
	void DebugRequestShare();

	UFUNCTION(Client, Reliable)
	void ClientShowShareGateWarning();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	UAbilitySystemComponent* AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	UMonolithVAttributeSet* AttributeSet;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GAS")
	TSubclassOf<UGameplayAbility> TestAbilityClass;

protected:
	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;

	/** Visual representation mesh before custom 3D art is imported */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Mesh, meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* VisualMesh;

	// Enhanced Input mapping context and actions
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	UInputMappingContext* DefaultMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	UInputAction* MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	UInputAction* LookAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	UInputAction* TestAbilityAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	UInputAction* ShareItemAction;

	// Enhanced Input callback handlers
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void OnTestAbilityPressed(const FInputActionValue& Value);
	void OnShareItemPressed(const FInputActionValue& Value);

private:
	double LastShareRequestTime = 0.0;
};
