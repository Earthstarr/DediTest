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
#include "SNegativeActionButton.h"
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
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
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

	// Create weapon mesh (attached to character mesh's WeaponSocket)
	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	WeaponMesh->SetupAttachment(GetMesh(), TEXT("WeaponSocket"));
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
		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Started, this, &ADediTestCharacter::Fire);
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

void ADediTestCharacter::Fire()
{
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