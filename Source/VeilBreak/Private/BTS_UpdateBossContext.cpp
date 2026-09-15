#include "BTS_UpdateBossContext.h"
#include "AIController.h"
#include "BossCharacterBase.h"
#include "BossStatComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Navigation/PathFollowingComponent.h"

// 대상 확인 주기와 BT 표시명 설정
UBTS_UpdateBossContext::UBTS_UpdateBossContext()
{
	NodeName = TEXT("Update Boss Context");
	Interval = 0.1f;
	RandomDeviation = 0.0f;
}

// Player 0을 Blackboard 대상과 거리 값으로 반영
void UBTS_UpdateBossContext::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);
	AAIController* Controller = OwnerComp.GetAIOwner();
	APawn* BossPawn = Controller ? Controller->GetPawn() : nullptr;
	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	APawn* PlayerPawn = BossPawn ? UGameplayStatics::GetPlayerPawn(BossPawn, 0) : nullptr;
	if (!Blackboard || !BossPawn) return;

	// 플레이어 부재 시 대상과 거리 초기화
	if (!PlayerPawn)
	{
		if (ABossCharacterBase* Boss = Cast<ABossCharacterBase>(BossPawn)) Boss->UpdatePlayerNoDamageState(nullptr);
		Controller->StopMovement();
		bIsChasingTarget = false;
		Blackboard->ClearValue(TEXT("TargetActor"));
		Blackboard->SetValueAsFloat(TEXT("TargetDistance"), 0.0f);
		return;
	}

	// 플레이어 참조와 현재 보스 거리 저장
	const float TargetDistance = FVector::Distance(BossPawn->GetActorLocation(), PlayerPawn->GetActorLocation());
	Blackboard->SetValueAsObject(TEXT("TargetActor"), PlayerPawn);
	Blackboard->SetValueAsFloat(TEXT("TargetDistance"), TargetDistance);

	// 시전 중 이동 중지
	ABossCharacterBase* Boss = Cast<ABossCharacterBase>(BossPawn);
	if (!Boss) return;
	// 플레이어 컴포넌트의 공개 체력을 관찰해 발악용 무피격 시간 갱신
	Boss->UpdatePlayerNoDamageState(PlayerPawn);
	// 체력 컴포넌트의 현재 페이즈를 Blackboard에 반영
	if (const UBossStatComponent* Stat = Boss->GetBossStatComponent()) Blackboard->SetValueAsInt(TEXT("CurrentPhase"), static_cast<int32>(Stat->GetCurrentPhase()));
	if (Boss->IsPatternRunning())
	{
		Controller->StopMovement();
		return;
	}

	// 3000cm 초과 시 추적 시작, 2000cm 도달 시 멈춤
	const bool bShouldChase = bIsChasingTarget ? TargetDistance > Boss->GetChaseStopDistance() : TargetDistance > Boss->GetChaseStartDistance();
	if (bShouldChase)
	{
		// 완료 후에도 타겟이 3000cm 초과 시 재이동
		if (Controller->GetMoveStatus() != EPathFollowingStatus::Moving)
		{
			Controller->MoveToActor(PlayerPawn, Boss->GetChaseStopDistance());
		}
		bIsChasingTarget = true;
	}
	else if (bIsChasingTarget)
	{
		Controller->StopMovement();
		bIsChasingTarget = false;
	}
}
