#include "PlayerHealthComponent.h"

#include "Engine/World.h"

UPlayerHealthComponent::UPlayerHealthComponent()
{
	// 체력은 함수 호출로 변경되므로 Tick 비활성화
	PrimaryComponentTick.bCanEverTick = false;
}

void UPlayerHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	// 최대 체력이 1보다 작아지지 않도록 보정
	MaxHealth = FMath::Max(MaxHealth, 1.0f);

	// 게임 시작 시 체력을 최대치로 설정
	CurrentHealth = MaxHealth;
	bIsDead = false;

	if (GetWorld() != nullptr)
	{
		// 게임 시작 시점부터 무피격 시간을 계산
		LastDamageReceivedTime =
			GetWorld()->GetTimeSeconds();
	}

	// 초기 체력 정보를 UI에 전달
	BroadcastHealthChanged();
}

void UPlayerHealthComponent::ApplyDamage(float DamageAmount)
{
	if (bIsDead)
	{
		// 이미 사망한 상태에서는 추가 피해를 받지 않음
		return;
	}

	if (DamageAmount <= 0.0f)
	{
		// 피해량이 0 이하라면 처리하지 않음
		return;
	}

	if (GetWorld() != nullptr)
	{
		// 실제 피해를 받을 때마다 무피격 시간 초기화
		LastDamageReceivedTime =
			GetWorld()->GetTimeSeconds();
	}

	// 현재 체력이 0 아래로 내려가지 않도록 피해 적용
	CurrentHealth = FMath::Clamp(
		CurrentHealth - DamageAmount,
		0.0f,
		MaxHealth
	);

	// 변경된 체력 정보를 UI에 전달
	BroadcastHealthChanged();

	if (CurrentHealth <= 0.0f)
	{
		// 사망 이벤트가 중복되지 않도록 상태 저장
		bIsDead = true;

		// 캐릭터와 UI에 플레이어 사망 전달
		OnPlayerDeath.Broadcast();
	}
}

void UPlayerHealthComponent::Heal(float HealAmount)
{
	if (bIsDead)
	{
		// 사망한 상태에서는 일반 회복 불가
		return;
	}

	if (HealAmount <= 0.0f)
	{
		// 회복량이 0 이하라면 처리하지 않음
		return;
	}

	if (CurrentHealth >= MaxHealth)
	{
		// 이미 최대 체력이면 회복하지 않음
		return;
	}

	// 현재 체력이 최대치를 넘지 않도록 회복 적용
	CurrentHealth = FMath::Clamp(
		CurrentHealth + HealAmount,
		0.0f,
		MaxHealth
	);

	// 변경된 체력 정보를 UI에 전달
	BroadcastHealthChanged();
}

void UPlayerHealthComponent::ResetHealth()
{
	// 플레이어 체력과 사망 상태 초기화
	CurrentHealth = MaxHealth;
	bIsDead = false;

	if (GetWorld() != nullptr)
	{
		// 부활 시점부터 무피격 시간을 다시 계산
		LastDamageReceivedTime =
			GetWorld()->GetTimeSeconds();
	}

	// 초기화된 체력 정보를 UI에 전달
	BroadcastHealthChanged();
}

float UPlayerHealthComponent::GetCurrentHealth() const
{
	// UI에서 사용할 현재 체력 반환
	return CurrentHealth;
}

float UPlayerHealthComponent::GetMaxHealth() const
{
	// UI에서 사용할 최대 체력 반환
	return MaxHealth;
}

bool UPlayerHealthComponent::IsDead() const
{
	// 현재 사망 상태 반환
	return bIsDead;
}

bool UPlayerHealthComponent::HP20Seconds() const
{
	if (bIsDead)
	{
		// 사망 상태는 무피격 상태로 판단하지 않음
		return false;
	}

	if (GetWorld() == nullptr)
	{
		// 월드가 없으면 시간을 계산할 수 없음
		return false;
	}

	// 마지막 피격 이후 경과 시간 계산
	const float ElapsedTime =
		GetWorld()->GetTimeSeconds() -
		LastDamageReceivedTime;

	// 설정된 무피격 시간 이상 지났는지 반환
	return ElapsedTime >= NoDamageRequiredTime;
}

void UPlayerHealthComponent::BroadcastHealthChanged()
{
	// 현재 체력과 최대 체력을 UI에 전달
	OnHealthChanged.Broadcast(
		CurrentHealth,
		MaxHealth
	);
}
