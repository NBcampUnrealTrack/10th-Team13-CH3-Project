#include "BossFallingRockActor.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "PlayerHealthComponent.h"
#include "Engine/World.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/Character.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

// DragonCave Rock 메시와 착지 이펙트 설정
ABossFallingRockActor::ABossFallingRockActor()
{
	PrimaryActorTick.bCanEverTick = true;
	RockMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RockMesh"));
	SetRootComponent(RockMesh);
	// 기존 낙석 대비 2배 크기 적용
	RockMesh->SetRelativeScale3D(FVector(2.f));
	RockMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	// 착지 시점에만 활성화할 추후 데미지 판정용 구형 콜리전
	ImpactCollision = CreateDefaultSubobject<USphereComponent>(TEXT("ImpactCollision"));
	ImpactCollision->SetupAttachment(RockMesh);
	ImpactCollision->SetAbsolute(false, false, true);
	ImpactCollision->InitSphereRadius(ImpactCollisionRadius);
	ImpactCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ImpactCollision->SetCollisionObjectType(ECC_WorldDynamic);
	ImpactCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
	ImpactCollision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	ImpactCollision->SetGenerateOverlapEvents(true);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Rock(TEXT("/Game/DragonCave/Meshes/SM_LargeRock03.SM_LargeRock03"));
	// DragonCave LargeRock03 표면 머티리얼
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> RockMaterial(TEXT("/Game/DragonCave/Materials/MI_LargeRocks03.MI_LargeRocks03"));
	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> Sand(TEXT("/Game/SlashTrail_SoftTofu/Niagara/Sand/NS_AuraFX_Sand.NS_AuraFX_Sand"));
	if (Rock.Succeeded()) RockMesh->SetStaticMesh(Rock.Object);
	if (RockMaterial.Succeeded()) RockMesh->SetMaterial(0, RockMaterial.Object);
	if (Sand.Succeeded()) ArrivalEffect = Sand.Object;
	InitialLifeSpan = 10.f;
}

// 목표 위치와 발사 순간 위치 저장
void ABossFallingRockActor::LaunchAt(const FVector& InTarget)
{
	StartLocation = GetActorLocation();
	TargetLocation = InTarget;
	ElapsedFlightTime = 0.f;
	FlightDuration = FMath::Max(FVector::Distance(StartLocation, TargetLocation) / FMath::Max(FlightSpeed, 1.f), 0.1f);
	bLaunched = true;
	SetLifeSpan(FlightDuration + 2.f);
}

// 보스 PatternSetter의 착지 피해와 비행속도 적용
void ABossFallingRockActor::Configure(float InDamage, float InFlightSpeed)
{
	Damage = FMath::Max(0.f, InDamage);
	FlightSpeed = FMath::Max(1.f, InFlightSpeed);
}

// 선형 수평 이동과 포물선 높이·회전 적용
void ABossFallingRockActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!bLaunched) return;
	ElapsedFlightTime += DeltaSeconds;
	const float Alpha = FMath::Clamp(ElapsedFlightTime / FMath::Max(FlightDuration, KINDA_SMALL_NUMBER), 0.f, 1.f);
	FVector Location = FMath::Lerp(StartLocation, TargetLocation, Alpha);
	Location.Z += 4.f * ArcHeight * Alpha * (1.f - Alpha);
	SetActorLocation(Location);
	AddActorLocalRotation(RotationRate * DeltaSeconds);
	if (Alpha >= 1.f) FinishFallingRock();
}

// 착지 이펙트 2배 재생과 추후 데미지용 Overlap 콜리전 활성화
void ABossFallingRockActor::FinishFallingRock()
{
	bLaunched = false;
	RockMesh->SetVisibility(false, true);
	ImpactCollision->SetSphereRadius(ImpactCollisionRadius, true);
	ImpactCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	ApplyImpactDamage();
	if (ArrivalEffect) UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, ArrivalEffect, TargetLocation, FRotator::ZeroRotator, FVector(ImpactEffectScale));
	// 착지 판정이 유지되는 시간 뒤 액터 제거
	FTimerHandle ImpactCollisionTimer;
	GetWorldTimerManager().SetTimer(ImpactCollisionTimer, this, &ABossFallingRockActor::FinishImpactCollision, ImpactCollisionDuration, false);
}

// 착지 경고 범위 안 Pawn을 한 번씩 검사해 낙석 피해 적용
void ABossFallingRockActor::ApplyImpactDamage() const
{
	if (!GetWorld() || Damage <= 0.f) return;
	TArray<FOverlapResult> Overlaps;
	FCollisionObjectQueryParams ObjectTypes;
	ObjectTypes.AddObjectTypesToQuery(ECC_Pawn);
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(FallingRockImpactDamage), false, GetOwner());
	GetWorld()->OverlapMultiByObjectType(Overlaps, TargetLocation, FQuat::Identity, ObjectTypes, FCollisionShape::MakeSphere(ImpactCollisionRadius), QueryParams);
	TSet<AActor*> DamagedActors;
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* TargetActor = Overlap.GetActor();
		if (!TargetActor || DamagedActors.Contains(TargetActor)) continue;
		UPlayerHealthComponent* Health = TargetActor->FindComponentByClass<UPlayerHealthComponent>();
		if (!Health || Health->IsDead()) continue;
		Health->ApplyDamage(Damage);
		// 낙석 중심에서 바깥쪽으로 수평 600·수직 200 기본 넉백 적용
		if (ACharacter* HitCharacter = Cast<ACharacter>(TargetActor))
		{
			FVector KnockbackDirection = HitCharacter->GetActorLocation() - TargetLocation;
			KnockbackDirection.Z = 0.f;
			KnockbackDirection = KnockbackDirection.GetSafeNormal();
			FVector KnockbackVelocity = KnockbackDirection * KnockbackHorizontalStrength;
			KnockbackVelocity.Z = KnockbackVerticalStrength;
			HitCharacter->LaunchCharacter(KnockbackVelocity, true, true);
		}
		DamagedActors.Add(TargetActor);
	}
}

// 착지 Overlap 판정 종료 후 낙석 액터 제거
void ABossFallingRockActor::FinishImpactCollision()
{
	ImpactCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Destroy();
}
