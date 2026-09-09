#include "AttackRangeBase.h"
#include "TimerManager.h"

void AAttackRangeBase::StartAttack()
{
    // 이미 공격 중이면 스탑
    if (isAttacking)
    {
        return;
    }
    //공격 중
    isAttacking = true;

    UE_LOG(LogTemp, Log, TEXT("RangeAttack - BossSkillStart!!!!@@@@@"));

    // 예고 시간 없으면 겅격
    if (WarningDuration <= 0.0f)
    {
        ActivateAttack();
        return;
    }

    // 예고 시간 있으면 공격 대기
    GetWorldTimerManager().SetTimer(
        WarningTimer,//예약을 나중에 취소하거나 확인할 때 쓸 핸들
        this,//함수를 실행할 대상인 현재 액터
        &AAttackRangeBase::ActivateAttack,//나중에 실행할 함수의 주소
        WarningDuration,//기다릴 시간
        false//반복하지 않고 한 번만 실행
    );
}

void AAttackRangeBase::ActivateAttack()
{
    UE_LOG(LogTemp, Log, TEXT("RangeAttack - Attack!!@@@@@"));

    // 지금은 실제 공격 없이 바로 종료
    FinishAttack();
}

void AAttackRangeBase::FinishAttack()
{
    GetWorldTimerManager().ClearTimer(WarningTimer);
    isAttacking = false;

    UE_LOG(LogTemp, Log, TEXT("RangeAttack -  AttackEnd@@@@@"));
}

void AAttackRangeBase::CancelAttack()
{
    // 예약된 발동을 취소
    GetWorldTimerManager().ClearTimer(WarningTimer);
    isAttacking = false;

    UE_LOG(LogTemp, Log, TEXT("RangeAttack - AttackCancel!!@@@@@"));
}
