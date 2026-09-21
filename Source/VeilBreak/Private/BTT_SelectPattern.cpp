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
	
	//3페이즈 처음에 중앙 공격
	if (Stat->GetCurrentPhase() == EBossPhase::Phase3 &&
		!Boss->HasFinishedPhase3Opening() &&
		!Stat->IsDead())
	{
		// 다른 패턴이 진행 중이면 아직 선택하지 않음
		if (Boss->IsPatternRunning())
		{
			return EBTNodeResult::Failed;
		}

		Boss->PreparePatternAnimation(EBossPattern::CenterProjectile);

		Blackboard->SetValueAsInt(
			TEXT("SelectedPattern"),
			static_cast<int32>(EBossPattern::CenterProjectile)
		);

		UE_LOG(LogTemp, Log, TEXT("Phase3 opening: CenterProjectile"));

		return EBTNodeResult::Succeeded;
	}

	AActor* TargetActor = Cast<AActor>(Blackboard->GetValueAsObject(TEXT("TargetActor")));
	// 모든 페이즈의 기본 패턴 후보
	TArray<EBossPattern> Candidates = { EBossPattern::MagicAttack, EBossPattern::FallingRock };
	// 20초 무피격과 120초 쿨타임 조건을 만족할 때만 발악 후보 추가
	if (Boss->CanStartBerserk(TargetActor)) Candidates.Add(EBossPattern::Berserk);
	// Phase1에서만 추적 소용돌이 후보 추가
	if (Boss->CanStartVortex(TargetActor)) Candidates.Add(EBossPattern::Vortex);
	// Phase2 전용 블랙홀 후보 추가
	if (Stat->GetCurrentPhase() == EBossPhase::Phase2) Candidates.Add(EBossPattern::BlackHole);
	// Phase3에서만 땅찍기와 중앙 광역 투사체 후보 추가
	if (Boss->CanStartGroundSmash(TargetActor))
	{
		Candidates.Add(EBossPattern::GroundSmash);
	}

	if (Boss->CanStartCenterProjectile(TargetActor))
	{
		Candidates.Add(EBossPattern::CenterProjectile);
	}

	// 후보 중 하나를 선택해 BT Selector 분기에 전달
	const EBossPattern SelectedPattern = Candidates[FMath::RandRange(0, Candidates.Num() - 1)];
	Boss->PreparePatternAnimation(SelectedPattern);

	UE_LOG(LogTemp, Warning, TEXT("Selected Pattern: %d"),
		static_cast<int32>(SelectedPattern));

	Blackboard->SetValueAsInt(TEXT("SelectedPattern"), static_cast<int32>(SelectedPattern));
	return EBTNodeResult::Succeeded;
}
