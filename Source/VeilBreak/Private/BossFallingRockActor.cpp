#include "BossFallingRockActor.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "UObject/ConstructorHelpers.h"

// DragonCave Rock 메시와 착지 이펙트 설정
ABossFallingRockActor::ABossFallingRockActor()
{
	PrimaryActorTick.bCanEverTick = true;
	RockMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RockMesh"));
	SetRootComponent(RockMesh);
	RockMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
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
	bLaunched = true;
	SetLifeSpan(FlightDuration + 2.f);
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

// 착지 이펙트 재생 후 낙석 액터 제거
void ABossFallingRockActor::FinishFallingRock()
{
	if (ArrivalEffect) UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, ArrivalEffect, TargetLocation);
	Destroy();
}
