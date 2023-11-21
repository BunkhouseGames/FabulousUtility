#pragma once

#include "FindSessionsCallbackProxy.h"
#include "FuAsyncAction_CreateSession.generated.h"

UCLASS(DisplayName = "Fu Create Session Async Action")
class FABULOUSONLINE_API UFuAsyncAction_CreateSession : public UOnlineBlueprintCallProxyBase
{
	GENERATED_BODY()

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", Transient)
	TWeakObjectPtr<APlayerController> Player;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", Transient)
	int32 PublicConnections;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", Transient)
	uint8 bLanOnly : 1;

public:
	UPROPERTY(BlueprintAssignable, Category = "Fabulous Utility|Create Session Async Action")
	FEmptyOnlineDelegate OnSuccess;

	UPROPERTY(BlueprintAssignable, Category = "Fabulous Utility|Create Session Async Action")
	FEmptyOnlineDelegate OnFailure;

public:
	UFUNCTION(BlueprintCallable, Category = "Fabulous Utility|Session", BlueprintInternalUseOnly, Meta = (DefaultToSelf = "InPlayer"))
	static UFuAsyncAction_CreateSession* CreateSession(UPARAM(DisplayName = "Player") APlayerController* InPlayer,
	                                                   UPARAM(DisplayName = "Public Connections") int32 InPublicConnections,
	                                                   UPARAM(DisplayName = "Lan Only") bool bInLanOnly);

	virtual void Activate() override;

private:
	void Session_OnCreated(FName SessionName, bool bSuccess);

	void Session_OnStarted(FName SessionName, bool bSuccess);
};
