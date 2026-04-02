// Fill out your copyright notice in the Description page of Project Settings.


#include "DediTestPlayerState.h"

#include "Net/UnrealNetwork.h"


ADediTestPlayerState::ADediTestPlayerState()
{
	MaxHealth = 100.0f;
	Health = MaxHealth;
	Team = ETeam::None;
	Kills = 0;
	Deaths = 0;
}

void ADediTestPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(ADediTestPlayerState, Health);
	DOREPLIFETIME(ADediTestPlayerState, Team);
	DOREPLIFETIME(ADediTestPlayerState, Kills);
	DOREPLIFETIME(ADediTestPlayerState, Deaths);
	
}

void ADediTestPlayerState::OnRep_Health()
{
	// TODO : HP 변경시
}

void ADediTestPlayerState::SetHealth(float NewHealth)
{
	// 서버라면
	if (HasAuthority())
	{
		Health = FMath::Clamp(NewHealth, 0.0f, MaxHealth);
	}
}

void ADediTestPlayerState::OnRep_Team()
{
	// TODO : 팀 변경시
}

void ADediTestPlayerState::SetTeam(ETeam NewTeam)
{
	if (HasAuthority())
	{
		Team = NewTeam;
	}
}

void ADediTestPlayerState::AddKill()
{
	if (HasAuthority())
	{
		Kills++;
	}
}

void ADediTestPlayerState::AddDeath()
{
	if (HasAuthority())
	{
		Deaths++;
	}
}
