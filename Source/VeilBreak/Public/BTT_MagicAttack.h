#pragma once

#include "CoreMinimal.h"
#include "BTT_BossPatternBase.h"
#include "BTT_MagicAttack.generated.h"

/** 마법 투척 실행 요청·완료 대기용 BT 태스크 골격, 실제 공격은 마법 투척 액터에서 구현 예정 */
UCLASS()
class VEILBREAK_API UBTT_MagicAttack : public UBTT_BossPatternBase
{
	GENERATED_BODY()
};
