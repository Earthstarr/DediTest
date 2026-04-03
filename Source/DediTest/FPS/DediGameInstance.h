#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "DediGameInstance.generated.h"

// 전방 선언
enum class EStratagemType : uint8;
class USoundMix;
class USoundClass;
class UInputMappingContext;
class UInputAction;

UCLASS()
class DEDITEST_API UDediGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	UDediGameInstance();

	virtual void Init() override;

	// ===== BGM 시스템 =====

	UFUNCTION(BlueprintCallable, Category = "Audio")
	void PlayBGM(USoundBase* Music, float FadeInDuration = 1.0f);

	UFUNCTION(BlueprintCallable, Category = "Audio")
	void StopBGM(float FadeOutDuration = 1.0f);

	UFUNCTION(BlueprintCallable, Category = "Audio")
	void SetBGMVolume(float Volume);

	UFUNCTION(BlueprintPure, Category = "Audio")
	USoundBase* GetCurrentBGM() const { return CurrentBGMSound; }

	UFUNCTION(BlueprintPure, Category = "Audio")
	bool IsBGMPlaying() const;

protected:
	// === BGM ===
	UPROPERTY()
	class UAudioComponent* BGMComponent;

	UPROPERTY()
	USoundBase* CurrentBGMSound;

	UPROPERTY(EditDefaultsOnly, Category = "Audio")
	float DefaultBGMVolume = 0.5f;
};
