#include "BossAIController.h"
#include "BossCharacterBase.h"
#include "BTS_UpdateBossContext.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BTCompositeNode.h"
#include "BrainComponent.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

// 기본 BT 에셋 연결
ABossAIController::ABossAIController()
{
    // 보스 메인 BT
    static ConstructorHelpers::FObjectFinder<UBehaviorTree> Tree(TEXT("/Game/Boss/AI/BT_BossMain.BT_BossMain"));
    if (Tree.Succeeded()) BTAsset = Tree.Object;
}

// 보스 사망 시 이동 요청과 Behavior Tree 실행 즉시 정지
void ABossAIController::StopBossBehavior()
{
    StopMovement();
    if (BrainComponent) BrainComponent->StopLogic(TEXT("Boss died"));
}

// Blackboard 초기화 후 BT의 Wait → 패턴 선택 반복 시작
void ABossAIController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);
    // BT 루트에 대상 갱신 서비스가 없으면 런타임으로 연결
    if (BTAsset && BTAsset->RootNode)
    {
        const bool bHasContextService = BTAsset->RootNode->Services.ContainsByPredicate([](const UBTService* Service)
        {
            return Service && Service->IsA<UBTS_UpdateBossContext>();
        });
        if (!bHasContextService)
        {
            BTAsset->RootNode->Services.Add(NewObject<UBTS_UpdateBossContext>(BTAsset->RootNode, NAME_None, RF_Transient));
        }
    }
    // 소유한 보스와 초기화할 블랙보드
    ABossCharacterBase* Boss = Cast<ABossCharacterBase>(InPawn);
    UBlackboardComponent* BossBlackboard = GetBlackboardComponent();
    if (Boss && BTAsset && BTAsset->BlackboardAsset && UseBlackboard(BTAsset->BlackboardAsset, BossBlackboard))
    {
        // BT 시작 시점의 플레이어 대상을 먼저 저장
        if (APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0))
        {
            BossBlackboard->SetValueAsObject(TEXT("TargetActor"), PlayerPawn);
            BossBlackboard->SetValueAsFloat(TEXT("TargetDistance"), FVector::Distance(Boss->GetActorLocation(), PlayerPawn->GetActorLocation()));
        }
        RunBehaviorTree(BTAsset);
    }
}

// 소유 해제 시 진행 중 트리 종료
void ABossAIController::OnUnPossess()
{
    StopBossBehavior();
    Super::OnUnPossess();
}

