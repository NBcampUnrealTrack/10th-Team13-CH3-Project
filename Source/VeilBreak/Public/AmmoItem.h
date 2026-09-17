#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AmmoItem.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UPrimitiveComponent;

UCLASS()
class VEILBREAK_API AAmmoItem : public AActor
{
	GENERATED_BODY()

public:
	AAmmoItem();

protected:
	virtual void BeginPlay() override;

	// 플레이어 접근을 감지하는 영역
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ammo Item")
	TObjectPtr<USphereComponent> PickupSphere;

	// 아이템 외형
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ammo Item")
	TObjectPtr<UStaticMeshComponent> ItemMesh;

	// 아이템 하나가 지급할 예비 탄약 수
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Ammo Item",
		meta = (ClampMin = "1")
	)
	int32 AmmoAmount = 6;

private:
	// 이벤트가 중복 발생하더라도 탄약 중복 지급 방지
	bool bPickupInProgress = false;

	UFUNCTION()
	void OnPickupBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	);

	void TryPickup(AActor* OtherActor);
};