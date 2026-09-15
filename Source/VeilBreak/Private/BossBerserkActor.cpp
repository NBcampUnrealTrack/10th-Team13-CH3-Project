#include "BossBerserkActor.h"
#include "BossCharacterBase.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

// 사격 판정 콜리전과 발악 구체 메시·머티리얼 생성
ABossBerserkActor::ABossBerserkActor()
{
	PrimaryActorTick.bCanEverTick = false;
	HitCollision = CreateDefaultSubobject<USphereComponent>(TEXT("HitCollision"));
	SetRootComponent(HitCollision);
	HitCollision->InitSphereRadius(90.f);
	HitCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	HitCollision->SetCollisionObjectType(ECC_WorldDynamic);
	HitCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
	// 기존 Visibility 사격과 WeaponTrace용 GameTraceChannel1 모두 허용
	HitCollision->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	HitCollision->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Block);

	OrbMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("OrbMesh"));
	OrbMesh->SetupAttachment(HitCollision);
	OrbMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Game/BlinkAndDashVFX/Meshes/SM_VFX_Sphere.SM_VFX_Sphere"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> AuraMaterial(TEXT("/Game/BlinkAndDashVFX/Materials/Blink_Curse/MI_VFX_Sphere_Aura.MI_VFX_Sphere_Aura"));
	if (SphereMesh.Succeeded()) OrbMesh->SetStaticMesh(SphereMesh.Object);
	if (AuraMaterial.Succeeded()) OrbMesh->SetMaterial(0, AuraMaterial.Object);
}

// 첫 유효 사격 피해에서 보스에 파괴 통지 후 구체 제거
float ABossBerserkActor::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (DamageAmount <= 0.f || bDestroyedByDamage) return 0.f;
	bDestroyedByDamage = true;
	if (ABossCharacterBase* Boss = Cast<ABossCharacterBase>(GetOwner())) Boss->HandleBerserkOrbDestroyed(this);
	Destroy();
	return DamageAmount;
}
