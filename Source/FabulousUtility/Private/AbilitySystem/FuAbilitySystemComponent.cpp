#include "AbilitySystem/FuAbilitySystemComponent.h"

#include "AbilitySystemInterface.h"
#include "AbilitySystem/FuGameplayAbility.h"
#include "AbilitySystem/FuGameplayEffect.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FuAbilitySystemComponent)

bool UFuAbilitySystemComponent::TryGetFuAbilitySystem(const UObject* Object, ThisClass*& AbilitySystem, const bool bAllowFindComponent)
{
	const auto* Accessor{Cast<IAbilitySystemInterface>(Object)};
	if (Accessor != nullptr)
	{
		AbilitySystem = Cast<ThisClass>(Accessor->GetAbilitySystemComponent());
		if (IsValid(AbilitySystem))
		{
			return true;
		}
	}

	const auto* Actor{Cast<AActor>(Object)};
	if (bAllowFindComponent && IsValid(Actor))
	{
		AbilitySystem = Actor->FindComponentByClass<ThisClass>();
		return IsValid(AbilitySystem);
	}

	AbilitySystem = nullptr;
	return false;
}

void UFuAbilitySystemComponent::OnRegister()
{
	Super::OnRegister();

	// Subscribe to the event here after calling Super::OnRegister() so that UFuAbilitySystemComponent::OnAnyTagChanged()
	// is called before FActiveGameplayEffectsContainer::OnOwnerTagChange().

	OnGameplayEffectAppliedDelegateToSelf.AddUObject(this, &ThisClass::AbilitySystem_OnGameplayEffectApplied);

	RegisterGenericGameplayTagEvent().AddUObject(this, &ThisClass::AbilitySystem_OnAnyTagChanged);
}

FActiveGameplayEffectHandle UFuAbilitySystemComponent::ApplyGameplayEffectSpecToSelf(const FGameplayEffectSpec& EffectSpecification,
                                                                                     const FPredictionKey PredictionKey)
{
	// The code block below should be called after the call to FActiveGameplayEffectsContainer::HasApplicationImmunityToSpec()
	// in the UAbilitySystemComponent::ApplyGameplayEffectSpecToSelf() function, but currently it is not possible.

	const auto* Effect{Cast<UFuGameplayEffect>(EffectSpecification.Def)};

	if (IsValid(Effect) && Effect->GetRemovalRequirementAnyTag().HasAny(GameplayTagCountContainer.GetExplicitGameplayTags()))
	{
		return {};
	}

	return Super::ApplyGameplayEffectSpecToSelf(EffectSpecification, PredictionKey);
}

void UFuAbilitySystemComponent::NotifyAbilityActivated(const FGameplayAbilitySpecHandle AbilityHandle, UGameplayAbility* Ability)
{
	Super::NotifyAbilityActivated(AbilityHandle, Ability);

	OnAbilityActivated.Broadcast(AbilityHandle, Ability);
}

void UFuAbilitySystemComponent::NotifyAbilityFailed(const FGameplayAbilitySpecHandle AbilityHandle, UGameplayAbility* Ability,
                                                    const FGameplayTagContainer& FailureTags)
{
	Super::NotifyAbilityFailed(AbilityHandle, Ability, FailureTags);

	OnAbilityFailed.Broadcast(AbilityHandle, Ability, FailureTags);
}

bool UFuAbilitySystemComponent::ShouldDoServerAbilityRPCBatch() const
{
	return true;
}

void UFuAbilitySystemComponent::OnGiveAbility(FGameplayAbilitySpec& AbilitySpecification)
{
	Super::OnGiveAbility(AbilitySpecification);

	OnAbilityGiven.Broadcast(AbilitySpecification);
}

void UFuAbilitySystemComponent::OnRemoveAbility(FGameplayAbilitySpec& AbilitySpecification)
{
	Super::OnRemoveAbility(AbilitySpecification);

	OnAbilityRemoved.Broadcast(AbilitySpecification);
}

bool UFuAbilitySystemComponent::AreAbilityTagsBlocked(const FGameplayTagContainer& Tags) const
{
	return Super::AreAbilityTagsBlocked(Tags) ||
	       // ReSharper disable once CppRedundantParentheses
	       (BlockedAbilityWithoutTags.GetExplicitGameplayTags().IsValid() &&
	        !Tags.HasAny(BlockedAbilityWithoutTags.GetExplicitGameplayTags()));
}

void UFuAbilitySystemComponent::AbilityLocalInputPressed(const int32 InputId)
{
	// Based on UAbilitySystemComponent::AbilityLocalInputPressed().

	if (IsGenericConfirmInputBound(InputId))
	{
		LocalInputConfirm();
		return;
	}

	if (IsGenericCancelInputBound(InputId))
	{
		LocalInputCancel();
		return;
	}

	ABILITYLIST_SCOPE_LOCK();

	for (auto& AbilitySpecification : GetActivatableAbilities())
	{
		if (AbilitySpecification.InputID != InputId || !IsValid(AbilitySpecification.Ability))
		{
			continue;
		}

		AbilitySpecification.InputPressed = true;

		if (AbilitySpecification.IsActive())
		{
			if (AbilitySpecification.Ability->bReplicateInputDirectly && !IsOwnerActorAuthoritative())
			{
				ServerSetInputPressed(AbilitySpecification.Handle);
			}

			AbilitySpecInputPressed(AbilitySpecification);

			InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputPressed, AbilitySpecification.Handle,
			                      AbilitySpecification.ActivationInfo.GetActivationPredictionKey());
			continue;
		}

		const auto* FuAbility{Cast<UFuGameplayAbility>(AbilitySpecification.Ability)};

		if (!IsValid(FuAbility) || FuAbility->IsActivationByInputAllowed())
		{
			TryActivateAbility(AbilitySpecification.Handle);
		}
	}
}

