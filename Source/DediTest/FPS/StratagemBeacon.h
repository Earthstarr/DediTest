#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FPSCharacter.h"
#include "Eagle.h"

#include "StratagemBeacon.generated.h"

class UNiagaraSystem;
class UNiagaraComponent;

UCLASS()
class DEDITEST_API AStratagemBeacon : public AActor
{
    GENERATED_BODY()

public:
    AStratagemBeacon();

    // 캐릭터에서 전달받을 스트라타젬 타입
    EStratagemType MyStratagemType;

    // 외부(C++)에서 색상을 설정하고 나이아가라 파라미터를 업데이트하는 함수
    void UpdateBeaconVisual(FLinearColor NewColor);

protected:
    virtual void BeginPlay() override;

    // 물리적 형태와 충돌을 담당 (루트 컴포넌트)
    UPROPERTY(VisibleAnywhere, Category = "Components")
    class USphereComponent* CollisionComp;

    // 비컨의 외형 (작은 원통이나 구체 에셋)
    UPROPERTY(VisibleAnywhere, Category = "Components")
    class UStaticMeshComponent* MeshComp;

    // 투사체 컴포넌트
    UPROPERTY(VisibleAnywhere, Category = "Components")
    class UProjectileMovementComponent* ProjectileMovement;

    // 바닥에 부딪혔을 때 호출될 함수
    UFUNCTION()
    void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

    // 바닥에 부착시 사운드
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
    class USoundBase* BeaconActivateSound;

    // 폭격 대기 시간
    UPROPERTY(EditAnywhere, Category = "Stratagem")
    float DelayToStrike = 3.8f;

    // 폭격 실행할 함수
    void TriggerStrike();

    // -------------------- Stratagem 액터 클래스 --------------------

    // 500kg 포탄 액터
    UPROPERTY(EditAnywhere, Category = "Stratagem")
    TSubclassOf<AActor> Eagle500kgProjectileClass;

    // 보급품 포드 액터
    UPROPERTY(EditAnywhere, Category = "Stratagem")
    TSubclassOf<AActor> SupplyClass;

    // 센트리 액터
    UPROPERTY(EditAnywhere, Category = "Stratagem")
    TSubclassOf<AActor> SentryClass;

    // 궤도 레이저 액터
    UPROPERTY(EditAnywhere, Category = "Stratagem")
    TSubclassOf<AActor> OrbitalLaserClass;

    // -------------------- Effects --------------------

    // 빨간 빛기둥 이펙트
    UPROPERTY(EditAnywhere, Category = "Effects")
    UNiagaraSystem* BeamEffect;

    // 집속탄 나이아가라
    UPROPERTY(EditAnywhere, Category = "Effects")
    class UNiagaraSystem* ClusterExplosionFX;

    // 집속탄 폭발 사운드
    UPROPERTY(EditAnywhere, Category = "Effects")
    class USoundBase* ClusterExplosionSound;

    // 보급품, 센트리 낙하 사운드
    UPROPERTY(EditAnywhere, Category = "Effects")
    class USoundBase* FallingSound;

    // 이글 비행기 액터
    UPROPERTY(EditAnywhere, Category = "Effects")
    TSubclassOf<AEagle> EagleClass;

    // 이글 비행기 엔진 사운드
    UPROPERTY(EditAnywhere, Category = "Effects")
    class USoundBase* FighterSound;

    // 생성된 이펙트를 관리할 컴포넌트
    UPROPERTY()
    UNiagaraComponent* SpawnedBeam;

    void ActivateBeacon();

    // 착지 이펙트/사운드 (모든 클라이언트)
    UFUNCTION(NetMulticast, Reliable)
    void Multicast_OnLanded(FVector Location);

    // 빔 이펙트 + 이글/사운드 (모든 클라이언트)
    UFUNCTION(NetMulticast, Reliable)
    void Multicast_ActivateBeacon();

    // 집속탄 개별 폭발 이펙트 (모든 클라이언트)
    UFUNCTION(NetMulticast, Unreliable)
    void Multicast_ClusterEffect(FVector Location);

    // 집속탄 범위 설정
    float ClusterWidth = 3000.f;
    float ClusterDepth = 1500.f;
    int32 ClusterCount = 100;

private:
    FLinearColor PendingColor = FLinearColor::White;
};
