#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TimerManager.h"
#include "PlayerCombatComponent.generated.h"

// 탄약이 변경될 때 UI에 현재 탄약을 전달
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnAmmoChanged,
	int32,
	CurrentAmmo,
	int32,
	ReserveAmmo
);

// 재장전 상태가 변경될 때 UI와 애니메이션에 전달
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FOnReloadStateChanged,
	bool,
	bIsReloading
);

// 사격이 정상적으로 실행됐을 때 외부 시스템에 전달
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnWeaponFired);

UCLASS(
	ClassGroup = (Custom),
	meta = (BlueprintSpawnableComponent)
)
class VEILBREAK_API UPlayerCombatComponent
	: public UActorComponent
{
	GENERATED_BODY()

public:
	// 생성자
	UPlayerCombatComponent();

public:
	// 현재 장전된 탄약으로 한 발 사격 시도
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void TryFire();

	// 실린더에 총알을 한 발씩 장전하기 시작
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void StartReload();

	// 사격 등의 행동으로 진행 중인 재장전 취소
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void CancelReload();

	// 보급 아이템으로 예비 탄약을 추가하고 실제 추가량 반환
	UFUNCTION(BlueprintCallable, Category = "Combat")
	int32 AddReserveAmmo(int32 AmmoAmount);

	// 현재 실린더에 장전된 탄약 반환
	UFUNCTION(BlueprintPure, Category = "Combat")
	int32 GetCurrentAmmo() const;

	// 현재 보유 중인 예비 탄약 반환
	UFUNCTION(BlueprintPure, Category = "Combat")
	int32 GetReserveAmmo() const;

	// 실린더의 최대 탄약 수 반환
	UFUNCTION(BlueprintPure, Category = "Combat")
	int32 GetCylinderCapacity() const;

	// 현재 재장전 중인지 반환
	UFUNCTION(BlueprintPure, Category = "Combat")
	bool IsReloading() const;

	// 궁극기 활성 상태에 따라 공격력과 재장전 속도 변경
	UFUNCTION(BlueprintCallable, Category = "Combat|Ultimate")
	void SetUltimateBuffActive(bool bEnableUltimateBuff);

public:
	// 현재 탄약 또는 예비 탄약이 변경됐을 때 호출
	UPROPERTY(BlueprintAssignable, Category = "Combat")
	FOnAmmoChanged OnAmmoChanged;

	// 재장전 시작 또는 종료 시 호출
	UPROPERTY(BlueprintAssignable, Category = "Combat")
	FOnReloadStateChanged OnReloadStateChanged;

	// 사격 애니메이션과 효과를 실행할 때 사용
	UPROPERTY(BlueprintAssignable, Category = "Combat")
	FOnWeaponFired OnWeaponFired;

protected:
	// Unreal Override
	virtual void BeginPlay() override;

private:
	// 카메라 중앙에서 직선 판정을 실행해 적에게 피해 적용
	void PerformHitScan();

	// 사격 시 카메라에 강한 반동 적용
	void ApplyRecoil();

	// 발사 간격이 끝난 후 다시 사격 가능 상태로 변경
	void ResetFireCooldown();

	// 재장전 시간이 끝날 때 실린더에 한 발 추가
	void HandleReloadRound();

	// 진행 중인 재장전 정상 종료
	void FinishReload();

	// 변경된 탄약 정보를 UI에 전달
	void BroadcastAmmoChanged();

private:
	// 탄약 설정

	// 실린더에 들어갈 수 있는 최대 탄약 수
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Ammo")
	int32 CylinderCapacity = 6;

	// 현재 실린더에 장전된 탄약 수
	UPROPERTY(VisibleInstanceOnly, Category = "Combat|Ammo")
	int32 CurrentAmmo = 6;

	// 플레이어가 보유할 수 있는 최대 예비 탄약 수
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Ammo")
	int32 MaxReserveAmmo = 24;

	// 현재 플레이어가 보유한 예비 탄약 수
	UPROPERTY(VisibleInstanceOnly, Category = "Combat|Ammo")
	int32 ReserveAmmo = 24;

private:
	// 무기 설정

	// 리볼버 한 발의 기본 공격력
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Weapon")
	float BaseDamage = 20.0f;

	// 한 번 발사한 후 다음 발사까지 필요한 시간
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Weapon")
	float FireInterval = 0.4f;

	// 카메라 중앙에서 발사되는 직선 판정 거리
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Weapon")
	float TraceDistance = 10000.0f;

	// 총알 한 발을 실린더에 넣는 데 필요한 시간
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Reload")
	float ReloadTimePerRound = 0.69f;

private:
	// 궁극기 설정

	// 궁극기 중 기본 공격력에 적용되는 배율
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Ultimate")
	float UltimateDamageMultiplier = 2.0f;

	// 궁극기 중 재장전 시간에 적용되는 배율
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Ultimate")
	float UltimateReloadTimeMultiplier = 0.5f;

	// 현재 기본 공격력에 적용되는 배율
	float CurrentDamageMultiplier = 1.0f;

	// 현재 한 발 재장전 시간에 적용되는 배율
	float CurrentReloadTimeMultiplier = 1.0f;

private:
	// 반동 설정

	// 사격 시 위쪽으로 적용되는 반동 크기
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Recoil")
	float VerticalRecoil = 5.0f;

	// 사격 시 좌우로 무작위 적용되는 최대 반동 크기
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Recoil")
	float HorizontalRecoil = 1.2f;

	// 테스트용 사격 경로를 화면에 표시할지 설정
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Debug")
	bool bDrawDebugTrace = true;

private:
	// 전투 상태

	// 현재 총을 다시 발사할 수 있는지 저장
	bool bCanFire = true;

	// 현재 한 발씩 재장전하고 있는지 저장
	bool bIsReloading = false;

	// 발사 간격을 관리하는 타이머
	FTimerHandle FireCooldownTimerHandle;

	// 한 발씩 진행되는 재장전을 관리하는 타이머
	FTimerHandle ReloadTimerHandle;
};
