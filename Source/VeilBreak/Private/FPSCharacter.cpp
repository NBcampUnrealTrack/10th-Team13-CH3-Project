#include "FPSCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputActionValue.h"
#include "PlayerCombatComponent.h"
#include "PlayerConsumableComponent.h"
#include "PlayerHealthComponent.h"
#include "PlayerSkillComponent.h"
#include "PlayerStaminaComponent.h"
#include "StatusEffectReceiverComponent.h"
#include "TimerManager.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimSequence.h"
#include "Components/SkeletalMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Sound/SoundBase.h"

AFPSCharacter::AFPSCharacter()
{
	// 부드러운 카메라 전환이 필요할 때만 Tick 사용
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	// 캐릭터의 좌우 방향은 항상 카메라 조준 방향을 따름
	// 위아래와 기울기는 몸 전체에 적용하지 않음
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = false;

	// 옆/뒤로 이동할 때 이동 방향으로 몸을 돌리지 않음
	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->bUseControllerDesiredRotation = false;

	// 이동 방향이 변경될 때의 캐릭터 회전 속도 설정
	GetCharacterMovement()->RotationRate = FRotator(
		0.0,
		500.0,
		0.0
	);

	// 기본 이동 속도를 걷기 속도로 설정
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;

	// 3인칭 카메라 거리를 관리하는 스프링암 생성
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(
		TEXT("CameraBoom")
	);
	CameraBoom->SetupAttachment(GetCapsuleComponent());
	CameraBoom->TargetArmLength = DefaultCameraDistance;
	CameraBoom->bUsePawnControlRotation = true;

	// 스프링암 끝에 3인칭 카메라 생성
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(
		TEXT("FollowCamera")
	);
	FollowCamera->SetupAttachment(
		CameraBoom,
		USpringArmComponent::SocketName
	);
	FollowCamera->bUsePawnControlRotation = false;
	FollowCamera->SetFieldOfView(DefaultFieldOfView);

	// 사격, 조준 및 재장전을 담당할 컴포넌트 생성
	PlayerCombatComponent =
		CreateDefaultSubobject<UPlayerCombatComponent>(
			TEXT("PlayerCombatComponent")
		);

	// 체력 물약의 보유량과 지속 회복을 관리할 컴포넌트 생성
	PlayerConsumableComponent =
		CreateDefaultSubobject<UPlayerConsumableComponent>(
			TEXT("PlayerConsumableComponent")
		);

	// 체력, 피해, 회복 및 사망 상태를 관리할 컴포넌트 생성
	PlayerHealthComponent =
		CreateDefaultSubobject<UPlayerHealthComponent>(
			TEXT("PlayerHealthComponent")
		);

	// 스킬과 궁극기를 담당할 컴포넌트 생성
	PlayerSkillComponent =
		CreateDefaultSubobject<UPlayerSkillComponent>(
			TEXT("PlayerSkillComponent")
		);

	// 달리기와 대시에 사용할 스태미나 컴포넌트 생성
	PlayerStaminaComponent =
		CreateDefaultSubobject<UPlayerStaminaComponent>(
			TEXT("PlayerStaminaComponent")
		);

	// 넉백과 경직 상태를 관리할 컴포넌트 생성
	StatusEffectReceiverComponent =
		CreateDefaultSubobject<UStatusEffectReceiverComponent>(
			TEXT("StatusEffectReceiverComponent")
		);
	// 회복과 궁극기 파티클은 필요할 때만 활성화
	PotionLoopComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("PotionLoopComponent"));
	PotionLoopComponent->SetupAttachment(GetCapsuleComponent());
	PotionLoopComponent->SetAutoActivate(false);

	UltimateLoopComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("UltimateLoopComponent"));
	UltimateLoopComponent->SetupAttachment(GetCapsuleComponent());
	UltimateLoopComponent->SetAutoActivate(false);
}

