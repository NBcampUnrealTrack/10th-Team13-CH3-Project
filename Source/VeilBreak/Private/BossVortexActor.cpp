#include "BossVortexActor.h"
#include "Components/SceneComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "PlayerHealthComponent.h"
#include "UObject/ConstructorHelpers.h"

// 추적 범위 콜리전과 같은 크기의 반투명 회전 원기둥 생성
ABossVortexActor::ABossVortexActor()
{
	PrimaryActorTick.bCanEverTick = true;
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);
	DamageCollision = CreateDefaultSubobject<UCapsuleComponent>(TEXT("DamageCollision"));
	DamageCollision->SetupAttachment(SceneRoot);
	DamageCollision->InitCapsuleSize(260.f, 300.f);
	DamageCollision->SetRelativeLocation(FVector(0.f, 0.f, 300.f));
	DamageCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	DamageCollision->SetCollisionObjectType(ECC_WorldDynamic);
	DamageCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
	DamageCollision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	DamageCollision->SetGenerateOverlapEvents(true);
	VortexMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VortexBodyMesh"));
	VortexMesh->SetupAttachment(SceneRoot);
	VortexMesh->SetRelativeLocation(FVector(0.f, 0.f, 300.f));
	VortexMesh->SetRelativeScale3D(FVector(5.2f, 5.2f, 6.f));
	VortexMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	VortexMesh->SetCastShadow(false);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> VortexBodyMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> VortexBodyMaterial(TEXT("/Game/DragonCave/Materials/M_Glass.M_Glass"));
	if (VortexBodyMesh.Succeeded()) VortexMesh->SetStaticMesh(VortexBodyMesh.Object);
	if (VortexBodyMaterial.Succeeded()) VortexMesh->SetMaterial(0, VortexBodyMaterial.Object);
}

// 보스 PatternSetter의 대상·초당 피해·속도·유지시간 적용
void ABossVortexActor::Configure(AActor* InTargetActor, float InDamagePerSecond, float InMoveSpeed, float InDuration)
{
	TargetActor = InTargetActor;
	DamagePerSecond = FMath::Max(0.f, InDamagePerSecond);
	MoveSpeed = FMath::Max(0.f, InMoveSpeed);
	RemainingDuration = FMath::Max(0.1f, InDuration);
	DamageAccumulator = 0.f;
	SetActorHiddenInGame(false);
	if (VortexMesh)
	{
		VortexMesh->SetHiddenInGame(false, true);
		VortexMesh->SetVisibility(true, true);
	}
	SetLifeSpan(RemainingDuration);
}

// 본체 회전·플레이어 수평 추적·1초 피해 주기 갱신
void ABossVortexActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	RemainingDuration -= DeltaSeconds;
	if (RemainingDuration <= 0.f) { Destroy(); return; }
	if (VortexMesh) VortexMesh->AddLocalRotation(VortexRotationRate * DeltaSeconds);
	if (TargetActor.IsValid())
	{
		const FVector Offset = TargetActor->GetActorLocation() - GetActorLocation();
		const FVector HorizontalOffset(Offset.X, Offset.Y, 0.f);
		const FVector Movement = HorizontalOffset.GetClampedToMaxSize(MoveSpeed * DeltaSeconds);
		SetActorLocation(GetActorLocation() + Movement, true);
	}
	DamageAccumulator += DeltaSeconds;
	while (DamageAccumulator >= 1.f)
	{
		DamageAccumulator -= 1.f;
		ApplyDamageTick();
	}
}

// 소용돌이 범위 안 PlayerHealthComponent에 주기 피해 적용
void ABossVortexActor::ApplyDamageTick()
{
	TArray<AActor*> OverlappingActors;
	DamageCollision->GetOverlappingActors(OverlappingActors);
	for (AActor* OverlappingActor : OverlappingActors)
	{
		UPlayerHealthComponent* Health = OverlappingActor ? OverlappingActor->FindComponentByClass<UPlayerHealthComponent>() : nullptr;
		if (Health && !Health->IsDead()) Health->ApplyDamage(DamagePerSecond);
	}
}
