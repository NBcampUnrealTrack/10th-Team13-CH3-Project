#include "StatusEffectReceiverComponent.h"

UStatusEffectReceiverComponent::
UStatusEffectReceiverComponent()
{
	// CC 상태는 이벤트가 발생할 때만 바뀌므로 Tick 비활성화
	PrimaryComponentTick.bCanEverTick = false;
}

void UStatusEffectReceiverComponent::BeginPlay()
{
	Super::BeginPlay();

	// 게임 시작 시 적용 중인 CC가 없는 상태로 초기화
	ActiveCrowdControlCount = 0;
}

void UStatusEffectReceiverComponent::AddCrowdControl()
{
	// 첫 번째 CC가 적용되는지 확인
	const bool bWasCrowdControlled =
		IsCrowdControlled();

	// 현재 적용 중인 CC 효과 개수 증가
	++ActiveCrowdControlCount;

	if (!bWasCrowdControlled)
	{
		// CC 없음에서 CC 활성 상태로 바뀐 경우에만 이벤트 전달
		OnCrowdControlStateChanged.Broadcast(true);
	}
}

void UStatusEffectReceiverComponent::RemoveCrowdControl()
{
	if (ActiveCrowdControlCount <= 0)
	{
		// 제거할 CC가 없으면 음수가 되지 않도록 중단
		ActiveCrowdControlCount = 0;
		return;
	}

	// 종료된 CC 효과 하나를 개수에서 제거
	--ActiveCrowdControlCount;

	if (ActiveCrowdControlCount == 0)
	{
		// 모든 CC가 끝났을 때 행동 가능 상태를 외부에 전달
		OnCrowdControlStateChanged.Broadcast(false);
	}
}

void UStatusEffectReceiverComponent::ClearCrowdControl()
{
	if (ActiveCrowdControlCount == 0)
	{
		// 이미 정상 상태라면 추가 처리를 하지 않음
		return;
	}

	// 사망이나 리스폰 등에 대비해 모든 CC 상태 초기화
	ActiveCrowdControlCount = 0;

	// 행동 가능 상태로 돌아왔음을 외부에 전달
	OnCrowdControlStateChanged.Broadcast(false);
}

bool UStatusEffectReceiverComponent::
IsCrowdControlled() const
{
	// 적용 중인 CC가 하나 이상이면 행동 불가 상태로 판단
	return ActiveCrowdControlCount > 0;
}