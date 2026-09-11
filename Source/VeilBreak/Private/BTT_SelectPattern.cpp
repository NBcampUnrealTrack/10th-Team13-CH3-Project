#include "BTT_SelectPattern.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BossCharacterBase.h"
#include "BossStatComponent.h"

// BT 노드 표시명 설정
UBTT_SelectPattern::UBTT_SelectPattern()
{
	NodeName = TEXT("Select Pattern");
}

// 현재 페이즈의 허용 패턴 하나를 Blackboard SelectedPattern에 저장
EBTNodeResult::Type UBTT_SelectPattern::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* Controller = OwnerComp.GetAIOwner();
	ABossCharacterBase* Boss = Controller ? Cast<ABossCharacterBase>(Controller->GetPawn()) : nullptr;
	UBossStatComponent* Stat = Boss ? Boss->GetBossStatComponent() : nullptr;
	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	if (!Stat || !Blackboard) return EBTNodeResult::Failed;

	// 모든 페이즈의 기본 패턴 후보
	TArray<EBossPattern> Candidates = { EBossPattern::MagicAttack, EBossPattern::FallingRock, EBossPattern::Berserk };
	// Phase2 전용 블랙홀 후보 추가
	if (Stat->GetCurrentPhase() == EBossPhase::Phase2) Candidates.Add(EBossPattern::BlackHole);
	// 후보 중 하나를 선택해 BT Selector 분기에 전달
	const EBossPattern SelectedPattern = Candidates[FMath::RandRange(0, Candidates.Num() - 1)];
	Blackboard->SetValueAsInt(TEXT("SelectedPattern"), static_cast<int32>(SelectedPattern));
	return EBTNodeResult::Succeeded;
}
