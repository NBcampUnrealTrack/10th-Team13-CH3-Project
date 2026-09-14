#include "BossMagicAttackActor.h"
#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "NiagaraFunctionLibrary.h"
#include "UObject/ConstructorHelpers.h"

// 이동용 루트·구형 피격 콜리전·Dark 반복 이펙트 생성
ABossMagicAttackActor::ABossMagicAttackActor()
{
    PrimaryActorTick.bCanEverTick = true;
    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    // 반지름 25cm, Pawn만 겹침 처리
    HitCollision = CreateDefaultSubobject<USphereComponent>(TEXT("HitCollision"));
    SetRootComponent(HitCollision);
    HitCollision->InitSphereRadius(25.f);
    HitCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    HitCollision->SetCollisionObjectType(ECC_WorldDynamic);
    HitCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
    HitCollision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    HitCollision->SetGenerateOverlapEvents(true);
    SceneRoot->SetupAttachment(HitCollision);
    FireEffect = CreateDefaultSubobject<UNiagaraComponent>(TEXT("FireEffect"));
    FireEffect->SetupAttachment(SceneRoot);
    // 비행 외형용 Dark 반복 시스템
    static ConstructorHelpers::FObjectFinder<UNiagaraSystem> Fire(TEXT("/Game/SlashTrail_SoftTofu/Niagara/Dark/NS_SlashTrail_Dark_Loop.NS_SlashTrail_Dark_Loop"));
    // 도착 지점의 AuraFX Mystic 시스템
    static ConstructorHelpers::FObjectFinder<UNiagaraSystem> Hit(TEXT("/Game/SlashTrail_SoftTofu/Niagara/Mystic/NS_AuraFX_Mystic.NS_AuraFX_Mystic"));
    if (Fire.Succeeded()) FireEffect->SetAsset(Fire.Object);
    if (Hit.Succeeded()) ArrivalEffect = Hit.Object;
    InitialLifeSpan = 30.f;
}

// 목표 고정 후 시각적 진행 방향과 최대 수명 설정
void ABossMagicAttackActor::LaunchAt(const FVector& InTarget)
{
    TargetLocation = InTarget;
    bLaunched = true;
    SetActorRotation((TargetLocation - GetActorLocation()).Rotation());
    // 발사 시점부터 Dark 반복 이펙트 재생
    FireEffect->Activate(true);
    SetLifeSpan(FVector::Distance(GetActorLocation(), TargetLocation) / FMath::Max(Speed, 1.f) + 2.f);
}

// 프레임 이동 거리를 제한, 구형 충돌체와 함께 이동
void ABossMagicAttackActor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!bLaunched) return;
    // 남은 이동 거리와 이번 프레임 이동량
    const FVector Offset = TargetLocation - GetActorLocation();
    const float Step = FMath::Max(Speed, 1.f) * DeltaSeconds;
    if (Offset.SizeSquared() <= FMath::Square(Step))
    {
        SetActorLocation(TargetLocation, true);
        FinishProjectile(TargetLocation);
        return;
    }
    SetActorLocation(GetActorLocation() + Offset.GetSafeNormal() * Step, true);
}

// 도착 지점에서 AuraFX 재생 후 투사체 제거
void ABossMagicAttackActor::FinishProjectile(const FVector& EffectLocation)
{
    if (ArrivalEffect) UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, ArrivalEffect, EffectLocation);
    Destroy();
}