void AFPSCharacter::BeginPlay()
{
	Super::BeginPlay();

	// 기존 BP에 저장된 회전 기본값도 게임 시작 시 새 정책으로 적용
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = false;
	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->bUseControllerDesiredRotation = false;

	// 블루프린트에서 설정한 초기 이동 속도 적용
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;

	// 블루프린트에서 설정한 초기 카메라 값 적용
	CameraBoom->TargetArmLength = DefaultCameraDistance;
	FollowCamera->SetFieldOfView(DefaultFieldOfView);

	if (PlayerHealthComponent != nullptr)
	{
		// 체력이 0이 되면 캐릭터 사망 처리 함수 호출
		PlayerHealthComponent->OnPlayerDeath.AddUniqueDynamic(
			this,
			&AFPSCharacter::HandlePlayerDeath
		);
	}

	if (PlayerStaminaComponent != nullptr)
	{
		// 스태미나가 소진되면 달리기를 강제로 종료
		PlayerStaminaComponent->OnStaminaDepleted.AddUniqueDynamic(
			this,
			&AFPSCharacter::StopSprint
		);
	}

	if (PlayerSkillComponent != nullptr)
	{
		// 궁극기 시작과 종료 시 모든 강화 효과를 함께 적용
		PlayerSkillComponent->OnUltimateStateChanged.AddUniqueDynamic(
			this,
			&AFPSCharacter::HandleUltimateStateChanged
		);
	}


	// 실제 피해 이벤트에만 피격 효과 연결
	if (PlayerHealthComponent != nullptr)
	{
		PlayerHealthComponent->OnDamageReceived.AddUniqueDynamic(
			this, &AFPSCharacter::HandleDamageReceived
		);
	}

	// 물약 입력 성공 여부가 아니라 실제 회복 상태를 구독
	if (PlayerConsumableComponent != nullptr)
	{
		PlayerConsumableComponent->OnHealthPotionStateChanged.AddUniqueDynamic(
			this, &AFPSCharacter::HandlePotionStateChanged
		);

		// 이미 회복 중인 상태에서 시작된 경우에도 파티클 동기화
		if (PlayerConsumableComponent->IsHealingWithPotion())
		{
			HandlePotionStateChanged(true);
		}
	}

	// 현재 캐릭터를 조종하는 플레이어 컨트롤러 확인
	APlayerController* PlayerController =
		Cast<APlayerController>(Controller);

	if (PlayerController == nullptr)
	{
		// 플레이어 컨트롤러가 없으면 입력 설정 중단
		return;
	}

	// Enhanced Input을 사용할 로컬 플레이어 확인
	ULocalPlayer* LocalPlayer =
		PlayerController->GetLocalPlayer();

	if (LocalPlayer == nullptr)
	{
		// 로컬 플레이어가 없으면 입력 설정 중단
		return;
	}

	// 플레이어의 Enhanced Input Subsystem 가져오기
	UEnhancedInputLocalPlayerSubsystem* InputSubsystem =
		ULocalPlayer::GetSubsystem<
		UEnhancedInputLocalPlayerSubsystem
		>(LocalPlayer);

	if (
		InputSubsystem != nullptr &&
		PlayerMappingContext != nullptr
		)
	{
		// 플레이어 입력 매핑 활성화
		InputSubsystem->AddMappingContext(
			PlayerMappingContext,
			0
		);
	}
}

void AFPSCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 현재 조준 상태에 따라 카메라를 부드럽게 전환
	UpdateAimCamera(DeltaTime);
}

