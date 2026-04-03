#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FPSCharacter.h"

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

    // 빨간 빛기둥 이펙트
    UPROPERTY(EditAnywhere, Category = "Effects")
    UNiagaraSystem* BeamEffect;

    // 생성된 이펙트를 관리할 컴포넌트
    UPROPERTY()
    UNiagaraComponent* SpawnedBeam;

    void ActivateBeacon();

private:
    FLinearColor PendingColor = FLinearColor::White;
};
