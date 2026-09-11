// Fill out your copyright notice in the Description page of Project Settings.


#include "CenterProjectile.h"
#include "CenterMagicProjectile.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "TimerManager.h"


void ACenterProjectile::FireProjectiles() 
{
    if (!IsAttacking())//새 투사체를 발사X
    {
        return;
    }
    AActor* Boss = GetOwner();

    if (!IsValid(Boss) || !ProjectileClass || ProjectileCount <= 0)
    {
        FinishAttack();
        return;
    }

    FActorSpawnParameters Params;
    Params.Owner = Boss;
    Params.Instigator = Cast<APawn>(Boss);
    Params.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    //사이 각도
    const float AngleStep = 360.0f / ProjectileCount;
    // 발사할 때마다 수평 방향 조금씩 회전
    const float RotationOffset = FireCount * RotationPerShot;

    // 수평 → 위 → 아래 순서로 반복

    float Pitch = 0.0f;

    if (FireCount % 3 == 1)
    {
        Pitch = VerticalAngle;
    }
    else if (FireCount % 3 == 2)
    {
        Pitch = VerticalAngle * 2.0f;
    }

    for (int i = 0; i < ProjectileCount; i++)
    {
        // 보스의 수평 방향을 기준으로 회전
        const float Angle =
            Boss->GetActorRotation().Yaw
            + RotationOffset
            + AngleStep * i;

        const FVector Direction =
            FRotator(Pitch, Angle, 0.0f).Vector();

        const FVector SpawnLocation =
            Boss->GetActorLocation() + Direction * 100.0f;

        ACenterMagicProjectile* Projectile =
            GetWorld()->SpawnActor<ACenterMagicProjectile>(
                ProjectileClass,
                SpawnLocation,
                Direction.Rotation(),
                Params
            );

        if (IsValid(Projectile))
        {
            Projectile->Launch(Direction);
        }
    }
    FireCount++;
}
void ACenterProjectile::ActivateAttack()
{
    if (FireInterval <= 0.0f || PatternDuration <= 0.0f)
    {
        FinishAttack();
        return;
    }
    FireCount = 0;

    // 첫 발사는 즉시 실행
    FireProjectiles();

    if (!IsAttacking())
    {
        return;
    }

    // 전체 패턴 종료 예약
    GetWorldTimerManager().SetTimer(
        PatternTimer,
        this,
        &ACenterProjectile::EndPattern,
        PatternDuration,
        false
    );

    // 일정 간격으로 반복 발사
    GetWorldTimerManager().SetTimer(
        FireTimer,
        this,
        &ACenterProjectile::FireProjectiles,
        FireInterval,
        true
    );
}
void ACenterProjectile::EndPattern()
{
    GetWorldTimerManager().ClearTimer(FireTimer);
    GetWorldTimerManager().ClearTimer(PatternTimer);

    FinishAttack();
}

void ACenterProjectile::EndPlay(
    const EEndPlayReason::Type EndPlayReason
)
{
    GetWorldTimerManager().ClearTimer(FireTimer);
    GetWorldTimerManager().ClearTimer(PatternTimer);

    CancelAttack();

    Super::EndPlay(EndPlayReason);
}