#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "AbilitySystemInterface.h"
#include "GenericTeamAgentInterface.h"
#include "GameplayAbilitySpec.h"

#include "FPSCharacter.generated.h"

class AHDProjectile;
class UNiagaraSystem;
class UGameplayEffect;

// 무기 데이터 구조체
USTRUCT(BlueprintType)
struct FWeaponData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    USkeletalMesh* WeaponMesh = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TSubclassOf<class ADediProjectile> ProjectileClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    class UAnimMontage* FireMontage = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    class UAnimMontage* ReloadMontage = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    class UNiagaraSystem* MuzzleFlashFX = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    class USoundBase* FireSound = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float FireRate = 10.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float RecoilPitch = 0.1f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float RecoilYaw = 0.03f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 MaxAmmoInMag = 30;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grip Correction")
    FVector GripLocationOffset = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grip Correction")
    FRotator GripRotationOffset = FRotator::ZeroRotator;
};

// 스트라타젬 입력 화살표 방향
UENUM(BlueprintType)
enum class EStratagemDirection : uint8
{
    None, Up, Down, Left, Right
};

// 스트라타젬 타입
UENUM(BlueprintType)
enum class EStratagemType : uint8
{
    None,
    Bomb500kg,
    Supply,
    EagleCluster,
    OrbitalLaser,
    Sentry,
    Rearm,
};

// 스트라타젬 데이터 구조체
USTRUCT(BlueprintType)
struct FStratagemData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString Name;

    // 스트라타젬 아이콘 이미지
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stratagem")
    class UTexture2D* Icon = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<EStratagemDirection> Command;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FLinearColor BeaconColor = FLinearColor::White;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EStratagemType Type= EStratagemType::None;

    // 쿨타임 설정 (총 시간)
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float MaxCooldown = 10.0f;

    // 현재 남은 쿨타임 시간
    UPROPERTY(BlueprintReadWrite)
    float CurrentCooldown = 0.0f;

    // 현재 쿨타임 중인지 여부
    UPROPERTY(BlueprintReadWrite)
    bool bIsOnCooldown = false;

    // 스택형 스트라타젬 (이글)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stratagem")
    bool bUseStack = false;          // 스택 시스템 사용 여부

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stratagem")
    int32 MaxStack = 1;              // 최대 사용 횟수

    UPROPERTY(BlueprintReadWrite, Category = "Stratagem")
    int32 CurrentStack = 1;          // 현재 남은 횟수

    UPROPERTY(BlueprintReadWrite, Category = "Stratagem")
    bool bIsRearming = false;        // 재무장(쿨타임) 중인지 여부
};

// GAS
UENUM(BlueprintType)
enum class EAbilityInputID : uint8
{
    None,
    Confirm,
    Cancel,
    Aim,
    Fire,       // 사격용 ID
    Reload,     // 재장전용 ID
    Jump,        // 점프용 ID
    Sprint,
    Grenade,
};

// 델리게이트 선언
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnAmmoChangedSignature, int32, NewAmmo, int32, NewMag, int32, MaxMag);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHitEnemySignature, bool, bIsHeadshot);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStratagemStepUpdated, int32, NewStep);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnStratagemUpdate, int32, StratagemIndex, int32, CurrentStep);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStratagemStackUpdated, const TArray<EStratagemDirection>&, InputStack);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAttributeChanged, float, CurrentValue, float, MaxValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnGrenadeChanged, int32, Current, int32, Max);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponChanged, int32, NewWeaponIndex);

UCLASS(Blueprintable)
class DEDITEST_API AFPSCharacter : public ACharacter, public IAbilitySystemInterface, public IGenericTeamAgentInterface
{
    GENERATED_BODY()

    // 팀 설정 (플레이어 = TeamId 0)
    FGenericTeamId TeamId = FGenericTeamId(0);

    virtual FGenericTeamId GetGenericTeamId() const override { return TeamId; }

public:
    AFPSCharacter();

    // GAS
    virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

    UPROPERTY(BlueprintAssignable, Category = "Attributes")
    FOnAttributeChanged OnHealthChanged;

    UPROPERTY(BlueprintAssignable, Category = "Attributes")
    FOnAttributeChanged OnStaminaChanged;

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnHitEnemySignature OnHitEnemy;

    virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

protected:
    virtual void BeginPlay() override;
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
    virtual void PossessedBy(AController* NewController) override;
    virtual void OnRep_Controller() override;

    // 플레이어 AttributeSet
    UPROPERTY()
    class UPlayerAttributeSet* AttributeSet;

