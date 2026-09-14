#include "BossBlackHole.h"
#include "BossCharacterBase.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Components/AudioComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "DrawDebugHelpers.h"
#include "UObject/ConstructorHelpers.h"

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

	// 눈에 보이는 구체. 판정용이 아니라 순수 장식이라 콜리전은 꺼둠
	VisualSphere = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualSphere"));
	VisualSphere->SetupAttachment(Root);
	VisualSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	VisualSphere->SetCastShadow(false);
	VisualSphere->SetVisibility(false); // 발동 전엔 숨김

	// 엔진 기본 제공 구체 메시. 나중에 아티스트가 만든 전용 메시로 교체 가능
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMeshFinder(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMeshFinder.Succeeded())
	{
		VisualSphere->SetStaticMesh(SphereMeshFinder.Object);
	}

	// 발동 중 계속 도는 루프 사운드. Sound 애셋은 여기서 지정 안 하고
	// BP_BossBlackHole의 Components 패널에서 직접 할당함 (VisualSphere 메시랑 같은 방식)
	LoopingSound = CreateDefaultSubobject<UAudioComponent>(TEXT("LoopingSound"));
	LoopingSound->SetupAttachment(Root);
	LoopingSound->bAutoActivate = false; // BeginPlay/스폰 즉시 재생되지 않게, Activate 호출 시에만 재생

	// 손 위 구체를 중심으로 사방으로 부풀어오르는 파동. 평평한 원반이 아니라
	// 실제로 커지는 얇은 구 껍질(shell)로 만들어서 "사방으로 퍼진다"는 느낌을 줌
	ShockwaveDisc = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShockwaveDisc"));
	ShockwaveDisc->SetupAttachment(Root);
	ShockwaveDisc->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ShockwaveDisc->SetCastShadow(false);
	ShockwaveDisc->SetVisibility(false); // 발동 전엔 숨김

	if (SphereMeshFinder.Succeeded())
	{
		ShockwaveDisc->SetStaticMesh(SphereMeshFinder.Object);
	}
}

void ABossBlackHole::BeginPlay()
{
	Super::BeginPlay();

	PullRadiusComponent->SetSphereRadius(PullRadius);
	PullRadiusComponent->OnComponentBeginOverlap.AddDynamic(this, &ABossBlackHole::OnPullRadiusBeginOverlap);
	PullRadiusComponent->OnComponentEndOverlap.AddDynamic(this, &ABossBlackHole::OnPullRadiusEndOverlap);

	// VisualSphere 크기는 여기서 자동 계산하지 않음.
	// BP_BossBlackHole의 Class Defaults에서 아티스트가 Transform > Scale로 직접 조절한 값을 그대로 씀.
	if (BlackHoleMaterial)
	{
		VisualSphere->SetMaterial(0, BlackHoleMaterial);
	}

	// RangeDistortionSphere는 반대로 자동 계산함 - 실제 판정 반경(PullRadius)이랑
	// 시각적으로 어긋나면 안 되는 값이라, 엔진 기본 구체 메시(반지름 50uu 고정)를 기준으로 역산
	if (ShockwaveMaterial)
	{
		ShockwaveDisc->SetMaterial(0, ShockwaveMaterial);
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
	VisualSphere->SetVisibility(true);
	ShockwaveDisc->SetVisibility(true);
	ShockwaveElapsed = 0.f; // 파동을 처음(크기 0)부터 다시 시작

	if (LoopingSound && LoopingSound->Sound)
	{
		LoopingSound->Play();
	}

	UE_LOG(LogTemp, Log, TEXT("[BossBlackHole] Activated (Radius=%.0f, Speed=%.0f, Duration=%.1f)"), PullRadius, PullSpeed, Duration);
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, Duration, FColor::Magenta, TEXT("BlackHole Activated"));
	}

	// BT가 종료 호출을 놓치는 경우를 대비한 자체 타이머
	GetWorldTimerManager().SetTimer(DeactivateTimerHandle, this, &ABossBlackHole::DeactivateBlackHole, Duration, false);

	// 발동되는 이 순간 이미 범위 안에 서 있는 캐릭터가 있을 수 있음
	// (예: 보스 손에 스폰되자마자 플레이어가 이미 근접해있는 경우).
	// OnComponentBeginOverlap은 "들어오는 순간"에만 터지고 "이미 들어와 있는 상태"는 못 잡기 때문에,
	// 여기서 한 번 직접 훑어서 놓치지 않게 함.
	TArray<AActor*> AlreadyOverlapping;
	PullRadiusComponent->GetOverlappingActors(AlreadyOverlapping, ACharacter::StaticClass());
	for (AActor* Actor : AlreadyOverlapping)
	{
		if (ACharacter* Character = Cast<ACharacter>(Actor))
		{
			if (Character->IsPlayerControlled() && !Character->IsA<ABossCharacterBase>())
			{
				OverlappingCharacter = Character;
				break;
			}
		}
	}
}

void ABossBlackHole::DeactivateBlackHole()
{
	if (!bIsActive)
	{
		return;
	}

	bIsActive = false;
	SetActorTickEnabled(false);
	VisualSphere->SetVisibility(false);
	ShockwaveDisc->SetVisibility(false);
	LoopingSound->Stop();
	GetWorldTimerManager().ClearTimer(DeactivateTimerHandle);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::White, TEXT("BlackHole Deactivated"));
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

	// 파동 원반을 실제로 키움: 0초일 땐 크기 0, ShockwaveInterval초가 지나면 PullRadius 크기가 됨.
	// 그 순간 ShockwaveElapsed를 0으로 리셋해서 처음부터 다시 시작 -> 계속 반복되는 파동
	ShockwaveElapsed += DeltaTime;
	if (ShockwaveElapsed >= ShockwaveInterval)
	{
		ShockwaveElapsed = 0.f;
	}
	const float ShockwaveProgress = ShockwaveElapsed / ShockwaveInterval; // 0~1
	constexpr float DefaultEngineSphereRadius = 50.f; // 엔진 기본 구체 메시의 실제 반지름(uu)
	// (1 - Progress)를 써서 반대로 만듦: 0초일 땐 PullRadius(범위 끝)만큼 크다가,
	// ShockwaveInterval초가 지나면 크기 0(중심)까지 줄어듦 -> 밖에서 안으로 빨려들어가는 파동
	const float CurrentWorldRadius = (1.f - ShockwaveProgress) * PullRadius;
	ShockwaveDisc->SetRelativeScale3D(FVector(CurrentWorldRadius / DefaultEngineSphereRadius));

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
		// 1차 필터: AI가 조종하는 보스는 제외
		// 2차 필터(이중 안전장치): 혹시 테스트 중 Possess 등으로 보스를 사람이 조종하게 되더라도,
		// ABossCharacterBase 계열이면 어쨌든 당김 대상에서 제외
		if (Character->IsPlayerControlled() && !Character->IsA<ABossCharacterBase>())
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