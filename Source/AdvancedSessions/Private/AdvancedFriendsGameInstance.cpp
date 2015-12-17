// Fill out your copyright notice in the Description page of Project Settings.
#include "OnlineSubSystemHeader.h"
#include "AdvancedFriendsGameInstance.h"

//General Log
DEFINE_LOG_CATEGORY(AdvancedFriendsInterfaceLog);

UAdvancedFriendsGameInstance::UAdvancedFriendsGameInstance(const FObjectInitializer& ObjectInitializer) 
	: Super(ObjectInitializer)
	, SessionInviteAcceptedDelegate(FOnSessionUserInviteAcceptedDelegate::CreateUObject(this, &ThisClass::OnSessionInviteAcceptedMaster))
	, bCallFriendInterfaceEventsOnPlayerControllers(true)
	, PlayerTalkingStateChangedDelegate(FOnPlayerTalkingStateChangedDelegate::CreateUObject(this, &ThisClass::OnPlayerTalkingStateChangedMaster))
	, bEnableTalkingStatusDelegate(true)
	, bCallVoiceInterfaceEventsOnPlayerControllers(true)
{
}

void UAdvancedFriendsGameInstance::Shutdown()
{
	IOnlineSessionPtr SessionInterface = Online::GetSessionInterface();
	
	if (!SessionInterface.IsValid())
	{
		UE_LOG(AdvancedFriendsInterfaceLog, Warning, TEXT("UAdvancedFriendsGameInstance Failed to get session system!"));
		//return;
	}
	else
	{
		// Clear all of the delegate handles here
		SessionInterface->ClearOnSessionUserInviteAcceptedDelegate_Handle(SessionInviteAcceptedDelegateHandle);

	}


	if (bEnableTalkingStatusDelegate)
	{
		IOnlineVoicePtr VoiceInterface = Online::GetVoiceInterface();

		if (VoiceInterface.IsValid())
		{
			VoiceInterface->ClearOnPlayerTalkingStateChangedDelegate_Handle(PlayerTalkingStateChangedDelegateHandle);
		}
		else
		{

			UE_LOG(AdvancedFriendsInterfaceLog, Warning, TEXT("UAdvancedFriendsInstance Failed to get voice interface!"));
		}
	}

	Super::Shutdown();
}

void UAdvancedFriendsGameInstance::Init()
{
	IOnlineSessionPtr SessionInterface = Online::GetSessionInterface();//OnlineSub->GetSessionInterface();

	if (SessionInterface.IsValid())
	{
		// Currently doesn't store a handle or assign a delegate to any local player beyond the first.....should handle?
		// Thought about directly handling it but friends for multiple players probably isn't required
		// Iterating through the local player TArray only works if it has had players assigned to it, most of the online interfaces don't support
		// Multiple logins either (IE: Steam)
		SessionInviteAcceptedDelegateHandle = SessionInterface->AddOnSessionUserInviteAcceptedDelegate_Handle(SessionInviteAcceptedDelegate);

	}
	else
	{
		UE_LOG(AdvancedFriendsInterfaceLog, Warning, TEXT("UAdvancedFriendsInstance Failed to get session interface!"));
		//return;
	}

	// Beginning work on the voice interface
	if (bEnableTalkingStatusDelegate)
	{
		IOnlineVoicePtr VoiceInterface = Online::GetVoiceInterface();

		if (VoiceInterface.IsValid())
		{
			PlayerTalkingStateChangedDelegateHandle = VoiceInterface->AddOnPlayerTalkingStateChangedDelegate_Handle(PlayerTalkingStateChangedDelegate);
		}
		else
		{

			UE_LOG(AdvancedFriendsInterfaceLog, Warning, TEXT("UAdvancedFriendsInstance Failed to get voice interface!"));
		}
	}

	Super::Init();
}

/*void UAdvancedFriendsGameInstance::PostLoad()
{
	Super::PostLoad();
}*/


