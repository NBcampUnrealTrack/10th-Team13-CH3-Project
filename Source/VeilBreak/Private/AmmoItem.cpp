#include "AmmoItem.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Pawn.h"
#include "PlayerCombatComponent.h"
#include "PlayerHealthComponent.h"

AAmmoItem::AAmmoItem()
{
	// 접촉 이벤트로 처리
	PrimaryActorTick.bCanEverTick = false;

	// 감지 구 생성 및 루트 설정
	PickupSphere = CreateDefaultSubobject<USphereComponent>(TEXT("PickupSphere"));
	SetRootComponent(PickupSphere);

	PickupSphere->InitSphereRadius(100.0f);

	// 물리적으로 막지 않고 겹침만 감지
	PickupSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	PickupSphere->SetCollisionObjectType(ECC_WorldDynamic);

	// Pawn만 겹침 감지
	PickupSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	PickupSphere->SetCollisionResponseToChannel(
		ECC_Pawn,
		ECR_Overlap
	);
	PickupSphere->SetGenerateOverlapEvents(true);

	// 외형 메시 생성
	ItemMesh = CreateDefaultSubobject<UStaticMeshComponent>(
		TEXT("ItemMesh")
	);
	ItemMesh->SetupAttachment(PickupSphere);

	// 외형은 충돌 및 물리에 참여하지 않음
	ItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ItemMesh->SetGenerateOverlapEvents(false);
	ItemMesh->SetSimulatePhysics(false);
}

void AAmmoItem::BeginPlay()
{
	Super::BeginPlay();

	// 에디터 외의 경로로 잘못된 값이 들어와도 최소 1발 지급
	AmmoAmount = FMath::Max(AmmoAmount, 1);

	// 감지 구에 액터가 들어오면 습득 함수로 연결
	PickupSphere->OnComponentBeginOverlap.AddUniqueDynamic(
		this,
		&AAmmoItem::OnPickupBeginOverlap
	);
}

void AAmmoItem::OnPickupBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult
)
{
	TryPickup(OtherActor);
}

void AAmmoItem::TryPickup(AActor* OtherActor)
{
	if (bPickupInProgress || !IsValid(OtherActor))
	{
		return;
	}

	// 플레이어가 조종하는 Pawn만 습득 가능
	APawn* PlayerPawn = Cast<APawn>(OtherActor);

	if (!IsValid(PlayerPawn) || !PlayerPawn->IsPlayerControlled())
	{
		return;
	}

	// 살아 있는 플레이어인지 확인
	UPlayerHealthComponent* Health =
		PlayerPawn->FindComponentByClass<UPlayerHealthComponent>();

	if (!IsValid(Health) || Health->IsDead())
	{
		return;
	}

	// 기존 전투 컴포넌트 검색
	UPlayerCombatComponent* Combat =
		PlayerPawn->FindComponentByClass<UPlayerCombatComponent>();

	if (!IsValid(Combat))
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[AmmoItem] 플레이어 전투 컴포넌트 없음")
		);
		return;
	}

	// 탄약 변경 이벤트 중에도 중복 지급되지 않도록 먼저 잠금
	bPickupInProgress = true;

	// 기존 함수가 최대치 제한과 UI 변경 이벤트를 처리
	const int32 AddedAmmo = Combat->AddReserveAmmo(AmmoAmount);

	if (AddedAmmo <= 0)
	{
		// 예비 탄약이 가득 찼다면 아이템을 남김
		bPickupInProgress = false;

		UE_LOG(
			LogTemp,
			Log,
			TEXT("[AmmoItem] 보충하지 않음: 예비 탄약 최대치")
		);
		return;
	}

	UE_LOG(
		LogTemp,
		Log,
		TEXT("[AmmoItem] 습득 성공: +%d발 / 장전=%d / 예비=%d"),
		AddedAmmo,
		Combat->GetCurrentAmmo(),
		Combat->GetReserveAmmo()
	);

	// 추가 접촉을 막고 아이템 제거
	SetActorEnableCollision(false);
	Destroy();
}