#include "PlayerSkillComponent.h"

#include "Engine/Engine.h"
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
		// 궁극기 활성화 중에는 새로운 스택을 획득하지 않음
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
		// 조건 미충족 또는 이미 활성화된 상태라면 사용 실패
		return false;
	}

	// 궁극기 활성 상태로 변경
	bIsUltimateActive = true;

	if (bRequireTargetStacks)
	{
		// 실제 과녁 시스템을 사용할 때만 누적 스택 소모
		CurrentTargetStacks = 0;

		// 변경된 스택을 UI에 전달
		BroadcastTargetStackChanged();
	}

	// 외부 시스템에 궁극기 시작 전달
	OnUltimateStateChanged.Broadcast(true);

	if (GEngine != nullptr)
	{
		// 과녁 연결 전 E 입력과 활성화를 확인하는 테스트 메시지
		GEngine->AddOnScreenDebugMessage(
			-1,
			2.0f,
			FColor::Cyan,
			TEXT("Ultimate Started - 8 Seconds")
		);
	}

	if (UltimateDuration <= 0.0f)
	{
		// 지속시간이 0이라면 즉시 궁극기 종료
		FinishUltimate();
		return true;
	}

	// 설정된 지속시간 후 궁극기 상태 종료
	GetWorld()->GetTimerManager().SetTimer(
		UltimateDurationTimerHandle,
		this,
		&UPlayerSkillComponent::FinishUltimate,
		UltimateDuration,
		false
	);

	// 궁극기 사용 성공 반환
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
		// 이미 활성화됐다면 중복 사용 불가
		return false;
	}

	if (!bRequireTargetStacks)
	{
		// 테스트 모드에서는 과녁 스택 없이 즉시 사용 가능
		return true;
	}

	// 실제 게임에서는 필요한 과녁 스택을 채워야 사용 가능
	return CurrentTargetStacks >= RequiredTargetStacks;
}

bool UPlayerSkillComponent::IsUltimateActive() const
{
	// 현재 궁극기 활성 상태 반환
	return bIsUltimateActive;
}

float UPlayerSkillComponent::GetUltimateRemainingTime() const
{
	if (!bIsUltimateActive || GetWorld() == nullptr)
	{
		// 궁극기가 꺼져 있거나 월드가 없다면 남은 시간 없음
		return 0.0f;
	}

	// 현재 궁극기 타이머의 남은 시간 반환
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

	// 궁극기 비활성 상태로 변경
	bIsUltimateActive = false;

	// 외부 시스템에 궁극기 종료 전달
	OnUltimateStateChanged.Broadcast(false);

	if (GEngine != nullptr)
	{
		// 8초 지속시간이 끝났는지 확인하는 테스트 메시지
		GEngine->AddOnScreenDebugMessage(
			-1,
			2.0f,
			FColor::White,
			TEXT("Ultimate Finished")
		);
	}
}

void UPlayerSkillComponent::BroadcastTargetStackChanged()
{
	// 현재 스택과 필요 스택을 UI에 전달
	OnUltimateTargetStackChanged.Broadcast(
		CurrentTargetStacks,
		RequiredTargetStacks
	);
}
