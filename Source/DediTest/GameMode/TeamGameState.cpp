// TeamGameState.cpp

#include "TeamGameState.h"
#include "Net/UnrealNetwork.h"

ATeamGameState::ATeamGameState()
{
    TeamAScore = 0;
    TeamBScore = 0;
    ScoreToWin = 5;
    bGameOver = false;
    WinningTeam = -1;
}

void ATeamGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ATeamGameState, TeamAScore);
    DOREPLIFETIME(ATeamGameState, TeamBScore);
    DOREPLIFETIME(ATeamGameState, bGameOver);
    DOREPLIFETIME(ATeamGameState, WinningTeam);
}

void ATeamGameState::AddScore(int32 TeamID)
{
    if (bGameOver) return;

    if (TeamID == 0)
    {
        TeamAScore++;
    }
    else if (TeamID == 1)
    {
        TeamBScore++;
    }

    OnScoreChanged.Broadcast(TeamAScore, TeamBScore);
    CheckWinCondition();
}

void ATeamGameState::OnRep_TeamAScore()
{
    OnScoreChanged.Broadcast(TeamAScore, TeamBScore);
}

void ATeamGameState::OnRep_TeamBScore()
{
    OnScoreChanged.Broadcast(TeamAScore, TeamBScore);
}

void ATeamGameState::CheckWinCondition()
{
    if (TeamAScore >= ScoreToWin)
    {
        bGameOver = true;
        WinningTeam = 0;
        OnGameOver.Broadcast(WinningTeam);
    }
    else if (TeamBScore >= ScoreToWin)
    {
        bGameOver = true;
        WinningTeam = 1;
        OnGameOver.Broadcast(WinningTeam);
    }
}
