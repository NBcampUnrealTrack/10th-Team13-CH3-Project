#include "PlayerHealthComponent.h"

UPlayerHealthComponent::UPlayerHealthComponent()
{
	// 체력은 피해와 회복 이벤트로 처리
	PrimaryComponentTick.bCanEverTick = false;
}

void UPlayerHealthComponent::BeginPlay()
{
	Super::BeginPlay();
	MaxHealth = FMath::Max(MaxHealth, 1.0f);
	CurrentHealth = MaxHealth;
	bIsDead = false;
	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);
}

void UPlayerHealthComponent::ApplyDamage(float DamageAmount)
{
	if (!FMath::IsFinite(DamageAmount) || DamageAmount <= 0.0f || bIsDead)
	{
		return;
	}

	// 이번 공격의 실제 HP 감소량을 이벤트 전에 확정
	const float NewHealth = FMath::Clamp(CurrentHealth - DamageAmount, 0.0f, MaxHealth);
	const float AppliedDamage = CurrentHealth - NewHealth;
	if (AppliedDamage <= 0.0f)
	{
		return;
	}

	// UI 콜백 중 죽은 플레이어가 회복하거나 추가 피해를 받지 않도록 먼저 상태 확정
	const bool bLethalDamage = NewHealth <= 0.0f;
	if (bLethalDamage)
	{
		bIsDead = true;
	}
	SetCurrentHealth(NewHealth);

	// 회복 이벤트와 구분되는 피격 전용 이벤트
	OnDamageReceived.Broadcast(AppliedDamage);

	if (bLethalDamage)
	{
		// 사망 상태는 이미 설정했으므로 이벤트만 한 번 전달
		OnPlayerDeath.Broadcast();
	}
}

void UPlayerHealthComponent::Heal(float HealAmount)
{
	if (!FMath::IsFinite(HealAmount) || HealAmount <= 0.0f || bIsDead || CurrentHealth >= MaxHealth)
	{
		return;
	}
	// 회복은 OnDamageReceived를 호출하지 않음
	SetCurrentHealth(CurrentHealth + HealAmount);
}

void UPlayerHealthComponent::ResetHealth()
{
	// 기존 체력 초기화 기능 유지
	bIsDead = false;
	SetCurrentHealth(MaxHealth);
}

float UPlayerHealthComponent::GetCurrentHealth() const
{
	return CurrentHealth;
}

float UPlayerHealthComponent::GetMaxHealth() const
{
	return MaxHealth;
}

bool UPlayerHealthComponent::IsDead() const
{
	return bIsDead;
}

void UPlayerHealthComponent::SetCurrentHealth(float NewHealth)
{
	const float ClampedHealth = FMath::Clamp(NewHealth, 0.0f, MaxHealth);
	if (CurrentHealth == ClampedHealth)
	{
		return;
	}
	CurrentHealth = ClampedHealth;
	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);
}

void UPlayerHealthComponent::HandleDeath()
{
	// 중복 사망 처리 방지
	if (bIsDead)
	{
		return;
	}
	bIsDead = true;
	OnPlayerDeath.Broadcast();
}
