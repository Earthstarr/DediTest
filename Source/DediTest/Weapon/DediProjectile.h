// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DediProjectile.generated.h"

class USphereComponent;
class UProjectileMovementComponent;
class UStaticMeshComponent;
class UNiagaraSystem;

UCLASS()
class DEDITEST_API ADediProjectile : public AActor
{
	GENERATED_BODY()

public:
	ADediProjectile();

protected:
	virtual void BeginPlay() override;

	// 충돌 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Projectile")
	USphereComponent* CollisionComp;

	// 메시 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mesh")
	UStaticMeshComponent* ProjectileMesh;

	// 투사체 무브먼트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Movement")
	UProjectileMovementComponent* ProjectileMovement;

	// 데미지
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat")
	float DamageAmount = 10.0f;

	// 이펙트
	UPROPERTY(EditDefaultsOnly, Category="Effects")
	UNiagaraSystem* ImpactEffect;

	// 사운드
	UPROPERTY(EditDefaultsOnly, Category="Effects")
	class USoundBase* ImpactSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Effects")
	class USoundAttenuation* HitAttenuation;
	
	// 트레일 이펙트
	UPROPERTY(EditDefaultsOnly, Category="Effects")
	class UNiagaraSystem* TrailEffect;

public:
	// 충돌 콜백
	UFUNCTION()
	virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	                   FVector NormalImpulse, const FHitResult& Hit);

	// Getter
	USphereComponent* GetCollisionComp() const { return CollisionComp; }
	UProjectileMovementComponent* GetProjectileMovement() const { return ProjectileMovement; }
};
