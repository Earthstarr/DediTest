#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "Eagle.generated.h"

UCLASS()
class DEDITEST_API AEagle : public AActor
{
    GENERATED_BODY()

public:
    AEagle();

protected:
    virtual void BeginPlay() override;

public:
    virtual void Tick(float DeltaTime) override;

    // 비콘에서 호출하여 시작/끝 위치 설정 및 비행 시작
    void InitEagle(FVector Start, FVector End, float Duration);

private:

    UPROPERTY(VisibleAnywhere)
    class USceneComponent* SceneRoot;

    UPROPERTY(VisibleAnywhere)
    UStaticMeshComponent* MeshComp;

    // 날개 트레일
    UPROPERTY(VisibleAnywhere, Category = "Effects")
    class UNiagaraComponent* LeftWingTrail;

    UPROPERTY(VisibleAnywhere, Category = "Effects")
    class UNiagaraComponent* RightWingTrail;

    UPROPERTY(EditAnywhere, Category = "Effects")
    class UNiagaraSystem* WingTrailTemplate;

    UPROPERTY(EditAnywhere, Category = "Effects")
    FVector LeftWingOffset = FVector(0.f, -300.f, 0.f);

    UPROPERTY(EditAnywhere, Category = "Effects")
    FVector RightWingOffset = FVector(0.f, 300.f, 0.f);

    UPROPERTY(EditAnywhere, Category = "Effects")
    class USoundBase* EagleVoice;

    UPROPERTY(EditAnywhere, Category = "Effects")
    class UNiagaraSystem* FlareTemplate;

    // 플레어 뿌리는 타이밍 설정 (0.0 ~ 1.0)
    UPROPERTY(EditAnywhere, Category = "Effects")
    float FlareAlpha1 = 0.4f;

    UPROPERTY(EditAnywhere, Category = "Effects")
    float FlareAlpha2 = 0.42f;

    UPROPERTY(EditAnywhere, Category = "Effects")
    float FlareAlpha3 = 0.45f;

    // 이글 보이스 타이밍
    UPROPERTY(EditAnywhere, Category = "Effects")
    float VoiceStartAlpha = 0.2f;

    bool bVoicePlayed = false;
    bool bHasFiredFlare1 = false;
    bool bHasFiredFlare2 = false;
    bool bHasFiredFlare3 = false;

    FVector StartLoc;
    FVector EndLoc;
    float FlightDuration;
    float ElapsedTime;
    bool bIsFlying;

    // 페이드아웃
    bool bIsFadingOut = false;
    float FadeOutElapsed = 0.0f;

    UPROPERTY(EditAnywhere, Category = "Flight")
    float FadeOutDuration = 0.5f;

    // 트레일 잔존 시간 (페이드아웃 후 파티클이 자연 소멸할 때까지)
    UPROPERTY(EditAnywhere, Category = "Flight")
    float TrailLingerTime = 3.0f;

    // Sin 곡선 강하 크기 (0이면 직선 비행)
    UPROPERTY(EditAnywhere, Category = "Flight")
    float CurveHeight = -135000.f;

    // 곡선 방향 축 (기본: UpVector = 위아래 곡선)
    FVector CurveAxis = FVector::UpVector;

    // 플레어 스폰 함수
    void SpawnFlareEffect();

public:
    void SetCurveAxis(const FVector& Axis) { CurveAxis = Axis; }
};
