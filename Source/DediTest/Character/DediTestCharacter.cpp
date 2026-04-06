// Copyright Epic Games, Inc. All Rights Reserved.

#include "DediTestCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "DediTest.h"
#include "FireballProjectile.h"
//#include "SNegativeActionButton.h"
#include "AbilitySystemComponent.h"
#include "DediTestAttributeSet.h"
#include "Components/SkeletalMeshComponent.h"
#include "DediProjectile.h"

ADediTestCharacter::ADediTestCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 500.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = NormalWalkSpeed;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->bUsePawnControlRotation = true;

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	// Mesh setup for weapon animation retargeting
	GetMesh()->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
	GetMesh()->SetRelativeLocation(FVector(0.f, 0.f, -96.f));

	// Create RetargetMesh attached to main mesh
	RetargetMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("RetargetMesh"));
	RetargetMesh->SetupAttachment(GetMesh());
	RetargetMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);

	// Create weapon mesh (attached to RetargetMesh's WeaponSocket)
	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	WeaponMesh->SetupAttachment(RetargetMesh, TEXT("WeaponSocket"));
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// AbilitySystemComponent 생성
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	// AttributeSet 생성
	AttributeSet = CreateDefaultSubobject<UDediTestAttributeSet>(TEXT("AttributeSet"));

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character)
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)
}

void ADediTestCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {

		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ADediTestCharacter::Move);
		EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &ADediTestCharacter::Look);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ADediTestCharacter::Look);

		// Fire
		if (FireAction)
		{
			EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Started, this, &ADediTestCharacter::OnFireStarted);
			EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Completed, this, &ADediTestCharacter::OnFireCompleted);
		}

		// Aim
		if (AimAction)
		{
			EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Triggered, this, &ADediTestCharacter::OnAimStarted);
			EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Completed, this, &ADediTestCharacter::OnAimCompleted);
		}

		// Reload
		if (ReloadAction)
		{
			EnhancedInputComponent->BindAction(ReloadAction, ETriggerEvent::Started, this, &ADediTestCharacter::Reload);
		}
	}
	else
	{
		UE_LOG(LogDediTest, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void ADediTestCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	// route the input
	DoMove(MovementVector.X, MovementVector.Y);
}

void ADediTestCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	// route the input
	DoLook(LookAxisVector.X, LookAxisVector.Y);
}

void ADediTestCharacter::DoMove(float Right, float Forward)
{
	if (GetController() != nullptr)
	{
		// find out which way is forward
		const FRotator Rotation = GetController()->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add movement 
		AddMovementInput(ForwardDirection, Forward);
		AddMovementInput(RightDirection, Right);
	}
}

void ADediTestCharacter::DoLook(float Yaw, float Pitch)
{
	if (GetController() != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(Yaw);
		AddControllerPitchInput(Pitch);
	}
}

void ADediTestCharacter::DoJumpStart()
{
	// signal the character to jump
	Jump();
}

void ADediTestCharacter::DoJumpEnd()
{
	// signal the character to stop jumping
	StopJumping();
}

void ADediTestCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	UpdateAimOffset(DeltaTime);
}

void ADediTestCharacter::UpdateAimOffset(float DeltaTime)
{
	// 컨트롤러의 회전값과 캐릭터의 회전값 비교
	FRotator ControlRotation = GetControlRotation();
	FRotator ActorRotation = GetActorRotation();

	// 두 회전값의 차이(Delta)를 정규화하여 저장
	FRotator Delta = (ControlRotation - ActorRotation).GetNormalized();

	// Pitch 값 보간
	AimPitch = FMath::FInterpTo(AimPitch, Delta.Pitch, DeltaTime, 15.0f);
}

void ADediTestCharacter::OnAimStarted_Implementation()
{
	bIsAiming = true;

	// 조준 시 캐릭터 회전을 컨트롤러 방향으로 고정
	bUseControllerRotationYaw = true;
	GetCharacterMovement()->bOrientRotationToMovement = false;
}

void ADediTestCharacter::OnAimCompleted_Implementation()
{
	bIsAiming = false;

	// 조준 해제 시 원래대로 복원
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
}

