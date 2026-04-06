#pragma once

#include "CoreMinimal.h"
#include "DediProjectile.h"
#include "DediGrenade.generated.h"

UCLASS()
class DEDITEST_API ADediGrenade : public ADediProjectile
{
    GENERATED_BODY()

public:
    ADediGrenade();

    virtual void BeginPlay() override;

    // 외부에서 타이머 딜레이 오버라이드 가능 (0이면 TimeToExplode 사용)
    void StartExplosionTimer(float Delay = 0.f);

protected:
    virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
                       FVector NormalImpulse, const FHitResult& Hit) override;

    // 폭발 반경
    UPROPERTY(EditAnywhere, Category = "Explosion")
    float InnerRadius = 300.f;

    UPROPERTY(EditAnywhere, Category = "Explosion")
    float ExplosionRadius = 800.f;

    // 던진 후 폭발까지 시간
    UPROPERTY(EditAnywhere, Category = "Explosion")
    float TimeToExplode = 3.f;

    // 카메라 쉐이크
    UPROPERTY(EditAnywhere, Category = "Effects")
    TSubclassOf<class UCameraShakeBase> ExplosionShakeClass;

private:
    FTimerHandle ExplosionTimerHandle;

    void Explode();

    UFUNCTION(NetMulticast, Reliable)
    void Multicast_PlayExplosionEffects(FVector ImpactPoint, FVector ImpactNormal);
};
