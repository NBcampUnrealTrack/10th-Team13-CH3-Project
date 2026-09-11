// Fill out your copyright notice in the Description page of Project Settings.


#include "CenterProjectile.h"
#include "BossMagicAttackActor.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"

void ACenterProjectile::ActivateAttack()
{
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

    // 투사체 사이의 각도
    const float AngleStep = 360.0f / ProjectileCount;

    for (int i = 0; i < ProjectileCount; i++)
    {
        // 보스의 수평 방향을 기준으로 회전
        const float Angle = Boss->GetActorRotation().Yaw + AngleStep * i;
        const FVector Direction = FRotator(0.0f, Angle, 0.0f).Vector();

        const FVector SpawnLocation =
            Boss->GetActorLocation() + Direction * 100.0f;

        const FVector TargetLocation =
            SpawnLocation + Direction * 1500.0f;

        ABossMagicAttackActor* Projectile =
            GetWorld()->SpawnActor<ABossMagicAttackActor>(
                ProjectileClass,
                SpawnLocation,
                Direction.Rotation(),
                Params
            );

        if (IsValid(Projectile))
        {
            Projectile->LaunchAt(TargetLocation);
        }
    }

    FinishAttack();
}