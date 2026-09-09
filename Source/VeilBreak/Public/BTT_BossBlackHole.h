#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTT_BossBlackHole.generated.h"

class ABossBlackHole;

/**
 * BT Task - 블랙홀 발동
 * 실행되면 보스 머리 위에 BossBlackHole을 스폰하고, ActiveDuration만큼 유지하다가
 * 자동으로 종료(Deactivate + Destroy)하고 Task를 Succeeded로 끝낸다.
 *
 * BT 입장에서 이 노드는 "블랙홀 다 쓸 때까지" 계속 실행 중(InProgress) 상태를 유지하는
 * '레이턴트(latent) Task'다. Wait 노드처럼 몇 초간 트리 흐름을 붙잡아두는 노드라고 보면 된다.
 */
UCLASS()
class VEILBREAK_API UBTT_BossBlackHole : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTT_BossBlackHole();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

protected:
	/** 스폰할 블랙홀 클래스. 필요하면 BP 자식을 만들어서 여기에 지정 가능 */
	UPROPERTY(EditAnywhere, Category = "BlackHole")
	TSubclassOf<ABossBlackHole> BlackHoleClass;

	/** 블랙홀을 유지할 시간(초). BossBlackHole 자체 Duration 값과 굳이 맞출 필요는 없음 - 이쪽이 최종 기준 */
	UPROPERTY(EditAnywhere, Category = "BlackHole")
	float ActiveDuration = 3.f;

	/** 보스 메시에서 블랙홀을 스폰할 소켓 이름. 소켓이 없으면 캡슐 상단으로 자동 대체됨 */
	UPROPERTY(EditAnywhere, Category = "BlackHole")
	FName SpawnSocketName = TEXT("head");

private:
	/** 이 Task 하나가 실행되는 동안 유지해야 하는 임시 데이터 */
	struct FBTBlackHoleMemory
	{
		TWeakObjectPtr<ABossBlackHole> SpawnedBlackHole;
		float ElapsedTime = 0.f;
	};

	virtual uint16 GetInstanceMemorySize() const override { return sizeof(FBTBlackHoleMemory); }

	void CleanUpBlackHole(FBTBlackHoleMemory* Memory);
};
