#pragma once

#include "CoreMinimal.h"
#include "BossPatternActorBase.h"
#include "BossBerserkActor.generated.h"

// 발악 중 플레이어 사격으로 파괴되는 구체, BP_BossBerserk의 부모
UCLASS(Blueprintable)
class VEILBREAK_API ABossBerserkActor : public ABossPatternActorBase
{
	GENERATED_BODY()

public:
	// 사격으로 파괴되는 발악 구체 기본 구성
	ABossBerserkActor();
	// 표준 PointDamage를 받아 구체 파괴와 보스 통지
	virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

protected:
	// 사격 판정용 구형 콜리전
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Boss|Berserk")
	TObjectPtr<class USphereComponent> HitCollision;
	// 발악 구체 외형 메시
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Boss|Berserk")
	TObjectPtr<class UStaticMeshComponent> OrbMesh;

private:
	// 중복 피격 통지 방지
	bool bDestroyedByDamage = false;
};
