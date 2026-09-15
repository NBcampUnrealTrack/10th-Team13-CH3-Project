#include "BossMagicAttackActor.h"
#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "NiagaraFunctionLibrary.h"
#include "PlayerHealthComponent.h"
#include "Engine/World.h"
#include "Engine/OverlapResult.h"
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
    HitCollision->OnComponentBeginOverlap.AddDynamic(this, &ABossMagicAttackActor::HandleProjectileOverlap);
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

// 보스 PatternSetter의 직접 피해·도착 범위 피해·이동속도 적용
void ABossMagicAttackActor::Configure(float InProjectileDamage, float InExplosiveDamage, float InSpeed)
{
    ProjectileDamage = FMath::Max(0.f, InProjectileDamage);
    ExplosiveDamage = FMath::Max(0.f, InExplosiveDamage);
    Speed = FMath::Max(1.f, InSpeed);
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
    ApplyArrivalDamage(EffectLocation);
    if (ArrivalEffect) UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, ArrivalEffect, EffectLocation);
    Destroy();
}

// 투사체 직접 충돌 시 대상별 한 번만 피해 적용
void ABossMagicAttackActor::HandleProjectileOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (!OtherActor || OtherActor == GetOwner() || DirectDamagedActors.Contains(OtherActor)) return;
    if (ApplyPatternDamage(OtherActor, ProjectileDamage)) DirectDamagedActors.Add(OtherActor);
}

// PlayerHealthComponent가 있는 생존 대상에 지정 피해 적용
bool ABossMagicAttackActor::ApplyPatternDamage(AActor* TargetActor, float DamageAmount) const
{
    if (!TargetActor || DamageAmount <= 0.f) return false;
    UPlayerHealthComponent* Health = TargetActor->FindComponentByClass<UPlayerHealthComponent>();
    if (!Health || Health->IsDead()) return false;
    Health->ApplyDamage(DamageAmount);
    return true;
}

// 도착 원형 범위 안 Pawn을 한 번씩 검사해 폭발 피해 적용
void ABossMagicAttackActor::ApplyArrivalDamage(const FVector& DamageLocation) const
{
    if (!GetWorld()) return;
    TArray<FOverlapResult> Overlaps;
    FCollisionObjectQueryParams ObjectTypes;
    ObjectTypes.AddObjectTypesToQuery(ECC_Pawn);
    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(MagicArrivalDamage), false, GetOwner());
    GetWorld()->OverlapMultiByObjectType(Overlaps, DamageLocation, FQuat::Identity, ObjectTypes, FCollisionShape::MakeSphere(ArrivalDamageRadius), QueryParams);
    TSet<AActor*> DamagedActors;
    for (const FOverlapResult& Overlap : Overlaps)
    {
        AActor* TargetActor = Overlap.GetActor();
        if (TargetActor && !DamagedActors.Contains(TargetActor) && ApplyPatternDamage(TargetActor, ExplosiveDamage)) DamagedActors.Add(TargetActor);
    }
}
