#include "PlayerHealthComponent.h"

UPlayerHealthComponent::UPlayerHealthComponent()
{
	// 체력은 피해나 회복 이벤트가 발생할 때만 변경되므로 Tick 비활성화
	PrimaryComponentTick.bCanEverTick = false;
}

void UPlayerHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	// 최대 체력이 0 이하로 설정되지 않도록 최소값 보정
	MaxHealth = FMath::Max(MaxHealth, 1.0f);

	// 게임 시작 시 현재 체력을 최대 체력으로 설정
	CurrentHealth = MaxHealth;

	// 게임 시작 시 살아 있는 상태로 초기화
	bIsDead = false;

	// 초기 체력값을 UI 등의 외부 시스템에 전달
	OnHealthChanged.Broadcast(
		CurrentHealth,
		MaxHealth
	);
}

void UPlayerHealthComponent::ApplyDamage(
	float DamageAmount
)
{
	if (DamageAmount <= 0.0f)
	{
		// 피해량이 0 이하라면 체력을 변경하지 않음
		return;
	}

	if (bIsDead)
	{
		// 이미 사망한 플레이어에게는 추가 피해를 적용하지 않음
		return;
	}

	// 현재 체력에서 전달받은 피해량만큼 차감
	SetCurrentHealth(CurrentHealth - DamageAmount);

	if (CurrentHealth <= 0.0f)
	{
		// 피해 적용 후 체력이 0이면 사망 처리
		HandleDeath();
	}
}

void UPlayerHealthComponent::Heal(
	float HealAmount
)
{
	if (HealAmount <= 0.0f)
	{
		// 회복량이 0 이하라면 체력을 변경하지 않음
		return;
	}

	if (bIsDead)
	{
		// 사망한 플레이어는 일반 회복으로 부활하지 않음
		return;
	}

	if (CurrentHealth >= MaxHealth)
	{
		// 이미 최대 체력이면 회복하지 않음
		return;
	}

	// 현재 체력에 전달받은 회복량을 추가
	SetCurrentHealth(CurrentHealth + HealAmount);
}

void UPlayerHealthComponent::ResetHealth()
{
	// 사망 상태를 해제하고 체력을 최대치로 복구
	bIsDead = false;
	SetCurrentHealth(MaxHealth);
}

float UPlayerHealthComponent::GetCurrentHealth() const
{
	// UI와 외부 시스템에서 사용할 현재 체력 반환
	return CurrentHealth;
}

float UPlayerHealthComponent::GetMaxHealth() const
{
	// UI와 외부 시스템에서 사용할 최대 체력 반환
	return MaxHealth;
}

bool UPlayerHealthComponent::IsDead() const
{
	// 현재 플레이어의 사망 상태 반환
	return bIsDead;
}

void UPlayerHealthComponent::SetCurrentHealth(
	float NewHealth
)
{
	// 현재 체력이 0 미만 또는 최대 체력 초과가 되지 않도록 제한
	const float ClampedHealth = FMath::Clamp(
		NewHealth,
		0.0f,
		MaxHealth
	);

	if (FMath::IsNearlyEqual(
		CurrentHealth,
		ClampedHealth
	))
	{
		// 체력이 실제로 변하지 않았다면 이벤트를 보내지 않음
		return;
	}

	// 제한된 값을 현재 체력에 적용
	CurrentHealth = ClampedHealth;

	// 변경된 체력값을 UI 등의 외부 시스템에 전달
	OnHealthChanged.Broadcast(
		CurrentHealth,
		MaxHealth
	);
}

void UPlayerHealthComponent::HandleDeath()
{
	if (bIsDead)
	{
		// 사망 처리가 중복으로 실행되지 않도록 방지
		return;
	}

	// 플레이어를 사망 상태로 변경
	bIsDead = true;

	// 캐릭터와 외부 시스템에 사망 이벤트 전달
	OnPlayerDeath.Broadcast();
}