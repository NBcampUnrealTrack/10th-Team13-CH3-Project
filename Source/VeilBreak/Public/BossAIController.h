#pragma once
#include "CoreMinimal.h"
#include "AIController.h"
#include "BossAIController.generated.h"

// 보스 BT 실행과 최초 마법 공격 대기값 초기화
UCLASS()
class VEILBREAK_API ABossAIController : public AAIController
{
    GENERATED_BODY()
public:
    // BT_BossMain 기본 에셋 지정
    ABossAIController();
    // 보스 사망 시 이동과 Behavior Tree 실행 정지
    void StopBossBehavior();
protected:
    // 보스 소유 시 BB 초기화 후 BT 반복 실행
    virtual void OnPossess(APawn* InPawn) override;
    // 보스 소유 해제 시 BT 정지
    virtual void OnUnPossess() override;
    // 에디터에서 편집하는 보스 메인 트리
    UPROPERTY(EditDefaultsOnly, Category="Boss|AI")
    TObjectPtr<class UBehaviorTree> BTAsset;
};
