#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TimerManager.h"
#include "PlayerCombatComponent.generated.h"

class UNiagaraSystem;
class USoundBase;

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
// DamageAmount는 실제 HP 감소량이 아닌 공격 데미지
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

	// 발사 애니메이션 실행
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

	// 총구 소켓에서 발사 이펙트와 사운드 재생
	void PlayFireEffects();

	// 명중 위치에서 이펙트와 사운드 재생
	void PlayImpactEffects(
		const FHitResult& HitResult,
		bool bHitBoss
	);

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
	// 보스 본체 판별에 사용할 BP 클래스
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Hit")
	TSubclassOf<AActor> BossActorClass;

private:
	// 캐릭터 메시의 총구 소켓 이름
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Effects|Fire")
	FName MuzzleSocketName = TEXT("FX_Gun_Barrel");

	// 총구에서 재생할 나이아가라
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Effects|Fire")
	TObjectPtr<UNiagaraSystem> MuzzleEffect;

	// 총구 소켓 기준 이펙트 위치 보정
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Effects|Fire")
	FVector MuzzleEffectLocationOffset = FVector::ZeroVector;

	// 총구 소켓 기준 이펙트 회전 보정
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Effects|Fire")
	FRotator MuzzleEffectRotationOffset = FRotator::ZeroRotator;

	// 총구 이펙트 크기
	UPROPERTY(
		EditDefaultsOnly,
		Category = "Combat|Effects|Fire",
		meta = (ClampMin = "0.01")
	)
	float MuzzleEffectScale = 1.0f;

	// 실제 발사 성공 시 재생할 소리
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Effects|Fire")
	TObjectPtr<USoundBase> FireSound;

	// 발사음 음량 배율
	UPROPERTY(
		EditDefaultsOnly,
		Category = "Combat|Effects|Fire",
		meta = (ClampMin = "0.0")
	)
	float FireSoundVolume = 1.0f;

private:
	// 벽과 일반 대상에 명중했을 때 사용할 이펙트
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Effects|Impact")
	TObjectPtr<UNiagaraSystem> DefaultImpactEffect;

	// 벽과 일반 대상에 명중했을 때 사용할 소리
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Effects|Impact")
	TObjectPtr<USoundBase> DefaultImpactSound;

	// 보스 본체에 명중했을 때 사용할 이펙트
	// 미지정 시 일반 명중 이펙트 사용
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Effects|Impact")
	TObjectPtr<UNiagaraSystem> BossImpactEffect;

	// 보스 본체에 명중했을 때 사용할 소리
	// 미지정 시 일반 명중 소리 사용
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Effects|Impact")
	TObjectPtr<USoundBase> BossImpactSound;

	// 명중 이펙트 크기
	UPROPERTY(
		EditDefaultsOnly,
		Category = "Combat|Effects|Impact",
		meta = (ClampMin = "0.01")
	)
	float ImpactEffectScale = 1.0f;

	// 표면 안에 묻히지 않도록 이펙트를 바깥으로 이동할 거리
	UPROPERTY(
		EditDefaultsOnly,
		Category = "Combat|Effects|Impact",
		meta = (ClampMin = "0.0")
	)
	float ImpactEffectSurfaceOffset = 2.0f;

	// 이펙트의 기본 진행 축이 다를 때 회전 보정
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Effects|Impact")
	FRotator ImpactEffectRotationOffset = FRotator::ZeroRotator;

	// 명중음 음량 배율
	UPROPERTY(
		EditDefaultsOnly,
		Category = "Combat|Effects|Impact",
		meta = (ClampMin = "0.0")
	)
	float ImpactSoundVolume = 1.0f;

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
	float VerticalRecoil = 5.0f;

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

	// 중복 스택 획득 방지용 과녁 기록
	TSet<TWeakObjectPtr<AActor>> HitUltimateTargets;

	// 발사 간격 타이머
	FTimerHandle FireCooldownTimerHandle;

	// 재장전 타이머
	FTimerHandle ReloadTimerHandle;
};