    // Health 속성이 변경될 때 ASC에 의해 호출될 콜백 함수
    void HandleHealthChanged(const FOnAttributeChangeData& Data);

    // Stamina 속성이 변경될 때 ASC에 의해 호출될 콜백 함수
    void HandleStaminaChanged(const FOnAttributeChangeData& Data);

    // GAS 컴포넌트
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
    class UAbilitySystemComponent* AbilitySystemComponent;

    // GAS 수류탄 던지기 능력 클래스
    UPROPERTY(EditAnywhere, Category = "GAS")
    TSubclassOf<UGameplayAbility> GrenadeAbilityClass;

    // GAS 달리기 능력 클래스
    UPROPERTY(EditAnywhere, Category = "GAS")
    TSubclassOf<UGameplayAbility> SprintAbilityClass;

    // GAS 스태미나 리젠 능력 클래스
    UPROPERTY(EditAnywhere, Category = "GAS")
    TSubclassOf<UGameplayEffect> StaminaRegenEffectClass;

    void StartStaminaRegen();
    void DrainStamina();
    void RegenStamina();

    FTimerHandle TimerHandle_StaminaRegen;
    FTimerHandle TimerHandle_StaminaDrain;
    FTimerHandle TimerHandle_StaminaRegenTick;

public:
    void OnSprintStarted();
    void OnSprintCompleted();

    UFUNCTION(Server, Reliable)
    void Server_StartSprint();

    UFUNCTION(Server, Reliable)
    void Server_StopSprint();

    void Die();

    bool bIsDead = false;

protected:

    /* 입력 처리 함수들 */
    void Move(const FInputActionValue& Value);
    void Look(const FInputActionValue& Value);

    void OnFireStarted();
    void OnFireCompleted();
    void ResetRotationMode();
    void StartFiring();
    void StopFiring();
    void Reload();
    void OnGrenadeStart();
    void OnWeapon1();
    void OnWeapon2();

    void InitInputAndCamera();

    UFUNCTION(BlueprintCallable)
    void OnGrenadeCompleted();

    // 스트라타젬 함수
    void OnStratagemMenuAction(const struct FInputActionValue& Value);
    void OnStratagemInputAction(const struct FInputActionValue& Value);

    // 이동 속도
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float NormalWalkSpeed = 300.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float SprintWalkSpeed = 600.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float AimWalkSpeed = 200.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float ReloadWalkSpeed = 300.0f;

    UFUNCTION(BlueprintCallable, Category = "Combat")
    void FinishReload();

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Combat")
    bool bIsReloading = false;

    bool bSprintButtonDown = false;  // 토글 상태
    bool bIsSprintActive = false;     // 실제 스프린트 어빌리티 활성 상태

    // 스태미나 시스템
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stamina")
    float CurrentStamina = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stamina")
    float MaxStamina = 100.0f;



    // 에임
    UFUNCTION(BlueprintNativeEvent, Category = "Combat")
    void OnAimStarted();
    virtual void OnAimStarted_Implementation();

    UFUNCTION(BlueprintNativeEvent, Category = "Combat")
    void OnAimCompleted();
    virtual void OnAimCompleted_Implementation();

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Combat")
    bool bIsAiming = false;

    FTimerHandle TimerHandle_RotationReset;

    UPROPERTY(EditAnywhere, Category = "Combat")
    float AimRotationRecoveryTime = 0.3f;

    UFUNCTION(BlueprintCallable, Category = "Combat")
    void FireWeapon();

    // 서버에 발사 요청
    UFUNCTION(Server, Reliable)
    void Server_FireWeapon(FVector MuzzleLocation, FRotator TargetRotation);

    // 모든 클라이언트에 이펙트 재생
    UFUNCTION(NetMulticast, Unreliable)
    void Multicast_FireEffects(FVector MuzzleLocation, FRotator TargetRotation);

    FTimerHandle TimerHandle_AutomaticFire;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    float FireRate = 10.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    float RecoilPitch = 0.1f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    float RecoilYaw = 0.03f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    TSubclassOf<class UCameraShakeBase> FireCameraShakeClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    UNiagaraSystem* MuzzleFlashFX;

public:
    // 탄약 시스템
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    int32 MaxAmmoInMag = 30;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    int32 CurrentAmmo = 30;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    int32 CurrentMagCount = 8;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    int32 MaxMagCount = 8;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    int32 CurrentGrenadeCount = 5;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    int32 MaxGrenadeCount = 5;

    bool bIsThrowingGrenade = false;

    FTimerHandle TimerHandle_Reload;

protected:

    // 애니메이션
    UPROPERTY(BlueprintReadOnly, Category = "Animation")
    float AimPitch;

