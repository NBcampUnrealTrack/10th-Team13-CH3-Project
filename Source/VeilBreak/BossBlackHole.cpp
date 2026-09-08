#include "BossBlackHole.h"
#include "Components/SphereComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "DrawDebugHelpers.h"

ABossBlackHole::ABossBlackHole()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false; // 발동 전엔 Tick 꺼둠

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	PullRadiusComponent = CreateDefaultSubobject<USphereComponent>(TEXT("PullRadiusComponent"));
	PullRadiusComponent->SetupAttachment(Root);
	PullRadiusComponent->SetSphereRadius(PullRadius);
	// 캐릭터 캡슐이 기본 Pawn 프로필일 때 서로 Block으로 엇갈려서
	// Overlap 이벤트가 안 터지는 문제를 피하려고, 언리얼이 미리 준비해둔
	// "무조건 겹치기만 하는" 전용 프로필을 씀. 트리거/판정 볼륨엔 이게 정석.
	PullRadiusComponent->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
}

void ABossBlackHole::BeginPlay()
{
	Super::BeginPlay();

	PullRadiusComponent->SetSphereRadius(PullRadius);
	PullRadiusComponent->OnComponentBeginOverlap.AddDynamic(this, &ABossBlackHole::OnPullRadiusBeginOverlap);
	PullRadiusComponent->OnComponentEndOverlap.AddDynamic(this, &ABossBlackHole::OnPullRadiusEndOverlap);

	// 테스트 모드: BT 없이도 레벨에 놓기만 하면 잠시 후 자동으로 첫 발동
	if (bAutoActivateForTesting)
	{
		GetWorldTimerManager().SetTimer(AutoTestTimerHandle, this, &ABossBlackHole::ActivateBlackHole, InitialTestDelay, false);
	}
}

void ABossBlackHole::ActivateBlackHole()
{
	if (bIsActive)
	{
		return;
	}

	bIsActive = true;
	SetActorTickEnabled(true);

	UE_LOG(LogTemp, Log, TEXT("[BossBlackHole] Activated (Radius=%.0f, Speed=%.0f, Duration=%.1f)"), PullRadius, PullSpeed, Duration);
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, Duration, FColor::Magenta, TEXT("BlackHole Activated"));
	}

	// BT가 종료 호출을 놓치는 경우를 대비한 자체 타이머 (테스트 모드에선 이 타이머가 곧 발동 시간)
	GetWorldTimerManager().SetTimer(DeactivateTimerHandle, this, &ABossBlackHole::DeactivateBlackHole, Duration, false);
}

void ABossBlackHole::DeactivateBlackHole()
{
	if (!bIsActive)
	{
		return;
	}

	bIsActive = false;
	SetActorTickEnabled(false);
	GetWorldTimerManager().ClearTimer(DeactivateTimerHandle);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::White, TEXT("BlackHole Deactivated"));
	}

	// 테스트 모드면 잠시 쉬었다가 다시 자동 발동 (반복 테스트용)
	if (bAutoActivateForTesting)
	{
		GetWorldTimerManager().SetTimer(AutoTestTimerHandle, this, &ABossBlackHole::ActivateBlackHole, TestCooldown, false);
	}
}

void ABossBlackHole::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bIsActive)
	{
		return;
	}

	// 판정 범위를 눈으로 볼 수 있도록 표시 (테스트용. 실제 출시 빌드에선 지워도 됨)
	DrawDebugSphere(GetWorld(), GetActorLocation(), PullRadius, 24, FColor::Purple, false, -1.f, 0, 1.5f);

	if (IsValid(OverlappingCharacter))
	{
		ApplyPullToCharacter(OverlappingCharacter, DeltaTime);
	}
}

void ABossBlackHole::ApplyPullToCharacter(ACharacter* Character, float DeltaTime)
{
	UCharacterMovementComponent* MoveComp = Character->GetCharacterMovement();
	if (!MoveComp)
	{
		return;
	}

	const FVector PullDirection = (GetActorLocation() - Character->GetActorLocation()).GetSafeNormal();
	const FVector PullVelocity = PullDirection * PullSpeed;

	// 플레이어가 그 순간 실제로 이동하려던 방향/세기.
	// MaxWalkSpeed는 걷기(400)/뛰기(650) 상태에 따라 이미 팀원 캐릭터 코드에서 바뀌어 있으므로,
	// 여기서 다시 SprintSpeed인지 WalkSpeed인지 따로 안 물어봐도 자동으로 반영된다.
	const FVector InputDir = Character->GetLastMovementInputVector();
	const FVector PlayerIntendedVelocity = InputDir.IsNearlyZero()
		? FVector::ZeroVector
		: InputDir.GetSafeNormal() * MoveComp->MaxWalkSpeed;

	// 당김 속도 + 플레이어가 내려던 속도를 그대로 합산.
	// 반대 방향으로 뛰면(전력질주 650 > 당김 500) 벡터가 상쇄되어 실제로 빠져나갈 수 있고,
	// 걷기(400)만으로는 500을 못 이겨서 계속 끌려간다.
	MoveComp->Velocity = PullVelocity + PlayerIntendedVelocity;
}

void ABossBlackHole::OnPullRadiusBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (ACharacter* Character = Cast<ACharacter>(OtherActor))
	{
		// 보스 본인(AI 컨트롤러가 조종)이나 다른 AI 캐릭터는 무시하고,
		// 플레이어가 직접 조종하는 캐릭터만 당김 대상으로 추적한다.
		// 이게 없으면 블랙홀이 보스 위치에 붙어있을 때 보스 자신도 끌어당기려고 해서
		// 보스 AI의 이동을 매 프레임 방해하게 된다.
		if (Character->IsPlayerControlled())
		{
			OverlappingCharacter = Character;
		}
	}
}

void ABossBlackHole::OnPullRadiusEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (ACharacter* Character = Cast<ACharacter>(OtherActor))
	{
		if (OverlappingCharacter == Character)
		{
			OverlappingCharacter = nullptr;
		}
	}
}