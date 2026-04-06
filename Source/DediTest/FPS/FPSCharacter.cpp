#include "FPSCharacter.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Blueprint/UserWidget.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "AbilitySystemComponent.h"
#include "PlayerAttributeSet.h"
#include "GameplayEffect.h"
#include "Perception/AISense_Hearing.h"
#include "DediTest/Weapon/DediProjectile.h"
#include "StratagemBeacon.h"

AFPSCharacter::AFPSCharacter()
{
    // ASC 컴포넌트 생성
    AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));

    // 서버/클라이언트 간 태그 복제 설정
    AbilitySystemComponent->SetIsReplicated(true);

    // 어빌리티 복제 모드 설정
    AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

    // 캡슐 크기 설정
    GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

    // 캐릭터 회전 설정 (3인칭 설정)
    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;

    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->RotationRate = FRotator(0.0f, 300.0f, 0.0f);

    // 스프링 암(CameraBoom) 설정
    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(RootComponent);
    CameraBoom->TargetArmLength = 500.0f;
    CameraBoom->bUsePawnControlRotation = true;

    // 카메라 캐릭터 오른쪽 어깨 쪽으로 살짝 이동
    CameraBoom->SocketOffset = FVector(0.f, 120.f, 80.f);

    // 3인칭 카메라 설정
    ThirdPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("ThirdPersonCamera"));
    ThirdPersonCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
    ThirdPersonCamera->bUsePawnControlRotation = false;

    // 메시 및 총구 설정
    GetMesh()->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
    GetMesh()->SetRelativeLocation(FVector(0.f, 0.f, -96.f));
    RetargetMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("RetargetMesh"));
    RetargetMesh->SetupAttachment(GetMesh());
    RetargetMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);

    WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
    WeaponMesh->SetupAttachment(RetargetMesh, TEXT("WeaponSocket"));

    // 체력, 스태미나 관리용 AttributeSet
    AttributeSet = CreateDefaultSubobject<UPlayerAttributeSet>(TEXT("AttributeSet"));

    // 문제가 되는 물리 및 에셋 로드 관련 설정을 이 조건문 안에 넣기
    if (!HasAnyFlags(RF_ClassDefaultObject))
    {
        // 웨폰 메시의 물리 처리를 생성자 시점에서 하지 않도록 방어
        if (WeaponMesh)
        {
            WeaponMesh->bApplyImpulseOnDamage = false;
        }
    }
}

UAbilitySystemComponent* AFPSCharacter::GetAbilitySystemComponent() const
{
    return AbilitySystemComponent;
}

float AFPSCharacter::TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent,
    class AController* EventInstigator, AActor* DamageCauser)
{
    float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

    if (AttributeSet && ActualDamage > 0.0f)
    {
        float NewHealth = AttributeSet->GetHealth() - ActualDamage;
        AttributeSet->SetHealth(FMath::Max(NewHealth, 0.0f));
    }

    return ActualDamage;
}

