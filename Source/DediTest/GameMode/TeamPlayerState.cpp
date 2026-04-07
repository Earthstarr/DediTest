// TeamPlayerState.cpp

#include "TeamPlayerState.h"
#include "Net/UnrealNetwork.h"

ATeamPlayerState::ATeamPlayerState()
{
    TeamID = -1;
}

void ATeamPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ATeamPlayerState, TeamID);
}

void ATeamPlayerState::SetTeam(int32 NewTeamID)
{
    TeamID = NewTeamID;
}
