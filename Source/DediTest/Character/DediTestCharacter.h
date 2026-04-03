// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "AbilitySystemInterface.h"
#include "DediTestCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputAction;
struct FInputActionValue;
class UAbilitySystemComponent;
class UDediTestAttributeSet;
class USkeletalMeshComponent;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

/**
 *  A simple player-controllable third person character
 *  Implements a controllable orbiting camera
 */
UCLASS(Blueprintable)
class ADediTestCharacter : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;

	/** Retarget Mesh for weapon animation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mesh", meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* RetargetMesh;

	/** Weapon Mesh */
	UPROPERTY(BlueprintReadWrite, Category="Mesh", meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* WeaponMesh;

protected:

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* LookAction;

	/** Mouse Look Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* MouseLookAction;

	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* FireAction;

	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* AimAction;

	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* ReloadAction;

	UPROPERTY(EditAnywhere, Category="Combat")
	TSubclassOf<class ADediProjectile> ProjectileClass;

	// Firing system
	bool bAimButtonDown = false;
	bool bFireButtonDown = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float FireRate = 10.0f;

	FTimerHandle TimerHandle_AutomaticFire;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float RecoilPitch = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float RecoilYaw = 0.03f;

	// GAS Components
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GAS")
	UAbilitySystemComponent* AbilitySystemComponent;

	UPROPERTY()
	UDediTestAttributeSet* AttributeSet;

	void Fire();

	// 서버에서 실행될 발사 함수 (RPC)
	UFUNCTION(Server, Reliable)
	void ServerFire(FVector SpawnLocation, FRotator SpawnRotation);

	// Firing functions
	void OnFireStarted();
	void OnFireCompleted();
	void StartFiring();
	void StopFiring();

	// Reload system
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Combat")
	bool bIsReloading = false;

	FTimerHandle TimerHandle_Reload;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	class USoundBase* ReloadSound;

	void Reload();

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void FinishReload();

public:
	// Ammo system
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	int32 MaxAmmoInMag = 30;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	int32 CurrentAmmo = 30;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	int32 CurrentMagCount = 8;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	int32 MaxMagCount = 8;

protected:
	
	// 에임 피치
	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	float AimPitch;
	
	void UpdateAimOffset(float DeltaTime);

	// Movement Speed
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float NormalWalkSpeed = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float SprintWalkSpeed = 600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float AimWalkSpeed = 200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float ReloadWalkSpeed = 300.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Combat")
	bool bIsAiming = false;

	UFUNCTION(BlueprintNativeEvent, Category = "Combat")
	void OnAimStarted();
	virtual void OnAimStarted_Implementation();

	UFUNCTION(BlueprintNativeEvent, Category = "Combat")
	void OnAimCompleted();
	virtual void OnAimCompleted_Implementation();

public:

	/** Constructor */
	ADediTestCharacter();

	virtual void Tick(float DeltaTime) override;

protected:

	/** Initialize input action bindings */
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:

	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);

public:

	/** Handles move inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoMove(float Right, float Forward);

	/** Handles look inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoLook(float Yaw, float Pitch);

	/** Handles jump pressed inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpStart();

	/** Handles jump pressed inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpEnd();
	
	void Die();

public:

	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }

	// IAbilitySystemInterface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	// AttributeSet Getter
	UDediTestAttributeSet* GetAttributeSet() const { return AttributeSet; }
};

