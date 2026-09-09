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
#include "PlayerHealthComponent.h"
#include "PlayerSkillComponent.h"
#include "PlayerStaminaComponent.h"
#include "StatusEffectReceiverComponent.h"
#include "TimerManager.h"

AFPSCharacter::AFPSCharacter()
{
	// 부드러운 카메라 전환이 필요할 때만 Tick 사용
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	// 평상시에는 카메라 회전이 캐릭터에 직접 적용되지 않도록 설정
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// 평상시에는 캐릭터가 이동하는 방향을 바라보도록 설정
	GetCharacterMovement()->bOrientRotationToMovement = true;

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
}

void AFPSCharacter::BeginPlay()
{
	Super::BeginPlay();

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
	// ACharacter가 제공하는 기본 점프 실행
	Jump();
}

void AFPSCharacter::StopJump()
{
	// 점프 입력이 끝났음을 ACharacter에 전달
	StopJumping();
}

void AFPSCharacter::StartSprint()
{
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

	// 기본 걷기 속도로 복구
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
}

void AFPSCharacter::StartDash()
{
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
	SetActorLocation(
		FinalLocation,
		false,
		nullptr,
		ETeleportType::TeleportPhysics
	);

	// 설정한 대기시간 후 다시 대시할 수 있도록 타이머 실행
	FTimerHandle DashCooldownTimerHandle;

	GetWorldTimerManager().SetTimer(
		DashCooldownTimerHandle,
		this,
		&AFPSCharacter::ResetDash,
		DashCooldown,
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

	// 조준 중에는 캐릭터가 카메라 좌우 방향을 바라보게 설정
	bUseControllerRotationYaw = true;

	// 조준 중에는 이동 방향 자동 회전을 비활성화
	GetCharacterMovement()->bOrientRotationToMovement = false;

	// 부드러운 카메라 전환을 위해 Tick 활성화
	SetActorTickEnabled(true);
}

void AFPSCharacter::StopAim()
{
	// 현재 캐릭터의 조준 상태 해제
	bIsAiming = false;

	// 카메라 방향에 따른 캐릭터 회전 해제
	bUseControllerRotationYaw = false;

	// 다시 이동 방향을 바라보도록 설정
	GetCharacterMovement()->bOrientRotationToMovement = true;

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
}