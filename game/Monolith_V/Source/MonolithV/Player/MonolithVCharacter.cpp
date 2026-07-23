#include "MonolithVCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "InputAction.h"
#include "UObject/ConstructorHelpers.h"
#include "Net/UnrealNetwork.h"
#include "AbilitySystemComponent.h"
#include "../Combat/MonolithVAttributeSet.h"
#include "../Combat/GA_TestAbility.h"
#include "../Networking/BackendApiClient.h"
#include "Engine/GameInstance.h"

AMonolithVCharacter::AMonolithVCharacter()
{
	bReplicates = true;

	// Don't rotate character when controller rotates. Let that just affect the camera/look.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character net update frequency for 30Hz server-authoritative tick rate
	SetNetUpdateFrequency(30.f);
	SetMinNetUpdateFrequency(10.f);

	// Configure character movement component deliberately for server-authoritative networked movement
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->bOrientRotationToMovement = true; // Character rotates to movement direction
		GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f);

		// Client-side prediction & smoothing tuning for 30Hz tick responsiveness
		GetCharacterMovement()->NetworkSmoothingMode = ENetworkSmoothingMode::Exponential;

		// Deliberate, documented movement tuning (these become tuning knobs later for jetpack traversal in Phase 3)
		GetCharacterMovement()->MaxWalkSpeed = 600.f;
		GetCharacterMovement()->GravityScale = 1.0f;
		GetCharacterMovement()->JumpZVelocity = 600.f;
		GetCharacterMovement()->AirControl = 0.35f;
	}

	// Create AbilitySystemComponent and AttributeSet for GAS
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	AttributeSet = CreateDefaultSubobject<UMonolithVAttributeSet>(TEXT("AttributeSet"));

	TestAbilityClass = UGA_TestAbility::StaticClass();

	// Create a visual mesh so characters are visible to each other in multiplayer sessions
	VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
	VisualMesh->SetupAttachment(GetCapsuleComponent());
	VisualMesh->SetCollisionProfileName(TEXT("NoCollision")); // Prevent camera boom from hitting the visual mesh
	VisualMesh->bOwnerNoSee = false; // Ensure owner sees their own body in TPP
	VisualMesh->SetIsReplicated(true); // Replicate visual mesh component to all clients
	
	// Use Cylinder shape since Capsule is not in BasicShapes
	static ConstructorHelpers::FObjectFinder<UStaticMesh> MeshAsset(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (MeshAsset.Succeeded())
	{
		VisualMesh->SetStaticMesh(MeshAsset.Object);
		VisualMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -40.0f));
		VisualMesh->SetRelativeScale3D(FVector(0.6f, 0.6f, 1.3f));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("AMonolithVCharacter: Failed to find /Engine/BasicShapes/Cylinder.Cylinder!"));
	}

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 500.0f; // Pull camera 500 units behind character for clear TPP
	CameraBoom->SocketOffset = FVector(0.0f, 0.0f, 80.0f); // Raise camera look target up above head level
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller
	CameraBoom->bDoCollisionTest = false; // Disable collision testing so camera NEVER collapses inside character into FPP

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Load default Enhanced Input assets created in Content Browser
	// Check root Content (/Game/) first, then /Game/Input/
	static ConstructorHelpers::FObjectFinder<UInputMappingContext> IMCAsset(TEXT("/Game/IMC_Default.IMC_Default"));
	if (IMCAsset.Succeeded())
	{
		DefaultMappingContext = IMCAsset.Object;
	}
	else
	{
		static ConstructorHelpers::FObjectFinder<UInputMappingContext> IMCAssetSub(TEXT("/Game/Input/IMC_Default.IMC_Default"));
		if (IMCAssetSub.Succeeded())
		{
			DefaultMappingContext = IMCAssetSub.Object;
		}
	}

	static ConstructorHelpers::FObjectFinder<UInputAction> MoveAsset(TEXT("/Game/IA_Move.IA_Move"));
	if (MoveAsset.Succeeded())
	{
		MoveAction = MoveAsset.Object;
	}
	else
	{
		static ConstructorHelpers::FObjectFinder<UInputAction> MoveAssetSub(TEXT("/Game/Input/IA_Move.IA_Move"));
		if (MoveAssetSub.Succeeded())
		{
			MoveAction = MoveAssetSub.Object;
		}
	}

	static ConstructorHelpers::FObjectFinder<UInputAction> LookAsset(TEXT("/Game/IA_Look.IA_Look"));
	if (LookAsset.Succeeded())
	{
		LookAction = LookAsset.Object;
	}
	else
	{
		static ConstructorHelpers::FObjectFinder<UInputAction> LookAssetSub(TEXT("/Game/Input/IA_Look.IA_Look"));
		if (LookAssetSub.Succeeded())
		{
			LookAction = LookAssetSub.Object;
		}
	}

	static ConstructorHelpers::FObjectFinder<UInputAction> TestAbilityAsset(TEXT("/Game/IA_TestAbility.IA_TestAbility"));
	if (TestAbilityAsset.Succeeded())
	{
		TestAbilityAction = TestAbilityAsset.Object;
	}
	else
	{
		static ConstructorHelpers::FObjectFinder<UInputAction> TestAbilityAssetSub(TEXT("/Game/Input/IA_TestAbility.IA_TestAbility"));
		if (TestAbilityAssetSub.Succeeded())
		{
			TestAbilityAction = TestAbilityAssetSub.Object;
		}
	}
}

