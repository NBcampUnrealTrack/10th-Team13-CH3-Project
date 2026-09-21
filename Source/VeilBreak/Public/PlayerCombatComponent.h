#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TimerManager.h"
#include "PlayerCombatComponent.generated.h"

// 탄약 변경 이벤트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnAmmoChanged,
	int32, CurrentAmmo,
	int32, ReserveAmmo
);

// 재장전 상태 변경 이벤트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FOnReloadStateChanged,
	bool, bIsReloading
);

// 정상 발사 이벤트
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnWeaponFired);

// 보스 명중 이벤트
// DamageAmount는 실제 HP 감소량이 아닌 이번 공격의 데미지
// HitLocation은 월드 좌표 기준 명중 위치
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnBossHitConfirmed,
	float, DamageAmount,
	FVector, HitLocation
);

UCLASS(
	ClassGroup = (Custom),
	meta = (BlueprintSpawnableComponent)
)
class VEILBREAK_API UPlayerCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// 생성자
	UPlayerCombatComponent();

	// 한 발 사격 시도
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void TryFire();

	// 한 발씩 재장전 시작
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void StartReload();

	// 진행 중인 재장전 취소
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void CancelReload();

	// 예비 탄약 추가 후 실제 추가량 반환
	UFUNCTION(BlueprintCallable, Category = "Combat")
	int32 AddReserveAmmo(int32 AmmoAmount);

	// 현재 장전된 탄약 반환
	UFUNCTION(BlueprintPure, Category = "Combat")
	int32 GetCurrentAmmo() const;

	// 현재 예비 탄약 반환
	UFUNCTION(BlueprintPure, Category = "Combat")
	int32 GetReserveAmmo() const;

	// 최대 장전 수 반환
	UFUNCTION(BlueprintPure, Category = "Combat")
	int32 GetCylinderCapacity() const;

	// 재장전 중인지 반환
	UFUNCTION(BlueprintPure, Category = "Combat")
	bool IsReloading() const;

	// 궁극기 공격력 및 재장전 배율 적용
	UFUNCTION(BlueprintCallable, Category = "Combat|Ultimate")
	void SetUltimateBuffActive(bool bEnableUltimateBuff);

public:
	// 탄약 UI 갱신
	UPROPERTY(BlueprintAssignable, Category = "Combat")
	FOnAmmoChanged OnAmmoChanged;

	// 재장전 UI 및 애니메이션 갱신
	UPROPERTY(BlueprintAssignable, Category = "Combat")
	FOnReloadStateChanged OnReloadStateChanged;

	// 발사 애니메이션 및 효과 실행
	UPROPERTY(BlueprintAssignable, Category = "Combat")
	FOnWeaponFired OnWeaponFired;

	// 보스 명중 시 히트마커 및 데미지 텍스트 출력
	UPROPERTY(BlueprintAssignable, Category = "Combat|Hit")
	FOnBossHitConfirmed OnBossHitConfirmed;

protected:
	// 게임 시작 시 초기화
	virtual void BeginPlay() override;

private:
	// 카메라 중앙에서 명중 검사 및 피해 적용
	void PerformHitScan();

	// 과녁 명중 시 궁극기 스택 처리
	void HandleUltimateTargetHit(AActor* HitActor);

	// 사격 반동 적용
	void ApplyRecoil();

	// 발사 간격 종료
	void ResetFireCooldown();

	// 총알 한 발 장전
	void HandleReloadRound();

	// 재장전 종료
	void FinishReload();

	// 탄약 변경 이벤트 전달
	void BroadcastAmmoChanged();

private:
	// 실린더 최대 탄약 수
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Ammo")
	int32 CylinderCapacity = 6;

	// 현재 장전된 탄약
	UPROPERTY(VisibleInstanceOnly, Category = "Combat|Ammo")
	int32 CurrentAmmo = 6;

	// 최대 예비 탄약
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Ammo")
	int32 MaxReserveAmmo = 24;

	// 현재 예비 탄약
	UPROPERTY(VisibleInstanceOnly, Category = "Combat|Ammo")
	int32 ReserveAmmo = 24;

private:
	// 한 발의 기본 공격력
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Weapon")
	float BaseDamage = 20.0f;

	// 발사 간격
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Weapon")
	float FireInterval = 0.4f;

	// 명중 검사 거리
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Weapon")
	float TraceDistance = 10000.0f;

	// 한 발 재장전 시간
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Reload")
	float ReloadTimePerRound = 0.69f;

private:
	// 보스 명중 판별에 사용할 클래스
	// 캐릭터 BP의 전투 컴포넌트에서 BP_BossCharacterBase 지정
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Hit")
	TSubclassOf<AActor> BossActorClass;

private:
	// 궁극기 공격력 배율
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Ultimate")
	float UltimateDamageMultiplier = 2.0f;

	// 궁극기 재장전 시간 배율
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Ultimate")
	float UltimateReloadTimeMultiplier = 0.5f;

	// 현재 공격력 배율
	float CurrentDamageMultiplier = 1.0f;

	// 현재 재장전 시간 배율
	float CurrentReloadTimeMultiplier = 1.0f;

private:
	// 수직 반동
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Recoil")
	float VerticalRecoil = 4.0f;

	// 좌우 반동
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Recoil")
	float HorizontalRecoil = 1.2f;

	// 사격 경로 디버그 표시
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Debug")
	bool bDrawDebugTrace = true;

private:
	// 사격 가능 여부
	bool bCanFire = true;

	// 재장전 여부
	bool bIsReloading = false;

	// 중복 스택 획득을 방지하기 위한 과녁 기록
	TSet<TWeakObjectPtr<AActor>> HitUltimateTargets;

	// 발사 간격 타이머
	FTimerHandle FireCooldownTimerHandle;

	// 재장전 타이머
	FTimerHandle ReloadTimerHandle;
};