void AFPSCharacter::BeginPlay()
{
    Super::BeginPlay();

    UE_LOG(LogTemp, Warning, TEXT("FPSCharacter BeginPlay - HasAuthority: %d, IsLocallyControlled: %d, Name: %s"),
        HasAuthority(), IsLocallyControlled(), *GetName());

    // 초기 이동 속도 설정
    GetCharacterMovement()->MaxWalkSpeed = NormalWalkSpeed;

    // 무기별 초기 탄약 설정
    for (int32 i = 0; i < Weapons.Num() && i < 2; i++)
    {
        WeaponAmmo[i] = Weapons[i].MaxAmmoInMag;
    }

    // 기존 캐릭터 메시 숨기기 (RetargetMesh 사용)
    GetMesh()->SetVisibility(false);


    if (AbilitySystemComponent)
    {
        // ASC 초기화, 서버에서만 어빌리티 부여
        AbilitySystemComponent->InitAbilityActorInfo(this, this);
    }

    // Attribute 설정
    if (AbilitySystemComponent && AttributeSet)
    {
        // Health 속성 변화 감지
        AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
            AttributeSet->GetHealthAttribute()).AddUObject(this, &AFPSCharacter::HandleHealthChanged);

        // Stamina 속성 변화 감지
        AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
            AttributeSet->GetStaminaAttribute()).AddUObject(this, &AFPSCharacter::HandleStaminaChanged);

        // 초기 UI 업데이트 위해 바로 호출
        OnHealthChanged.Broadcast(AttributeSet->GetHealth(), AttributeSet->GetMaxHealth());
    }

    // 스태미나 초기 UI 업데이트
    OnStaminaChanged.Broadcast(CurrentStamina, MaxStamina);

    if (HasAuthority())
    {
        // GAS: 수류탄 던지기 능력
        if (GrenadeAbilityClass)
        {
            AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(GrenadeAbilityClass, 1, static_cast<int32>(EAbilityInputID::Grenade)));
        }
    }

    OnGrenadeChanged.Broadcast(CurrentGrenadeCount, MaxGrenadeCount);
    OnAmmoChanged.Broadcast(CurrentAmmo, CurrentMagCount, MaxMagCount);

    // 무기 메시 설정
    if (DefaultWeaponMesh && WeaponMesh)
    {
        WeaponMesh->SetSkeletalMesh(DefaultWeaponMesh);
    }

    InitInputAndCamera();

    // 500kg 폭탄
    FStratagemData Bomb;
    Bomb.Name = TEXT("500kg Bomb");
    Bomb.Command = { EStratagemDirection::Up, EStratagemDirection::Right, EStratagemDirection::Down, EStratagemDirection::Down, EStratagemDirection::Down };
    Bomb.BeaconColor = FLinearColor::Red;
    Bomb.MaxCooldown = 120.0f;
    Bomb.CurrentCooldown = 0.0f;
    Bomb.bUseStack = true;
    Bomb.MaxStack = 2;
    Bomb.CurrentStack = 2;
    Bomb.bIsOnCooldown = false;
    Bomb.Type = EStratagemType::Bomb500kg;

    // 이글 클러스터
    FStratagemData ClusterBomb;
    ClusterBomb.Name = TEXT("Eagle Cluster Bomb");
    ClusterBomb.Command = { EStratagemDirection::Up, EStratagemDirection::Right, EStratagemDirection::Down, EStratagemDirection::Down, EStratagemDirection::Right };
    ClusterBomb.BeaconColor = FLinearColor(1.0f, 0.5f, 0.0f);
    ClusterBomb.MaxCooldown = 90.0f;
    ClusterBomb.CurrentCooldown = 0.0f;
    ClusterBomb.bUseStack = true;
    ClusterBomb.MaxStack = 3;
    ClusterBomb.CurrentStack = 3;
    ClusterBomb.bIsOnCooldown = false;
    ClusterBomb.Type = EStratagemType::EagleCluster;

    // 궤도 레이저
    FStratagemData OrbitalLaser;
    OrbitalLaser.Name = TEXT("Orbital Laser");
    OrbitalLaser.Command = {
        EStratagemDirection::Right,
        EStratagemDirection::Down,
        EStratagemDirection::Up,
        EStratagemDirection::Right,
        EStratagemDirection::Down
    };
    OrbitalLaser.BeaconColor = FLinearColor(0.5f, 0.0f, 1.0f);
    OrbitalLaser.MaxCooldown = 300.0f;
    OrbitalLaser.CurrentCooldown = 0.0f;
    OrbitalLaser.bIsOnCooldown = false;
    OrbitalLaser.Type = EStratagemType::OrbitalLaser;

    // 센트리
    FStratagemData Sentry;
    Sentry.Name = TEXT("Sentry");
    Sentry.Command = { EStratagemDirection::Down, EStratagemDirection::Up, EStratagemDirection::Right, EStratagemDirection::Left };
    Sentry.BeaconColor = FLinearColor::Green;
    Sentry.MaxCooldown = 180.0f;
    Sentry.CurrentCooldown = 0.0f;
    Sentry.bIsOnCooldown = false;
    Sentry.Type = EStratagemType::Sentry;

    // 보급품 (파랑)
    FStratagemData Supply;
    Supply.Name = TEXT("Supply Pod");
    Supply.Command = { EStratagemDirection::Down, EStratagemDirection::Down, EStratagemDirection::Up, EStratagemDirection::Right };
    Supply.BeaconColor = FLinearColor::Blue;
    Supply.MaxCooldown = 120.0f;
    Supply.CurrentCooldown = 0.0f;
    Supply.bIsOnCooldown = false;
    Supply.Type = EStratagemType::Supply;

    // 이글 재무장
    FStratagemData EagleRearm;
    EagleRearm.Name = TEXT("Eagle Rearm");
    EagleRearm.Command = { EStratagemDirection::Up,
        EStratagemDirection::Up,
        EStratagemDirection::Left,
        EStratagemDirection::Up,
        EStratagemDirection::Right
    };
    EagleRearm.Type = EStratagemType::Rearm;
    EagleRearm.bUseStack = false;
    EagleRearm.MaxCooldown = 1.0f;
    EagleRearm.CurrentCooldown = 0.0f;

    // 에디터에서 설정한 스트라타젬 스택을 다시 초기화
    for (FStratagemData& Data : StratagemList)
    {
        if (Data.bUseStack)
        {
            Data.CurrentStack = Data.MaxStack;
        }
    }
}