void AFPSCharacter::SetupPlayerInputComponent(
	UInputComponent* PlayerInputComponent
)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// 기본 입력 컴포넌트를 Enhanced Input 형식으로 변환
	UEnhancedInputComponent* EnhancedInputComponent =
		Cast<UEnhancedInputComponent>(PlayerInputComponent);

	if (EnhancedInputComponent == nullptr)
	{
		// Enhanced Input을 사용할 수 없으면 입력 연결 중단
		return;
	}

	if (MoveAction != nullptr)
	{
		// WASD 입력이 들어오는 동안 이동 처리
		EnhancedInputComponent->BindAction(
			MoveAction,
			ETriggerEvent::Triggered,
			this,
			&AFPSCharacter::Move
		);
	}

	if (LookAction != nullptr)
	{
		// 마우스 입력이 들어오는 동안 시점 회전 처리
		EnhancedInputComponent->BindAction(
			LookAction,
			ETriggerEvent::Triggered,
			this,
			&AFPSCharacter::Look
		);
	}

	if (JumpAction != nullptr)
	{
		// Space를 처음 누른 순간 점프 실행
		EnhancedInputComponent->BindAction(
			JumpAction,
			ETriggerEvent::Started,
			this,
			&AFPSCharacter::StartJump
		);

		// Space를 뗐을 때 점프 입력 종료
		EnhancedInputComponent->BindAction(
			JumpAction,
			ETriggerEvent::Completed,
			this,
			&AFPSCharacter::StopJump
		);
	}

	if (FireAction != nullptr)
	{
		// 마우스 왼쪽 버튼을 누른 순간 리볼버 사격 시도
		EnhancedInputComponent->BindAction(
			FireAction,
			ETriggerEvent::Started,
			this,
			&AFPSCharacter::StartFire
		);
	}

	if (ReloadAction != nullptr)
	{
		// R을 누른 순간 한 발씩 재장전 시작
		EnhancedInputComponent->BindAction(
			ReloadAction,
			ETriggerEvent::Started,
			this,
			&AFPSCharacter::StartReload
		);
	}

	if (PotionAction != nullptr)
	{
		// F를 누른 순간 체력 물약 사용 시도
		EnhancedInputComponent->BindAction(
			PotionAction,
			ETriggerEvent::Started,
			this,
			&AFPSCharacter::UseHealthPotion
		);
	}

	if (UltimateAction != nullptr)
	{
		// E를 누른 순간 8초 궁극기 사용 시도
		EnhancedInputComponent->BindAction(
			UltimateAction,
			ETriggerEvent::Started,
			this,
			&AFPSCharacter::StartUltimate
		);
	}

	if (SprintAction != nullptr)
	{
		// Shift를 처음 누른 순간 달리기 시작
		EnhancedInputComponent->BindAction(
			SprintAction,
			ETriggerEvent::Started,
			this,
			&AFPSCharacter::StartSprint
		);

		// Shift를 뗐을 때 달리기 종료
		EnhancedInputComponent->BindAction(
			SprintAction,
			ETriggerEvent::Completed,
			this,
			&AFPSCharacter::StopSprint
		);

		// 입력이 취소된 경우에도 달리기 종료
		EnhancedInputComponent->BindAction(
			SprintAction,
			ETriggerEvent::Canceled,
			this,
			&AFPSCharacter::StopSprint
		);
	}

	if (DashAction != nullptr)
	{
		// Q를 처음 누른 순간 대시 시도
		EnhancedInputComponent->BindAction(
			DashAction,
			ETriggerEvent::Started,
			this,
			&AFPSCharacter::StartDash
		);
	}

	if (AimAction != nullptr)
	{
		// 마우스 우클릭을 누른 순간 조준 시작
		EnhancedInputComponent->BindAction(
			AimAction,
			ETriggerEvent::Started,
			this,
			&AFPSCharacter::StartAim
		);

		// 마우스 우클릭을 뗐을 때 조준 종료
		EnhancedInputComponent->BindAction(
			AimAction,
			ETriggerEvent::Completed,
			this,
			&AFPSCharacter::StopAim
		);

		// 입력이 취소된 경우에도 조준 종료
		EnhancedInputComponent->BindAction(
			AimAction,
			ETriggerEvent::Canceled,
			this,
			&AFPSCharacter::StopAim
		);
	}
}

void AFPSCharacter::Move(
	const FInputActionValue& Value
)
{
	if (Controller == nullptr)
	{
		// 조종 중인 컨트롤러가 없으면 이동하지 않음
		return;
	}

	// IA_Move에서 전달된 좌우와 전후 입력값 가져오기
	const FVector2D MovementInput =
		Value.Get<FVector2D>();

	// 현재 카메라의 회전값 가져오기
	const FRotator ControlRotation =
		Controller->GetControlRotation();

	// 위아래 각도를 제외하고 좌우 회전만 사용
	const FRotator YawRotation(
		0.0,
		ControlRotation.Yaw,
		0.0
	);

	// 카메라 기준의 전방 방향 계산
	const FVector ForwardDirection =
		FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

	// 카메라 기준의 오른쪽 방향 계산
	const FVector RightDirection =
		FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	// W와 S 입력으로 전진과 후진 처리
	AddMovementInput(
		ForwardDirection,
		MovementInput.Y
	);

	// A와 D 입력으로 좌우 이동 처리
	AddMovementInput(
		RightDirection,
		MovementInput.X
	);
}

void AFPSCharacter::Look(
	const FInputActionValue& Value
)
{
	// IA_Look에서 전달된 마우스 이동값 가져오기
	const FVector2D LookInput =
		Value.Get<FVector2D>();

	// 마우스 가로 입력으로 카메라 좌우 회전
	AddControllerYawInput(LookInput.X);

	// 마우스 세로 입력값을 반대로 적용
	AddControllerPitchInput(-LookInput.Y);
}

void AFPSCharacter::StartJump()
{
	if (bDeathFeedbackPlayed || (PlayerHealthComponent && PlayerHealthComponent->IsDead()))
	{
		return;
	}
	// ACharacter가 제공하는 기본 점프 실행
	Jump();
}

void AFPSCharacter::StopJump()
{
	// 점프 입력이 끝났음을 ACharacter에 전달
	StopJumping();
}

