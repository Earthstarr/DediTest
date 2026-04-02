// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "DediTestPlayerState.generated.h"

UENUM(BlueprintType)
enum class ETeam : uint8
{
	None UMETA(DisplayName = "None"),
	Red UMETA(DisplayName = "Red"),
	Blue UMETA(DisplayName = "Blue"),
};

UCLASS()
class DEDITEST_API ADediTestPlayerState : public APlayerState
{
	GENERATED_BODY()
	
public:
	ADediTestPlayerState();
	
protected:
	// HP
	UPROPERTY(ReplicatedUsing=OnRep_Health, BlueprintReadOnly, Category="Stats")
	float Health;
	
	// MaxHP
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Stats")
	float MaxHealth;
	
	// Team
	UPROPERTY(ReplicatedUsing=OnRep_Team, BlueprintReadOnly, Category="Team")
	ETeam Team;
	
	// 킬
	UPROPERTY(Replicated, BlueprintReadOnly, Category="Stats")
	int32 Kills;
	
	// 데스
	UPROPERTY(Replicated, BlueprintReadOnly, Category="Stats")
	int32 Deaths;
	
public:
	// Replication
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	// HP
	UFUNCTION()
	void OnRep_Health();
	
	UFUNCTION(BlueprintCallable, Category="Stats")
	void SetHealth(float NewHealth);
	
	UFUNCTION(BlueprintCallable, Category="Stats")
	float GetHealth() const { return Health; }
	
	UFUNCTION(BlueprintCallable, Category="Stats")
	float GetMaxHealth() const { return MaxHealth; }
	
	// 팀
	UFUNCTION()
	void OnRep_Team();
	
	UFUNCTION(BlueprintCallable, Category="Team")
	void SetTeam(ETeam NewTeam);
	
	UFUNCTION(BlueprintCallable, Category="Team")
	ETeam GetTeam() const { return Team; }
	
	// 킬/데스
	UFUNCTION(BlueprintCallable, Category="Stats")
	void AddKill();
	
	UFUNCTION(BlueprintCallable, Category="Stats")
	void AddDeath();
	
	UFUNCTION(BlueprintCallable, Category="Stats")
	int32 GetKills() const { return Kills; }
	
	UFUNCTION(BlueprintCallable, Category="Stats")
	int32 GetDeaths() const { return Deaths; }	
	
};