void AFPSCharacter::UpdateAimOffset(float DeltaTime)
{
    // 컨트롤러의 회전값과 캐릭터의 회전값 비교
    FRotator ControlRotation = GetControlRotation();
    FRotator ActorRotation = GetActorRotation();

    // 두 회전값의 차이(Delta)를 정규화하여 저장
    FRotator Delta = (ControlRotation - ActorRotation).GetNormalized();

    // Pitch 값 보간
    AimPitch = FMath::FInterpTo(AimPitch, Delta.Pitch, DeltaTime, 15.0f);
}

void AFPSCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    UpdateAimOffset(DeltaTime);

    // 카메라가 벽에 의해 캐릭터 가까이 당겨지면 메시 숨김
    float CameraDist = FVector::Dist(ThirdPersonCamera->GetComponentLocation(), GetActorLocation());
    bCameraTooClose = CameraDist < 250.f;

    RetargetMesh->SetVisibility(!bCameraTooClose);
    WeaponMesh->SetVisibility(!bCameraTooClose);

    // 스트라타젬 쿨타임 업데이트
    for (FStratagemData& Data : StratagemList)
    {
        if (Data.bIsOnCooldown)
        {
            Data.CurrentCooldown -= DeltaTime;

            if (Data.CurrentCooldown <= 0.0f)
            {
                Data.CurrentCooldown = 0.0f;
                Data.bIsOnCooldown = false;

                // 재무장이 끝났다면 스택 복구
                if (Data.bUseStack && Data.bIsRearming)
                {
                    Data.CurrentStack = Data.MaxStack;
                    Data.bIsRearming = false;
                }
            }
        }
    }
}

void AFPSCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
    {
        // 이동 및 시야 처리
        EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AFPSCharacter::Move);
        EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AFPSCharacter::Look);

        // 점프
        EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
        EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

        // 달리기 (홀드)
        EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Started, this, &AFPSCharacter::OnSprintStarted);
        EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, this, &AFPSCharacter::OnSprintCompleted);

        // 수류탄 던지기
        EnhancedInputComponent->BindAction(GrenadeAction, ETriggerEvent::Started, this, &AFPSCharacter::OnGrenadeStart);

        if (Weapon1Action)
            EnhancedInputComponent->BindAction(Weapon1Action, ETriggerEvent::Started, this, &AFPSCharacter::OnWeapon1);
        if (Weapon2Action)
            EnhancedInputComponent->BindAction(Weapon2Action, ETriggerEvent::Started, this, &AFPSCharacter::OnWeapon2);

        // 사격 (FireWeapon 함수 연동)
        if (FireAction)
        {
            // 누를 때 OnFireStarted 호출
            EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Started, this, &AFPSCharacter::OnFireStarted);

            // 뗄 때 OnFireCompleted 호출
            EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Completed, this, &AFPSCharacter::OnFireCompleted);
        }

        if (AimAction)
        {
            EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Triggered, this, &AFPSCharacter::OnAimStarted);
            EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Completed, this, &AFPSCharacter::OnAimCompleted);
        }

        if (ReloadAction)
        {
            EnhancedInputComponent->BindAction(ReloadAction, ETriggerEvent::Started, this, &AFPSCharacter::Reload);
        }

        // Q 키 (메뉴 열기/닫기)
        EnhancedInputComponent->BindAction(StratagemMenuAction, ETriggerEvent::Started, this, &AFPSCharacter::OnStratagemMenuAction);

        // 스트라타젬 방향 입력
        EnhancedInputComponent->BindAction(StratagemInputAction, ETriggerEvent::Triggered, this, &AFPSCharacter::OnStratagemInputAction);
    }
}

