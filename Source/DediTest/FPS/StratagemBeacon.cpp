#include "StratagemBeacon.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Kismet/GameplayStatics.h"

AStratagemBeacon::AStratagemBeacon()
{
    // 충돌체 설정
    CollisionComp = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComp"));
    CollisionComp->InitSphereRadius(10.0f);
    CollisionComp->SetCollisionProfileName(TEXT("Projectile"));

    // 물리적 충돌 신호(Simulation Generates Hit Events)를 확실히 켬
    CollisionComp->SetCanEverAffectNavigation(false);
    CollisionComp->bReturnMaterialOnMove = true;
    CollisionComp->SetNotifyRigidBodyCollision(true);

    // 부딪히면 OnHit 실행되도록 연결
    CollisionComp->OnComponentHit.AddDynamic(this, &AStratagemBeacon::OnHit);
    RootComponent = CollisionComp;

    // 외형 설정
    MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
    MeshComp->SetupAttachment(RootComponent);

    // 투사체 설정 (수류탄 느낌)
    ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileComp"));
    ProjectileMovement->UpdatedComponent = CollisionComp;
    ProjectileMovement->InitialSpeed = 2000.f;
    ProjectileMovement->MaxSpeed = 2000.f;
    ProjectileMovement->bShouldBounce = true; // 바닥에서 살짝 튀게 설정
    ProjectileMovement->Bounciness = 0.3f;
}

void AStratagemBeacon::UpdateBeaconVisual(FLinearColor NewColor)
{
    PendingColor = NewColor; // 전달받은 색상을 저장해둠

    // 만약 이미 빔이 생성된 상태라면 즉시 적용
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
    // 이미 안착했다면 무시 (여러 번 부딪히는 현상 방지)
    if (ProjectileMovement->Velocity.IsNearlyZero()) return;

    // 움직임 즉시 정지 및 물리 계산 끄기
    ProjectileMovement->StopMovementImmediately();
    ProjectileMovement->Deactivate();

    // 충돌 설정 변경 (더 이상 튕기지 않게)
    CollisionComp->SetSimulatePhysics(false);
    CollisionComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    if (BeaconActivateSound)
    {
        UGameplayStatics::PlaySound2D(this, BeaconActivateSound);
    }

    // 액터에 달라붙기 (Attach)
    if (OtherActor)
    {
        FAttachmentTransformRules AttachRules(
            EAttachmentRule::KeepWorld,
            EAttachmentRule::KeepWorld,
            EAttachmentRule::KeepWorld,
            false
        );
        this->AttachToActor(OtherActor, AttachRules);
    }

    ActivateBeacon();

    // 타이머 시작
    FTimerHandle StrikeTimer;
    GetWorldTimerManager().SetTimer(StrikeTimer, this, &AStratagemBeacon::TriggerStrike, DelayToStrike, false);
}

void AStratagemBeacon::TriggerStrike()
{
    // 스트라타젬 실행 로직은 나중에 구현
    // 폭격 지점
    FVector TargetLocation = GetActorLocation();

    MeshComp->SetVisibility(false);
    if (SpawnedBeam) SpawnedBeam->Deactivate();
    CollisionComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    SetLifeSpan(5.0f);
}

void AStratagemBeacon::ActivateBeacon()
{
    if (BeamEffect)
    {
        // 비컨 위치에서 하늘 방향으로 나이아가라 스폰
        SpawnedBeam = UNiagaraFunctionLibrary::SpawnSystemAttached(
            BeamEffect,
            RootComponent,
            NAME_None,
            FVector::ZeroVector,
            FRotator::ZeroRotator,
            EAttachLocation::KeepRelativeOffset,
            true
        );

        if (SpawnedBeam)
        {
            SpawnedBeam->SetVariableLinearColor(FName("User.Color"), PendingColor);
            SpawnedBeam->SetWorldRotation(FRotator(0.f, 0.f, 0.f));
        }
    }
}
