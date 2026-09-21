// Fill out your copyright notice in the Description page of Project Settings.


#include "CenterProjectile.h"
#include "CenterMagicProjectile.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "TimerManager.h"
#include "Animation/AnimSequence.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "BossCharacterBase.h"


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

    // 수평 -> 위 -> 아래 순서로 반복

    float Pitch = 0.0f;

    if (FireCount % 3 == 1)
    {
        Pitch = VerticalAngle;
    }
    else if (FireCount % 3 == 2)
    {
        Pitch = VerticalAngle * 2.0f;
    }
    PlayFireAnimation();

    if (FireSound)
    {
        UGameplayStatics::PlaySoundAtLocation(
            this,
            FireSound,
            Boss->GetActorLocation()
        );
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
    
    //처음에 한번 대사용
    if (StartVoice && IsValid(GetOwner()))
    {
        UGameplayStatics::PlaySoundAtLocation(
            this,
            StartVoice,
            GetOwner()->GetActorLocation()
        );
    }

    // 첫 발사
    FireProjectiles();

    if (!IsAttacking())
    {
        return;
    }

    //패턴 종료
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
    GetWorldTimerManager().ClearTimer(AnimationTimer);
    RestoreAnimation();
    FinishAttack();

}

void ACenterProjectile::EndPlay(
    const EEndPlayReason::Type EndPlayReason
)
{
    GetWorldTimerManager().ClearTimer(FireTimer);
    GetWorldTimerManager().ClearTimer(PatternTimer);
    GetWorldTimerManager().ClearTimer(AnimationTimer);
    RestoreAnimation();
    CancelAttack();

    Super::EndPlay(EndPlayReason);
}

void ACenterProjectile::PlayFireAnimation()
{
    ABossCharacterBase* Boss =
        Cast<ABossCharacterBase>(GetOwner());

    if (!IsValid(Boss) || !FireMotion || AnimationPlaying)
    {
        return;
    }

    const float Duration = FireMotion->GetPlayLength();

    if (Duration <= 0.0f)
    {
        return;
    }

    AnimationPlaying = true;
    Boss->SetCenterProjectileAnimating(true);

    GetWorldTimerManager().SetTimer(
        AnimationTimer,
        this,
        &ACenterProjectile::RestoreAnimation,
        Duration,
        false
    );
}

void ACenterProjectile::RestoreAnimation()
{
    if (!AnimationPlaying)
    {
        return;
    }

    AnimationPlaying = false;
    GetWorldTimerManager().ClearTimer(AnimationTimer);

    ABossCharacterBase* Boss =
        Cast<ABossCharacterBase>(GetOwner());

    if (IsValid(Boss))
    {
        Boss->SetCenterProjectileAnimating(false);
    }
}