#include "DediBombProjectile.h"
#include "Components/AudioComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "DediTest/FPS/FPSCharacter.h"
#include "DediTest/FPS/PlayerAttributeSet.h"
#include "AbilitySystemComponent.h"

ADediBombProjectile::ADediBombProjectile()
{
    // 낙하 속도 설정
    ProjectileMovement->InitialSpeed = 10000.f;
    ProjectileMovement->MaxSpeed = 10000.f;
    ProjectileMovement->ProjectileGravityScale = 1.0f;
    ProjectileMovement->bShouldBounce = false;

    FallingAudioComp = CreateDefaultSubobject<UAudioComponent>(TEXT("FallingAudio"));
    FallingAudioComp->SetupAttachment(RootComponent);
    FallingAudioComp->bAutoActivate = false;
}

void ADediBombProjectile::BeginPlay()
{
    Super::BeginPlay();

    // 낙하음은 모든 클라이언트에서 들려야 하므로 리플리케이트된 AudioComponent로 재생
    if (FallingSound)
    {
        FallingAudioComp->SetSound(FallingSound);
        FallingAudioComp->Play();
    }
}

void ADediBombProjectile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
                                 FVector NormalImpulse, const FHitResult& Hit)
{
    // 이미 타이머 돌고 있으면 무시
    if (GetWorldTimerManager().IsTimerActive(ExplosionTimer)) return;

    // 낙하음 정지
    if (FallingAudioComp && FallingAudioComp->IsPlaying())
    {
        FallingAudioComp->Stop();
    }

    // 움직임 정지
    ProjectileMovement->StopMovementImmediately();
    ProjectileMovement->Deactivate();

    CollisionComp->SetSimulatePhysics(false);
    CollisionComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    SavedHit = Hit;

    if (ProjectileMesh)
    {
        ProjectileMesh->SetVisibility(false);
    }

    // 착지 이펙트는 모든 클라이언트에서
    Multicast_PlayLandEffects(Hit.ImpactPoint, Hit.ImpactNormal);

    // 서버에서만 폭발 타이머 설정
    if (HasAuthority())
    {
        GetWorldTimerManager().SetTimer(ExplosionTimer, this, &ADediBombProjectile::Explode, ExplosionDelay, false);
    }
}

void ADediBombProjectile::Explode()
{
    // 데미지는 서버에서만
    if (HasAuthority())
    {
        FVector ExplosionOrigin = SavedHit.ImpactPoint + (SavedHit.ImpactNormal * 20.0f);

        UGameplayStatics::ApplyRadialDamageWithFalloff(
            this, DamageAmount, 10.f, ExplosionOrigin,
            ExplosionRadius, ExplosionRadius * 1.2f, 1.f,
            UDamageType::StaticClass(), TArray<AActor*>(),
            this, GetInstigatorController(), ECC_Visibility
        );
    }

    // 이펙트/사운드/쉐이크는 모든 클라이언트에서
    Multicast_PlayExplosionEffects(SavedHit.ImpactPoint, SavedHit.ImpactNormal);

    Destroy();
}

void ADediBombProjectile::Multicast_PlayLandEffects_Implementation(FVector ImpactPoint, FVector ImpactNormal)
{
    if (LandImpactEffect)
    {
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(
            GetWorld(), LandImpactEffect, ImpactPoint, ImpactNormal.Rotation()
        );
    }

    if (LandImpactSound)
    {
        UGameplayStatics::PlaySoundAtLocation(this, LandImpactSound, ImpactPoint);
    }
}

void ADediBombProjectile::Multicast_PlayExplosionEffects_Implementation(FVector ImpactPoint, FVector ImpactNormal)
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
            ShakeInnerRadius, ShakeOuterRadius, 1.0f
        );
    }
}