// Removed because it never gets called by the online subsystems
/*void UAdvancedFriendsGameInstance::OnSessionInviteReceivedMaster(const FUniqueNetId &InvitedPlayer, const FUniqueNetId &FriendInviting, const FOnlineSessionSearchResult& Session)
{
	// Just call the blueprint event to let the user handle this

	FBPUniqueNetId IP, FI;

	IP.SetUniqueNetId(&InvitedPlayer);

	FI.SetUniqueNetId(&FriendInviting);

	FBlueprintSessionResult BPS;
	BPS.OnlineResult = Session;
	OnSessionInviteReceived(IP,FI,BPS);

	TArray<class APlayerState*>& PlayerArray = GetWorld()->GetGameState()->PlayerArray;
	const TArray<class ULocalPlayer*>&ControllerArray = this->GetLocalPlayers();

	for (int i = 0; i < ControllerArray.Num(); i++)
	{
		if (*PlayerArray[ControllerArray[i]->PlayerController->NetPlayerIndex]->UniqueId.GetUniqueNetId().Get() == InvitedPlayer)
		{
			//Run the Event specific to the actor, if the actor has the interface, otherwise ignore
			if (ControllerArray[i]->PlayerController->GetClass()->ImplementsInterface(UAdvancedFriendsInterface::StaticClass()))
			{
				IAdvancedFriendsInterface::Execute_OnSessionInviteReceived(ControllerArray[i]->PlayerController, FI, BPS);
			}
			break;
		}
	}
}*/

void UAdvancedFriendsGameInstance::OnPlayerTalkingStateChangedMaster(TSharedRef<const FUniqueNetId> PlayerId, bool bIsTalking)
{
	FBPUniqueNetId PlayerTalking;
	PlayerTalking.SetUniqueNetId(PlayerId);
	OnPlayerTalkingStateChanged(PlayerTalking, bIsTalking);

	if (bCallVoiceInterfaceEventsOnPlayerControllers)
	{
		APlayerController* Player = NULL;

		for (const ULocalPlayer* LPlayer : LocalPlayers)
		{
			Player = UGameplayStatics::GetPlayerController(GetWorld(), LPlayer->GetControllerId());

			if (Player != NULL)
			{
				//Run the Event specific to the actor, if the actor has the interface, otherwise ignore
				if (Player->GetClass()->ImplementsInterface(UAdvancedFriendsInterface::StaticClass()))
				{
					IAdvancedFriendsInterface::Execute_OnPlayerVoiceStateChanged(Player, PlayerTalking, bIsTalking);
				}
			}
			else
			{
				UE_LOG(AdvancedFriendsInterfaceLog, Warning, TEXT("UAdvancedFriendsInstance Failed to get a controller with the specified index in OnVoiceStateChanged!"));
			}
		}
	}
}

void UAdvancedFriendsGameInstance::OnSessionInviteAcceptedMaster(const bool bWasSuccessful, int32 LocalPlayer, TSharedPtr<const FUniqueNetId> PersonInviting, const FOnlineSessionSearchResult& SessionToJoin)
{
	if (bWasSuccessful)
	{
		if (SessionToJoin.IsValid())
		{

			FBlueprintSessionResult BluePrintResult;
			BluePrintResult.OnlineResult = SessionToJoin;

			FBPUniqueNetId PInviting;
			PInviting.SetUniqueNetId(PersonInviting);

			OnSessionInviteAccepted(LocalPlayer,PInviting, BluePrintResult);

			APlayerController* Player = UGameplayStatics::GetPlayerController(GetWorld(), LocalPlayer);

			IAdvancedFriendsInterface* TheInterface = NULL;

			if (Player != NULL)
			{
				//Run the Event specific to the actor, if the actor has the interface, otherwise ignore
				if (Player->GetClass()->ImplementsInterface(UAdvancedFriendsInterface::StaticClass()))
				{
					IAdvancedFriendsInterface::Execute_OnSessionInviteAccepted(Player,PInviting, BluePrintResult);
				}
			}
			else
			{ 
				UE_LOG(AdvancedFriendsInterfaceLog, Warning, TEXT("UAdvancedFriendsInstance Failed to get a controller with the specified index in OnSessionInviteAccepted!"));
			}
		}
		else
		{
			UE_LOG(AdvancedFriendsInterfaceLog, Warning, TEXT("UAdvancedFriendsInstance Return a bad search result in OnSessionInviteAccepted!"));
		}
	}
}