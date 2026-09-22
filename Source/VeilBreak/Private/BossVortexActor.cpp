#include "BossVortexActor.h"
#include "Components/SceneComponent.h"
#include "Components/CapsuleComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "PlayerHealthComponent.h"
#include "UObject/ConstructorHelpers.h"

// 추적 범위 콜리전과 Niagara 소용돌이 효과 생성
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
	VortexEffect = CreateDefaultSubobject<UNiagaraComponent>(TEXT("VortexEffect"));
	VortexEffect->SetupAttachment(SceneRoot);
	VortexEffect->SetRelativeLocation(FVector(0.f, 0.f, 10.f));
	VortexEffect->SetAutoActivate(false);
	VortexEffect->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> VortexSystem(TEXT("/Game/Boss/Patterns/NS_BossVortex.NS_BossVortex"));
	if (VortexSystem.Succeeded()) VortexEffect->SetAsset(VortexSystem.Object);
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
	if (VortexEffect) VortexEffect->Activate(true);
	SetLifeSpan(RemainingDuration);
}

// 플레이어 수평 추적·1초 피해 주기 갱신
void ABossVortexActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	RemainingDuration -= DeltaSeconds;
	if (RemainingDuration <= 0.f) { Destroy(); return; }
	if (TargetActor.IsValid())
	{
		const FVector Offset = TargetActor->GetActorLocation() - GetActorLocation();
		const FVector HorizontalOffset(Offset.X, Offset.Y, 0.f);
		const FVector Movement = HorizontalOffset.GetClampedToMaxSize(MoveSpeed * DeltaSeconds);
		FVector NextLocation = GetActorLocation() + Movement;
		FCollisionQueryParams GroundParams(SCENE_QUERY_STAT(VortexFollowGround), false, this);
		GroundParams.AddIgnoredActor(TargetActor.Get());
		if (GetOwner()) GroundParams.AddIgnoredActor(GetOwner());
		FHitResult GroundHit;
		const FVector TraceStart = NextLocation + FVector(0.f, 0.f, 200.f);
		const FVector TraceEnd = NextLocation - FVector(0.f, 0.f, 1500.f);
		if (GetWorld()->LineTraceSingleByChannel(GroundHit, TraceStart, TraceEnd, ECC_Visibility, GroundParams))
		{
			NextLocation.Z = GroundHit.ImpactPoint.Z;
		}
		SetActorLocation(NextLocation, true);
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
