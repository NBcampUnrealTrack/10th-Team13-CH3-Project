#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "BossCharacterBase.generated.h"

/** 보스 캐릭터 공통 부모, Sevarog 메시·idle 반복 재생 기본 설정, BP_BossCharacterBase가 상속 */
UCLASS()
class VEILBREAK_API ABossCharacterBase : public ACharacter
{
	GENERATED_BODY()

public:
	/** 생성자: Sevarog 메시·idle 자동 반복 재생 기본값 설정 */
	ABossCharacterBase();
};
