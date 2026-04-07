// TeamGameState.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "TeamGameState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnScoreChanged, int32, TeamAScore, int32, TeamBScore);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGameOver, int32, WinningTeam);

UCLASS()
class DEDITEST_API ATeamGameState : public AGameStateBase
{
    GENERATED_BODY()

public:
    ATeamGameState();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    // 팀 스코어
    UPROPERTY(ReplicatedUsing = OnRep_TeamAScore, BlueprintReadOnly, Category = "Score")
    int32 TeamAScore = 0;

    UPROPERTY(ReplicatedUsing = OnRep_TeamBScore, BlueprintReadOnly, Category = "Score")
    int32 TeamBScore = 0;

    // 승리 점수
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Score")
    int32 ScoreToWin = 5;

    // 게임 종료 여부
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Game")
    bool bGameOver = false;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Game")
    int32 WinningTeam = -1;

    // 델리게이트
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnScoreChanged OnScoreChanged;

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnGameOver OnGameOver;

    // 스코어 추가
    UFUNCTION(BlueprintCallable, Category = "Score")
    void AddScore(int32 TeamID);

private:
    UFUNCTION()
    void OnRep_TeamAScore();

    UFUNCTION()
    void OnRep_TeamBScore();

    void CheckWinCondition();
};
