// Fill out your copyright notice in the Description page of Project Settings.


#include "DediProjectile.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "DediTestCharacter.h"
#include "AbilitySystemComponent.h"
#include "DediTestAttributeSet.h"

ADediProjectile::ADediProjectile()
{
	PrimaryActorTick.bCanEverTick = false;

	// 멀티플레이어 리플리케이션 설정
	bReplicates = true;
	SetReplicateMovement(true);

	// 충돌 컴포넌트
	CollisionComp = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComp"));
	CollisionComp->InitSphereRadius(5.0f);
	CollisionComp->BodyInstance.SetCollisionProfileName("Projectile");
	CollisionComp->OnComponentHit.AddDynamic(this, &ADediProjectile::OnHit);
	CollisionComp->SetWalkableSlopeOverride(FWalkableSlopeOverride(WalkableSlope_Unwalkable, 0.f));
	CollisionComp->CanCharacterStepUpOn = ECB_No;
	RootComponent = CollisionComp;

	// 메시
	ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ProjectileMesh"));
	ProjectileMesh->SetupAttachment(RootComponent);
	ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 투사체 무브먼트
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileComp"));
	ProjectileMovement->UpdatedComponent = CollisionComp;
	ProjectileMovement->InitialSpeed = 3000.0f;
	ProjectileMovement->MaxSpeed = 3000.0f;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bShouldBounce = false;
	ProjectileMovement->ProjectileGravityScale = 0.0f;

	// 8초 후 자동 삭제
	InitialLifeSpan = 8.0f;
}

void ADediProjectile::BeginPlay()
{
	Super::BeginPlay();

	// Owner 무시
	if (GetOwner())
	{
		CollisionComp->IgnoreActorWhenMoving(GetOwner(), true);
	}
	
	if (TrailEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAttached(
			TrailEffect, GetRootComponent(), NAME_None,
			FVector::ZeroVector, FRotator::ZeroRotator,
			EAttachLocation::SnapToTarget, true
		);
	}
}

void ADediProjectile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
                             FVector NormalImpulse, const FHitResult& Hit)
{
	// 서버에서만 처리
	if (!HasAuthority())
		return;

	// 자기 자신, Owner 무시
	if (!OtherActor || OtherActor == this || OtherActor == GetOwner())
		return;

	// 캐릭터에 맞았을 때 데미지 처리
	if (ADediTestCharacter* HitCharacter = Cast<ADediTestCharacter>(OtherActor))
	{
		// GAS를 통한 데미지 처리
		UAbilitySystemComponent* TargetASC = HitCharacter->GetAbilitySystemComponent();
		if (TargetASC)
		{
			// Attribute 감소
			const UDediTestAttributeSet* AttributeSet = TargetASC->GetSet<UDediTestAttributeSet>();
			if (AttributeSet)
			{
				float CurrentHealth = AttributeSet->GetHealth();
				float NewHealth = FMath::Max(0.0f, CurrentHealth - DamageAmount);

				// Health 설정
				TargetASC->SetNumericAttributeBase(UDediTestAttributeSet::GetHealthAttribute(), NewHealth);

				UE_LOG(LogTemp, Warning, TEXT("Projectile Hit! Damage: %f, Health: %f -> %f"),
					DamageAmount, CurrentHealth, NewHealth);
			}
		}
	}

	// 이펙트 생성 (모든 클라이언트에서 보이도록)
	if (ImpactEffect)
	{
		FRotator SpawnRotation = Hit.ImpactNormal.Rotation();
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(),
			ImpactEffect,
			Hit.ImpactPoint,
			SpawnRotation
		);
	}

	// 사운드 재생
	if (ImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, Hit.ImpactPoint, 1.f, 1.f, 0.f, HitAttenuation);
	}

	// 투사체 제거
	Destroy();
}
