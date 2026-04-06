#include "DediGrenade.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"

ADediGrenade::ADediGrenade()
{
    // 수류탄 물리 설정
    ProjectileMovement->InitialSpeed = 2000.f;
    ProjectileMovement->MaxSpeed = 2000.f;
    ProjectileMovement->bShouldBounce = true;
    ProjectileMovement->Bounciness = 0.3f;
    ProjectileMovement->ProjectileGravityScale = 1.0f;
    ProjectileMovement->bRotationFollowsVelocity = false;
    ProjectileMovement->Friction = 0.5f;
    ProjectileMovement->bBounceAngleAffectsFriction = true;

    DamageAmount = 200.f;

    // 수류탄은 LifeSpan 끄고 타이머로 관리
    InitialLifeSpan = 0.f;
}

void ADediGrenade::BeginPlay()
{
    Super::BeginPlay();

    // 트레일 이펙트
    if (TrailEffect)
    {
        UNiagaraFunctionLibrary::SpawnSystemAttached(
            TrailEffect, CollisionComp, NAME_None,
            FVector::ZeroVector, FRotator::ZeroRotator,
            EAttachLocation::SnapToTarget, true
        );
    }

    // 서버에서만 폭발 타이머 시작 (던지는 순간부터 카운트다운)
    if (HasAuthority())
    {
        StartExplosionTimer();
    }
}

void ADediGrenade::StartExplosionTimer(float Delay)
{
    float FinalDelay = (Delay > 0.f) ? Delay : TimeToExplode;
    GetWorldTimerManager().SetTimer(ExplosionTimerHandle, this, &ADediGrenade::Explode, FinalDelay, false);
}

void ADediGrenade::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
                          FVector NormalImpulse, const FHitResult& Hit)
{
    // 던진 플레이어와 충돌 무시
    if (OtherActor && (OtherActor == GetOwner() || OtherActor == GetInstigator())) return;

    // 충돌은 튕기도록 두고 타이머가 폭발 처리 (착탄 즉시 폭발 원하면 여기서 Explode() 호출)
}

void ADediGrenade::Explode()
{
    FVector ExplosionOrigin = GetActorLocation();

    // 데미지는 서버에서만
    if (HasAuthority())
    {
        UGameplayStatics::ApplyRadialDamageWithFalloff(
            this, DamageAmount, 10.f, ExplosionOrigin,
            InnerRadius, ExplosionRadius, 1.f,
            UDamageType::StaticClass(), TArray<AActor*>(),
            this, GetInstigatorController(), ECC_Visibility
        );
    }

    // 이펙트/사운드/쉐이크는 모든 클라이언트에서
    Multicast_PlayExplosionEffects(ExplosionOrigin, FVector::UpVector);

    Destroy();
}

void ADediGrenade::Multicast_PlayExplosionEffects_Implementation(FVector ImpactPoint, FVector ImpactNormal)
{
    if (ImpactEffect)
    {
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(
            GetWorld(), ImpactEffect, ImpactPoint, ImpactNormal.Rotation()
        );
    }

    if (ImpactSound)
    {
        UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, ImpactPoint);
    }

    if (ExplosionShakeClass)
    {
        UGameplayStatics::PlayWorldCameraShake(
            GetWorld(), ExplosionShakeClass, ImpactPoint,
            0.f, ExplosionRadius * 2.f, 1.0f
        );
    }
}
