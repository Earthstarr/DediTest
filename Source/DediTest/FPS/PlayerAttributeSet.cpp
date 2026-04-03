// PlayerAttributeSet.cpp

#include "PlayerAttributeSet.h"
#include "GameplayEffectExtension.h"
#include "FPSCharacter.h"

UPlayerAttributeSet::UPlayerAttributeSet()
{
	// 게임 시작 시 모든 플레이어에게 부여되는 절대적인 고정값입니다.
	InitHealth(100.0f);
	InitMaxHealth(100.0f);
	InitStamina(100.0f);
	InitMaxStamina(100.0f);
}

void UPlayerAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	// 부모 클래스의 기본 동작 수행
	Super::PreAttributeChange(Attribute, NewValue);

	// 체력(Health) 값이 변경될 때 0 ~ MaxHealth 사이로 고정
	if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxHealth());
	}

	// 스테미나(Stamina) 값이 변경될 때 0 ~ MaxStamina 사이로 고정
	if (Attribute == GetStaminaAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxStamina());
	}
}

// PlayerAttributeSet.cpp

void UPlayerAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	// 스태미나가 변경된 후 호출됨
	if (Data.EvaluatedData.Attribute == GetStaminaAttribute())
	{
		// 실제 스태미나 값을 0~MaxStamina 사이로 강제 고정
		SetStamina(FMath::Clamp(GetStamina(), 0.0f, GetMaxStamina()));

		// 스테미나가 0이되면 달리기 중지
		if (GetStamina() <= 0.0f)
		{
			if (AFPSCharacter* OwnerChar = Cast<AFPSCharacter>(Data.Target.GetAvatarActor()))
			{
				OwnerChar->OnSprintCompleted();
			}
		}
	}
}
