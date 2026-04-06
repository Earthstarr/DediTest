#include "StratagemBeacon.h"
#include "Eagle.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Math/Quat.h"

AStratagemBeacon::AStratagemBeacon()
{
    // 클라이언트에도 보이도록 리플리케이션 설정
    bReplicates = true;
    SetReplicateMovement(true);

    CollisionComp = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComp"));
    CollisionComp->InitSphereRadius(10.0f);
    CollisionComp->SetCollisionProfileName(TEXT("Projectile"));
    CollisionComp->SetCanEverAffectNavigation(false);
    CollisionComp->bReturnMaterialOnMove = true;
    CollisionComp->SetNotifyRigidBodyCollision(true);
    CollisionComp->OnComponentHit.AddDynamic(this, &AStratagemBeacon::OnHit);
    RootComponent = CollisionComp;

    MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
    MeshComp->SetupAttachment(RootComponent);

    ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileComp"));
    ProjectileMovement->UpdatedComponent = CollisionComp;
    ProjectileMovement->InitialSpeed = 2000.f;
    ProjectileMovement->MaxSpeed = 2000.f;
    ProjectileMovement->bShouldBounce = true;
    ProjectileMovement->Bounciness = 0.3f;
}

void AStratagemBeacon::UpdateBeaconVisual(FLinearColor NewColor)
{
    PendingColor = NewColor;

    if (SpawnedBeam)
    {
        SpawnedBeam->SetVariableLinearColor(FName("User.Color"), PendingColor);
    }
}

void AStratagemBeacon::BeginPlay()
{
    Super::BeginPlay();
}

void AStratagemBeacon::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
    if (ProjectileMovement->Velocity.IsNearlyZero()) return;

    ProjectileMovement->StopMovementImmediately();
    ProjectileMovement->Deactivate();

    CollisionComp->SetSimulatePhysics(false);
    CollisionComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    if (OtherActor)
    {
        FAttachmentTransformRules AttachRules(
            EAttachmentRule::KeepWorld,
            EAttachmentRule::KeepWorld,
            EAttachmentRule::KeepWorld,
            false
        );
        AttachToActor(OtherActor, AttachRules);
    }

    // 착지 이펙트/사운드는 모든 클라이언트에
    Multicast_OnLanded(Hit.ImpactPoint);

    // 비콘 활성화(빔 이펙트, 이글 스폰 등)는 서버에서 판단 후 Multicast
    if (HasAuthority())
    {
        Multicast_ActivateBeacon();

        FTimerHandle StrikeTimer;
        GetWorldTimerManager().SetTimer(StrikeTimer, this, &AStratagemBeacon::TriggerStrike, DelayToStrike, false);
    }
}

void AStratagemBeacon::Multicast_OnLanded_Implementation(FVector Location)
{
    if (BeaconActivateSound)
    {
        UGameplayStatics::PlaySoundAtLocation(this, BeaconActivateSound, Location);
    }
}

void AStratagemBeacon::TriggerStrike()
{
    // 서버에서만 실행
    if (!HasAuthority()) return;

    FVector TargetLocation = GetActorLocation();

    if (MyStratagemType == EStratagemType::Bomb500kg)
    {
        FVector LaunchLocation = TargetLocation + FVector(0.f, 0.f, 10000.f);

        if (Eagle500kgProjectileClass)
        {
            AActor* LargeShell = GetWorld()->SpawnActor<AActor>(Eagle500kgProjectileClass, LaunchLocation, FRotator(-90.f, 0.f, 0.f));
            if (LargeShell)
            {
                if (UProjectileMovementComponent* ProjComp = LargeShell->FindComponentByClass<UProjectileMovementComponent>())
                {
                    ProjComp->InitialSpeed = 10000.f;
                    ProjComp->MaxSpeed = 10000.f;
                    ProjComp->Velocity = FVector(0.f, 0.f, -10000.f);
                }
            }
        }
    }
    else if (MyStratagemType == EStratagemType::Supply)
    {
        FVector LaunchLocation = TargetLocation + FVector(0.f, 0.f, 10000.f);

        if (SupplyClass)
        {
            AActor* SpawnedPod = GetWorld()->SpawnActor<AActor>(SupplyClass, LaunchLocation, FRotator::ZeroRotator);
            if (SpawnedPod)
            {
                if (UStaticMeshComponent* PodMesh = Cast<UStaticMeshComponent>(SpawnedPod->GetRootComponent()))
                {
                    PodMesh->SetSimulatePhysics(true);
                    PodMesh->AddImpulse(FVector(0.f, 0.f, -10000.f), NAME_None, true);
                }
            }
        }
    }
    else if (MyStratagemType == EStratagemType::EagleCluster)
    {
        FVector RightVector = GetActorRightVector();
        FVector ForwardVector = GetActorForwardVector();

        for (int32 i = 0; i < ClusterCount; ++i)
        {
            float RandRight = FMath::FRandRange(-ClusterWidth, ClusterWidth);
            float RandForward = FMath::FRandRange(-ClusterDepth, ClusterDepth);
            FVector HorizontalPos = TargetLocation + (RightVector * RandRight) + (ForwardVector * RandForward);
            float Delay = FMath::FRandRange(0.5f, 3.0f);

            FTimerHandle TempHandle;
            GetWorldTimerManager().SetTimer(TempHandle, [this, HorizontalPos]()
            {
                FVector TraceStart = HorizontalPos + FVector(0.f, 0.f, 2000.f);
                FVector TraceEnd   = HorizontalPos + FVector(0.f, 0.f, -2000.f);

                FHitResult TerrainHit;
                FCollisionQueryParams Params;
                Params.AddIgnoredActor(this);

                FVector FinalPos = HorizontalPos;
                if (GetWorld()->LineTraceSingleByChannel(TerrainHit, TraceStart, TraceEnd, ECC_Visibility, Params))
                    FinalPos = TerrainHit.ImpactPoint;

                // 데미지는 서버에서
                TArray<FHitResult> PointHits;
                FCollisionShape SphereShape = FCollisionShape::MakeSphere(500.f);
                if (GetWorld()->SweepMultiByChannel(PointHits, FinalPos, FinalPos, FQuat::Identity, ECC_Pawn, SphereShape))
                {
                    for (auto& PHit : PointHits)
                    {
                        UGameplayStatics::ApplyPointDamage(PHit.GetActor(), 30.f,
                            FinalPos - PHit.ImpactPoint, PHit,
                            GetInstigatorController(), this, UDamageType::StaticClass());
                    }
                }

                // 이펙트는 Multicast
                if (FMath::FRand() < 0.5f)
                    Multicast_ClusterEffect(FinalPos);

            }, Delay, false);
        }
    }
    else if (MyStratagemType == EStratagemType::Sentry)
    {
        FVector LaunchLocation = TargetLocation + FVector(0.f, 0.f, 10000.f);

        if (SentryClass)
        {
            AActor* SpawnedSentry = GetWorld()->SpawnActor<AActor>(SentryClass, LaunchLocation, FRotator::ZeroRotator);
            if (SpawnedSentry)
            {
                if (UStaticMeshComponent* PodMesh = Cast<UStaticMeshComponent>(SpawnedSentry->GetRootComponent()))
                {
                    PodMesh->SetSimulatePhysics(true);
                    PodMesh->AddImpulse(FVector(0.f, 0.f, -10000.f), NAME_None, true);
                }
            }
        }
    }
    else if (MyStratagemType == EStratagemType::OrbitalLaser)
    {
        if (OrbitalLaserClass)
        {
            FActorSpawnParameters SpawnParams;
            SpawnParams.Owner = this;
            SpawnParams.Instigator = GetInstigator();
            SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            GetWorld()->SpawnActor<AActor>(OrbitalLaserClass, GetActorLocation(), FRotator::ZeroRotator, SpawnParams);
        }
    }

    MeshComp->SetVisibility(false);
    if (SpawnedBeam) SpawnedBeam->Deactivate();
    CollisionComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    SetLifeSpan(5.0f);
}

