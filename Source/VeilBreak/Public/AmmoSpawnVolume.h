#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AmmoSpawnVolume.generated.h"

class UBoxComponent;
class AAmmoItem;

UCLASS()
class VEILBREAK_API AAmmoSpawnVolume : public AActor
{
	GENERATED_BODY()

public:
	AAmmoSpawnVolume();

	// 볼륨 안에 탄약 생성. 이미 실행한 경우 중복 실행하지 않음
	UFUNCTION(BlueprintCallable, Category = "Ammo Spawn")
	void SpawnAmmo();

	// 생성한 탄약 제거 및 생성 상태 초기화
	UFUNCTION(BlueprintCallable, Category = "Ammo Spawn")
	void ClearSpawnedAmmo();

protected:
	// 생성할 공간
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ammo Spawn")
	TObjectPtr<UBoxComponent> SpawnBox;

	// BP_AmmoItem 지정
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ammo Spawn")
	TSubclassOf<AAmmoItem> AmmoItemClass;

	// 이 볼륨에서 생성할 아이템 수
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ammo Spawn",
		meta = (ClampMin = "1"))
	int32 SpawnCount = 5;

	// 아이템 한 개당 위치 탐색 최대 시도 횟수
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ammo Spawn",
		meta = (ClampMin = "1"))
	int32 MaxAttemptsPerItem = 30;

	// 생성 위치 주변에 확보할 공간의 반지름
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ammo Spawn",
		meta = (ClampMin = "1.0"))
	float ClearanceRadius = 40.0f;

	// 확보한 공간과 바닥 사이의 여유
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ammo Spawn",
		meta = (ClampMin = "1.0"))
	float GroundGap = 5.0f;

	// 아이템끼리 떨어져 있어야 하는 최소 거리
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ammo Spawn",
		meta = (ClampMin = "0.0"))
	float MinSpacing = 200.0f;

	// 이보다 가파른 표면에는 생성하지 않음
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ammo Spawn",
		meta = (ClampMin = "0.0", ClampMax = "60.0"))
	float MaxGroundSlopeDegrees = 30.0f;

private:
	bool FindSpawnLocation(FVector& OutLocation) const;

	bool bSpawnRequested = false;

	// 플레이어가 습득하여 제거된 아이템은 자동으로 무효화되는 약한 참조
	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<AAmmoItem>> SpawnedItems;
};