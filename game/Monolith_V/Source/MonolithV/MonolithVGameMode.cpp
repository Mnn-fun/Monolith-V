#include "MonolithVGameMode.h"
#include "Player/MonolithVCharacter.h"
#include "Player/MonolithVPlayerController.h"

AMonolithVGameMode::AMonolithVGameMode()
{
	DefaultPawnClass = AMonolithVCharacter::StaticClass();
	PlayerControllerClass = AMonolithVPlayerController::StaticClass();
}
