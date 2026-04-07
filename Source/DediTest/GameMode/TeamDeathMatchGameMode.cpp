// TeamDeathMatchGameMode.cpp

#include "TeamDeathMatchGameMode.h"
#include "TeamPlayerState.h"
#include "TeamGameState.h"
#include "GameFramework/PlayerStart.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"

ATeamDeathMatchGameMode::ATeamDeathMatchGameMode()
{
    // PlayerState, GameState 클래스 설정
    PlayerStateClass = ATeamPlayerState::StaticClass();
    GameStateClass = ATeamGameState::StaticClass();

    TeamACount = 0;
    TeamBCount = 0;
}

void ATeamDeathMatchGameMode::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);

    if (!NewPlayer) return;

    // 팀 할당
    ATeamPlayerState* PS = NewPlayer->GetPlayerState<ATeamPlayerState>();
    if (PS)
    {
        int32 AssignedTeam = AssignTeam();
        PS->SetTeam(AssignedTeam);

        UE_LOG(LogTemp, Warning, TEXT("Player assigned to Team %d"), AssignedTeam);
    }
}

int32 ATeamDeathMatchGameMode::AssignTeam()
{
    // 적은 쪽 팀에 할당
    int32 Team = (TeamACount <= TeamBCount) ? 0 : 1;

    if (Team == 0)
        TeamACount++;
    else
        TeamBCount++;

    return Team;
}

AActor* ATeamDeathMatchGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
    if (!Player) return Super::ChoosePlayerStart_Implementation(Player);

    ATeamPlayerState* PS = Player->GetPlayerState<ATeamPlayerState>();
    if (!PS) return Super::ChoosePlayerStart_Implementation(Player);

    // 팀에 맞는 PlayerStart 찾기
    return FindPlayerStartForTeam(PS->TeamID);
}

AActor* ATeamDeathMatchGameMode::FindPlayerStartForTeam(int32 TeamID)
{
    // PlayerStart 태그로 구분: "TeamA", "TeamB"
    FName TeamTag = (TeamID == 0) ? FName("TeamA") : FName("TeamB");

    TArray<AActor*> PlayerStarts;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerStart::StaticClass(), PlayerStarts);

    TArray<AActor*> TeamStarts;
    for (AActor* Start : PlayerStarts)
    {
        APlayerStart* PS = Cast<APlayerStart>(Start);
        if (PS && PS->PlayerStartTag == TeamTag)
        {
            TeamStarts.Add(PS);
        }
    }

    // 랜덤으로 선택
    if (TeamStarts.Num() > 0)
    {
        int32 RandomIndex = FMath::RandRange(0, TeamStarts.Num() - 1);
        return TeamStarts[RandomIndex];
    }

    // 태그 없으면 기본 스폰
    return Super::ChoosePlayerStart_Implementation(nullptr);
}

void ATeamDeathMatchGameMode::RespawnPlayer(AController* Controller)
{
    if (!Controller) return;

    // 기존 Pawn 제거
    if (Controller->GetPawn())
    {
        Controller->GetPawn()->Destroy();
    }

    // 새 Pawn 스폰
    RestartPlayer(Controller);
}

void ATeamDeathMatchGameMode::OnPlayerKilled(AController* Killer, AController* Victim)
{
    if (!Victim) return;

    // 사망자 팀 확인 후 상대 팀 스코어 증가
    ATeamPlayerState* VictimPS = Victim->GetPlayerState<ATeamPlayerState>();
    if (VictimPS && VictimPS->TeamID >= 0)
    {
        int32 OpponentTeam = (VictimPS->TeamID == 0) ? 1 : 0;

        ATeamGameState* GS = GetGameState<ATeamGameState>();
        if (GS)
        {
            GS->AddScore(OpponentTeam);
        }
    }

    // 3초 후 리스폰
    FTimerHandle RespawnTimer;
    GetWorldTimerManager().SetTimer(RespawnTimer, [this, Victim]()
    {
        RespawnPlayer(Victim);
    }, 3.0f, false);
}
