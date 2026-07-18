#include "MonolithVGameMode.h"
#include "Player/MonolithVCharacter.h"

AMonolithVGameMode::AMonolithVGameMode()
{
	DefaultPawnClass = AMonolithVCharacter::StaticClass();
}
