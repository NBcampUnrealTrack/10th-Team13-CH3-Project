#include "BossLoopDummy.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "VeilBreakGameMode.h"

ABossLoopDummy::ABossLoopDummy()
{
	PrimaryActorTick.bCanEverTick = false;

	DummyMesh =
		CreateDefaultSubobject<UStaticMeshComponent>(
			TEXT("DummyMesh")
		);

	SetRootComponent(DummyMesh);

	// 테스트용 기본값
	MaxHealth = 1000.0f;
	CurrentHealth = 1000.0f;
	CurrentPhase = EBossPhase::Phase1;

	bRunAutomaticTest = true;
	AutomaticDamageAmount = 250.0f;
	AutomaticDamageInterval = 2.0f;
}

void ABossLoopDummy::BeginPlay()
{
	Super::BeginPlay();

	// 0으로 나누는 문제 방지
	MaxHealth = FMath::Max(MaxHealth, 1.0f);
	CurrentHealth = MaxHealth;
	CurrentPhase = EBossPhase::Phase1;

	ShowStatusOnScreen();

	if (bRunAutomaticTest)
	{
		const float SafeInterval =
			FMath::Max(AutomaticDamageInterval, 0.1f);

		GetWorldTimerManager().SetTimer(
			AutomaticDamageTimerHandle,
			this,
			&ABossLoopDummy::ApplyAutomaticTestDamage,
			SafeInterval,
			true,
			SafeInterval
		);
	}
}

void ABossLoopDummy::ApplyAutomaticTestDamage()
{
	ApplyTestDamage(AutomaticDamageAmount);
}

void ABossLoopDummy::ApplyTestDamage(
	float DamageAmount
)
{
	// 체력이 이미 0이면 추가 피해를 처리하지 않음
	if (CurrentHealth <= 0.0f)
	{
		return;
	}

	if (DamageAmount <= 0.0f)
	{
		return;
	}

	CurrentHealth = FMath::Clamp(
		CurrentHealth - DamageAmount,
		0.0f,
		MaxHealth
	);

	UpdatePhase();
	ShowStatusOnScreen();

	// 사망은 Phase가 아니라 체력 상태로 판정
	if (CurrentHealth <= 0.0f)
	{
		GetWorldTimerManager().ClearTimer(
			AutomaticDamageTimerHandle
		);

		if (AVeilBreakGameMode* GameMode =
			GetWorld()->GetAuthGameMode<AVeilBreakGameMode>())
		{
			GameMode->NotifyBossDefeated();
		}
	}
}

void ABossLoopDummy::UpdatePhase()
{
	// 사망했으면 페이즈를 변경하지 않음
	if (CurrentHealth <= 0.0f)
	{
		return;
	}

	const float HealthRatio =
		CurrentHealth / MaxHealth;

	EBossPhase NewPhase = EBossPhase::Phase1;

	if (HealthRatio <= 0.34f)
	{
		NewPhase = EBossPhase::Phase3;
	}
	else if (HealthRatio <= 0.67f)
	{
		NewPhase = EBossPhase::Phase2;
	}

	if (CurrentPhase == NewPhase)
	{
		return;
	}

	CurrentPhase = NewPhase;

	if (AVeilBreakGameMode* GameMode =
		GetWorld()->GetAuthGameMode<AVeilBreakGameMode>())
	{
		GameMode->NotifyBossPhaseChanged(NewPhase);
	}
}

void ABossLoopDummy::ShowStatusOnScreen() const
{
	if (!GEngine)
	{
		return;
	}

	const FString PhaseText = GetPhaseText();

	const FString StatusMessage =
		FString::Printf(
			TEXT("Dummy Boss | HP: %.0f / %.0f | %s"),
			CurrentHealth,
			MaxHealth,
			*PhaseText
		);

	FColor MessageColor = FColor::White;

	if (CurrentHealth <= 0.0f)
	{
		MessageColor = FColor::Purple;
	}
	else
	{
		switch (CurrentPhase)
		{
		case EBossPhase::Phase1:
			MessageColor = FColor::Green;
			break;

		case EBossPhase::Phase2:
			MessageColor = FColor::Yellow;
			break;

		case EBossPhase::Phase3:
			MessageColor = FColor::Red;
			break;
		}
	}

	GEngine->AddOnScreenDebugMessage(
		1001,
		AutomaticDamageInterval + 0.5f,
		MessageColor,
		StatusMessage
	);
}

FString ABossLoopDummy::GetPhaseText() const
{
	if (CurrentHealth <= 0.0f)
	{
		return TEXT("Dead");
	}

	switch (CurrentPhase)
	{
	case EBossPhase::Phase1:
		return TEXT("Phase 1");

	case EBossPhase::Phase2:
		return TEXT("Phase 2");

	case EBossPhase::Phase3:
		return TEXT("Phase 3");

	default:
		return TEXT("Unknown");
	}
}