void AFPSCharacter::StartFire()
{
	if (
		PlayerHealthComponent != nullptr &&
		PlayerHealthComponent->IsDead()
		)
	{
		// 사망한 상태에서는 사격 불가
		return;
	}

	if (
		StatusEffectReceiverComponent != nullptr &&
		StatusEffectReceiverComponent->IsCrowdControlled()
		)
	{
		// 경직이나 넉백 등의 CC 상태에서는 사격 불가
		return;
	}

	if (PlayerCombatComponent == nullptr)
	{
		// 전투 컴포넌트가 없으면 사격 불가
		return;
	}

	// 전투 컴포넌트에서 탄약과 발사 간격을 확인한 뒤 사격
	PlayerCombatComponent->TryFire();
}

void AFPSCharacter::StartReload()
{
	if (
		PlayerHealthComponent != nullptr &&
		PlayerHealthComponent->IsDead()
		)
	{
		// 사망한 상태에서는 재장전 불가
		return;
	}

	if (
		StatusEffectReceiverComponent != nullptr &&
		StatusEffectReceiverComponent->IsCrowdControlled()
		)
	{
		// 경직이나 넉백 등의 CC 상태에서는 재장전 불가
		return;
	}

	if (PlayerCombatComponent == nullptr)
	{
		// 전투 컴포넌트가 없으면 재장전 불가
		return;
	}

	// 실린더가 가득 차거나 예비 탄약이 없을 때는 컴포넌트가 거부
	PlayerCombatComponent->StartReload();
}

void AFPSCharacter::UseHealthPotion()
{
	if (
		PlayerHealthComponent != nullptr &&
		PlayerHealthComponent->IsDead()
		)
	{
		// 사망한 상태에서는 체력 물약 사용 불가
		return;
	}

	if (
		StatusEffectReceiverComponent != nullptr &&
		StatusEffectReceiverComponent->IsCrowdControlled()
		)
	{
		// 경직이나 넉백 등의 CC 상태에서는 물약 사용 불가
		return;
	}

	if (PlayerConsumableComponent == nullptr)
	{
		// 소모품 컴포넌트가 없다면 물약 사용 불가
		return;
	}

	// 보유량과 체력 상태를 확인한 뒤 체력 물약 사용
	PlayerConsumableComponent->UseHealthPotion();
}

void AFPSCharacter::StartUltimate()
{
	if (
		PlayerHealthComponent != nullptr &&
		PlayerHealthComponent->IsDead()
		)
	{
		// 사망한 상태에서는 궁극기 사용 불가
		return;
	}

	if (
		StatusEffectReceiverComponent != nullptr &&
		StatusEffectReceiverComponent->IsCrowdControlled()
		)
	{
		// 경직이나 넉백 등의 CC 상태에서는 궁극기 사용 불가
		return;
	}

	if (PlayerSkillComponent == nullptr)
	{
		// 스킬 컴포넌트가 없다면 궁극기 사용 불가
		return;
	}

	// 과녁 스택 조건을 만족하면 궁극기 활성화
	PlayerSkillComponent->ActivateUltimate();
}

void AFPSCharacter::HandleUltimateStateChanged(
	bool bUltimateActive
)
{
	// 사망 후 타이머 종료 이벤트로 효과가 다시 켜지지 않도록 처리
	if (bDeathFeedbackPlayed || (PlayerHealthComponent && PlayerHealthComponent->IsDead()))
	{
		StopPersistentFeedback();
		return;
	}

	if (bUltimateActive)
	{
		PlayFeedbackCue(UltimateStartFeedback, GetActorLocation(), GetActorRotation());

		if (UltimateLoopEffect != nullptr && UltimateLoopComponent != nullptr)
		{
			UltimateLoopComponent->SetAsset(UltimateLoopEffect.Get());
			UltimateLoopComponent->SetRelativeLocation(UltimateLoopOffset);
			UltimateLoopComponent->SetRelativeScale3D(FVector(FMath::Max(UltimateLoopScale, 0.01f)));
			UltimateLoopComponent->Activate(true);
		}
	}
	else
	{
		if (UltimateLoopComponent != nullptr)
		{
			UltimateLoopComponent->DeactivateImmediate();
		}
		PlayFeedbackCue(UltimateEndFeedback, GetActorLocation(), GetActorRotation());
	}
	// 캐릭터가 현재 궁극기 상태인지 저장
	bIsUltimateActive = bUltimateActive;

	if (PlayerCombatComponent != nullptr)
	{
		// 궁극기 중 공격력 2배와 재장전 시간 절반 적용
		PlayerCombatComponent->SetUltimateBuffActive(bUltimateActive);
	}

	if (PlayerStaminaComponent != nullptr)
	{
		// 궁극기 중 스태미나를 최대치로 유지하고 소모 방지
		PlayerStaminaComponent->SetInfiniteStamina(bUltimateActive);
	}

	if (bIsUltimateActive)
	{
		// 궁극기 8초 동안 Shift 입력 없이 상시 달리기 속도 적용
		GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;
		return;
	}

	// 궁극기 종료 시 지속 소모를 끄고 기본 걷기 속도로 복구
	StopSprint();
}

