// Fill out your copyright notice in the Description page of Project Settings.


#include "DediTestAttributeSet.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffectExtension.h"
#include "DediTestCharacter.h"

UDediTestAttributeSet::UDediTestAttributeSet()
{
	InitHealth(100.0f);
	InitMaxHealth(100.0f);
}

void UDediTestAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UDediTestAttributeSet, Health);
	DOREPLIFETIME(UDediTestAttributeSet, MaxHealth);
}

void UDediTestAttributeSet::OnRep_Health(const FGameplayAttributeData& OldHealth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UDediTestAttributeSet, Health, OldHealth);

	// UI 업데이트 등
	UE_LOG(LogTemp, Warning, TEXT("Health changed: %f -> %f"), OldHealth.GetCurrentValue(), Health.GetCurrentValue());
}

void UDediTestAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxHealth());
	}
}

void UDediTestAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		SetHealth(FMath::Clamp(GetHealth(), 0.0f, GetMaxHealth()));

		// HP 0 -> 사망 처리
		if (GetHealth() <= 0.0f)
		{
			if (ADediTestCharacter* OwnerChar = Cast<ADediTestCharacter>(Data.Target.GetAvatarActor()))
			{
				// TODO: Die() 함수 호출
				UE_LOG(LogTemp, Warning, TEXT("Player Died!"));
			}
		}
	}
}
