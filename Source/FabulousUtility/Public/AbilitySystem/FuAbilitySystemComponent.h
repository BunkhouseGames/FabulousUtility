#pragma once

#include "AbilitySystemComponent.h"
#include "FuAbilitySystemComponent.generated.h"

using FFuAbilityAddedOrRemovedDelegate = TMulticastDelegate<void(const FGameplayAbilitySpec& AbilitySpecification)>;

using FFuAbilityActivatedDelegate = TMulticastDelegate<void(FGameplayAbilitySpecHandle AbilityHandle, UGameplayAbility* Ability)>;

using FFuAbilityFailedDelegate = TMulticastDelegate<void(FGameplayAbilitySpecHandle AbilityHandle, UGameplayAbility* Ability,
                                                         const FGameplayTagContainer& FailureTags)>;

UCLASS()
class FABULOUSUTILITY_API UFuAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	FGameplayTag ConfirmInputTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	FGameplayTag CancelInputTag;

protected:
	FGameplayTagCountContainer BlockedAbilityWithoutTags;

public:
	FFuAbilityAddedOrRemovedDelegate OnAbilityGiven;

	FFuAbilityAddedOrRemovedDelegate OnAbilityRemoved;

	FFuAbilityActivatedDelegate OnAbilityActivated;

	FFuAbilityFailedDelegate OnAbilityFailed;

public:
	UFUNCTION(BlueprintCallable, Category = "Fabulous Utility|Fu Ability System", Meta = (ExpandBoolAsExecs = "ReturnValue"))
	static bool TryGetFuAbilitySystem(const UObject* Object, UFuAbilitySystemComponent*& AbilitySystem, bool bAllowFindComponent = true);

protected:
	virtual void OnRegister() override;

public:
	virtual FActiveGameplayEffectHandle ApplyGameplayEffectSpecToSelf(const FGameplayEffectSpec& EffectSpecification,
	                                                                  FPredictionKey PredictionKey) override;

	virtual void NotifyAbilityActivated(FGameplayAbilitySpecHandle AbilityHandle, UGameplayAbility* Ability) override;

	virtual void NotifyAbilityFailed(FGameplayAbilitySpecHandle AbilityHandle, UGameplayAbility* Ability,
	                                 const FGameplayTagContainer& FailureTags) override;

protected:
	virtual bool ShouldDoServerAbilityRPCBatch() const override;

	virtual void OnGiveAbility(FGameplayAbilitySpec& AbilitySpecification) override;

	virtual void OnRemoveAbility(FGameplayAbilitySpec& AbilitySpecification) override;

	virtual bool AreAbilityTagsBlocked(const FGameplayTagContainer& Tags) const override;

	virtual void AbilityLocalInputPressed(int32 InputId) override;

public:
	const FActiveGameplayEffectsContainer& GetActiveEffects() const;

	FActiveGameplayEffectsContainer& GetActiveEffects();

	void InputTagPressed(const FGameplayTag& InputTag);

	void InputTagReleased(const FGameplayTag& InputTag);

	void BlockAbilitiesWithoutTags(const FGameplayTagContainer& Tags);

	void UnBlockAbilitiesWithoutTags(const FGameplayTagContainer& Tags);

private:
	void AbilitySystem_OnGameplayEffectApplied(UAbilitySystemComponent* InstigatorAbilitySystem,
	                                           const FGameplayEffectSpec& EffectSpecification, FActiveGameplayEffectHandle EffectHandle);

	void AbilitySystem_OnAnyTagChanged(FGameplayTag Tag, int32 Count);
};

inline const FActiveGameplayEffectsContainer& UFuAbilitySystemComponent::GetActiveEffects() const
{
	return ActiveGameplayEffects;
}

inline FActiveGameplayEffectsContainer& UFuAbilitySystemComponent::GetActiveEffects()
{
	return ActiveGameplayEffects;
}
