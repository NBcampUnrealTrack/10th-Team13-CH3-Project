#include "PlayerStaminaComponent.h"

UPlayerStaminaComponent::UPlayerStaminaComponent()
{
	// 스태미나 소모 및 회복이 필요할 때만 Tick을 사용
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UPlayerStaminaComponent::BeginPlay()
{
	Super::BeginPlay();

	// 게임 시작 시 스태미나를 최대치로 설정
	CurrentStamina = MaxStamina;

	// 초기 스태미나 값을 UI 등에 전달
	OnStaminaChanged.Broadcast(
		CurrentStamina,
		MaxStamina
	);
}

void UPlayerStaminaComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction
)
{
	Super::TickComponent(
		DeltaTime,
		TickType,
		ThisTickFunction
	);

	if (bIsConsumingSprintStamina)
	{
		// 달리기 중이면 매 프레임 스태미나를 소모
		ConsumeSprintStamina(DeltaTime);
		return;
	}

	// 스태미나를 사용하지 않는 동안 회복 처리
	RecoverStamina(DeltaTime);
}

bool UPlayerStaminaComponent::HasEnoughStamina(
	float Amount
) const
{
	// 잘못된 소모량이 들어오지 않았는지 함께 확인
	return Amount > 0.0f && CurrentStamina >= Amount;
}

bool UPlayerStaminaComponent::TryConsumeStamina(
	float Amount
)
{
	if (!HasEnoughStamina(Amount))
	{
		// 스태미나가 부족하면 소모하지 않음
		return false;
	}

	// 대시처럼 한 번에 사용하는 스태미나를 차감
	SetCurrentStamina(CurrentStamina - Amount);

	// 스태미나를 사용했으므로 회복 대기시간을 초기화
	TimeSinceLastStaminaUse = 0.0f;

	// 회복 처리를 위해 Tick을 활성화
	SetComponentTickEnabled(true);

	return true;
}

bool UPlayerStaminaComponent::StartSprintConsumption()
{
	if (CurrentStamina <= 0.0f)
	{
		// 스태미나가 없으면 달리기를 시작할 수 없음
		return false;
	}

	// 달리기 중 지속 소모 상태로 변경
	bIsConsumingSprintStamina = true;

	// 지속 소모 처리를 위해 Tick을 활성화
	SetComponentTickEnabled(true);

	return true;
}

void UPlayerStaminaComponent::StopSprintConsumption()
{
	// 달리기 스태미나 소모를 중지
	bIsConsumingSprintStamina = false;

	// 마지막 소모를 기준으로 회복 대기시간을 적용
	TimeSinceLastStaminaUse = 0.0f;

	// 스태미나 회복을 계속 처리하기 위해 Tick 유지
	if (CurrentStamina < MaxStamina)
	{
		SetComponentTickEnabled(true);
	}
}

float UPlayerStaminaComponent::GetCurrentStamina() const
{
	// 외부에서 현재 스태미나를 읽을 수 있도록 반환
	return CurrentStamina;
}

float UPlayerStaminaComponent::GetMaxStamina() const
{
	// 외부에서 최대 스태미나를 읽을 수 있도록 반환
	return MaxStamina;
}

void UPlayerStaminaComponent::SetCurrentStamina(
	float NewStamina
)
{
	// 스태미나가 0 미만 또는 최대치 초과가 되지 않게 제한
	const float ClampedStamina = FMath::Clamp(
		NewStamina,
		0.0f,
		MaxStamina
	);

	if (FMath::IsNearlyEqual(
		CurrentStamina,
		ClampedStamina
	))
	{
		// 값이 실제로 변하지 않았다면 이벤트를 보내지 않음
		return;
	}

	// 제한된 값을 현재 스태미나에 적용
	CurrentStamina = ClampedStamina;

	// 변경된 수치를 UI 등의 외부 시스템에 전달
	OnStaminaChanged.Broadcast(
		CurrentStamina,
		MaxStamina
	);
}

void UPlayerStaminaComponent::ConsumeSprintStamina(
	float DeltaTime
)
{
	// 프레임 시간에 맞춰 달리기 스태미나 소모량 계산
	const float StaminaCost =
		SprintCostPerSecond * DeltaTime;

	SetCurrentStamina(CurrentStamina - StaminaCost);

	// 소모 중에는 회복 대기시간을 계속 초기화
	TimeSinceLastStaminaUse = 0.0f;

	if (CurrentStamina > 0.0f)
	{
		return;
	}

	// 스태미나가 모두 소진되면 지속 소모를 중단
	bIsConsumingSprintStamina = false;

	// 캐릭터가 강제로 달리기를 중단하도록 이벤트 전달
	OnStaminaDepleted.Broadcast();
}

void UPlayerStaminaComponent::RecoverStamina(
	float DeltaTime
)
{
	if (CurrentStamina >= MaxStamina)
	{
		// 최대치에 도달하면 더 이상 Tick을 사용할 필요가 없음
		SetComponentTickEnabled(false);
		return;
	}

	// 마지막 사용 이후 지난 시간을 누적
	TimeSinceLastStaminaUse += DeltaTime;

	if (TimeSinceLastStaminaUse < RecoveryDelay)
	{
		// 회복 대기시간 전에는 스태미나를 회복하지 않음
		return;
	}

	// 프레임 시간에 맞춰 스태미나 회복량 계산
	const float RecoveryAmount =
		RecoveryPerSecond * DeltaTime;

	SetCurrentStamina(CurrentStamina + RecoveryAmount);

	if (CurrentStamina >= MaxStamina)
	{
		// 완전히 회복되면 불필요한 Tick을 종료
		SetComponentTickEnabled(false);
	}
}