// Fill out your copyright notice in the Description page of Project Settings.


#include "BTT_CenterProjectile.h"
#include "CenterProjectile.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "BossMagicAttackActor.h"




UBTT_CenterProjectile::UBTT_CenterProjectile()
{
    NodeName = TEXT("Center Projectile");//BT에 표시되는 이름

    bNotifyTick = true;//종료를 계속 확인
    bCreateNodeInstance = true; //ActiveAttack 섞이지 않게함
}
EBTNodeResult::Type UBTT_CenterProjectile::ExecuteTask(
    UBehaviorTreeComponent& OwnerComp,
    uint8* NodeMemory
)
{
    //전공격 정리
    CleanupAttack();

    /*
    AI 컨트롤러 찾기
    OwnerComp는 이Task를 실행하고있는 BT 컴포넌트임 여기서 BT를 사용하는 AI 컨트롤러를 가져옴
    */
    AAIController* Controller = OwnerComp.GetAIOwner();
    if (!IsValid(Controller))//사용할 수없는 상태면 Task 종료
    {
        return EBTNodeResult::Failed;
    }

    APawn* Boss = Controller->GetPawn();//보스 찾기
    if (!IsValid(Boss) || !AttackClass)
    {
        return EBTNodeResult::Failed;
    }

    // 누가 생성한 공격인지 지정
    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = Boss;//Owner 이 공격의 소유자를 보스로 지정
    SpawnParams.Instigator = Boss;//Instigator 이 공격을 일으킨 Pawn을 보스로 지정
    SpawnParams.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn; //생성 위치에 다른 물체가 겹쳐 있어도 액터를 생성

    //공격 생성 Instantiate랑 비슷함
    ActiveAttack = Boss->GetWorld()->SpawnActor<ACenterProjectile>(
        AttackClass,//어떤 BP로 만들지
        Boss->GetActorLocation(),//어디에
        Boss->GetActorRotation(),//어느 방향
        SpawnParams//생성 설정
    );

    //생성성공 확인하고 공격
    if (!IsValid(ActiveAttack))
    {
        return EBTNodeResult::Failed;
    }

    ActiveAttack->StartAttack();

    //BT가 다음 행동으로 넘어가도 되는지
    return EBTNodeResult::InProgress;
}

void UBTT_CenterProjectile::TickTask(
    UBehaviorTreeComponent& OwnerComp,
    uint8* NodeMemory,
    float DeltaSeconds
)
{
    // 공격이나 공격 주인이 사라졌으면 실패 처리
    //!IsValid 사용할 수 없는 객체 확인
    if (!IsValid(ActiveAttack) ||//ActiveAttack 생성한 중앙 공격 액터
        !IsValid(ActiveAttack->GetOwner()))//ActiveAttack ->GetOwner()-> 생성할 때 소유자로 지정했던 보스
    {
        CleanupAttack();
        FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
        return;
    }

    // 아직 공격 중이면 기다림
    if (ActiveAttack->IsAttacking())
    {
        return;
    }

    // 공격이 끝났으면 완료
    CleanupAttack();
    FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
}

//오브젝트를 Destroy하고 보관하던 변수를 null로 만듬
void UBTT_CenterProjectile::CleanupAttack()
{
    if (IsValid(ActiveAttack))
    {
        if (ActiveAttack->IsAttacking())
        {
            ActiveAttack->CancelAttack();
        }

        ActiveAttack->Destroy();
    }

    ActiveAttack = nullptr;
}

EBTNodeResult::Type UBTT_CenterProjectile::AbortTask(
    UBehaviorTreeComponent& OwnerComp,
    uint8* NodeMemory
)
{
    CleanupAttack();//진행중인 공격 취소

    return EBTNodeResult::Aborted;//공격 중단하고 다른 행동으로
}