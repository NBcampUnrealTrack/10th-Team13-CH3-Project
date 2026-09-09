#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BossBlackHole.generated.h"

class USphereComponent;
class ACharacter;

/**
 * 보스 패턴 - 블랙홀
 * 발동 시 일정 반경 안의 캐릭터를 이 액터 방향으로 일정 속도로 끌어당긴다.
 * BT의 BTT_BossBlackHole이 ActivateBlackHole() / DeactivateBlackHole()을 호출해서 제어한다.
 */
UCLASS()
class VEILBREAK_API ABossBlackHole : public AActor
{
	GENERATED_BODY()

public:
	ABossBlackHole();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	/** BT Task에서 호출할 함수: 블랙홀 발동 시작 */
	UFUNCTION(BlueprintCallable, Category = "BlackHole")
	void ActivateBlackHole();

	/** BT Task에서 호출할 함수: 블랙홀 종료 */
	UFUNCTION(BlueprintCallable, Category = "BlackHole")
	void DeactivateBlackHole();

protected:
	UFUNCTION()
	void OnPullRadiusBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnPullRadiusEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	/** 실제 당김 속도 계산 및 적용 */
	void ApplyPullToCharacter(ACharacter* Character, float DeltaTime);

protected:
	UPROPERTY(VisibleAnywhere, Category = "BlackHole")
	TObjectPtr<USceneComponent> Root;

	/** 당김 판정 범위 (오버랩 트리거) */
	UPROPERTY(VisibleAnywhere, Category = "BlackHole")
	TObjectPtr<USphereComponent> PullRadiusComponent;

	// 나이아가라 이펙트는 나중에 연출 붙일 때 다시 추가.
	// 지금은 Tick의 DrawDebugSphere로 범위를 대신 확인함.

	/** 당김 판정 반경 (uu 단위, 언리얼 기본 캐릭터 캡슐 반경이 약 34uu) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BlackHole|Config")
	float PullRadius = 2000.f;

	/** 끌려가는 속도. 걷기 400 < PullSpeed < 뛰기 650 사이로 맞춘 기본값 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BlackHole|Config")
	float PullSpeed = 500.f;

	/** 블랙홀 지속 시간 (초) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BlackHole|Config")
	float Duration = 3.f;

	bool bIsActive = false;

	FTimerHandle DeactivateTimerHandle;

	/** 현재 판정 범위 안에 들어와 있는 캐릭터. 싱글 플레이어라 여러 명 관리할 필요가 없어서 단일 포인터로 관리 */
	UPROPERTY()
	TObjectPtr<ACharacter> OverlappingCharacter;
};