void AFPSCharacter::StartSprint()
{
	if (bIsUltimateActive)
	{
		// 궁극기 중에는 스태미나 소모 없이 달리기 속도 유지
		GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;
		return;
	}

	if (PlayerStaminaComponent == nullptr)
	{
		// 스태미나 컴포넌트가 없으면 달리기 불가
		return;
	}

	if (!PlayerStaminaComponent->StartSprintConsumption())
	{
		// 스태미나가 없으면 달리기를 시작하지 않음
		return;
	}

	// 스태미나 소모에 성공하면 달리기 속도 적용
	GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;
}

void AFPSCharacter::StopSprint()
{
	if (PlayerStaminaComponent != nullptr)
	{
		// 달리기에 사용되는 지속 스태미나 소모 중단
		PlayerStaminaComponent->StopSprintConsumption();
	}

	// 궁극기 중에는 Shift를 떼어도 상시 달리기 속도 유지
	GetCharacterMovement()->MaxWalkSpeed = bIsUltimateActive
		? SprintSpeed
		: WalkSpeed;
}

void AFPSCharacter::StartDash()
{
	if (bDeathFeedbackPlayed || (PlayerHealthComponent && PlayerHealthComponent->IsDead()))
	{
		return;
	}
	if (!bCanDash)
	{
		// 대시 재사용 대기시간 중이면 실행하지 않음
		return;
	}

	if (
		StatusEffectReceiverComponent != nullptr &&
		StatusEffectReceiverComponent->IsCrowdControlled()
		)
	{
		// 경직이나 넉백 등의 CC 상태에서는 대시 불가
		return;
	}

	if (PlayerStaminaComponent == nullptr)
	{
		// 스태미나 컴포넌트가 없으면 대시 불가
		return;
	}

	if (
		!PlayerStaminaComponent->TryConsumeStamina(
			DashStaminaCost
		)
		)
	{
		// 스태미나가 30 미만이면 대시 불가
		return;
	}

	// 대시 재사용 대기시간 시작
	bCanDash = false;

	// 마지막으로 입력한 WASD 이동 방향 가져오기
	FVector DashDirection =
		GetLastMovementInputVector();

	if (DashDirection.IsNearlyZero())
	{
		// 이동 입력이 없다면 캐릭터가 바라보는 방향 사용
		DashDirection = GetActorForwardVector();
	}

	// 대시는 수평 방향을 기준으로 계산
	DashDirection.Z = 0.0f;

	// 항상 같은 거리를 이동하도록 방향 벡터 정규화
	DashDirection.Normalize();

	// 대시를 시작하기 전 지상 상태인지 확인
	const bool bStartedOnGround =
		!GetCharacterMovement()->IsFalling();

	// 현재 캡슐 중심 위치 저장
	const FVector StartLocation =
		GetActorLocation();

	// 입력 방향을 기준으로 기본 목표 위치 계산
	const FVector DesiredLocation =
		StartLocation +
		DashDirection * DashDistance;

	// 장애물 검사 시 자기 자신을 제외
	FCollisionQueryParams DashQueryParams;
	DashQueryParams.AddIgnoredActor(this);

	// 바닥에 걸리지 않고 벽만 검사하도록
	// 캐릭터 캡슐보다 작은 구 형태를 사용
	const float ObstacleTraceRadius =
		GetCapsuleComponent()
		->GetScaledCapsuleRadius() * 0.8f;

	FHitResult ObstacleHit;

	// 캐릭터 중심 높이에서 대시 경로의 벽과 장애물 검사
	const bool bHitObstacle =
		GetWorld()->SweepSingleByChannel(
			ObstacleHit,
			StartLocation,
			DesiredLocation,
			FQuat::Identity,
			ECC_Visibility,
			FCollisionShape::MakeSphere(
				ObstacleTraceRadius
			),
			DashQueryParams
		);

	// 장애물이 없다면 원래 대시 목표 위치 사용
	FVector FinalLocation = DesiredLocation;

	if (bHitObstacle)
	{
		// 벽에 닿았다면 충돌 지점 바로 앞을 목표 위치로 사용
		FinalLocation =
			ObstacleHit.Location -
			DashDirection * 5.0f;
	}

	if (bStartedOnGround)
	{
		// 최종 목표 위치 위쪽에서 아래쪽으로 바닥 탐색
		const FVector GroundTraceStart =
			FinalLocation +
			FVector::UpVector * 300.0f;

		const FVector GroundTraceEnd =
			FinalLocation -
			FVector::UpVector * 600.0f;

		FHitResult GroundHit;

		const bool bFoundGround =
			GetWorld()->LineTraceSingleByChannel(
				GroundHit,
				GroundTraceStart,
				GroundTraceEnd,
				ECC_Visibility,
				DashQueryParams
			);

		if (
			bFoundGround &&
			GetCharacterMovement()->IsWalkable(
				GroundHit
			)
			)
		{
			// 캡슐이 찾은 바닥 위에 정확히 놓이도록 높이 계산
			const float CapsuleHalfHeight =
				GetCapsuleComponent()
				->GetScaledCapsuleHalfHeight();

			FinalLocation.Z =
				GroundHit.ImpactPoint.Z +
				CapsuleHalfHeight +
				2.0f;
		}
		else
		{
			// 걸을 수 있는 바닥이 없다면 현재 높이 유지
			FinalLocation.Z = StartLocation.Z;
		}
	}
	else
	{
		// 공중 대시는 시작 높이를 그대로 유지
		FinalLocation.Z = StartLocation.Z;
	}

	// 기존 이동 속도를 제거해 대시 후 미끄러짐 방지
	GetCharacterMovement()->StopMovementImmediately();

	// 경사면의 중간 충돌에 막히지 않도록
	// 계산이 끝난 최종 위치로 즉시 이동
	const bool bDashMoved = SetActorLocation(
		FinalLocation,
		false,
		nullptr,
		ETeleportType::TeleportPhysics
	);


	// 텔레포트 대시는 이동 후 출발/도착에 각각 단발 효과 재생
	// 실제 이동이 없는 경우에는 효과를 출력하지 않음
	if (bDashMoved && FVector::DistSquared(StartLocation, GetActorLocation()) > 1.0f)
	{
		PlayFeedbackCue(DashStartFeedback, StartLocation, DashDirection.Rotation());
		PlayFeedbackCue(DashEndFeedback, GetActorLocation(), DashDirection.Rotation());
	}

	// 설정한 대기시간 후 다시 대시할 수 있도록 타이머 실행
	FTimerHandle DashCooldownTimerHandle;

	GetWorldTimerManager().SetTimer(
		DashCooldownTimerHandle,
		this,
		&AFPSCharacter::ResetDash,
		FMath::Max(DashCooldown, 0.01f),
		false
	);
}
void AFPSCharacter::ResetDash()
{
	// 대시 재사용 대기시간 종료
	bCanDash = true;
}