    void UpdateAimOffset(float DeltaTime);

    bool bAimButtonDown = false;
    bool bFireButtonDown = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    class USoundBase* FireSound;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    class USoundBase* ReloadSound;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    class UAnimMontage* ReloadMontage;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    class UAnimMontage* FireMontage;

    // ----------------스트라타젬---------------------
    UPROPERTY(EditAnywhere, Category = "Stratagem")
    TSubclassOf<class AStratagemBeacon> BeaconClass;

    void ThrowBeacon();

    UFUNCTION(Server, Reliable)
    void Server_ThrowBeacon(TSubclassOf<AStratagemBeacon> InBeaconClass, FVector SpawnLocation, FRotator SpawnRotation, EStratagemType StratagemType, FLinearColor BeaconColor);

    UPROPERTY(EditAnywhere, Category = "Stratagem")
    float ThrowForce = 3000.0f;

    int32 CurrentCommandStep = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stratagem")
    bool bIsSelectingStratagem = false;

    bool bIsStratagemReady = false;
    float ComboExpireTime = 7.0f;

    // --- 스트라타젬 사운드 ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stratagem")
    class USoundBase* StratagemInputSound;    // 입력 성공 (소리 1)

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stratagem")
    class USoundBase* StratagemErrorSound;    // 입력 실패 (소리 2)

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stratagem")
    class USoundBase* StratagemCompleteSound; // 입력 완료 (소리 3)

    // 중복 및 이름 오류 수정: StratagemList는 FStratagemData 타입을 사용합니다.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stratagem")
    TArray<FStratagemData> StratagemList;

    int32 ActiveStratagemIndex = 0;

    TArray<EStratagemDirection> CurrentInputStack;

public:
    virtual void Tick(float DeltaTime) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
    class USpringArmComponent* CameraBoom;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
    class UCameraComponent* ThirdPersonCamera;

    // 카메라가 벽에 밀려 캐릭터에 가까워지면 true
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
    bool bCameraTooClose = false;

    // UI
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
    TSubclassOf<class UUserWidget> MainHUDWidgetClass;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
    class UUserWidget* MainHUDWidget;

    // 입력 시스템
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    class UInputMappingContext* DefaultMappingContext;

    UPROPERTY(EditAnywhere, Category = "Input")
    class UInputMappingContext* StratagemMappingContext;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    class UInputAction* MoveAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    class UInputAction* LookAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    class UInputAction* JumpAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    class UInputAction* SprintAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    class UInputAction* GrenadeAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    class UInputAction* Weapon1Action;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    class UInputAction* Weapon2Action;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    class UInputAction* FireAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    class UInputAction* AimAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    class UInputAction* ReloadAction;

    UPROPERTY(EditAnywhere, Category = "Input")
    class UInputAction* StratagemMenuAction;

    UPROPERTY(EditAnywhere, Category = "Input")
    class UInputAction* StratagemInputAction;

    // 이벤트 델리게이트
    UPROPERTY(BlueprintAssignable, Category = "Stratagem")
    FOnStratagemStepUpdated OnStratagemStepUpdated;

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnAmmoChangedSignature OnAmmoChanged;

    UPROPERTY(BlueprintAssignable, Category = "Stratagem")
    FOnStratagemUpdate OnStratagemUpdate;

    UPROPERTY(BlueprintAssignable, Category = "Stratagem")
    FOnStratagemStackUpdated OnStratagemStackUpdated;

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnGrenadeChanged OnGrenadeChanged;

    UPROPERTY(BlueprintAssignable, Category = "Weapon")
    FOnWeaponChanged OnWeaponChanged;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
    float MouseSensitivity = 0.3f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh")
    class USkeletalMeshComponent* RetargetMesh;

protected:

    UPROPERTY(BlueprintReadWrite, Category = "Mesh")
    class USkeletalMeshComponent* WeaponMesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
    class USkeletalMesh* DefaultWeaponMesh;

    // 무기 2개 데이터
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
    TArray<FWeaponData> Weapons;

    // ABP에서 읽을 수 있도록 BlueprintReadOnly
    UPROPERTY(BlueprintReadOnly, Category = "Weapon")
    int32 CurrentWeaponIndex = 0;

    UFUNCTION(BlueprintCallable, Category = "Weapon")
    void SwitchToWeapon(int32 Index);

    // 무기별 탄약 저장 (교체해도 각자 유지)
    int32 WeaponAmmo[2] = { 30, 30 };

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    float FireRange = 300000.0f;

    UPROPERTY(EditAnywhere, Category = "Combat")
    TSubclassOf<class ADediProjectile> ProjectileClass;
};
