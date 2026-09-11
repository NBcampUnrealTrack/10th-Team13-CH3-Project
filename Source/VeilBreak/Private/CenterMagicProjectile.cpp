
#include "CenterMagicProjectile.h"
#include "Components/SceneComponent.h"

ACenterMagicProjectile::ACenterMagicProjectile()
{
    PrimaryActorTick.bCanEverTick = true;

    // 액터 위치의 기준이 되는 컴포넌트
    RootComponent =
        CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));

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

    //다음 위치 = 현재 위치 + 방향 * 속도 * 프레임 시간
    const FVector NextLocation =
        GetActorLocation() + MoveDirection * Speed * DeltaTime;

    SetActorLocation(NextLocation);
}
