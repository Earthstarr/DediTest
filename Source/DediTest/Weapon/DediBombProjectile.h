#pragma once

#include "CoreMinimal.h"
#include "DediProjectile.h"
#include "DediBombProjectile.generated.h"

UCLASS()
class DEDITEST_API ADediBombProjectile : public ADediProjectile
{
    GENERATED_BODY()

public:
    ADediBombProjectile();

protected:
    virtual void BeginPlay() override;

    virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
                       FVector NormalImpulse, const FHitResult& Hit) override;

    // 착지 이펙트/사운드
    UPROPERTY(EditDefaultsOnly, Category = "Effects")
    class UNiagaraSystem* LandImpactEffect;

    UPROPERTY(EditDefaultsOnly, Category = "Effects")
    class USoundBase* LandImpactSound;

    // 낙하음 (AudioComponent로 붙여두면 리플리케이션 자동)
    UPROPERTY(VisibleAnywhere, Category = "Effects")
    class UAudioComponent* FallingAudioComp;

    UPROPERTY(EditAnywhere, Category = "Effects")
    class USoundBase* FallingSound;

    // 폭발 반경
    UPROPERTY(EditAnywhere, Category = "Explosion")
    float ExplosionRadius = 2000.f;

    // 카메라 쉐이크
    UPROPERTY(EditAnywhere, Category = "Effects")
    TSubclassOf<class UCameraShakeBase> ExplosionShakeClass;

    UPROPERTY(EditAnywhere, Category = "Effects")
    float ShakeInnerRadius = 500.f;

    UPROPERTY(EditAnywhere, Category = "Effects")
    float ShakeOuterRadius = 3000.f;

    // 착지 후 폭발 딜레이
    UPROPERTY(EditAnywhere, Category = "Combat")
    float ExplosionDelay = 0.5f;

private:
    FTimerHandle ExplosionTimer;
    FHitResult SavedHit;

    void Explode();

    // 클라이언트 포함 모든 인스턴스에서 이펙트 재생
    UFUNCTION(NetMulticast, Reliable)
    void Multicast_PlayLandEffects(FVector ImpactPoint, FVector ImpactNormal);

    UFUNCTION(NetMulticast, Reliable)
    void Multicast_PlayExplosionEffects(FVector ImpactPoint, FVector ImpactNormal);
};
