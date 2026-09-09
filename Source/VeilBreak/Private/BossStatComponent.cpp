#include "BossStatComponent.h"

// Tick 비활성화와 체력 기본값 설정
UBossStatComponent::UBossStatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

// 시작 시 체력 2000과 사망 상태 초기화
void UBossStatComponent::BeginPlay()
{
	Super::BeginPlay();
	CurrentHealth = MaxHealth;
	bIsDead = false;
}

// 무적·사망 상태를 제외하고 피해량만큼 체력 차감
float UBossStatComponent::ApplyDamage(float DamageAmount)
{
	if (DamageAmount <= 0.f || bIsInvulnerable || bIsDead) return 0.f;
	const float PreviousHealth = CurrentHealth;
	CurrentHealth = FMath::Clamp(CurrentHealth - DamageAmount, 0.f, MaxHealth);
	const float AppliedDamage = PreviousHealth - CurrentHealth;
	if (AppliedDamage <= 0.f) return 0.f;

	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);
	if (CurrentHealth <= 0.f)
	{
		bIsDead = true;
		OnBossDied.Broadcast();
	}
	return AppliedDamage;
}

// 최대 체력 기준 현재 체력 비율 계산
float UBossStatComponent::GetHealthPercent() const
{
	return MaxHealth > KINDA_SMALL_NUMBER ? CurrentHealth / MaxHealth : 0.f;
}

// 외부 패턴·연출용 무적 상태 갱신
void UBossStatComponent::SetInvulnerable(bool bNewIsInvulnerable)
{
	bIsInvulnerable = bNewIsInvulnerable;
}