void AFPSCharacter::StartAim()
{
	// 현재 캐릭터를 조준 상태로 변경
	bIsAiming = true;

	// 캐릭터 회전은 상시 조준 방향 고정. 우클릭은 카메라만 전환

	// 부드러운 카메라 전환을 위해 Tick 활성화
	SetActorTickEnabled(true);
}

void AFPSCharacter::StopAim()
{
	// 현재 캐릭터의 조준 상태 해제
	bIsAiming = false;

	// 우클릭을 떼어도 캐릭터는 계속 카메라 좌우 방향을 바라봄

	// 기본 카메라로 돌아가는 동안 Tick 활성화
	SetActorTickEnabled(true);
}

void AFPSCharacter::UpdateAimCamera(
	float DeltaTime
)
{
	if (
		CameraBoom == nullptr ||
		FollowCamera == nullptr
		)
	{
		// 카메라 컴포넌트가 없으면 Tick 종료
		SetActorTickEnabled(false);
		return;
	}

	// 현재 조준 상태에 맞는 목표 카메라 거리 결정
	const float TargetDistance =
		bIsAiming
		? AimCameraDistance
		: DefaultCameraDistance;

	// 현재 조준 상태에 맞는 목표 시야각 결정
	const float TargetFieldOfView =
		bIsAiming
		? AimFieldOfView
		: DefaultFieldOfView;

	// 카메라 거리를 목표 값까지 부드럽게 변경
	CameraBoom->TargetArmLength = FMath::FInterpTo(
		CameraBoom->TargetArmLength,
		TargetDistance,
		DeltaTime,
		AimInterpolationSpeed
	);

	// 카메라 시야각을 목표 값까지 부드럽게 변경
	const float NewFieldOfView = FMath::FInterpTo(
		FollowCamera->FieldOfView,
		TargetFieldOfView,
		DeltaTime,
		AimInterpolationSpeed
	);

	FollowCamera->SetFieldOfView(NewFieldOfView);

	// 카메라 거리가 목표 값에 도달했는지 확인
	const bool bDistanceFinished = FMath::IsNearlyEqual(
		CameraBoom->TargetArmLength,
		TargetDistance,
		0.5f
	);

	// 카메라 시야각이 목표 값에 도달했는지 확인
	const bool bFieldOfViewFinished = FMath::IsNearlyEqual(
		FollowCamera->FieldOfView,
		TargetFieldOfView,
		0.5f
	);

	if (bDistanceFinished && bFieldOfViewFinished)
	{
		// 오차 없이 정확한 최종 값으로 보정
		CameraBoom->TargetArmLength = TargetDistance;
		FollowCamera->SetFieldOfView(TargetFieldOfView);

		// 카메라 전환이 끝났으므로 Tick 비활성화
		SetActorTickEnabled(false);
	}
}

