#include "BTS_UpdateBossContext.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Kismet/GameplayStatics.h"

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
		Blackboard->ClearValue(TEXT("TargetActor"));
		Blackboard->SetValueAsFloat(TEXT("TargetDistance"), 0.0f);
		return;
	}

	// 플레이어 참조와 현재 보스 거리 저장
	Blackboard->SetValueAsObject(TEXT("TargetActor"), PlayerPawn);
	Blackboard->SetValueAsFloat(TEXT("TargetDistance"), FVector::Distance(BossPawn->GetActorLocation(), PlayerPawn->GetActorLocation()));
}
