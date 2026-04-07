// TeamPlayerState.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "TeamPlayerState.generated.h"

UCLASS()
class DEDITEST_API ATeamPlayerState : public APlayerState
{
    GENERATED_BODY()

public:
    ATeamPlayerState();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    // 팀 ID (0 = Team A, 1 = Team B)
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Team")
    int32 TeamID = -1;

    UFUNCTION(BlueprintCallable, Category = "Team")
    void SetTeam(int32 NewTeamID);
};
