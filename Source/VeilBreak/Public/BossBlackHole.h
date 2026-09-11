#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BossBlackHole.generated.h"

class USphereComponent;
class ACharacter;
class UStaticMeshComponent;
class UMaterialInterface;
class UAudioComponent;

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

	/** 눈에 보이는 구체. 판정 범위(PullRadiusComponent)와는 별개의 순수 시각 요소 */
	UPROPERTY(VisibleAnywhere, Category = "BlackHole|Visual")
	TObjectPtr<UStaticMeshComponent> VisualSphere;

	/** 구체에 입힐 머티리얼. 나중에 아티스트가 만든 블랙홀 전용 머티리얼로 교체 가능. 비워두면 엔진 기본 회색 구체로 보임 */
	UPROPERTY(EditAnywhere, Category = "BlackHole|Visual")
	TObjectPtr<UMaterialInterface> BlackHoleMaterial;

	/**
	 * 손 위 구체를 중심으로 실제로 커지면서 판정 범위 끝까지 퍼져나가는 파동 원반.
	 * 머티리얼 안의 패턴이 아니라, 이 컴포넌트의 실제 스케일 값을 Tick에서 계속 키웠다 리셋하는 방식.
	 */
	UPROPERTY(VisibleAnywhere, Category = "BlackHole|Visual")
	TObjectPtr<UStaticMeshComponent> ShockwaveDisc;

	/** 파동 원반에 입힐 머티리얼. VisualSphere에 쓴 왜곡 머티리얼을 그대로 넣어도 됨 */
	UPROPERTY(EditAnywhere, Category = "BlackHole|Visual")
	TObjectPtr<UMaterialInterface> ShockwaveMaterial;

	/** 파동이 한 번 다 퍼지는(0 → PullRadius) 데 걸리는 시간(초). 다 퍼지면 즉시 리셋하고 다시 시작 */
	UPROPERTY(EditAnywhere, Category = "BlackHole|Visual", meta = (ClampMin = "0.1"))
	float ShockwaveInterval = 1.2f;

	/** 지금 파동이 시작된 후 몇 초 지났는지 (내부 계산용) */
	float ShockwaveElapsed = 0.f;

	/**
	 * 발동 중 계속 재생되는 루프 사운드 (웅웅거리는 흡입음 등).
	 * BP_BossBlackHole의 Components 패널에서 이 컴포넌트를 선택하고 Sound 슬롯에 루프 사운드를 직접 지정하면 됨
	 * (Visual Sphere에 Static Mesh 넣었던 것과 같은 방식).
	 */
	UPROPERTY(VisibleAnywhere, Category = "BlackHole|Visual")
	TObjectPtr<UAudioComponent> LoopingSound;

	// 나이아가라 이펙트(빨려들어가는 파티클)는 2단계에서 추가 예정.
	// 지금은 Tick의 DrawDebugSphere로 판정 범위를 대신 확인함.

	/** 당김 판정 반경 (uu 단위, 언리얼 기본 캐릭터 캡슐 반경이 약 34uu) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BlackHole|Config")
	float PullRadius = 6000.f;

	/** 끌려가는 속도. 걷기 400 < PullSpeed < 뛰기 650 사이로 맞춘 기본값 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BlackHole|Config")
	float PullSpeed = 500.f;

	/** 블랙홀 지속 시간 (초) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BlackHole|Config")
	float Duration = 20.f;

	bool bIsActive = false;

	FTimerHandle DeactivateTimerHandle;

	/** 현재 판정 범위 안에 들어와 있는 캐릭터. 싱글 플레이어라 여러 명 관리할 필요가 없어서 단일 포인터로 관리 */
	UPROPERTY()
	TObjectPtr<ACharacter> OverlappingCharacter;
};