void AMonolithVCharacter::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogTemp, Warning, TEXT("AMonolithVCharacter BeginPlay on %s (HasAuthority: %d)"), *GetName(), HasAuthority() ? 1 : 0);

	if (VisualMesh && VisualMesh->GetStaticMesh())
	{
		UE_LOG(LogTemp, Warning, TEXT("VisualMesh is VALID with StaticMesh: %s on %s"), *VisualMesh->GetStaticMesh()->GetName(), *GetName());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("VisualMesh or StaticMesh is NULL on %s! (Note: Live Coding cannot add CreateDefaultSubobject to existing CDO! Close & Relaunch Editor to run C++ constructor!)"), *GetName());
	}

	if (CameraBoom)
	{
		UE_LOG(LogTemp, Warning, TEXT("CameraBoom is VALID on %s (ArmLength: %f)"), *GetName(), CameraBoom->TargetArmLength);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("CameraBoom is NULL on %s! (Note: Live Coding cannot add CreateDefaultSubobject to existing CDO! Close & Relaunch Editor to run C++ constructor!)"), *GetName());
	}

	// Add input mapping context if local controller
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			if (DefaultMappingContext)
			{
				Subsystem->AddMappingContext(DefaultMappingContext, 0);
				UE_LOG(LogTemp, Warning, TEXT("Successfully added DefaultMappingContext to local player on %s"), *GetName());
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("DefaultMappingContext is NULL on %s! Input mapping context asset not found!"), *GetName());
			}
		}
	}
}

void AMonolithVCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Also ensure mapping context is added when possessed by player controller
	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			if (DefaultMappingContext)
			{
				Subsystem->AddMappingContext(DefaultMappingContext, 0);
			}
		}
	}

	// Bind Enhanced Input actions
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (MoveAction)
		{
			EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AMonolithVCharacter::Move);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("MoveAction is NULL on %s! Cannot bind Move!"), *GetName());
		}

		if (LookAction)
		{
			EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AMonolithVCharacter::Look);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("LookAction is NULL on %s! Cannot bind Look!"), *GetName());
		}

		if (TestAbilityAction)
		{
			EnhancedInputComponent->BindAction(TestAbilityAction, ETriggerEvent::Triggered, this, &AMonolithVCharacter::OnTestAbilityPressed);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("TestAbilityAction is NULL on %s! Cannot bind OnTestAbilityPressed!"), *GetName());
		}
	}
}

void AMonolithVCharacter::Move(const FInputActionValue& Value)
{
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// Find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// Get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		// Get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// Add movement input to CharacterMovementComponent (automatically client-predicted & server-corrected)
		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

void AMonolithVCharacter::Look(const FInputActionValue& Value)
{
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void AMonolithVCharacter::OnTestAbilityPressed(const FInputActionValue& Value)
{
	if (AbilitySystemComponent && TestAbilityClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("AMonolithVCharacter::OnTestAbilityPressed on %s - TryActivateAbilityByClass(%s)"), *GetName(), *TestAbilityClass->GetName());
		AbilitySystemComponent->TryActivateAbilityByClass(TestAbilityClass);
	}
}

void AMonolithVCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
		UE_LOG(LogTemp, Warning, TEXT("AMonolithVCharacter::PossessedBy - InitAbilityActorInfo called on Server for %s"), *GetName());

		if (HasAuthority() && TestAbilityClass)
		{
			AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(TestAbilityClass, 1, 0, this));
			UE_LOG(LogTemp, Warning, TEXT("AMonolithVCharacter::PossessedBy - Granted TestAbilityClass %s to %s"), *TestAbilityClass->GetName(), *GetName());
		}
	}
}

void AMonolithVCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
		UE_LOG(LogTemp, Warning, TEXT("AMonolithVCharacter::OnRep_PlayerState - InitAbilityActorInfo called on Client for %s"), *GetName());
	}
}

UAbilitySystemComponent* AMonolithVCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void AMonolithVCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AMonolithVCharacter, bDebugShareConfirmed);
}

void AMonolithVCharacter::OnRep_DebugShareConfirmed()
{
	UE_LOG(LogTemp, Warning, TEXT("[Client] Server confirmed the debug share event! bDebugShareConfirmed is TRUE on %s"), *GetName());
}

bool AMonolithVCharacter::ServerRequestShareItem_Validate()
{
	return true;
}

void AMonolithVCharacter::DebugRequestShare()
{
	if (IsLocallyControlled())
	{
		UE_LOG(LogTemp, Warning, TEXT("[Client] Requesting Share Item..."));
		ServerRequestShareItem();
	}
}

void AMonolithVCharacter::ServerRequestShareItem_Implementation()
{
	// TODO Phase 3: Check distance/possession server-side here before talking to backend

	if (UGameInstance* GI = GetGameInstance())
	{
		// Note: Include "Networking/BackendApiClient.h" at the top of the file
		if (UBackendApiClient* ApiClient = GI->GetSubsystem<UBackendApiClient>())
		{
			UE_LOG(LogTemp, Warning, TEXT("[Server] Calling BackendApiClient->PostShareEvent for %s"), *GetName());
			ApiClient->PostShareEvent(TEXT("season_1"), TEXT("player_a"), TEXT("player_b"), TEXT("relic"), [this](bool bSuccess, bool bAlreadyShared)
			{
				if (bSuccess)
				{
					this->bDebugShareConfirmed = true;
					// Force a net update so clients see it immediately
					this->ForceNetUpdate();
					UE_LOG(LogTemp, Warning, TEXT("[Server] PostShareEvent succeeded. Replicating bDebugShareConfirmed=true to clients."));
				}
				else
				{
					UE_LOG(LogTemp, Error, TEXT("[Server] PostShareEvent failed."));
				}
			});
		}
	}
}
