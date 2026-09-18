#include "BTS_BossFlyChase.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

UBTS_BossFlyChase::UBTS_BossFlyChase()
{
	NodeName = TEXT("BossFlyChase");

	// 도착/거리 체크는 매 프레임 안 해도 충분하므로 0.2초마다 한 번씩만 갱신
	Interval = 0.2f;
	RandomDeviation = 0.f;
}

void UBTS_BossFlyChase::OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	Super::OnBecomeRelevant(OwnerComp, NodeMemory);
	bIsChasing = false;
	bIsFleeing = false;
	bHasLiftedOff = false;
	// 비행 모드 전환은 여기서 안 함 - 게임 시작(1페이즈)부터 이 노드가 항상 활성 상태라서,
	// 여기서 바로 켜버리면 1페이즈부터 날게 됨. 실제 전환은 TickNode에서 페이즈 체크 후 처리함
}

void UBTS_BossFlyChase::OnCeaseRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	if (AAIController* AIController = OwnerComp.GetAIOwner())
	{
		AIController->StopMovement();
	}

	Super::OnCeaseRelevant(OwnerComp, NodeMemory);
}

void UBTS_BossFlyChase::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	AAIController* AIController = OwnerComp.GetAIOwner();
	APawn* Boss = AIController ? AIController->GetPawn() : nullptr;
	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	if (!AIController || !Boss || !Blackboard)
	{
		return;
	}

	// 지정한 페이즈가 아니면 아무것도 안 함. BTD_CheckPhase랑 완전히 같은 방식으로 비교함
	// (Blackboard의 CurrentPhase는 Int로 저장되어 있고, EBossPhase를 int32로 캐스팅해서 비교)
	const int32 CurrentPhase = Blackboard->GetValueAsInt(PhaseKeyName);
	if (CurrentPhase != static_cast<int32>(RequiredPhase))
	{
		return;
	}

	// 2페이즈에 막 들어온 순간에도, 아직 비행 모드가 아니면 여기서 전환.
	// 이미 Flying이면 매번 다시 호출해도 무해해서 조건 없이 그냥 보장해둠
	if (UCharacterMovementComponent* MoveComp = Boss->FindComponentByClass<UCharacterMovementComponent>())
	{
		if (MoveComp->MovementMode != MOVE_Flying)
		{
			MoveComp->SetMovementMode(MOVE_Flying);
			MoveComp->MaxFlySpeed = ChaseSpeed;
		}
	}

	// 2페이즈 들어온 첫 순간, 플레이어가 이미 가까이 있어서 추격 로직이 안 켜지더라도
	// 무조건 한 번은 제자리에서 위로 떠오르게 함 (안 그러면 바닥에 그대로 서있는 채 Flying 모드만 켜진 이상한 상태가 됨)
	if (!bHasLiftedOff)
	{
		const FVector LiftOffDestination = Boss->GetActorLocation() + FVector(0.f, 0.f, FlightAltitude);
		AIController->MoveToLocation(
			LiftOffDestination,
			AcceptanceRadius,
			/*bStopOnOverlap=*/ false,
			/*bUsePathfinding=*/ false,
			/*bProjectDestinationToNavigation=*/ false,
			/*bCanStrafe=*/ true
		);
		bHasLiftedOff = true;
	}

	// BTS_UpdateBossContext(황승용님이 이미 만들어둔 서비스)가 매 프레임 갱신해주는 값을 그대로 읽음
	const float TargetDistance = Blackboard->GetValueAsFloat(TEXT("TargetDistance"));
	AActor* TargetActor = Cast<AActor>(Blackboard->GetValueAsObject(TEXT("TargetActor")));
	if (!TargetActor)
	{
		return;
	}

	// 도망 상태 전환 (히스테리시스: 시작 기준 FleeDistance, 멈추는 기준 FleeStopDistance)
	if (!bIsFleeing && TargetDistance < FleeDistance)
	{
		bIsFleeing = true;
		bIsChasing = false; // 도망 시작하면 추격 중이었더라도 취소
	}
	else if (bIsFleeing && TargetDistance >= FleeStopDistance)
	{
		AIController->StopMovement();
		bIsFleeing = false;
	}

	if (bIsFleeing)
	{
		// 플레이어 -> 보스 방향(수평만)으로 밀려나는 목적지 계산.
		// 목표 지점은 "플레이어 기준으로 FleeStopDistance만큼 떨어진 자리"로 잡아서,
		// 도망이 끝나는 지점(FleeStopDistance)까지 자연스럽게 밀려나게 함
		FVector AwayDirection = Boss->GetActorLocation() - TargetActor->GetActorLocation();
		AwayDirection.Z = 0.f;
		if (AwayDirection.IsNearlyZero())
		{
			// 보스가 플레이어 바로 위(수평 거리 0)에 있으면 방향이 안 정해지니, 보고 있는 반대 방향으로 대체
			AwayDirection = -Boss->GetActorForwardVector();
			AwayDirection.Z = 0.f;
		}
		AwayDirection.Normalize();

		const FVector FleeDestination = TargetActor->GetActorLocation() + AwayDirection * FleeStopDistance + FVector(0.f, 0.f, FlightAltitude);
		AIController->MoveToLocation(
			FleeDestination,
			AcceptanceRadius,
			/*bStopOnOverlap=*/ false,
			/*bUsePathfinding=*/ false,
			/*bProjectDestinationToNavigation=*/ false,
			/*bCanStrafe=*/ true
		);
		return; // 도망 중일 땐 추격 로직은 아예 안 봄 (우선순위: 도망 > 추격)
	}

	if (!bIsChasing && TargetDistance > ChaseStartDistance)
	{
		// 플레이어 위치보다 FlightAltitude만큼 위를 목적지로 잡음.
		// bProjectDestinationToNavigation을 false로 둬서, 목적지가 바닥(NavMesh) 높이로
		// 강제로 눌리지 않고 공중 그 위치 그대로 유지되게 함
		const FVector Destination = TargetActor->GetActorLocation() + FVector(0.f, 0.f, FlightAltitude);
		AIController->MoveToLocation(
			Destination,
			AcceptanceRadius,
			/*bStopOnOverlap=*/ false,
			// NavMesh는 바닥에만 생성되고 공중엔 안 생기기 때문에, 직선으로 날아가게 함
			/*bUsePathfinding=*/ false,
			/*bProjectDestinationToNavigation=*/ false,
			/*bCanStrafe=*/ true
		);
		bIsChasing = true;
	}
	else if (bIsChasing && TargetDistance <= ChaseStopDistance)
	{
		AIController->StopMovement();
		bIsChasing = false;
	}
	else if (bIsChasing)
	{
		// 추격 중엔 플레이어가 계속 움직이니, 목적지도 매 틱 갱신해줘야 계속 따라감
		const FVector Destination = TargetActor->GetActorLocation() + FVector(0.f, 0.f, FlightAltitude);
		AIController->MoveToLocation(
			Destination,
			AcceptanceRadius,
			/*bStopOnOverlap=*/ false,
			/*bUsePathfinding=*/ false,
			/*bProjectDestinationToNavigation=*/ false,
			/*bCanStrafe=*/ true
		);
	}
	else
	{
		// 추격도 도망도 아닌 "가만히 떠있는" 상태.
		// 수평 위치(X, Y)는 지금 있는 자리 그대로 두고, 높이(Z)만 플레이어 기준으로 계속 맞춰줌.
		// 안 그러면 플레이어가 아래로 내려가도 보스는 예전 높이에 계속 떠있게 됨
		const FVector CurrentLocation = Boss->GetActorLocation();
		const float DesiredZ = TargetActor->GetActorLocation().Z + FlightAltitude;

		// 이미 거의 맞는 높이면 매 틱 이동 명령을 다시 안 보내서, 불필요하게 흔들리지 않게 함
		if (!FMath::IsNearlyEqual(CurrentLocation.Z, DesiredZ, AcceptanceRadius))
		{
			const FVector HeightOnlyDestination(CurrentLocation.X, CurrentLocation.Y, DesiredZ);
			AIController->MoveToLocation(
				HeightOnlyDestination,
				AcceptanceRadius,
				/*bStopOnOverlap=*/ false,
				/*bUsePathfinding=*/ false,
				/*bProjectDestinationToNavigation=*/ false,
				/*bCanStrafe=*/ true
			);
		}
	}
}