void AFPSCharacter::Move(const FInputActionValue& Value)
{
    if (bIsDead) return;

    FVector2D MovementVector = Value.Get<FVector2D>();

    if (Controller != nullptr)
    {
        const FRotator Rotation = Controller->GetControlRotation();
        const FRotator YawRotation(0, Rotation.Yaw, 0);
        const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
        const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

        AddMovementInput(ForwardDirection, MovementVector.Y);
        AddMovementInput(RightDirection, MovementVector.X);
    }
}

void AFPSCharacter::Look(const FInputActionValue& Value)
{
    FVector2D LookAxisVector = Value.Get<FVector2D>();

    if (Controller != nullptr)
    {
        float Sensitivity = MouseSensitivity;

        AddControllerYawInput(LookAxisVector.X * Sensitivity);
        AddControllerPitchInput(LookAxisVector.Y * Sensitivity);
    }
}

void AFPSCharacter::HandleHealthChanged(const FOnAttributeChangeData& Data)
{
    OnHealthChanged.Broadcast(Data.NewValue, AttributeSet->GetMaxHealth());

    if (Data.NewValue <= 0.0f)
    {
        Die();
    }
}

void AFPSCharacter::HandleStaminaChanged(const FOnAttributeChangeData& Data)
{
    float NewStamina = Data.NewValue;
    OnStaminaChanged.Broadcast(NewStamina, AttributeSet->GetMaxStamina());
}

// 달리기
void AFPSCharacter::OnSprintStarted()
{
    bSprintButtonDown = true;

    // 조준 중이거나 사격 중이면 입력만 저장하고 리턴
    if (bIsAiming || bFireButtonDown) return;

    // 스태미나가 0보다 크다면 달리기 시작
    if (CurrentStamina > 0.0f)
    {
        bIsSprintActive = true;

        // 회복 타이머 중단
        GetWorldTimerManager().ClearTimer(TimerHandle_StaminaRegen);
        GetWorldTimerManager().ClearTimer(TimerHandle_StaminaRegenTick);

        // 속도 변경
        GetCharacterMovement()->MaxWalkSpeed = SprintWalkSpeed;

        // 스태미나 감소 타이머 시작 (0.1초마다)
        GetWorldTimerManager().SetTimer(TimerHandle_StaminaDrain, this, &AFPSCharacter::DrainStamina, 0.1f, true);
    }
}

void AFPSCharacter::OnSprintCompleted()
{
    bSprintButtonDown = false;
    bIsSprintActive = false;

    // 스태미나 감소 타이머 중단
    GetWorldTimerManager().ClearTimer(TimerHandle_StaminaDrain);

    // 속도 원복
    if (!bIsAiming)
    {
        GetCharacterMovement()->MaxWalkSpeed = NormalWalkSpeed;
    }

    // 2초 후 회복 시작
    GetWorldTimerManager().SetTimer(TimerHandle_StaminaRegen, this, &AFPSCharacter::StartStaminaRegen, 2.0f, false);
}

void AFPSCharacter::DrainStamina()
{
    CurrentStamina = FMath::Max(CurrentStamina - 1.0f, 0.0f); // 초당 10

    // UI 업데이트
    OnStaminaChanged.Broadcast(CurrentStamina, MaxStamina);

    // 스태미나가 0이면 달리기 중지
    if (CurrentStamina <= 0.0f)
    {
        OnSprintCompleted();
    }
}

void AFPSCharacter::StartStaminaRegen()
{
    // 달리는 중이 아닐 때만 회복
    if (!bIsSprintActive)
    {
        GetWorldTimerManager().SetTimer(TimerHandle_StaminaRegenTick, this, &AFPSCharacter::RegenStamina, 0.1f, true);
    }
}

void AFPSCharacter::RegenStamina()
{
    if (CurrentStamina >= MaxStamina)
    {
        GetWorldTimerManager().ClearTimer(TimerHandle_StaminaRegenTick);
        return;
    }

    CurrentStamina = FMath::Min(CurrentStamina + 2.0f, MaxStamina); // 초당 20

    // UI 업데이트
    OnStaminaChanged.Broadcast(CurrentStamina, MaxStamina);
}

void AFPSCharacter::Die()
{
    if (bIsDead) return;
    bIsDead = true;

    // 모든 이동/입력 차단
    GetCharacterMovement()->StopMovementImmediately();
    GetCharacterMovement()->DisableMovement();
    GetCharacterMovement()->SetComponentTickEnabled(false);

    // 래그돌 활성화
    /*GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    GetMesh()->SetSimulatePhysics(true);
    GetMesh()->SetCollisionProfileName(TEXT("Ragdoll"));*/
    
    Destroy();
}

