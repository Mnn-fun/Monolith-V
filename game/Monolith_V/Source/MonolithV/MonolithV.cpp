#include "MonolithV.h"
#include "Modules/ModuleManager.h"
#include "OnlineSubsystem.h"

class FMonolithVModule : public FDefaultGameModuleImpl
{
public:
	virtual void StartupModule() override
	{
		UE_LOG(LogTemp, Warning, TEXT("EOS Subsystem: %s"), IOnlineSubsystem::Get(TEXT("EOS")) ? TEXT("OK") : TEXT("NULL"));
	}
};

IMPLEMENT_PRIMARY_GAME_MODULE(FMonolithVModule, MonolithV, "MonolithV");
