#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BossData.h"
#include "BTS_BossFlyChase.generated.h"

/**
 * BT Service - 공중 추격/정지/도망
 * 최상단 Sequence에 붙여두면 항상 배경에서 계속 도는 서비스.
 * TargetDistance 블랙보드 값(BTS_UpdateBossContext가 이미 갱신해주는 값)을 계속 감시하면서
 * - FleeDistance(1000)보다 가까우면 플레이어 반대 방향으로 도망
 * - ChaseStartDistance(3000)보다 멀면 추격
 * - 그 사이(1500~3000)면 제자리에 멈춰서 떠있음
 *
 * 보스는 이 Service가 붙어있는 동안 항상 Flying 모드로 유지된다.
 */
UCLASS()
class VEILBREAK_API UBTS_BossFlyChase : public UBTService
{
	GENERATED_BODY()

public:
	UBTS_BossFlyChase();

protected:
	virtual void OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void OnCeaseRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	/** 비행 이동 속도 */
	UPROPERTY(EditAnywhere, Category = "Flight")
	float ChaseSpeed = 300.f;

	/** 이 거리보다 멀어지면 추격을 시작함 */
	UPROPERTY(EditAnywhere, Category = "Flight", meta = (ClampMin = "0.0"))
	float ChaseStartDistance = 3000.f;

	/** 이 거리 이하로 가까워지면 추격을 멈춤 (ChaseStartDistance보다 작아야 함, 안 그러면 계속 떨림) */
	UPROPERTY(EditAnywhere, Category = "Flight", meta = (ClampMin = "0.0"))
	float ChaseStopDistance = 2000.f;

	/** 플레이어 기준 몇 uu 위 높이를 유지하며 날지.
	 * 주의: 실제 3D 거리는 항상 최소 이 값만큼은 나오게 되므로,
	 * FleeDistance/ChaseStopDistance/ChaseStartDistance는 전부 이 값보다 커야 정상 작동함 */
	UPROPERTY(EditAnywhere, Category = "Flight")
	float FlightAltitude = 1000.f;

	/** 목적지에 이 정도 거리 안에 들어오면 도착한 걸로 침 */
	UPROPERTY(EditAnywhere, Category = "Flight")
	float AcceptanceRadius = 150.f;

	/** 이 거리보다 가까워지면 플레이어 반대 방향으로 도망감 */
	UPROPERTY(EditAnywhere, Category = "Flight", meta = (ClampMin = "0.0"))
	float FleeDistance = 1300.f;

	/** 도망가다가 이 거리 이상 벌어지면 멈춤 (FleeDistance보다 커야 함, 안 그러면 계속 떨림) */
	UPROPERTY(EditAnywhere, Category = "Flight", meta = (ClampMin = "0.0"))
	float FleeStopDistance = 1700.f;

	/** 이 페이즈일 때만 실제로 비행/추격 로직이 작동함 (그 외 페이즈에선 아무것도 안 함).
	 * BTD_CheckPhase랑 똑같이 EBossPhase 타입으로 둬서, 숫자로 헷갈릴 일 없이
	 * 드롭다운에서 "Phase 2"처럼 이름으로 바로 고를 수 있게 함 */
	UPROPERTY(EditAnywhere, Category = "Flight")
	EBossPhase RequiredPhase = EBossPhase::Phase2;

	/** 페이즈 값을 읽어올 블랙보드 키 이름 */
	UPROPERTY(EditAnywhere, Category = "Flight")
	FName PhaseKeyName = TEXT("CurrentPhase");

private:
	bool bIsChasing = false;
	bool bIsFleeing = false;

	/** 2페이즈에 처음 들어온 순간 한 번은 무조건 떠오르게 하기 위한 플래그 (안 그러면 그 순간 플레이어가 가까이 있으면 그냥 바닥에 계속 서있게 됨) */
	bool bHasLiftedOff = false;
};