void UFuAbilitySystemComponent::InputTagPressed(const FGameplayTag& InputTag)
{
	// Based on UAbilitySystemComponent::AbilityLocalInputPressed().

	if (!InputTag.IsValid())
	{
		return;
	}

	if (ConfirmInputTag == InputTag)
	{
		LocalInputConfirm();
		return;
	}

	if (CancelInputTag == InputTag)
	{
		LocalInputCancel();
		return;
	}

	ABILITYLIST_SCOPE_LOCK();

	for (auto& AbilitySpecification : GetActivatableAbilities())
	{
		if (!IsValid(AbilitySpecification.Ability) ||
		    // ReSharper disable once CppRedundantParentheses
		    (!AbilitySpecification.DynamicAbilityTags.HasTag(InputTag) &&
		     !AbilitySpecification.Ability->AbilityTags.HasTag(InputTag)))
		{
			continue;
		}

		AbilitySpecification.InputPressed = true;

		if (AbilitySpecification.IsActive())
		{
			if (AbilitySpecification.Ability->bReplicateInputDirectly && !IsOwnerActorAuthoritative())
			{
				ServerSetInputPressed(AbilitySpecification.Handle);
			}

			AbilitySpecInputPressed(AbilitySpecification);

			InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputPressed, AbilitySpecification.Handle,
			                      AbilitySpecification.ActivationInfo.GetActivationPredictionKey());
			continue;
		}

		const auto* FuAbility{Cast<UFuGameplayAbility>(AbilitySpecification.Ability)};

		if (!IsValid(FuAbility) || FuAbility->IsActivationByInputAllowed())
		{
			TryActivateAbility(AbilitySpecification.Handle);
		}
	}
}

void UFuAbilitySystemComponent::InputTagReleased(const FGameplayTag& InputTag)
{
	// Based on UAbilitySystemComponent::AbilityLocalInputReleased().

	if (!InputTag.IsValid())
	{
		return;
	}

	ABILITYLIST_SCOPE_LOCK();

	for (auto& AbilitySpecification : GetActivatableAbilities())
	{
		if (!IsValid(AbilitySpecification.Ability) ||
		    // ReSharper disable once CppRedundantParentheses
		    (!AbilitySpecification.DynamicAbilityTags.HasTag(InputTag) &&
		     !AbilitySpecification.Ability->AbilityTags.HasTag(InputTag)))
		{
			continue;
		}

		AbilitySpecification.InputPressed = false;

		if (AbilitySpecification.IsActive())
		{
			if (AbilitySpecification.Ability->bReplicateInputDirectly && !IsOwnerActorAuthoritative())
			{
				ServerSetInputReleased(AbilitySpecification.Handle);
			}

			AbilitySpecInputReleased(AbilitySpecification);

			InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputReleased, AbilitySpecification.Handle,
			                      AbilitySpecification.ActivationInfo.GetActivationPredictionKey());
		}
	}
}

void UFuAbilitySystemComponent::BlockAbilitiesWithoutTags(const FGameplayTagContainer& Tags)
{
	BlockedAbilityWithoutTags.UpdateTagCount(Tags, 1);
}

void UFuAbilitySystemComponent::UnBlockAbilitiesWithoutTags(const FGameplayTagContainer& Tags)
{
	BlockedAbilityWithoutTags.UpdateTagCount(Tags, -1);
}

void UFuAbilitySystemComponent::AbilitySystem_OnGameplayEffectApplied(UAbilitySystemComponent* InstigatorAbilitySystem,
                                                                      const FGameplayEffectSpec& EffectSpecification,
                                                                      const FActiveGameplayEffectHandle EffectHandle)
{
	// The code block below should be called right after the call to
	// FActiveGameplayEffectsContainer::AttemptRemoveActiveEffectsOnEffectApplication() in
	// the UAbilitySystemComponent::ApplyGameplayEffectSpecToSelf() function, but currently it is not possible.

	if (IsOwnerActorAuthoritative())
	{
		const auto* Tags{EffectSpecification.CapturedTargetTags.GetAggregatedTags()};
		FGameplayEffectQuery EffectQuery;

		EffectQuery.CustomMatchDelegate.BindLambda([Tags](const FActiveGameplayEffect& ActiveEffect)
		{
			const auto* Effect{Cast<UFuGameplayEffect>(ActiveEffect.Spec.Def)};

			return IsValid(Effect) && Effect->GetRemovalRequirementAnyTag().HasAny(*Tags);
		});

		ActiveGameplayEffects.RemoveActiveEffects(EffectQuery, -1);
	}
}

void UFuAbilitySystemComponent::AbilitySystem_OnAnyTagChanged(const FGameplayTag Tag, const int32 Count)
{
	// Unfortunately, there is currently no way to optimize this the way it
	// is done inside FActiveGameplayEffectsContainer::OnOwnerTagChange().

	if (Count > 0)
	{
		const auto& Tags{GameplayTagCountContainer.GetExplicitGameplayTags()};
		FGameplayEffectQuery EffectQuery;

		EffectQuery.CustomMatchDelegate.BindLambda([&Tags](const FActiveGameplayEffect& ActiveEffect)
		{
			const auto* Effect{Cast<UFuGameplayEffect>(ActiveEffect.Spec.Def)};

			return IsValid(Effect) && Effect->GetRemovalRequirementAnyTag().HasAny(Tags);
		});

		ActiveGameplayEffects.RemoveActiveEffects(EffectQuery, -1);
	}
}