void ADediTestCharacter::OnFireStarted()
{
	bFireButtonDown = true;

	if (!bIsAiming) return;

	StartFiring();
}

void ADediTestCharacter::OnFireCompleted()
{
	bFireButtonDown = false;
	StopFiring();
}

void ADediTestCharacter::StartFiring()
{
	// 첫 발 즉시 발사
	Fire();

	// FireRate에 따른 반복 타이머 설정
	float TimeBetweenShots = 1.0f / FireRate;
	GetWorldTimerManager().SetTimer(TimerHandle_AutomaticFire, this, &ADediTestCharacter::Fire, TimeBetweenShots, true);
}

void ADediTestCharacter::StopFiring()
{
	GetWorldTimerManager().ClearTimer(TimerHandle_AutomaticFire);
}

void ADediTestCharacter::Reload()
{
	if (CurrentAmmo >= MaxAmmoInMag || CurrentMagCount <= 0 || bIsReloading) return;

	bIsReloading = true;

	// 이동 속도를 재장전 속도로 변경
	GetCharacterMovement()->MaxWalkSpeed = ReloadWalkSpeed;

	// TODO: 재장전 애니메이션 몽타주 재생
	// TODO: GAS Reload Ability 호출 (나중에 추가)
}

void ADediTestCharacter::FinishReload()
{
	bIsReloading = false;

	if (CurrentMagCount > 0)
	{
		CurrentMagCount--;
		CurrentAmmo = MaxAmmoInMag;
	}

	// 이동 속도 복원
	if (bIsAiming)
	{
		GetCharacterMovement()->MaxWalkSpeed = AimWalkSpeed;
	}
	else
	{
		GetCharacterMovement()->MaxWalkSpeed = NormalWalkSpeed;
	}

	// TODO: UI 업데이트 델리게이트 호출
}

void ADediTestCharacter::Fire()
{
	// 탄약 체크
	if (CurrentAmmo <= 0 || bIsReloading)
	{
		StopFiring();
		return;
	}

	// 탄약 소모
	CurrentAmmo--;

	// 무기 메시의 MuzzleSocket에서 발사 (소켓이 있으면)
	FVector SpawnLocation;
	FRotator SpawnRotator;

	if (WeaponMesh && WeaponMesh->DoesSocketExist(TEXT("MuzzleSocket")))
	{
		SpawnLocation = WeaponMesh->GetSocketLocation(TEXT("MuzzleSocket"));
		SpawnRotator = WeaponMesh->GetSocketRotation(TEXT("MuzzleSocket"));
	}
	else
	{
		// 소켓이 없으면 캡슐 앞쪽에서 발사
		SpawnLocation = GetCapsuleComponent()->GetComponentLocation() + GetCapsuleComponent()->GetForwardVector() * 100.0f;
		SpawnRotator = GetCapsuleComponent()->GetComponentRotation();
	}
	
	if (HasAuthority())	// 서버라면
	{
		if (ProjectileClass)
		{
			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = this;
			SpawnParams.Instigator = GetInstigator();
			
			GetWorld()->SpawnActor<ADediProjectile>(ProjectileClass, SpawnLocation, SpawnRotator, SpawnParams);
			
			UE_LOG(LogTemp, Warning, TEXT("Fire - Server"));
		}
	}
	else // 클라이언트라면
	{
		ServerFire(SpawnLocation, SpawnRotator); // 서버에 요청
		UE_LOG(LogTemp, Warning, TEXT("Fire - Client"));
	}
}

void ADediTestCharacter::ServerFire_Implementation(FVector SpawnLocation, FRotator SpawnRotation)
{
	// 서버에서 투사체 스폰
	if (ProjectileClass)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.Instigator = GetInstigator();

		GetWorld()->SpawnActor<AFireballProjectile>(ProjectileClass, SpawnLocation, SpawnRotation, SpawnParams);
	}
}

void ADediTestCharacter::Die()
{
	Destroy();
}

UAbilitySystemComponent* ADediTestCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}