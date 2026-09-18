#include "PlayerSkillComponent.h"

#include "Engine/World.h"

UPlayerSkillComponent::UPlayerSkillComponent()
{
	// 궁극기는 입력과 타이머로 처리하므로 Tick 비활성화
	PrimaryComponentTick.bCanEverTick = false;
}

void UPlayerSkillComponent::BeginPlay()
{
	Super::BeginPlay();

	// 필요 스택이 최소 1 이상이 되도록 보정
	RequiredTargetStacks = FMath::Max(
		RequiredTargetStacks,
		1
	);

	// 궁극기 지속시간이 음수가 되지 않도록 보정
	UltimateDuration = FMath::Max(
		UltimateDuration,
		0.0f
	);

	// 게임 시작 시 궁극기 상태 초기화
	CurrentTargetStacks = 0;
	bIsUltimateActive = false;
}

void UPlayerSkillComponent::AddUltimateTargetStack()
{
	if (bIsUltimateActive)
	{
		// 궁극기 활성화 중에는 스택을 획득하지 않음
		return;
	}

	if (CurrentTargetStacks >= RequiredTargetStacks)
	{
		// 이미 최대 스택이라면 추가하지 않음
		return;
	}

	// 과녁 적중 스택 1 증가
	++CurrentTargetStacks;

	// 최대 필요 스택을 넘지 않도록 제한
	CurrentTargetStacks = FMath::Clamp(
		CurrentTargetStacks,
		0,
		RequiredTargetStacks
	);

	// 변경된 스택을 UI에 전달
	BroadcastTargetStackChanged();
}

bool UPlayerSkillComponent::ActivateUltimate()
{
	if (!CanActivateUltimate())
	{
		// 활성화 중이거나 5스택을 채우지 못했다면 사용 실패
		return false;
	}

	// 궁극기 활성 상태로 변경
	bIsUltimateActive = true;

	// 궁극기를 사용하면 누적 스택 전부 소모
	CurrentTargetStacks = 0;

	// UI에 0스택으로 초기화된 상태 전달
	BroadcastTargetStackChanged();

	// 공격력, 재장전, 이동속도와 스태미나 효과에 시작 전달
	OnUltimateStateChanged.Broadcast(true);

	if (UltimateDuration <= 0.0f)
	{
		// 지속시간이 0이라면 즉시 종료
		FinishUltimate();
		return true;
	}

	// 기존 궁극기 타이머가 남아 있다면 제거
	GetWorld()->GetTimerManager().ClearTimer(
		UltimateDurationTimerHandle
	);

	// 설정된 지속시간 이후 궁극기 종료
	GetWorld()->GetTimerManager().SetTimer(
		UltimateDurationTimerHandle,
		this,
		&UPlayerSkillComponent::FinishUltimate,
		UltimateDuration,
		false
	);

	return true;
}

int32 UPlayerSkillComponent::GetCurrentTargetStacks() const
{
	// UI에서 사용할 현재 과녁 스택 반환
	return CurrentTargetStacks;
}

int32 UPlayerSkillComponent::GetRequiredTargetStacks() const
{
	// UI에서 사용할 궁극기 필요 스택 반환
	return RequiredTargetStacks;
}

bool UPlayerSkillComponent::CanActivateUltimate() const
{
	if (bIsUltimateActive)
	{
		// 궁극기 활성화 중에는 다시 사용할 수 없음
		return false;
	}

	// 필요한 과녁 스택을 모두 채워야 사용 가능
	return CurrentTargetStacks >= RequiredTargetStacks;
}

bool UPlayerSkillComponent::IsUltimateActive() const
{
	// 현재 궁극기 활성 상태 반환
	return bIsUltimateActive;
}

float UPlayerSkillComponent::GetUltimateRemainingTime() const
{
	if (
		!bIsUltimateActive ||
		GetWorld() == nullptr
		)
	{
		// 궁극기가 꺼져 있거나 월드가 없다면 남은 시간 없음
		return 0.0f;
	}

	// 궁극기 타이머에 저장된 남은 시간 반환
	return FMath::Max(
		GetWorld()->GetTimerManager().GetTimerRemaining(
			UltimateDurationTimerHandle
		),
		0.0f
	);
}

void UPlayerSkillComponent::FinishUltimate()
{
	if (!bIsUltimateActive)
	{
		// 이미 종료된 상태라면 중복 처리하지 않음
		return;
	}

	// 궁극기 지속시간 타이머 정리
	GetWorld()->GetTimerManager().ClearTimer(
		UltimateDurationTimerHandle
	);

	// 궁극기 비활성 상태로 변경
	bIsUltimateActive = false;

	// 공격력, 재장전, 이동속도와 스태미나 효과에 종료 전달
	OnUltimateStateChanged.Broadcast(false);
}

void UPlayerSkillComponent::BroadcastTargetStackChanged()
{
	// 현재 스택과 필요 스택을 UI에 전달
	OnUltimateTargetStackChanged.Broadcast(
		CurrentTargetStacks,
		RequiredTargetStacks
	);
}