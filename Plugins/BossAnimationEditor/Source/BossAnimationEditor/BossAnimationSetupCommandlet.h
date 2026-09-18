#pragma once
#include "Commandlets/Commandlet.h"
#include "BossAnimationSetupCommandlet.generated.h"
UCLASS()
class UBossAnimationSetupCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    UBossAnimationSetupCommandlet();
    virtual int32 Main(const FString& Params) override;
};
