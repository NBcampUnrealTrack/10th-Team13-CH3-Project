#include "GroundSmashAttack.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "CollisionQueryParams.h"
#include "NiagaraComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"
#include "PlayerHealthComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "TimerManager.h"
#include "Sound/SoundBase.h"


AGroundSmashAttack::AGroundSmashAttack()
{
    // 매 프레임 Tick 실행 허용!!
    PrimaryActorTick.bCanEverTick = true;
}

void AGroundSmashAttack::ActivateAttack()
{
    if (!IsAttacking())
    {
        return;
    }
    waveActive = false;
    playerHit = false;
    // Task가 Owner로 지정한 보스의 현재 위치
    AActor* Boss = GetOwner();

    const FVector Start = IsValid(Boss)
        ? Boss->GetActorLocation()
        : GetActorLocation();

    // 보스 위치에서 아래검사
    const FVector End = Start - FVector(0.0f, 0.0f, 3000.0f);

    FCollisionQueryParams QueryParams;

    // 공격 자신과 보스는 검사 대상에서 제외
    QueryParams.AddIgnoredActor(this);

    if (IsValid(Boss))
    {
        QueryParams.AddIgnoredActor(Boss);
    }

    FHitResult Hit;

    const bool FoundGround = GetWorld()->LineTraceSingleByChannel(
        Hit,
        Start,
        End,
        ECC_Visibility,
        QueryParams
    );

    if (!FoundGround)
    {
        UE_LOG(LogTemp, Warning, TEXT("Ground not found!!!!!!!!!!!!!!!!!"));

        GetWorldTimerManager().ClearTimer(AnimationTimer);
        RestoreAnimation();
        FinishAttack();
        return;
    }

    // 충돌 지점으로 이동. 원이 바닥에 묻히지 않게 올림
    SetActorLocation(Hit.ImpactPoint + FVector(0.0f, 0.0f, 5.0f));

    CurrentRadius = FMath::Max(StartRadius, 0.0f);

    // BP에 추가한 Niagara 컴포넌트 찾기
    // 이 액터의 Niagara 컴포넌트를 모두 가져오기
    GetComponents<UNiagaraComponent>(WaveEffects);

    for (UNiagaraComponent* Effect : WaveEffects)
    {
        if (IsValid(Effect))
        {
            Effect->SetVariableFloat(
                TEXT("User.Radius"),
                CurrentRadius
            );

            Effect->Activate(true);
        }
    }
    if (SmashSound)
    {
        UGameplayStatics::PlaySoundAtLocation(
            this,
            SmashSound,
            GetActorLocation()
        );
    }
    waveActive = true;

    UE_LOG(LogTemp, Log, TEXT("Wave Starte!!!!!!!!@@!!!!"));
    
    CurrentStrikeCount++;

    UE_LOG(
        LogTemp,
        Log,
        TEXT("GroundSmash Strike %d / %d"),
        CurrentStrikeCount,
        StrikeCount
    );


}
//--------------------비사ㅏㅏㅏㅏㅏㅏㅏㅏㅏㅏㅏㅇ 먼말인지 모르겠음
void AGroundSmashAttack::Tick(float DeltaTime)//파장을 조금씩 확대
{
    Super::Tick(DeltaTime);
    if (IsAttacking())
    {
        PatternElapsedTime += DeltaTime;

        if (PatternElapsedTime >= PatternDuration)
        {
            waveActive = false;

            // 남은 애니메이션 종료 취소
            GetWorldTimerManager().ClearTimer(AnimationTimer);
            GetWorldTimerManager().ClearTimer(StrikeTimer);
            GetWorldTimerManager().ClearTimer(PreparationTimer);
            RestoreAnimation();

            // 모든 링 이펙트 정지
            for (UNiagaraComponent* Effect : WaveEffects)
            {
                if (IsValid(Effect))
                {
                    Effect->DeactivateImmediate();
                }
            }

            FinishAttack();
            return;
        }
    }
    if (animationRunning && (!IsAttacking() || !IsValid(GetOwner())))
    {
        GetWorldTimerManager().ClearTimer(AnimationTimer);
        RestoreAnimation();
        CancelAttack();
    }
    if (IsAttacking() &&
        IsValid(GetOwner()) &&
        IsValid(BossMesh) &&
        CurrentStrikeCount > 0 &&
        CurrentStrikeCount < StrikeCount &&
        !animationRunning &&
        !waveActive &&
        !GetWorldTimerManager().IsTimerActive(StrikeTimer))
    {
        // 1번 찍었으면 다음 타격은 6초, 2번이면 12초
        const float NextStrikeTime =
            CurrentStrikeCount * StrikeInterval;

        // 타격 시점보다 ImpactDelay만큼 먼저 애니메이션 시작
        if (PatternElapsedTime >= NextStrikeTime - ImpactDelay &&
            PatternElapsedTime + ImpactDelay < PatternDuration)
        {
            PlaySmashAnimation();

            if (ImpactDelay <= 0.0f)
            {
                ActivateAttack();
            }
            else
            {
                GetWorldTimerManager().SetTimer(
                    StrikeTimer,
                    this,
                    &AGroundSmashAttack::ActivateAttack,
                    ImpactDelay,
                    false
                );
            }
        }
    }
    if (!waveActive)
    {
        return;
    }

    // 공격이 취소됐다면 파장도 중단
    if (!IsAttacking())
    {
        waveActive = false;

        for (UNiagaraComponent* Effect : WaveEffects)
        {
            if (IsValid(Effect))
            {
                Effect->DeactivateImmediate();
            }
        }

        return;
    }

    const float EndRadius = FMath::Max(MaxRadius, 0.0f);

    //이번에 늘어날 거리 = 초당 속도 × 이번 프레임의 시간 라고함
    const float PreviousRadius = CurrentRadius;

    CurrentRadius += FMath::Max(WaveSpeed, 1.0f) * DeltaTime;
    CurrentRadius = FMath::Min(CurrentRadius, EndRadius);//FMath::Min은 최대 반경을 넘지 않도록 제한한다고함

    // 이번 프레임에 파장이 지나간 범위 검사
    CheckPlayerHit(PreviousRadius);

    // 계산한 반경을 이펙트에 전달
    for (UNiagaraComponent* Effect : WaveEffects)
    {
        if (IsValid(Effect))
        {
            Effect->SetVariableFloat(
                TEXT("User.Radius"),
                CurrentRadius
            );
        }
    }

    const float HalfWidth = FMath::Max(WaveWidth, 0.0f) * 0.5f;
    const float InnerRadius =
        FMath::Max(CurrentRadius - HalfWidth, 0.0f);
    const float OuterRadius = CurrentRadius + HalfWidth;

    // 테스트용: 액터 위치를 중심으로 수평 원 표시
    const FVector Center = GetActorLocation();

    DrawDebugCircle(
        GetWorld(), Center, InnerRadius, 64,
        FColor::Yellow, false, -1.0f, 0, 2.0f,
        FVector::ForwardVector, FVector::RightVector, false
    );

    DrawDebugCircle(
        GetWorld(), Center, OuterRadius, 64,
        FColor::Red, false, -1.0f, 0, 2.0f,
        FVector::ForwardVector, FVector::RightVector, false
    );

    if (CurrentRadius >= EndRadius)
    {
        waveActive = false;

        for (UNiagaraComponent* Effect : WaveEffects)
        {
            if (IsValid(Effect))
            {
                Effect->DeactivateImmediate();
            }
        }

        FinishWave();
    }

}
void AGroundSmashAttack::CheckPlayerHit(float PreviousRadius)
{
    //같은 파장에 두 번 맞지 않도록
    //맞았다면 다시 검사하지 않음
    if (playerHit)
    {
        return;
    }

    ACharacter* Player =
        UGameplayStatics::GetPlayerCharacter(this, 0);

    if (!IsValid(Player))
    {
        return;
    }

    UCapsuleComponent* Capsule = Player->GetCapsuleComponent();

    UPlayerHealthComponent* Health =
        Player->FindComponentByClass<UPlayerHealthComponent>();

    if (!Capsule || !Health || Health->IsDead())
    {
        return;
    }

    const FVector PlayerLocation = Capsule->GetComponentLocation();
    const FVector WaveLocation = GetActorLocation();

    const float CapsuleRadius = Capsule->GetScaledCapsuleRadius();
    const float CapsuleHalfHeight =
        Capsule->GetScaledCapsuleHalfHeight();

    // 높이는 제외하고 수평 거리만 계산
    const float Distance =
        FVector::Dist2D(PlayerLocation, WaveLocation);

    const float HalfWidth = FMath::Max(WaveWidth, 0.0f) * 0.5f;

    const float InnerRadius =
        FMath::Max(PreviousRadius - HalfWidth, 0.0f);

    const float OuterRadius = CurrentRadius + HalfWidth;

    // 플레이어 몸 너비까지 고려해서 링과 겹치는지 확인
    if (Distance + CapsuleRadius < InnerRadius ||
        Distance - CapsuleRadius > OuterRadius)
    {
        return;
    }

    // 캐릭터 중심이 아니라 캡슐 아래쪽을 발 위치로 사용
    const float FootZ = PlayerLocation.Z - CapsuleHalfHeight;
    const float HeadZ = PlayerLocation.Z + CapsuleHalfHeight;

    const float WaveTop =
        WaveLocation.Z + FMath::Max(WaveHeight, 0.0f);

    // 파장보다 높이 점프
    if (FootZ > WaveTop || HeadZ < WaveLocation.Z)
    {
        return;
    }

    playerHit = true;

    // 부모 클래스의 Damage 값 사용
    Health->ApplyDamage(Damage);

    FVector KnockbackDirection = PlayerLocation - WaveLocation;

    // 높이 차이는 빼고 수평 방향만 사용
    KnockbackDirection.Z = 0.0f;

    // 거리에 관계없이 방향의 길이를 1로 맞춤
    KnockbackDirection = KnockbackDirection.GetSafeNormal();
   
    // 바깥쪽으로 밀어낼 속도
    FVector KnockbackVelocity = KnockbackDirection * 600.0f;

    // 위로 살짝 띄우기
    KnockbackVelocity.Z = 200.0f;

    // 플레이어에게 적용
    Player->LaunchCharacter(KnockbackVelocity, true, true);

    UE_LOG(
        LogTemp,
        Log,
        TEXT("[GroundSmash] Hit! Damage: %.1f / HP: %.1f"),
        Damage,
        Health->GetCurrentHealth()
    );
}
bool AGroundSmashAttack::StartAnimatedAttack()//애니메이션부터 시작
{
    ACharacter* Boss = Cast<ACharacter>(GetOwner());//지정한 보스
    if (!IsValid(Boss) || IsAttacking()) return false;
    BossMesh = Boss->GetMesh();

    if (!IsValid(BossMesh) || !GroundSmashMotion)//보스 메시와 애니메이션이 있는지?
    {
        UE_LOG(LogTemp, Warning, TEXT("GroundSmash Ani Choose"));
        return false;
    }
    UAnimSingleNodeInstance* Previous = BossMesh->GetSingleNodeInstance();//현재 애니메이션 검사하기
    if (!Previous)
    {
        return false;
    }
    const float Duration = GroundSmashMotion->GetPlayLength();
    if (Duration <= 0.f || ImpactDelay < 0.f || ImpactDelay >= Duration)
    {
        return false;
    }

    PreviousAnimation = Previous->GetCurrentAsset();
    previousLooping = Previous->IsLooping();
    previousPlaying = Previous->IsPlaying();
    previousRate = Previous->GetPlayRate();
    previousTime = Previous->GetCurrentTime();

    // 준비 시간이 애니메이션의 타격 시점보다 짧으면 실행 불가
    if (PreparationTime < ImpactDelay)
    {
        UE_LOG(LogTemp, Warning, TEXT("PreparationTime must be >= ImpactDelay"));
        return false;
    }

    CurrentStrikeCount = 0;

    // 준비 시간이 지나면 0부터 공격 실행 시간을 계산
    PatternElapsedTime = -PreparationTime;

    // 준비 시간이 끝나면 첫 파장 발동
    WarningDuration = PreparationTime;
    StartAttack();

    if (!IsAttacking())
    {
        return false;
    }
    
    // 첫 파장이 나올 때 애니메이션의 타격 순간도 맞추기
    const float AnimationDelay = PreparationTime - ImpactDelay;

    if (AnimationDelay <= 0.f)
    {
        PlaySmashAnimation();
    }
    else
    {
        GetWorldTimerManager().SetTimer(
            PreparationTimer,
            this,
            &AGroundSmashAttack::PlaySmashAnimation,
            AnimationDelay,
            false
        );
    }

    return true;
}

