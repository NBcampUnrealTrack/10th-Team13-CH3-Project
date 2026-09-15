#include "PlayerStaminaComponent.h"

UPlayerStaminaComponent::UPlayerStaminaComponent()
{
	// 지속 소모와 자동 회복을 처리하기 위해 Tick 활성화
	PrimaryComponentTick.bCanEverTick = true;
}

void UPlayerStaminaComponent::BeginPlay()
{
	Super::BeginPlay();

	// 최대 스태미나가 음수가 되지 않도록 보정
	MaxStamina = FMath::Max(MaxStamina, 0.0f);

	// 게임 시작 시 스태미나를 최대치로 설정
	CurrentStamina = MaxStamina;

	// 게임 시작 시 소모 상태 초기화
	bIsConsumingSprintStamina = false;
	bInfiniteStamina = false;
	TimeSinceLastStaminaUse = RecoveryDelay;

	// 초기 스태미나 정보를 UI에 전달
	OnStaminaChanged.Broadcast(CurrentStamina, MaxStamina);
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

	if (bInfiniteStamina)
	{
		// 궁극기 중에는 최대 스태미나를 유지
		return;
	}

	if (bIsConsumingSprintStamina)
	{
		// 달리기 중에는 매 프레임 스태미나 소모
		ConsumeSprintStamina(DeltaTime);
		return;
	}

	// 스태미나를 사용하지 않는 동안 회복 대기시간 누적
	TimeSinceLastStaminaUse += DeltaTime;

	if (TimeSinceLastStaminaUse >= RecoveryDelay)
	{
		// 회복 대기시간이 끝나면 자동 회복
		RecoverStamina(DeltaTime);
	}
}

bool UPlayerStaminaComponent::HasEnoughStamina(
	float Amount
) const
{
	if (bInfiniteStamina)
	{
		// 궁극기 중에는 항상 스태미나 사용 가능
		return true;
	}

	// 요청량이 유효하고 현재 스태미나가 충분한지 반환
	return Amount >= 0.0f && CurrentStamina >= Amount;
}

bool UPlayerStaminaComponent::TryConsumeStamina(
	float Amount
)
{
	if (Amount < 0.0f)
	{
		// 음수 소모 요청은 허용하지 않음
		return false;
	}

	if (bInfiniteStamina)
	{
		// 궁극기 중에는 성공만 반환하고 실제 수치는 소모하지 않음
		return true;
	}

	if (!HasEnoughStamina(Amount))
	{
		// 현재 스태미나가 부족하면 사용 실패
		return false;
	}

	// 지정한 양만큼 현재 스태미나 감소
	SetCurrentStamina(CurrentStamina - Amount);

	// 마지막 사용 시간을 초기화해 즉시 회복되지 않도록 처리
	TimeSinceLastStaminaUse = 0.0f;

	return true;
}

bool UPlayerStaminaComponent::StartSprintConsumption()
{
	if (!bInfiniteStamina && CurrentStamina <= 0.0f)
	{
		// 스태미나가 없다면 달리기 시작 실패
		return false;
	}

	// 달리기 지속 소모 상태 시작
	bIsConsumingSprintStamina = true;
	return true;
}

void UPlayerStaminaComponent::StopSprintConsumption()
{
	// 달리기 지속 소모 상태 종료
	bIsConsumingSprintStamina = false;
}

void UPlayerStaminaComponent::SetInfiniteStamina(
	bool bEnableInfiniteStamina
)
{
	// 궁극기 상태에 따라 무한 스태미나 상태 변경
	bInfiniteStamina = bEnableInfiniteStamina;

	if (bInfiniteStamina)
	{
		// 궁극기 시작 시 스태미나를 즉시 최대치로 회복
		SetCurrentStamina(MaxStamina);

		// 궁극기 중 지속 소모가 실행되지 않도록 중단
		bIsConsumingSprintStamina = false;
	}

	// 궁극기 종료 후 회복 지연 시간을 정상적으로 다시 계산
	TimeSinceLastStaminaUse = 0.0f;
}

float UPlayerStaminaComponent::GetCurrentStamina() const
{
	// UI에서 사용할 현재 스태미나 반환
	return CurrentStamina;
}

float UPlayerStaminaComponent::GetMaxStamina() const
{
	// UI에서 사용할 최대 스태미나 반환
	return MaxStamina;
}

bool UPlayerStaminaComponent::IsInfiniteStamina() const
{
	// 현재 무한 스태미나 상태 반환
	return bInfiniteStamina;
}

void UPlayerStaminaComponent::SetCurrentStamina(
	float NewStamina
)
{
	// 이전 스태미나 수치 저장
	const float PreviousStamina = CurrentStamina;

	// 현재 스태미나를 0과 최대치 사이로 제한
	CurrentStamina = FMath::Clamp(
		NewStamina,
		0.0f,
		MaxStamina
	);

	if (!FMath::IsNearlyEqual(PreviousStamina, CurrentStamina))
	{
		// 실제 값이 변경됐을 때만 UI 이벤트 전달
		OnStaminaChanged.Broadcast(CurrentStamina, MaxStamina);
	}

	if (PreviousStamina > 0.0f && CurrentStamina <= 0.0f)
	{
		// 스태미나가 처음 0이 된 순간 소진 이벤트 전달
		OnStaminaDepleted.Broadcast();
	}
}

void UPlayerStaminaComponent::ConsumeSprintStamina(
	float DeltaTime
)
{
	// 프레임 시간에 비례해 달리기 스태미나 소모
	const float StaminaCost = SprintCostPerSecond * DeltaTime;

	SetCurrentStamina(CurrentStamina - StaminaCost);
	TimeSinceLastStaminaUse = 0.0f;

	if (CurrentStamina <= 0.0f)
	{
		// 스태미나가 소진되면 지속 소모 종료
		bIsConsumingSprintStamina = false;
	}
}

void UPlayerStaminaComponent::RecoverStamina(
	float DeltaTime
)
{
	if (CurrentStamina >= MaxStamina)
	{
		// 이미 최대치라면 추가 회복하지 않음
		return;
	}

	// 프레임 시간에 비례해 스태미나 자동 회복
	const float RecoveryAmount = RecoveryPerSecond * DeltaTime;
	SetCurrentStamina(CurrentStamina + RecoveryAmount);
}