void AFPSCharacter::HandlePlayerDeath()
{
	if (bDeathFeedbackPlayed)
	{
		return;
	}
	bDeathFeedbackPlayed = true;

	// 재장전 종료 이벤트를 먼저 전달해 사망 애니메이션이 취소되지 않게 함
	if (PlayerCombatComponent != nullptr)
	{
		PlayerCombatComponent->CancelReload();
		PlayerCombatComponent->SetUltimateBuffActive(false);
	}
	if (PlayerStaminaComponent != nullptr)
	{
		PlayerStaminaComponent->SetInfiniteStamina(false);
	}
	bIsUltimateActive = false;
	StopJumping();
	StopPersistentFeedback();

	// 사망 사운드와 단발 효과
	PlayFeedbackCue(DeathFeedback, GetActorLocation(), GetActorRotation());

	// 사망 시 달리기와 스태미나 소모 중단
	StopSprint();

	// 사망 시 조준 상태 해제
	StopAim();

	// 사망 직전의 이동 속도를 즉시 제거
	GetCharacterMovement()->StopMovementImmediately();

	// 캐릭터 이동 기능 비활성화
	GetCharacterMovement()->DisableMovement();

	// 현재 플레이어 컨트롤러 확인
	APlayerController* PlayerController =
		Cast<APlayerController>(Controller);

	if (PlayerController != nullptr)
	{
		// 사망 후 플레이어 입력 비활성화
		DisableInput(PlayerController);
	}
	// 사망 시 기존 몽타주를 정리하고 단발 애니메이션 시퀀스 재생
	// 단일 노드 재생으로 마지막 자세를 유지
	if (GetMesh() != nullptr && DeathAnimation != nullptr)
	{
		if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
		{
			AnimInstance->StopAllMontages(0.0f);
		}
		GetMesh()->PlayAnimation(DeathAnimation.Get(), false);
	}
}


// 단발 효과는 월드 위치에 생성하며 수명이 끝나면 자동 제거
void AFPSCharacter::PlayFeedbackCue(
	const FPlayerFeedbackCue& Cue,
	const FVector& Location,
	const FRotator& Rotation
)
{
	if (GetWorld() == nullptr || GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	const FVector EffectLocation = Location + Rotation.RotateVector(Cue.LocationOffset);
	const FRotator EffectRotation =
		(Rotation.Quaternion() * Cue.RotationOffset.Quaternion()).Rotator();

	if (Cue.Effect != nullptr)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			this, Cue.Effect.Get(), EffectLocation, EffectRotation,
			FVector(FMath::Max(Cue.Scale, 0.01f)), true, true
		);
	}
	if (Cue.Sound != nullptr)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this, Cue.Sound.Get(), EffectLocation, FMath::Max(Cue.Volume, 0.0f)
		);
	}
}

FVector AFPSCharacter::GetFeetLocation() const
{
	return GetActorLocation() - FVector::UpVector *
		GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
}

void AFPSCharacter::OnJumped_Implementation()
{
	Super::OnJumped_Implementation();

	// 입력만 누른 것이 아니라 실제 점프가 성공했을 때 실행
	if (!bDeathFeedbackPlayed && (!PlayerHealthComponent || !PlayerHealthComponent->IsDead()))
	{
		PlayFeedbackCue(JumpFeedback, GetFeetLocation(), GetActorRotation());
	}
}

void AFPSCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);

	if (!bDeathFeedbackPlayed && (!PlayerHealthComponent || !PlayerHealthComponent->IsDead()))
	{
		PlayFeedbackCue(
			LandFeedback,
			Hit.ImpactPoint + Hit.ImpactNormal * 2.0f,
			GetActorRotation()
		);
	}
}