// 수류탄 던지기
void AFPSCharacter::InitInputAndCamera()
{
    APlayerController* PlayerController = Cast<APlayerController>(GetController());
    if (!PlayerController) return;

    // 입력 모드
    PlayerController->bShowMouseCursor = false;
    PlayerController->SetInputMode(FInputModeGameOnly());

    // HUD 위젯
    if (MainHUDWidgetClass && !MainHUDWidget)
    {
        MainHUDWidget = CreateWidget<UUserWidget>(GetWorld(), MainHUDWidgetClass);
        if (MainHUDWidget)
            MainHUDWidget->AddToViewport();
    }
}

void AFPSCharacter::PossessedBy(AController* NewController)
{
    Super::PossessedBy(NewController);
    UE_LOG(LogTemp, Warning, TEXT("PossessedBy called - %s"), *GetName());
    InitInputAndCamera();
}

void AFPSCharacter::OnRep_Controller()
{
    Super::OnRep_Controller();
    UE_LOG(LogTemp, Warning, TEXT("OnRep_Controller called - %s"), *GetName());
    InitInputAndCamera();
}

void AFPSCharacter::OnGrenadeStart()
{
    if (bIsReloading) return;

    // 수류탄이 0보다 많으면 던지기 시작
    if (!bIsThrowingGrenade && CurrentGrenadeCount > 0 && AbilitySystemComponent)
    {
        bIsThrowingGrenade = true;
        AbilitySystemComponent->AbilityLocalInputPressed(static_cast<int32>(EAbilityInputID::Grenade));

        // 개수 감소 및 UI 갱신
        CurrentGrenadeCount--;
        OnGrenadeChanged.Broadcast(CurrentGrenadeCount, MaxGrenadeCount);
    }
}

void AFPSCharacter::OnGrenadeCompleted()
{
    bIsThrowingGrenade = false;
}

void AFPSCharacter::SwitchToWeapon(int32 Index)
{
    if (!Weapons.IsValidIndex(Index) || Index == CurrentWeaponIndex) return;
    if (bIsReloading || bIsThrowingGrenade) return;

    // 현재 무기 탄약 저장
    WeaponAmmo[CurrentWeaponIndex] = CurrentAmmo;

    CurrentWeaponIndex = Index;
    const FWeaponData& WData = Weapons[CurrentWeaponIndex];

    // 무기 메시 교체 및 트랜스폼 보정
    if (WeaponMesh && WData.WeaponMesh)
    {
        WeaponMesh->SetSkeletalMesh(WData.WeaponMesh);
        WeaponMesh->SetRelativeLocation(WData.GripLocationOffset);
        WeaponMesh->SetRelativeRotation(WData.GripRotationOffset);
    }

    // 전투 데이터 교체
    ProjectileClass  = WData.ProjectileClass;
    FireMontage      = WData.FireMontage;
    ReloadMontage    = WData.ReloadMontage;
    MuzzleFlashFX    = WData.MuzzleFlashFX;
    FireSound        = WData.FireSound;
    FireRate         = WData.FireRate;
    RecoilPitch      = WData.RecoilPitch;
    RecoilYaw        = WData.RecoilYaw;
    MaxAmmoInMag     = WData.MaxAmmoInMag;

    // 해당 무기의 저장된 탄약 복원
    CurrentAmmo = WeaponAmmo[CurrentWeaponIndex];
    OnAmmoChanged.Broadcast(CurrentAmmo, CurrentMagCount, MaxMagCount);
    OnWeaponChanged.Broadcast(CurrentWeaponIndex);
}

void AFPSCharacter::OnWeapon1()
{
    SwitchToWeapon(0);
}

void AFPSCharacter::OnWeapon2()
{
    SwitchToWeapon(1);
}

void AFPSCharacter::OnFireStarted()
{
    if (bIsDead || bIsReloading) return;

    bFireButtonDown = true;

    // 스트라타젬 준비 완료 → 비콘 던지기 (조준 여부 무관)
    if (bIsStratagemReady)
    {
        ThrowBeacon();
        return;
    }

    if (!bIsAiming) return;

    StartFiring();
}