void AGroundSmashAttack::RestoreAnimation()
{
    if (!animationRunning) return;
    animationRunning = false;
    if (!IsValid(BossMesh)) return;

    UAnimSingleNodeInstance* Current = BossMesh->GetSingleNodeInstance();
    if (!Current || Current->GetCurrentAsset() != GroundSmashMotion) return;

    BossMesh->PlayAnimation(PreviousAnimation, previousLooping);
    BossMesh->SetPlayRate(previousRate);
    BossMesh->SetPosition(previousTime, false);
    if (UAnimSingleNodeInstance* Restored = BossMesh->GetSingleNodeInstance())
    {
        Restored->SetPlaying(previousPlaying);
    }
}

void AGroundSmashAttack::FinishAnimation()
{
    //스킬 끝나면 애니메이션 복귀
    RestoreAnimation();
}

void AGroundSmashAttack::FinishWave()
{
    //WAVE 한번이 끝났다는 표시
    waveFinished = true;
}

void AGroundSmashAttack::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    GetWorldTimerManager().ClearTimer(AnimationTimer);
    GetWorldTimerManager().ClearTimer(StrikeTimer);
    GetWorldTimerManager().ClearTimer(PreparationTimer);
    CancelAttack();
    RestoreAnimation();
    Super::EndPlay(EndPlayReason);
}

void AGroundSmashAttack::PlaySmashAnimation()
{
    animationRunning = true;
    waveFinished = false;

    BossMesh->PlayAnimation(GroundSmashMotion, false);
    BossMesh->SetPlayRate(1.f);

    const float Duration = GroundSmashMotion->GetPlayLength();

    GetWorldTimerManager().SetTimer(
        AnimationTimer,
        this,
        &AGroundSmashAttack::FinishAnimation,
        Duration,
        false
    );
    if (VoiceSound)
    {
        UGameplayStatics::PlaySoundAtLocation(
            this,
            VoiceSound,
            BossMesh->GetComponentLocation()
        );
    }
}