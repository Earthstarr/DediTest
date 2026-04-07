// TeamDeathMatchGameMode.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "TeamDeathMatchGameMode.generated.h"

UCLASS()
class DEDITEST_API ATeamDeathMatchGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    ATeamDeathMatchGameMode();

    virtual void PostLogin(APlayerController* NewPlayer) override;
    virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;

    // 플레이어 리스폰
    UFUNCTION()
    void RespawnPlayer(AController* Controller);

    // 킬 처리
    UFUNCTION()
    void OnPlayerKilled(AController* Killer, AController* Victim);

private:
    // 팀 카운터
    int32 TeamACount = 0;
    int32 TeamBCount = 0;

    // 팀 할당
    int32 AssignTeam();

    // 팀별 플레이어 스타트 찾기
    AActor* FindPlayerStartForTeam(int32 TeamID);
};
