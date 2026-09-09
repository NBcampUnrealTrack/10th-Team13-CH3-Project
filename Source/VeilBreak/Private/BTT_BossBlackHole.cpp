#include "BTT_BossBlackHole.h"
#include "BossBlackHole.h"
#include "BossCharacterBase.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"

UBTT_BossBlackHole::UBTT_BossBlackHole()
{
	// BT 에디터의 New Task 목록과 트리 노드에 표시될 이름
	NodeName = TEXT("BossBlackHole");

	// TickTask를 실제로 호출받으려면 이 플래그가 켜져 있어야 함
	bNotifyTick = true;
}

EBTNodeResult::Type UBTT_BossBlackHole::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	FBTBlackHoleMemory* Memory = reinterpret_cast<FBTBlackHoleMemory*>(NodeMemory);
	Memory->ElapsedTime = 0.f;
	Memory->SpawnedBlackHole = nullptr;

	if (!BlackHoleClass)
	{
		// 클래스가 지정 안 된 상태 - BT 노드 Details 패널에서 Black Hole Class를 꼭 지정해야 함
		return EBTNodeResult::Failed;
	}

	AAIController* AIController = OwnerComp.GetAIOwner();
	ABossCharacterBase* Boss = AIController ? Cast<ABossCharacterBase>(AIController->GetPawn()) : nullptr;
	if (!Boss)
	{
		return EBTNodeResult::Failed;
	}

	// 소켓이 있으면 소켓 위치, 없으면 캡슐 상단 + 여유 높이로 대체
	FVector SpawnLocation;
	if (Boss->GetMesh() && Boss->GetMesh()->DoesSocketExist(SpawnSocketName))
	{
		SpawnLocation = Boss->GetMesh()->GetSocketLocation(SpawnSocketName);
	}
	else
	{
		const float HalfHeight = Boss->GetCapsuleComponent() ? Boss->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 88.f;
		SpawnLocation = Boss->GetActorLocation() + FVector(0.f, 0.f, HalfHeight + 50.f);
	}

	FActorSpawnParameters Params;
	Params.Owner = Boss;
	Params.Instigator = Boss;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ABossBlackHole* Spawned = OwnerComp.GetWorld()->SpawnActor<ABossBlackHole>(BlackHoleClass, SpawnLocation, FRotator::ZeroRotator, Params);
	if (!Spawned)
	{
		return EBTNodeResult::Failed;
	}

	// 보스 메시에 붙여서, 보스가 움직이면 블랙홀도 같이 따라가게 함
	if (Boss->GetMesh() && Boss->GetMesh()->DoesSocketExist(SpawnSocketName))
	{
		Spawned->AttachToComponent(Boss->GetMesh(), FAttachmentTransformRules::KeepWorldTransform, SpawnSocketName);
	}

	Spawned->ActivateBlackHole();
	Memory->SpawnedBlackHole = Spawned;

	// 아직 안 끝났다는 뜻. BT는 이 상태를 계속 유지하면서 매 프레임 TickTask를 불러줌
	return EBTNodeResult::InProgress;
}

void UBTT_BossBlackHole::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	FBTBlackHoleMemory* Memory = reinterpret_cast<FBTBlackHoleMemory*>(NodeMemory);
	Memory->ElapsedTime += DeltaSeconds;

	if (Memory->ElapsedTime >= ActiveDuration)
	{
		CleanUpBlackHole(Memory);
		// 이제 끝났다고 BT에게 알림 -> 여기서부터 다음 노드(또는 처음부터 재시작)로 넘어감
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}
}

EBTNodeResult::Type UBTT_BossBlackHole::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	// 보스가 죽거나 다른 이유로 BT가 이 Task를 중간에 강제 종료시킬 때 호출됨
	FBTBlackHoleMemory* Memory = reinterpret_cast<FBTBlackHoleMemory*>(NodeMemory);
	CleanUpBlackHole(Memory);
	return EBTNodeResult::Aborted;
}

void UBTT_BossBlackHole::CleanUpBlackHole(FBTBlackHoleMemory* Memory)
{
	if (Memory->SpawnedBlackHole.IsValid())
	{
		Memory->SpawnedBlackHole->DeactivateBlackHole();
		Memory->SpawnedBlackHole->Destroy();
	}
	Memory->SpawnedBlackHole = nullptr;
}