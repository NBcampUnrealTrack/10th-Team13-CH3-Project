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

	// 테스트용 기본값입니다.
	MaxHealth = 1000.0f;
	CurrentHealth = 1000.0f;
	CurrentPhase = EVeilBreakBossPhase::Phase1;

	bRunAutomaticTest = true;
	AutomaticDamageAmount = 250.0f;
	AutomaticDamageInterval = 2.0f;
}

void ABossLoopDummy::BeginPlay()
{
	Super::BeginPlay();

	CurrentHealth = FMath::Max(MaxHealth, 1.0f);
	CurrentPhase = EVeilBreakBossPhase::Phase1;

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
	if (CurrentPhase == EVeilBreakBossPhase::Dead)
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

	if (CurrentPhase == EVeilBreakBossPhase::Dead)
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
	if (CurrentHealth <= 0.0f)
	{
		CurrentPhase = EVeilBreakBossPhase::Dead;
		return;
	}

	const float HealthRatio =
		CurrentHealth / MaxHealth;

	EVeilBreakBossPhase NewPhase =
		EVeilBreakBossPhase::Phase1;

	if (HealthRatio <= 0.34f)
	{
		NewPhase = EVeilBreakBossPhase::Phase3;
	}
	else if (HealthRatio <= 0.67f)
	{
		NewPhase = EVeilBreakBossPhase::Phase2;
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
			TEXT(
				"Dummy Boss | HP: %.0f / %.0f | %s"
			),
			CurrentHealth,
			MaxHealth,
			*PhaseText
		);

	FColor MessageColor = FColor::White;

	switch (CurrentPhase)
	{
	case EVeilBreakBossPhase::Phase1:
		MessageColor = FColor::Green;
		break;

	case EVeilBreakBossPhase::Phase2:
		MessageColor = FColor::Yellow;
		break;

	case EVeilBreakBossPhase::Phase3:
		MessageColor = FColor::Red;
		break;

	case EVeilBreakBossPhase::Dead:
		MessageColor = FColor::Purple;
		break;
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
	switch (CurrentPhase)
	{
	case EVeilBreakBossPhase::Phase1:
		return TEXT("Phase 1");

	case EVeilBreakBossPhase::Phase2:
		return TEXT("Phase 2");

	case EVeilBreakBossPhase::Phase3:
		return TEXT("Phase 3");

	case EVeilBreakBossPhase::Dead:
		return TEXT("Dead");

	default:
		return TEXT("Unknown");
	}
}