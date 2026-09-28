#include "BTT_BossBlackHole.h"
#include "BossBlackHole.h"
#include "BossCharacterBase.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Sound/SoundBase.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"

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

	// 소켓이 있으면 소켓의 로컬 좌표계 기준으로 오프셋을 적용한 위치, 없으면 캡슐 상단 + 여유 높이로 대체
	FVector SpawnLocation;
	if (Boss->GetMesh() && Boss->GetMesh()->DoesSocketExist(SpawnSocketName))
	{
		// GetSocketTransform으로 소켓의 위치+회전을 같이 가져와서,
		// SpawnOffset을 "소켓이 보는 방향 기준"으로 변환함 (손이 어떻게 돌아가 있어도 항상 같은 방향으로 띄워짐)
		const FTransform SocketTransform = Boss->GetMesh()->GetSocketTransform(SpawnSocketName);
		SpawnLocation = SocketTransform.TransformPosition(SpawnOffset);
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

	// BT에서 설정한 지속시간을 액터한테 그대로 알려줌.
	// 이걸 안 하면 액터 자체 기본값(Duration)이랑 여기 ActiveDuration이 어긋날 때
	// 항상 더 짧은 쪽이 먼저 꺼버려서, BT에서 시간을 늘려도 반영 안 되는 것처럼 보임
	Spawned->SetDuration(ActiveDuration);

	Spawned->ActivateBlackHole();
	Memory->SpawnedBlackHole = Spawned;

	// 발동 사운드 재생 (블랙홀 스폰 위치에서 3D로 재생, 한 번만)
	if (ActivationSound)
	{
		UGameplayStatics::PlaySoundAtLocation(Boss, ActivationSound, SpawnLocation, ActivationSoundVolume);
	}

	// 반복 사운드는 손 소켓에 붙여서 계속 따라다니게 재생 (Duration 끝나면 CleanUpBlackHole에서 정지)
	if (LoopingSound && Boss->GetMesh())
	{
		UAudioComponent* LoopComp = UGameplayStatics::SpawnSoundAttached(
			LoopingSound,
			Boss->GetMesh(),
			SpawnSocketName,
			FVector::ZeroVector,
			EAttachLocation::SnapToTargetIncludingScale,
			/*bStopWhenAttachedToDestroyed=*/ false,
			LoopingSoundVolume
		);
		Memory->LoopingSoundComponent = LoopComp;
	}

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

	// 반복 재생 중이던 사운드 정지
	if (Memory->LoopingSoundComponent.IsValid())
	{
		Memory->LoopingSoundComponent->Stop();
		Memory->LoopingSoundComponent->DestroyComponent();
	}
	Memory->LoopingSoundComponent = nullptr;
}