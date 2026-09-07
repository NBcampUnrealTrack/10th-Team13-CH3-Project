#pragma once

#include "CoreMinimal.h"
#include "BTT_BossPatternBase.h"
#include "BTT_Berserk.generated.h"

/** 발악 실행 요청·완료 대기용 BT 태스크 골격, 공통 흐름은 부모에서 구현 예정 */
UCLASS()
class VEILBREAK_API UBTT_Berserk : public UBTT_BossPatternBase
{
	GENERATED_BODY()
};