void AFPSCharacter::StartFiring()
{
    // 첫 발 즉시 발사
    FireWeapon();

    // FireRate에 따른 반복 타이머 설정 (1.0 / 10.0 = 0.1초 간격)
    float TimeBetweenShots = 1.0f / FireRate;
    GetWorldTimerManager().SetTimer(TimerHandle_AutomaticFire, this, &AFPSCharacter::FireWeapon, TimeBetweenShots, true);
}

void AFPSCharacter::OnFireCompleted()
{
    bFireButtonDown = false;
    StopFiring();

    if (bSprintButtonDown)
    {
        OnSprintStarted();
    }
}

void AFPSCharacter::StopFiring()
{
    GetWorldTimerManager().ClearTimer(TimerHandle_AutomaticFire);
}

void AFPSCharacter::Reload()
{
    if (bIsReloading || GetWorldTimerManager().IsTimerActive(TimerHandle_Reload) || bIsDead) return;

    if (CurrentAmmo >= MaxAmmoInMag || bIsReloading || bIsThrowingGrenade) return;
    
    StopFiring(); // 사격중이었다면 사격 중지

    if (bSprintButtonDown)
    {
        GetCharacterMovement()->MaxWalkSpeed = SprintWalkSpeed;
    }

    bIsReloading = true;
    GetCharacterMovement()->MaxWalkSpeed = ReloadWalkSpeed;
    
    float ReloadDuration = 2.0f; // fallback

    if (ReloadMontage && GetMesh())
    {
        if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
        {
            const float MontageLength = AnimInstance->Montage_Play(ReloadMontage);
            if (MontageLength > 0.0f)
            {
                ReloadDuration = MontageLength;
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("Montage_Play failed, using fallback duration"));
            }
        }
    }

    GetWorldTimerManager().SetTimer(TimerHandle_Reload, this, &AFPSCharacter::FinishReload, ReloadDuration, false);

    // 재장전 사운드 재생
    if (ReloadSound)
    {
        UGameplayStatics::PlaySoundAtLocation(this, ReloadSound, GetActorLocation());
    }
}

void AFPSCharacter::FinishReload()
{
    GetWorldTimerManager().ClearTimer(TimerHandle_Reload);
    
    UE_LOG(LogTemp, Warning, TEXT("FinishReload called"));
    bIsReloading = false;

    // 무한 탄창 (탄창 수 감소 없음)
    CurrentAmmo = MaxAmmoInMag;

    if (bIsAiming)
    {
        // 조준 속도 적용
        GetCharacterMovement()->MaxWalkSpeed = AimWalkSpeed;
    }
    else if (bSprintButtonDown)
    {
        GetCharacterMovement()->MaxWalkSpeed = SprintWalkSpeed;
    }
    else
    {
        GetCharacterMovement()->MaxWalkSpeed = NormalWalkSpeed;
    }

    OnAmmoChanged.Broadcast(CurrentAmmo, CurrentMagCount, MaxMagCount);
}


void AFPSCharacter::ResetRotationMode()
{
    // 조준 상태 아니고 에임 버튼이 떨어진 타이밍에 회전 방식 복원
    if (!bIsAiming)
    {
        bUseControllerRotationYaw = false;
        GetCharacterMovement()->bOrientRotationToMovement = true;
    }
}

void AFPSCharacter::OnAimStarted_Implementation()
{
    if (bIsDead) return;

    bAimButtonDown = true;
    bIsAiming = true;
}

void AFPSCharacter::OnAimCompleted_Implementation()
{
    bAimButtonDown = false;
    bIsAiming = false;

    if (bSprintButtonDown)
    {
        OnSprintStarted();
    }
}

