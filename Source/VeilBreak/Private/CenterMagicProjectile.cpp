
#include "CenterMagicProjectile.h"
#include "Components/SceneComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "Components/SphereComponent.h"
#include "PlayerHealthComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Sound/SoundBase.h"

ACenterMagicProjectile::ACenterMagicProjectile()
{
    PrimaryActorTick.bCanEverTick = true;

    // 액터 위치의 기준이 되는 컴포넌트
    HitCollision =
        CreateDefaultSubobject<USphereComponent>(TEXT("HitCollision"));//구형 컴포넌트 만들기

    SetRootComponent(HitCollision);

    HitCollision->InitSphereRadius(25.0f);//구의 반지름
    HitCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);//겹침 같은 판정에 사용 물리적으로 밀어내는 충돌은 X
    HitCollision->SetCollisionObjectType(ECC_WorldDynamic);//WorldDynamic 충돌
    HitCollision->SetCollisionResponseToAllChannels(ECR_Ignore);//모든 종류를 무시 
    HitCollision->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);// 벽 바닥 장애물
    HitCollision->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);// 움직이는 장애물
    HitCollision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);//Pawn 종류에만 겹침 반응 키기
    HitCollision->SetGenerateOverlapEvents(true);//겹침 이벤트를 발생

    HitCollision->OnComponentBeginOverlap.AddDynamic(
        this,
        &ACenterMagicProjectile::OnProjectileOverlap//겹침 이벤트 연결
    );

    // 생성 후 5초가 지나면 제거
    InitialLifeSpan = 5.0f;
}

void ACenterMagicProjectile::Launch(FVector Direction)
{
    MoveDirection = Direction.GetSafeNormal();

    SetActorRotation(MoveDirection.Rotation());
}

void ACenterMagicProjectile::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    ACharacter* Player =
        UGameplayStatics::GetPlayerCharacter(this, 0);

    if (IsValid(Player))
    {
        const FVector ToPlayer =
            Player->GetActorLocation() - GetActorLocation();

        // 플레이어가 가까우면 추적 시작
        if (!HasTracked && ToPlayer.Size() <= TrackingRange)
        {
            IsTracking = true;
            HasTracked = true;
            TrackingElapsed = 0.0f;

            UE_LOG(LogTemp, Log, TEXT("Tracking Start"));
        }
        if (IsTracking)
        {
            // 플레이어 쪽으로 이동 방향 변경
            if (!ToPlayer.IsNearlyZero())
            {
                MoveDirection = ToPlayer.GetSafeNormal();
                SetActorRotation(MoveDirection.Rotation());
            }

            TrackingElapsed += DeltaTime;

            if (TrackingElapsed >= TrackingDuration)
            {
                IsTracking = false;
            }
        }
    }
    else
    {
        IsTracking = false;
    }

    const FVector NextLocation =
        GetActorLocation() + MoveDirection * Speed * DeltaTime;

    FHitResult Hit;
    SetActorLocation(NextLocation, true, &Hit);

    // 이동 도중 벽이나 바닥에 막히면 제거
        if (Hit.bBlockingHit)
        {
            Explode();
            return;
        }
}
void ACenterMagicProjectile::OnProjectileOverlap(
    UPrimitiveComponent* OverlappedComponent,
    AActor* OtherActor,
    UPrimitiveComponent* OtherComp,
    int32 OtherBodyIndex,
    bool bFromSweep,
    const FHitResult& SweepResult
)
{
    ACharacter* Player =
        UGameplayStatics::GetPlayerCharacter(this, 0);

    // 플레이어와 닿은 경우만 처리
    if (!IsValid(Player) || OtherActor != Player)
    {
        return;
    }

    UPlayerHealthComponent* Health =
        Player->FindComponentByClass<UPlayerHealthComponent>();

    if (!IsValid(Health) || Health->IsDead())
    {
        return;
    }

    // 여러 컴포넌트에 겹쳐도 중복 피격되지 않도록 끄기
    HitCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    Health->ApplyDamage(Damage);

    UE_LOG(LogTemp, Log, TEXT("Center Projectile Hit: %.1f"), Damage);

    Explode();
}
void ACenterMagicProjectile::Explode()
{
    const FVector Location = GetActorLocation();

    if (ImpactEffect)
    {
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(
            this,
            ImpactEffect,
            Location
        );
    }

    if (ImpactSound)
    {
        UGameplayStatics::PlaySoundAtLocation(
            this,
            ImpactSound,
            Location
        );
    }

    Destroy();
}