void AStratagemBeacon::Multicast_ActivateBeacon_Implementation()
{
    ActivateBeacon();
}

void AStratagemBeacon::ActivateBeacon()
{
    if (!BeamEffect) return;

    SpawnedBeam = UNiagaraFunctionLibrary::SpawnSystemAttached(
        BeamEffect, RootComponent, NAME_None,
        FVector::ZeroVector, FRotator::ZeroRotator,
        EAttachLocation::KeepRelativeOffset, true
    );

    if (SpawnedBeam)
    {
        SpawnedBeam->SetVariableLinearColor(FName("User.Color"), PendingColor);
        SpawnedBeam->SetWorldRotation(FRotator::ZeroRotator);
    }

    if ((MyStratagemType == EStratagemType::Bomb500kg || MyStratagemType == EStratagemType::EagleCluster)
        && FighterSound && EagleClass)
    {
        FVector BeaconLoc = GetActorLocation();

        // Instigator(비콘 던진 플레이어) 기준으로 이글 방향 계산
        FVector Forward = FVector::ForwardVector;
        FVector Right   = FVector::RightVector;

        if (GetInstigator())
        {
            Forward = (BeaconLoc - GetInstigator()->GetActorLocation()).GetSafeNormal2D();
            Right   = FVector::CrossProduct(FVector::UpVector, Forward);
        }

        FVector FlyStart = BeaconLoc + (Forward * -300000.f) + (Right * -30000.f) + (FVector::UpVector * 100000.f);
        FVector FlyEnd   = BeaconLoc + (Forward *  400000.f) + (Right *  30000.f) + (FVector::UpVector * 200000.f);

        // 이글은 서버에서 스폰 (bReplicates=true면 클라에도 보임)
        if (HasAuthority())
        {
            AEagle* Eagle = GetWorld()->SpawnActor<AEagle>(EagleClass, FlyStart, FRotator::ZeroRotator);
            if (Eagle)
                Eagle->InitEagle(FlyStart, FlyEnd, DelayToStrike + 2.5f);
        }

        UGameplayStatics::PlaySoundAtLocation(this, FighterSound, GetActorLocation());
    }
    else if (MyStratagemType == EStratagemType::Sentry && FallingSound)
    {
        UGameplayStatics::PlaySoundAtLocation(this, FallingSound, GetActorLocation());
    }
    else if (MyStratagemType == EStratagemType::Supply && FallingSound)
    {
        UGameplayStatics::PlaySoundAtLocation(this, FallingSound, GetActorLocation());
    }
}

void AStratagemBeacon::Multicast_ClusterEffect_Implementation(FVector Location)
{
    if (ClusterExplosionFX)
    {
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), ClusterExplosionFX, Location);
    }

    if (ClusterExplosionSound)
    {
        UGameplayStatics::PlaySoundAtLocation(this, ClusterExplosionSound, Location,
            FMath::FRandRange(0.4f, 0.6f), FMath::FRandRange(0.8f, 1.2f));
    }
}