void AFPSCharacter::PlayFootstepFeedback(FName FootSocketName)
{
	// 공중, 정지, 사망 중 발소리 차단
	if (GetWorld() == nullptr || bDeathFeedbackPlayed ||
		(PlayerHealthComponent && PlayerHealthComponent->IsDead()) ||
		!GetCharacterMovement()->IsMovingOnGround() ||
		GetVelocity().SizeSquared2D() < FMath::Square(FMath::Max(FootstepMinimumSpeed, 0.0f)))
	{
		return;
	}

	const double Now = GetWorld()->GetTimeSeconds();
	if (Now - LastFootstepTime < FMath::Max(FootstepMinimumInterval, 0.0f))
	{
		return;
	}

	// 발 본/소켓이 없으면 캡슐 발밑을 기준으로 처리
	FVector FootLocation = GetFeetLocation();
	if (GetMesh() && !FootSocketName.IsNone() && GetMesh()->DoesSocketExist(FootSocketName))
	{
		FootLocation = GetMesh()->GetSocketLocation(FootSocketName);
	}

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);
	FHitResult GroundHit;

	// 발 아래 실제 지면을 찾아 먼지가 공중에서 나오지 않도록 처리
	const bool bGroundHit = GetWorld()->LineTraceSingleByChannel(
		GroundHit,
		FootLocation + FVector::UpVector * 30.0f,
		FootLocation - FVector::UpVector * FMath::Max(FootstepTraceDistance, 1.0f),
		ECC_Visibility,
		Params
	);

	if (!bGroundHit || !GetCharacterMovement()->IsWalkable(GroundHit))
	{
		return;
	}

	LastFootstepTime = Now;
	PlayFeedbackCue(
		FootstepFeedback,
		GroundHit.ImpactPoint + GroundHit.ImpactNormal * 2.0f,
		GetActorRotation()
	);
}

void AFPSCharacter::PlayReloadFeedback()
{
	// 취소되거나 사망한 재장전에서는 장전음 재생 금지
	if (bDeathFeedbackPlayed ||
		(PlayerHealthComponent && PlayerHealthComponent->IsDead()) ||
		!PlayerCombatComponent || !PlayerCombatComponent->IsReloading() ||
		ReloadSound == nullptr)
	{
		return;
	}

	FVector SoundLocation = GetActorLocation();
	if (GetMesh() && GetMesh()->DoesSocketExist(ReloadSoundSocketName))
	{
		SoundLocation = GetMesh()->GetSocketLocation(ReloadSoundSocketName);
	}
	UGameplayStatics::PlaySoundAtLocation(
		this, ReloadSound.Get(), SoundLocation, FMath::Max(ReloadSoundVolume, 0.0f)
	);
}

void AFPSCharacter::HandleDamageReceived(float DamageAmount)
{
	// 치명타격에서는 일반 피격 효과 대신 사망 효과만 재생
	if (DamageAmount <= 0.0f || GetWorld() == nullptr || bDeathFeedbackPlayed ||
		(PlayerHealthComponent && PlayerHealthComponent->GetCurrentHealth() <= 0.0f))
	{
		return;
	}
	const double Now = GetWorld()->GetTimeSeconds();
	if (Now - LastDamageFeedbackTime < FMath::Max(DamageFeedbackInterval, 0.0f))
	{
		return;
	}
	LastDamageFeedbackTime = Now;

	// 현재 피해 API에는 피격 좌표가 없어 캐릭터 중심 기준으로 재생
	PlayFeedbackCue(DamageFeedback, GetActorLocation(), GetActorRotation());
}

void AFPSCharacter::HandlePotionStateChanged(bool bHealing)
{
	if (!bHealing || bDeathFeedbackPlayed ||
		(PlayerHealthComponent && PlayerHealthComponent->IsDead()))
	{
		if (PotionLoopComponent != nullptr)
		{
			PotionLoopComponent->DeactivateImmediate();
		}
		return;
	}

	PlayFeedbackCue(PotionStartFeedback, GetActorLocation(), GetActorRotation());
	if (PotionLoopEffect != nullptr && PotionLoopComponent != nullptr)
	{
		PotionLoopComponent->SetAsset(PotionLoopEffect.Get());
		PotionLoopComponent->SetRelativeLocation(PotionLoopOffset);
		PotionLoopComponent->SetRelativeScale3D(FVector(FMath::Max(PotionLoopScale, 0.01f)));
		PotionLoopComponent->Activate(true);
	}
}

void AFPSCharacter::StopPersistentFeedback()
{
	// 루프 에셋도 확실하게 제거하도록 즉시 비활성화
	if (PotionLoopComponent != nullptr)
	{
		PotionLoopComponent->DeactivateImmediate();
	}
	if (UltimateLoopComponent != nullptr)
	{
		UltimateLoopComponent->DeactivateImmediate();
	}
}

void AFPSCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopPersistentFeedback();
	Super::EndPlay(EndPlayReason);
}
