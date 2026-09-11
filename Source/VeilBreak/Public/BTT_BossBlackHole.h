#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTT_BossBlackHole.generated.h"

class ABossBlackHole;
class UAnimSequence;
class USoundBase;

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

	/**
	 * 소켓 위치 기준 오프셋. 소켓 자신의 로컬 좌표계 기준이라, 손이 어느 방향을 향하든
	 * 항상 "손바닥 기준 이 방향"으로 일정하게 띄워짐. X: 앞, Y: 옆, Z: 위.
	 * 소켓이 손 안쪽(뼈 원점)에 있어서 메시랑 겹쳐 보일 때 이 값으로 띄우면 됨.
	 */
	UPROPERTY(EditAnywhere, Category = "BlackHole")
	FVector SpawnOffset = FVector(0.f, 0.f, 15.f);

	/** 블랙홀 시전 시 재생할 애니메이션. 비워두면 애니메이션 없이 스폰만 됨 */
	UPROPERTY(EditAnywhere, Category = "BlackHole|Presentation")
	TObjectPtr<UAnimSequence> CastAnimation;

	/** 블랙홀 종료 후 되돌아갈 애니메이션(보통 Idle). 비워두면 캐스트 애니메이션 마지막 프레임에 멈춰있음 */
	UPROPERTY(EditAnywhere, Category = "BlackHole|Presentation")
	TObjectPtr<UAnimSequence> IdleAnimationAfter;

	/** 블랙홀 시전 시 재생할 사운드. 비워두면 소리 없음 */
	UPROPERTY(EditAnywhere, Category = "BlackHole|Presentation")
	TObjectPtr<USoundBase> ActivationSound;

	/** 사운드 크기 배율. 1.0이 원본 크기, 0.5면 절반, 2.0이면 두 배 */
	UPROPERTY(EditAnywhere, Category = "BlackHole|Presentation", meta = (ClampMin = "0.0"))
	float ActivationSoundVolume = 1.f;

private:
	/** 이 Task 하나가 실행되는 동안 유지해야 하는 임시 데이터 */
	struct FBTBlackHoleMemory
	{
		TWeakObjectPtr<ABossBlackHole> SpawnedBlackHole;
		TWeakObjectPtr<class ABossCharacterBase> Boss;
		float ElapsedTime = 0.f;
	};

	virtual uint16 GetInstanceMemorySize() const override { return sizeof(FBTBlackHoleMemory); }

	void CleanUpBlackHole(FBTBlackHoleMemory* Memory);
};