void AFPSCharacter::FireWeapon()
{
    if (CurrentAmmo <= 0)
    {
        StopFiring();
        return;
    }

    if (!ThirdPersonCamera) return;

    // 반동은 로컬에서 즉시 처리
    if (IsLocallyControlled())
    {
        AddControllerPitchInput(-RecoilPitch);
        float RandomYaw = FMath::FRandRange(-RecoilYaw, RecoilYaw);
        AddControllerYawInput(RandomYaw);

        // 카메라 쉐이크도 로컬만
        if (APlayerController* PC = Cast<APlayerController>(GetController()))
        {
            if (FireCameraShakeClass)
                PC->ClientStartCameraShake(FireCameraShakeClass);
        }
    }

    // 탄약 차감 및 UI는 로컬에서 즉시
    CurrentAmmo--;
    OnAmmoChanged.Broadcast(CurrentAmmo, CurrentMagCount, MaxMagCount);

    // 발사 위치/방향 계산
    FVector MuzzleLocation = WeaponMesh->GetSocketLocation(TEXT("MuzzleSocket"));

    FVector CameraLocation = ThirdPersonCamera->GetComponentLocation();
    FVector CameraForward = ThirdPersonCamera->GetForwardVector();
    FVector TraceEnd = CameraLocation + (CameraForward * FireRange);

    FHitResult AimHitResult;
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(this);

    FVector LookAtLocation;
    if (GetWorld()->LineTraceSingleByChannel(AimHitResult, CameraLocation, TraceEnd, ECC_Visibility, QueryParams))
        LookAtLocation = AimHitResult.ImpactPoint;
    else
        LookAtLocation = TraceEnd;

    FRotator TargetRotation = (LookAtLocation - MuzzleLocation).Rotation();

    // 이펙트/사운드/애니메이션은 Multicast로 모든 클라에
    Multicast_FireEffects(MuzzleLocation, TargetRotation);

    // 투사체 스폰은 서버에 요청
    Server_FireWeapon(MuzzleLocation, TargetRotation);
}

void AFPSCharacter::Server_FireWeapon_Implementation(FVector MuzzleLocation, FRotator TargetRotation)
{
    if (!ProjectileClass) return;

    FActorSpawnParameters ActorSpawnParams;
    ActorSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
    ActorSpawnParams.Owner = this;
    ActorSpawnParams.Instigator = this;

    GetWorld()->SpawnActor<ADediProjectile>(ProjectileClass, MuzzleLocation, TargetRotation, ActorSpawnParams);

    // 소음은 서버에서 처리
    UAISense_Hearing::ReportNoiseEvent(GetWorld(), GetActorLocation(), 1.0f, this, 0.0f, FName(TEXT("Noise")));
}

void AFPSCharacter::Multicast_FireEffects_Implementation(FVector MuzzleLocation, FRotator TargetRotation)
{
    // 총구 화염
    if (MuzzleFlashFX)
    {
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), MuzzleFlashFX, MuzzleLocation, TargetRotation);
    }

    // 발사 사운드
    if (FireSound)
    {
        UGameplayStatics::PlaySoundAtLocation(this, FireSound, GetActorLocation());
    }

    // 발사 애니메이션
    if (FireMontage)
    {
        if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
        {
            float FireInterval = 1.0f / FireRate;
            float MontageLength = FireMontage->GetPlayLength();
            AnimInstance->Montage_Play(FireMontage, MontageLength / FireInterval);
        }
    }
}

void AFPSCharacter::ThrowBeacon()
{
    if (!BeaconClass || !StratagemList.IsValidIndex(ActiveStratagemIndex)) return;

    FStratagemData& ActiveData = StratagemList[ActiveStratagemIndex];

    // 쿨타임 처리 (로컬 상태)
    if (ActiveData.bUseStack)
    {
        if (ActiveData.CurrentStack > 0)
        {
            ActiveData.CurrentStack--;
            if (ActiveData.CurrentStack <= 0)
            {
                ActiveData.bIsOnCooldown = true;
                ActiveData.bIsRearming = true;
                ActiveData.CurrentCooldown = ActiveData.MaxCooldown;
            }
        }
    }
    else
    {
        ActiveData.bIsOnCooldown = true;
        ActiveData.CurrentCooldown = ActiveData.MaxCooldown;
    }

    FVector SpawnLocation = WeaponMesh->GetSocketLocation(TEXT("MuzzleSocket"));
    FRotator ControlRot = GetControlRotation();
    FRotator SpawnRotation = ControlRot;

    // 비콘 스폰은 서버에 요청
    Server_ThrowBeacon(BeaconClass, SpawnLocation, SpawnRotation, ActiveData.Type, ActiveData.BeaconColor);

    // 상태 초기화는 로컬에서
    CurrentInputStack.Empty();
    bIsStratagemReady = false;
    bIsSelectingStratagem = false;
    OnStratagemStackUpdated.Broadcast(CurrentInputStack);
}

