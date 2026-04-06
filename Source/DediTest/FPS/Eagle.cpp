#include "Eagle.h"
#include "Components/AudioComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"

AEagle::AEagle()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;
    SetReplicateMovement(true);

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    RootComponent = SceneRoot;

    MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
    MeshComp->SetupAttachment(SceneRoot);

    LeftWingTrail = CreateDefaultSubobject<UNiagaraComponent>(TEXT("LeftWingTrail"));
    LeftWingTrail->SetupAttachment(SceneRoot);
    LeftWingTrail->bAutoActivate = true;
    LeftWingTrail->SetAutoDestroy(false);

    RightWingTrail = CreateDefaultSubobject<UNiagaraComponent>(TEXT("RightWingTrail"));
    RightWingTrail->SetupAttachment(SceneRoot);
    RightWingTrail->bAutoActivate = true;
    RightWingTrail->SetAutoDestroy(false);

    bIsFlying = false;
    ElapsedTime = 0.f;
}

void AEagle::BeginPlay()
{
    Super::BeginPlay();
}

void AEagle::InitEagle(FVector Start, FVector End, float Duration)
{
    StartLoc = Start;
    EndLoc = End;
    FlightDuration = Duration;
    bIsFlying = true;

    SetActorLocation(StartLoc);

    FRotator LookAtRot = (EndLoc - StartLoc).Rotation();
    SetActorRotation(LookAtRot);
}

void AEagle::SpawnFlareEffect()
{
    if (FlareTemplate)
    {
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(
            GetWorld(),
            FlareTemplate,
            GetActorLocation(),
            (GetActorRotation() + FRotator(90.f, 0.f, 90.f))
        );
    }
}

void AEagle::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (bIsFlying)
    {
        ElapsedTime += DeltaTime;
        float Alpha = ElapsedTime / FlightDuration;

        FVector CurrentLocation = FMath::Lerp(StartLoc, EndLoc, Alpha);

        float CurveOffset = FMath::Sin(Alpha * PI) * CurveHeight;
        CurrentLocation += CurveAxis * CurveOffset;

        SetActorLocation(CurrentLocation);

        FVector TangentDirection = EndLoc - StartLoc;
        TangentDirection += CurveAxis * CurveHeight * PI * FMath::Cos(Alpha * PI);
        SetActorRotation(TangentDirection.GetSafeNormal().Rotation());

        // 보이스
        if (!bVoicePlayed && Alpha >= VoiceStartAlpha)
        {
            if (EagleVoice)
            {
                UGameplayStatics::PlaySoundAtLocation(this, EagleVoice, GetActorLocation());
            }
            bVoicePlayed = true;
        }

        // 플레어 1차
        if (!bHasFiredFlare1 && Alpha >= FlareAlpha1)
        {
            SpawnFlareEffect();
            bHasFiredFlare1 = true;
        }

        // 플레어 2차
        if (!bHasFiredFlare2 && Alpha >= FlareAlpha2)
        {
            SpawnFlareEffect();
            bHasFiredFlare2 = true;
        }

        // 플레어 3차
        if (!bHasFiredFlare3 && Alpha >= FlareAlpha3)
        {
            SpawnFlareEffect();
            bHasFiredFlare3 = true;
        }

        // 비행 완료 → 페이드아웃
        if (Alpha >= 1.0f)
        {
            bIsFlying = false;
            bIsFadingOut = true;
            FadeOutElapsed = 0.0f;

            if (LeftWingTrail) LeftWingTrail->Deactivate();
            if (RightWingTrail) RightWingTrail->Deactivate();

            MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        }
    }

    // 페이드아웃 (메쉬 스케일 축소)
    if (bIsFadingOut)
    {
        FadeOutElapsed += DeltaTime;
        float FadeAlpha = FMath::Clamp(1.0f - (FadeOutElapsed / FadeOutDuration), 0.0f, 1.0f);
        MeshComp->SetWorldScale3D(FVector(FadeAlpha));

        if (FadeOutElapsed >= FadeOutDuration)
        {
            MeshComp->SetVisibility(false);
        }

        if (FadeOutElapsed >= FadeOutDuration + TrailLingerTime)
        {
            Destroy();
        }
    }
}
