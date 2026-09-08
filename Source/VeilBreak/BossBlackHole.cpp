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
	PullRadiusComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	PullRadiusComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	PullRadiusComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
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

	for (ACharacter* Character : AffectedCharacters)
	{
		if (IsValid(Character))
		{
			ApplyPullToCharacter(Character, DeltaTime);
		}
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

	// 1차 구현: 매 프레임 속도를 직접 덮어씀.
	// 플레이어가 반대 방향으로 이동 입력을 넣으면 힘겨루기하는 느낌이 날 수 있는데,
	// 이건 실제 플레이 테스트로 손맛을 본 다음 필요하면 RootMotionSource 등으로 교체할 부분.
	MoveComp->Velocity = PullDirection * PullSpeed;
}

void ABossBlackHole::OnPullRadiusBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (ACharacter* Character = Cast<ACharacter>(OtherActor))
	{
		AffectedCharacters.AddUnique(Character);
	}
}

void ABossBlackHole::OnPullRadiusEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (ACharacter* Character = Cast<ACharacter>(OtherActor))
	{
		AffectedCharacters.Remove(Character);
	}
}