void AFPSCharacter::Server_ThrowBeacon_Implementation(TSubclassOf<AStratagemBeacon> InBeaconClass, FVector SpawnLocation, FRotator SpawnRotation, EStratagemType StratagemType, FLinearColor BeaconColor)
{
    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = this;
    SpawnParams.Instigator = GetInstigator();

    AStratagemBeacon* SpawnedBeacon = GetWorld()->SpawnActor<AStratagemBeacon>(
        InBeaconClass, SpawnLocation, SpawnRotation, SpawnParams
    );

    if (SpawnedBeacon)
    {
        SpawnedBeacon->MyStratagemType = StratagemType;
        SpawnedBeacon->UpdateBeaconVisual(BeaconColor);

        if (UProjectileMovementComponent* ProjComp = SpawnedBeacon->FindComponentByClass<UProjectileMovementComponent>())
        {
            ProjComp->Velocity = SpawnRotation.Vector() * ThrowForce;
        }
    }
}

void AFPSCharacter::OnStratagemMenuAction(const FInputActionValue& Value)
{
    if (bIsThrowingGrenade) return;

    bIsSelectingStratagem = !bIsSelectingStratagem;

    if (bIsSelectingStratagem)
    {
        CurrentCommandStep = 0;
        OnStratagemStepUpdated.Broadcast(CurrentCommandStep);
    }
    else
    {
        // 메뉴가 닫힐 때 입력 상태 초기화
        CurrentInputStack.Empty();
        bIsStratagemReady = false;

        // UI에 빈 스택을 전달해 화살표들이 다시 회색으로 변경
        OnStratagemStackUpdated.Broadcast(CurrentInputStack);

        if (!bIsStratagemReady)
        {
            CurrentCommandStep = 0;
            OnStratagemStepUpdated.Broadcast(CurrentCommandStep);
        }
    }
}

void AFPSCharacter::OnStratagemInputAction(const FInputActionValue& Value)
{
    FVector2D InputValue = Value.Get<FVector2D>();

    EStratagemDirection DetectedDir = EStratagemDirection::None;

    // Vector2D 값을 분석하여 방향 판단
    if (InputValue.Y > 0.5f) DetectedDir = EStratagemDirection::Up;
    else if (InputValue.Y < -0.5f) DetectedDir = EStratagemDirection::Down;
    else if (InputValue.X < -0.5f) DetectedDir = EStratagemDirection::Left;
    else if (InputValue.X > 0.5f) DetectedDir = EStratagemDirection::Right;

    if (DetectedDir == EStratagemDirection::None) return;

    if (!bIsSelectingStratagem || bIsStratagemReady) return;

    bool bAnyValidStratagemAvailable = false;
    for (const FStratagemData& Data : StratagemList)
    {
        if (!Data.bIsOnCooldown)
        {
            bAnyValidStratagemAvailable = true;
            break;
        }
    }

    // 모두 쿨타임이면 return
    if (!bAnyValidStratagemAvailable) return;

    // 입력 스택에 추가
    CurrentInputStack.Add(DetectedDir);

    bool bAnyMatchFound = false;
    int32 CompletedIndex = -1;

    // 모든 스트라타젬 순회
    for (int32 i = 0; i < StratagemList.Num(); ++i)
    {
        // 쿨타임이면 continue
        if (StratagemList[i].bIsOnCooldown) continue;

        const TArray<EStratagemDirection>& TargetCommand = StratagemList[i].Command;
        bool bIsStillValid = true;

        // 입력된 길이만큼 비교
        for (int32 j = 0; j < CurrentInputStack.Num(); ++j)
        {
            if (j >= TargetCommand.Num() || CurrentInputStack[j] != TargetCommand[j])
            {
                bIsStillValid = false;
                break;
            }
        }

        if (bIsStillValid)
        {
            bAnyMatchFound = true;
            // 완전히 일치하는 커맨드인지 확인
            if (CurrentInputStack.Num() == TargetCommand.Num())
            {
                CompletedIndex = i;
            }
        }
    }

    // 결과 처리
    if (CompletedIndex != -1) // 커맨드 완성
    {
        if (StratagemCompleteSound) UGameplayStatics::PlaySound2D(this, StratagemCompleteSound);
        ActiveStratagemIndex = CompletedIndex;
        bIsStratagemReady = true;
    }
    else if (!bAnyMatchFound) // 하나도 맞는 게 없으면 초기화
    {
        CurrentInputStack.Empty();

        if (StratagemErrorSound)
        {
            UGameplayStatics::PlaySound2D(this, StratagemErrorSound);
        }
    }
    else
    {
        if (StratagemInputSound)
        {
            UGameplayStatics::PlaySound2D(this, StratagemInputSound);
        }
    }

    // UI에 현재 입력 상태 전달
    OnStratagemStackUpdated.Broadcast(CurrentInputStack);
}
