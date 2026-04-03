#include "DediGameInstance.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"

UDediGameInstance::UDediGameInstance()
{
	CurrentBGMSound = nullptr;
	BGMComponent = nullptr;
}

void UDediGameInstance::Init()
{
	Super::Init();
}

void UDediGameInstance::PlayBGM(USoundBase* Music, float FadeInDuration)
{
	if (!Music) return;

	// 같은 BGM이면 무시
	if (CurrentBGMSound == Music && IsBGMPlaying())
	{
		return;
	}

	// 기존 BGM 중지
	if (BGMComponent && BGMComponent->IsPlaying())
	{
		BGMComponent->FadeOut(FadeInDuration, 0.0f);
	}

	// 새 BGM 재생
	CurrentBGMSound = Music;
	BGMComponent = UGameplayStatics::SpawnSound2D(GetWorld(), Music, DefaultBGMVolume, 1.0f, 0.0f, nullptr, false, false);

	if (BGMComponent)
	{
		BGMComponent->FadeIn(FadeInDuration, DefaultBGMVolume);
	}
}

void UDediGameInstance::StopBGM(float FadeOutDuration)
{
	if (BGMComponent && BGMComponent->IsPlaying())
	{
		BGMComponent->FadeOut(FadeOutDuration, 0.0f);
	}
	CurrentBGMSound = nullptr;
}

void UDediGameInstance::SetBGMVolume(float Volume)
{
	DefaultBGMVolume = FMath::Clamp(Volume, 0.0f, 1.0f);

	if (BGMComponent)
	{
		BGMComponent->SetVolumeMultiplier(DefaultBGMVolume);
	}
}

bool UDediGameInstance::IsBGMPlaying() const
{
	return BGMComponent && BGMComponent->IsPlaying();
}
