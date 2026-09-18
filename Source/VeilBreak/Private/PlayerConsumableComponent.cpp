#include "PlayerConsumableComponent.h"

#include "Engine/World.h"
#include "PlayerHealthComponent.h"

UPlayerConsumableComponent::UPlayerConsumableComponent()
{
	// 물약 회복은 타이머로 처리하므로 Tick 비활성화
	PrimaryComponentTick.bCanEverTick = false;
}

void UPlayerConsumableComponent::BeginPlay()
{
	Super::BeginPlay();

	// 물약 설정값이 잘못되지 않도록 안전한 범위로 보정
	MaxHealthPotionCount = FMath::Max(
		MaxHealthPotionCount,
		0
	);

	StartingHealthPotionCount = FMath::Clamp(
		StartingHealthPotionCount,
		0,
		MaxHealthPotionCount
	);

	HealAmountPerTick = FMath::Max(
		HealAmountPerTick,
		0.0f
	);

	HealTickInterval = FMath::Max(
		HealTickInterval,
		0.01f
	);

	TotalHealTicks = FMath::Max(
		TotalHealTicks,
		1
	);

	// 소유 캐릭터에 생성된 체력 컴포넌트 가져오기
	PlayerHealthComponent =
		GetOwner()->FindComponentByClass<
		UPlayerHealthComponent
		>();

	// 게임 시작 시 체력 물약 5개 지급
	CurrentHealthPotionCount = StartingHealthPotionCount;
	bIsHealingWithPotion = false;
	RemainingHealTicks = 0;

	if (PlayerHealthComponent != nullptr)
	{
		// 플레이어 사망 시 진행 중인 물약 회복 즉시 종료
		PlayerHealthComponent->OnPlayerDeath.AddUniqueDynamic(
			this,
			&UPlayerConsumableComponent::StopPotionHealing
		);
	}

	// 초기 물약 개수를 UI에 전달
	BroadcastPotionCountChanged();
}

bool UPlayerConsumableComponent::UseHealthPotion()
{
	if (PlayerHealthComponent == nullptr)
	{
		// 체력 컴포넌트가 없다면 물약 사용 불가
		return false;
	}

	if (PlayerHealthComponent->IsDead())
	{
		// 사망 상태에서는 물약 사용 불가
		return false;
	}

	if (
		PlayerHealthComponent->GetCurrentHealth() >=
		PlayerHealthComponent->GetMaxHealth()
		)
	{
		// 이미 최대 체력이라면 물약을 소모하지 않음
		return false;
	}

	if (bIsHealingWithPotion)
	{
		// 이미 물약으로 회복 중이라면 중복 사용 불가
		return false;
	}

	if (CurrentHealthPotionCount <= 0)
	{
		// 보유 중인 체력 물약이 없다면 사용 불가
		return false;
	}

	// 물약 한 개 소모
	--CurrentHealthPotionCount;
	BroadcastPotionCountChanged();

	// 1초마다 총 5번 회복하는 상태 시작
	bIsHealingWithPotion = true;
	RemainingHealTicks = TotalHealTicks;
	OnHealthPotionStateChanged.Broadcast(true);

	GetWorld()->GetTimerManager().SetTimer(
		PotionHealTimerHandle,
		this,
		&UPlayerConsumableComponent::HandlePotionHealTick,
		HealTickInterval,
		true
	);

	return true;
}

int32 UPlayerConsumableComponent::AddHealthPotions(
	int32 PotionAmount
)
{
	if (PotionAmount <= 0)
	{
		// 추가할 물약 개수가 유효하지 않으면 처리하지 않음
		return 0;
	}

	const int32 PreviousPotionCount =
		CurrentHealthPotionCount;

	// 최대 보유 개수를 넘지 않도록 물약 추가
	CurrentHealthPotionCount = FMath::Clamp(
		CurrentHealthPotionCount + PotionAmount,
		0,
		MaxHealthPotionCount
	);

	const int32 AddedPotionCount =
		CurrentHealthPotionCount - PreviousPotionCount;

	if (AddedPotionCount > 0)
	{
		// 실제로 물약이 추가됐을 때만 UI에 전달
		BroadcastPotionCountChanged();
	}

	return AddedPotionCount;
}

int32 UPlayerConsumableComponent::
GetHealthPotionCount() const
{
	// UI에서 사용할 현재 체력 물약 개수 반환
	return CurrentHealthPotionCount;
}

bool UPlayerConsumableComponent::
IsHealingWithPotion() const
{
	// 현재 체력 물약으로 회복 중인지 반환
	return bIsHealingWithPotion;
}

void UPlayerConsumableComponent::HandlePotionHealTick()
{
	if (
		PlayerHealthComponent == nullptr ||
		PlayerHealthComponent->IsDead()
		)
	{
		// 체력 컴포넌트가 없거나 사망했다면 회복 종료
		StopPotionHealing();
		return;
	}

	if (
		PlayerHealthComponent->GetCurrentHealth() >=
		PlayerHealthComponent->GetMaxHealth()
		)
	{
		// 최대 체력에 도달했다면 남은 회복 종료
		StopPotionHealing();
		return;
	}

	// 이번 회복 주기의 체력 10 회복
	PlayerHealthComponent->Heal(HealAmountPerTick);
	--RemainingHealTicks;

	if (
		RemainingHealTicks <= 0 ||
		PlayerHealthComponent->GetCurrentHealth() >=
		PlayerHealthComponent->GetMaxHealth()
		)
	{
		// 5회 회복했거나 최대 체력에 도달했다면 종료
		StopPotionHealing();
	}
}

void UPlayerConsumableComponent::StopPotionHealing()
{
	if (GetWorld() != nullptr)
	{
		// 진행 중인 지속 회복 타이머 제거
		GetWorld()->GetTimerManager().ClearTimer(
			PotionHealTimerHandle
		);
	}

	if (!bIsHealingWithPotion)
	{
		// 이미 종료된 상태라면 이벤트를 중복 전달하지 않음
		return;
	}

	// 지속 회복 상태 종료
	bIsHealingWithPotion = false;
	RemainingHealTicks = 0;
	OnHealthPotionStateChanged.Broadcast(false);
}

void UPlayerConsumableComponent::BroadcastPotionCountChanged()
{
	// 현재 체력 물약 개수를 UI에 전달
	OnHealthPotionCountChanged.Broadcast(
		CurrentHealthPotionCount
	);
}
