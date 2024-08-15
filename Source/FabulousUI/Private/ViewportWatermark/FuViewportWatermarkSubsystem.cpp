#include "ViewportWatermark/FuViewportWatermarkSubsystem.h"

#if !UE_SERVER

#include "Engine/GameViewportClient.h"
#include "ViewportWatermark/FuViewportWatermarkSettings.h"
#include "ViewportWatermark/SFuViewportWatermark.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FuViewportWatermarkSubsystem)

bool UFuViewportWatermarkSubsystem::ShouldCreateSubsystem(UObject* OuterObject) const
{
	return !CastChecked<UGameInstance>(OuterObject)->IsDedicatedServerInstance() && Super::ShouldCreateSubsystem(OuterObject);
}

void UFuViewportWatermarkSubsystem::Initialize(FSubsystemCollectionBase& SubsystemCollection)
{
	Super::Initialize(SubsystemCollection);

	if (!GetDefault<UFuViewportWatermarkSettings>()->bEnabled)
	{
		return;
	}

#ifdef BUNKHOUSE_GAMES
	FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &ThisClass::OnPostLoadMapWithWorld);
#endif // BUNKHOUSE_GAMES

	if (IsValid(GetGameInstance()->GetGameViewportClient()))
	{
		GameViewport_OnViewportCreated();
	}
	else
	{
		UGameViewportClient::OnViewportCreated().AddUObject(this, &ThisClass::GameViewport_OnViewportCreated);
	}
}

void UFuViewportWatermarkSubsystem::Deinitialize()
{
	UGameViewportClient::OnViewportCreated().RemoveAll(this);

	Super::Deinitialize();
}

void UFuViewportWatermarkSubsystem::GameViewport_OnViewportCreated()
{
	auto* Viewport{GetGameInstance()->GetGameViewportClient()};
	if (IsValid(Viewport))
	{
		UGameViewportClient::OnViewportCreated().RemoveAll(this);

#ifdef BUNKHOUSE_GAMES		
		if (ViewportWatermark.IsValid())
		{
			Viewport->RemoveViewportWidgetContent(ViewportWatermark.ToSharedRef());
			ViewportWatermark.Reset();
		}
		ViewportWatermark = SNew(SFuViewportWatermark);
		Viewport->AddViewportWidgetContent(ViewportWatermark.ToSharedRef(), GetDefault<UFuViewportWatermarkSettings>()->ZOrder);
#else // BUNKHOUSE_GAMES
		Viewport->AddViewportWidgetContent(SNew(SFuViewportWatermark), GetDefault<UFuViewportWatermarkSettings>()->ZOrder);
#endif // BUNKHOUSE_GAMES
	}
}

#ifdef BUNKHOUSE_GAMES
void UFuViewportWatermarkSubsystem::OnPostLoadMapWithWorld(UWorld* World)
{
	if (IsValid(World))
	{
		GameViewport_OnViewportCreated();
		
	}
}
#endif // BUNKHOUSE_GAMES

#endif
