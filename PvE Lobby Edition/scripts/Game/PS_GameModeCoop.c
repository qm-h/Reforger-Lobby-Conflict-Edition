// Coop game mode
// Open lobby on game start
// Disable respawn logic

class PS_GameModeCoopClass : SCR_GameModeCampaignClass
{
}

class PS_GameModeCoop : SCR_GameModeCampaign
{
	[Attribute("120000", UIWidgets.EditBox, "Time during which disconnected players reserve role for reconnection in ms, -1 for infinity time", "", category: "Reforger Lobby")]
	int m_iReconnectTime;

	[Attribute("-1", UIWidgets.EditBox, "Time during which disconnected players reserve role for reconnection in ms, -1 for infinity time", "", category: "Reforger Lobby")]
	int m_iReconnectTimeAfterBriefing;

	[Attribute("1", uiwidget: UIWidgets.CheckBox, "Game may be started only if admin on server.", category: "Reforger Lobby")]
	protected bool m_bAdminMode;

	[Attribute("0", uiwidget: UIWidgets.CheckBox, "Anyone can open lobby in game stage.", category: "Reforger Lobby")]
	protected bool m_bTeamSwitch;

	//[Attribute("0", uiwidget: UIWidgets.CheckBox, "Faction locked after selection.", category: "Reforger Lobby")]
	protected bool m_bFactionLock;

	[Attribute("0", uiwidget: UIWidgets.CheckBox, "Markers can be placed only by squad leaders and only on briefing.", category: "Reforger Lobby")]
	protected bool m_bMarkersOnlyOnBriefing;

	[Attribute("0", UIWidgets.CheckBox, "Instead of just the leaders, every member is moved to their factions HQ room for a common briefing.\nMoving back to group channel is still possible.", category: "Reforger Lobby")]
	bool m_bPublicCommandBriefing;

	[Attribute("0", uiwidget: UIWidgets.CheckBox, "Remove units not occupied by players.", category: "Reforger Lobby")]
	protected bool m_bRemoveRedundantUnits;

	[Attribute("0", uiwidget: UIWidgets.CheckBox, "Remove default markers on squad leaders.", category: "Reforger Lobby")]
	protected bool m_bRemoveSquadMarkers;

	[Attribute("60000", UIWidgets.EditBox, "Time in milliseconds before restriction zones are removed.", category: "Reforger Lobby")]
	int m_iFreezeTime;
	
	[Attribute("0", UIWidgets.EditBox, "Time in milliseconds before characters are activated.", category: "Reforger Lobby (WIP)")]
	int m_iDisableTime;

	[Attribute("0", UIWidgets.CheckBox, "Disables text chat for alive players on game stage. Admins can always see text chat.", category: "Reforger Lobby")]
	protected bool m_bDisableChat;

	[RplProp()]
	protected float m_fCurrentFreezeTime = 1;
	[RplProp()]
	protected float m_fGameStartTime = 0;
	[RplProp()]
	protected float m_fGameStartElapsedTime = 0;

	[Attribute("0", UIWidgets.CheckBox, "Creates a whitelist on the server for players who have taken roles and also for players specified in $profile:PS_SlotsReserver_Config.json and kicks everyone else.", category: "Reforger Lobby")]
	protected bool m_bReserveSlots;

	[Attribute("", UIWidgets.Auto, "", category: "Reforger Lobby")]
	protected ref array<ref PS_FactionRespawnCount> m_aFactionRespawnCount;
	protected ref map<FactionKey, PS_FactionRespawnCount> m_mFactionRespawnCount = new map<FactionKey, PS_FactionRespawnCount>();

	[Attribute("0", UIWidgets.CheckBox, "", category: "Reforger Lobby")]
	protected bool m_bDisableVanillaGroupMenu;

	[Attribute("0", UIWidgets.CheckBox, "", category: "Reforger Lobby")]
	protected bool m_bDisablePlayablesStreaming;
	
	[Attribute("0", UIWidgets.CheckBox, "", category: "Reforger Lobby")]
	protected bool m_bDisableGarbageSystem;

	[Attribute("0", UIWidgets.CheckBox, "", category: "Reforger Lobby")]
	protected bool m_bFriendliesSpectatorOnly;

	[Attribute("0", UIWidgets.CheckBox, "", category: "Reforger Lobby")]
	protected bool m_bFreezeTimeShootingForbiden;
	
	[Attribute("1", UIWidgets.CheckBox, "", category: "Reforger Lobby")]
	protected bool m_bDisableArmaVision;
	
	[Attribute("0", UIWidgets.CheckBox, "", category: "Reforger Lobby")]
	protected bool m_bDisableBuildingModeAfterFreezeTime;
	
	[Attribute("-1", UIWidgets.Auto, "", category: "Reforger Lobby (WIP)")]
	protected int m_iFactionsBalance;

	[Attribute("0", UIWidgets.CheckBox, "", category: "Reforger Lobby (WIP)")]
	protected bool m_bShowCutscene;

	[Attribute("1", UIWidgets.CheckBox, "", category: "Reforger Lobby (WIP)")]
	protected bool m_bHolsterWeapon;

	[Attribute("0", UIWidgets.Auto, "", category: "Reforger Lobby (WIP)")]
	protected int m_iForceMenuFramerate;
	protected static int m_iOldMenuFramerate;

	protected ref ScriptInvokerInt m_OnGameStateChange = new ScriptInvokerInt();
	ScriptInvokerInt GetOnGameStateChange()
	{
		return m_OnGameStateChange;
	}

	protected ref ScriptInvokerString m_OnOnlyOneFactionAlive = new ScriptInvokerString();
	ScriptInvokerString GetOnOnlyOneFactionAlive()
	{
		return m_OnOnlyOneFactionAlive;
	}

	protected ref ScriptInvoker m_OnHandlePlayerKilled = new ScriptInvoker();
	ScriptInvoker GetOnHandlePlayerKilled()
	{
		return m_OnHandlePlayerKilled;
	}

	// Cache global
	protected PS_PlayableManager m_playableManager;
	protected PS_CutsceneManager m_CutsceneManager;

	// ------------------------------------------ Events ------------------------------------------
	
	override void EOnInit(IEntity owner)
	{
        super.EOnInit(owner);
        
        if (!GetGame().InPlayMode() || !Replication.IsServer()){
            return;
        }
        World world = GetGame().GetWorld();
        world.FindSystem(SCR_GarbageSystem).Enable(!m_bDisableGarbageSystem);
    }
	
	override void OnGameStart()
	{
		super.OnGameStart();

		InputManager inputManager = GetGame().GetInputManager();
		if (inputManager && m_bDisableVanillaGroupMenu)
		{
			inputManager.RemoveActionListener("ShowScoreboard", EActionTrigger.DOWN, ArmaReforgerScripted.OnShowPlayerList);
			inputManager.RemoveActionListener("ShowGroupMenu", EActionTrigger.DOWN, ArmaReforgerScripted.OnShowGroupMenu);
		}

		Widget FreezeTimeCounterOverlay = GetGame().GetWorkspace().FindAnyWidget("FreezeTimeCounterOverlay");
		if (FreezeTimeCounterOverlay)
			FreezeTimeCounterOverlay.RemoveFromHierarchy();

		m_playableManager = PS_PlayableManager.GetInstance();
		m_CutsceneManager = PS_CutsceneManager.GetInstance();

		foreach (PS_FactionRespawnCount factionRespawnCount : m_aFactionRespawnCount)
		{
			m_mFactionRespawnCount.Insert(
				factionRespawnCount.m_sFactionKey,
				factionRespawnCount
			);
		}
		/*
		string loadSave = GameSessionStorage.s_Data.Get("SCR_SaveFileManager_FileNameToLoad");
		if (loadSave != "")
		{
			SCR_SaveManagerCore saveManager = GetGame().GetSaveManager();
			saveManager.Load(loadSave);
		}
		*/
		if (Replication.IsServer())
		{
			PS_VoNRoomsManager.GetInstance().GetOrCreateRoomWithFaction("", "#PS-VoNRoom_Global");

			GenerateDeploymentGroups();

			m_fCurrentFreezeTime = m_iReconnectTime;
			Replication.BumpMe();
		}

		if (RplSession.Mode() != RplMode.Dedicated) {
			GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.WaitScreen);
			GetGame().GetInputManager().AddActionListener("OpenLobby", EActionTrigger.DOWN, Action_OpenLobby);
		}

		GetGame().GetCallqueue().CallLater(AddAdvanceAction, 0, false);

		GetGame().GetCallqueue().CallLater(RegisterEditorClosed, 100, false);

		if (!Replication.IsServer() && m_iForceMenuFramerate != 0)
		{
			BaseContainer video = GetGame().GetEngineUserSettings().GetModule("VideoUserSettings");
			video.Get("MaxFps", m_iOldMenuFramerate);
			video.Set("MaxFps", m_iForceMenuFramerate);
			GetGame().GetCallqueue().CallLater(ForceFramerate, 1000, true);
		}
	}
	void ForceFramerate()
	{
		BaseContainer video = GetGame().GetEngineUserSettings().GetModule("VideoUserSettings");
		if (PS_GameModeCoop.Cast(GetGame().GetGameMode()).GetState() == SCR_EGameModeState.GAME)
		{
			video.Set("MaxFps", m_iOldMenuFramerate);
			GetGame().UserSettingsChanged();

			GetGame().GetCallqueue().Remove(ForceFramerate);
		}
		else
		{
			int currentFramerate;
			video.Get("MaxFps", currentFramerate);
			if (currentFramerate != m_iForceMenuFramerate)
			{
				video.Set("MaxFps", m_iForceMenuFramerate);
				GetGame().UserSettingsChanged();
			}
		}
	}

	void RegisterEditorClosed()
	{
		SCR_EditorModeEntity editorModeEntity = SCR_EditorModeEntity.GetInstance();
		if (editorModeEntity)
		{
			editorModeEntity.GetOnClosed().Insert(EditorClosed);
		}
		else
			GetGame().GetCallqueue().CallLater(RegisterEditorClosed, 100, false);
	}
	
	void FreezeTimerAdvance(int time)
	{
		time = time * 1000;
		m_iFreezeTime += time;
		m_fCurrentFreezeTime += time;
		GetGame().GetCallqueue().Remove(restrictedZonesTimer);
		restrictedZonesTimer(m_fCurrentFreezeTime);
		
		FreezeTimerAdvance_Notify();
	}
	
	void FreezeTimerEnd()
	{
		GetGame().GetCallqueue().Remove(restrictedZonesTimer);
		restrictedZonesTimer(5000);
		
		FreezeTimerEnd_Notify();
	}

	void EditorClosed()
	{
		PlayerController playerController = GetGame().GetPlayerController();
		PS_PlayableControllerComponent playableController = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));

		playableController.SaveCameraTransform();
		playableController.SwitchFromObserver();

		// Persistent PvE: a player with no playable (e.g. a Game Master who deleted their own character)
		// must drop into the deployment lobby on editor close — not re-deploy a now-deleted body. Route
		// through the server so the lobby/initial camera entity is (re)created authoritatively and the
		// CoopLobby opens cleanly.
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		if (playableManager && playableManager.GetPlayableByPlayer(playerController.GetPlayerId()) == RplId.Invalid())
		{
			playableController.PS_RequestReturnToLobby();
			return;
		}

		IEntity entity = playerController.GetControlledEntity();
		if (!entity)
		{
			playableController.SwitchToMenu(SCR_EGameModeState.GAME);
			return;
		}
		
		PS_LobbyVoNComponent von = PS_LobbyVoNComponent.Cast(entity.FindComponent(PS_LobbyVoNComponent));
		if (von)
		{
			playableController.SwitchToMenu(SCR_EGameModeState.GAME);
			return;
		}
	}
	
	void AddAdvanceAction()
	{
		SCR_ChatPanelManager chatPanelManager = SCR_ChatPanelManager.GetInstance();
		ChatCommandInvoker invoker = chatPanelManager.GetCommandInvoker("adv");
		invoker.Insert(AdvanceStage_Callback);
		invoker = chatPanelManager.GetCommandInvoker("lom");
		invoker.Insert(LoadMap_Callback);
		invoker = chatPanelManager.GetCommandInvoker("sav");
		invoker.Insert(ExportMissionData_Callback);
		invoker = chatPanelManager.GetCommandInvoker("tst");
		invoker.Insert(Test_Callback);
		invoker = chatPanelManager.GetCommandInvoker("pgc");
		invoker.Insert(PlayGameConfig_Callback);
		invoker = chatPanelManager.GetCommandInvoker("res");
		invoker.Insert(Respawn_Callback);
		invoker = chatPanelManager.GetCommandInvoker("rei");
		invoker.Insert(RespawnInit_Callback);
		invoker = chatPanelManager.GetCommandInvoker("unc");
		invoker.Insert(ForceUnconsious_Callback);
		invoker = chatPanelManager.GetCommandInvoker("spw");
		invoker.Insert(SpawnInit_Callback);
		invoker = chatPanelManager.GetCommandInvoker("spp");
		invoker.Insert(SpawnPosition_Callback);
		invoker = chatPanelManager.GetCommandInvoker("fta");
		invoker.Insert(FreezeTimerAdvance_Callback);
		invoker = chatPanelManager.GetCommandInvoker("fte");
		invoker.Insert(FreezeTimerEnd_Callback);
		invoker = chatPanelManager.GetCommandInvoker("cmc");
		invoker.Insert(CopyAllMarkersToClipboard_Callback);
		invoker = chatPanelManager.GetCommandInvoker("lmc");
		invoker.Insert(LoadAllMarkersToClipboard_Callback);
	}
	
	
	void CopyAllMarkersToClipboard_Callback(SCR_ChatPanel panel, string data)
	{
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		PlayerController playerController = GetGame().GetPlayerController();
		if (!playableManager.IsPlayerGroupLeader(playerController.GetPlayerId())) return;
		
		SCR_MapMarkerManagerComponent markerMgr = SCR_MapMarkerManagerComponent.GetInstance();
		array<SCR_MapMarkerBase> markers = markerMgr.GetStaticMarkers();
		
		PS_MapMarkersBaseJson mapMarkers = new PS_MapMarkersBaseJson();
		foreach (SCR_MapMarkerBase marker : markers)
		{
			PS_MapMarkerBaseJson markerJson = marker.PS_GetMapMarkerBaseJson();
			mapMarkers.m_aMapMarkers.Insert(markerJson);
		}
		SCR_JsonSaveContext saveContext = new SCR_JsonSaveContext();
		saveContext.WriteValue("", mapMarkers);
		System.ExportToClipboard(saveContext.ExportToString());
	}
	
	
	void LoadAllMarkersToClipboard_Callback(SCR_ChatPanel panel, string data)
	{
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		PlayerController playerController = GetGame().GetPlayerController();
		if (!playableManager.IsPlayerGroupLeader(playerController.GetPlayerId())) return;
		
		if (GetState() != SCR_EGameModeState.BRIEFING)
			return;
		
		string json = System.ImportFromClipboard();
		
		SCR_JsonLoadContext loadContext = new SCR_JsonLoadContext();
		loadContext.ImportFromString(json);
		
		PS_MapMarkersBaseJson mapMarkers = new PS_MapMarkersBaseJson();
		loadContext.ReadValue("", mapMarkers);
		
		SCR_MapMarkerManagerComponent markerMgr = SCR_MapMarkerManagerComponent.GetInstance();
		foreach (PS_MapMarkerBaseJson markerJson : mapMarkers.m_aMapMarkers)
		{
			SCR_MapMarkerBase marker = markerJson.GetMapMarkerBase();
			marker.SetMarkerFactionFlags(0);
			FactionManager factionManager = GetGame().GetFactionManager();
			if (factionManager)
			{
				Faction markerOwnerFaction = SCR_FactionManager.SGetPlayerFaction(GetGame().GetPlayerController().GetPlayerId());
				if (markerOwnerFaction)
					marker.AddMarkerFactionFlags(factionManager.GetFactionIndex(markerOwnerFaction));
			}
			
			markerMgr.InsertStaticMarker(marker, false, false);
		}
	}
	
	void FreezeTimerAdvance_Callback(SCR_ChatPanel panel, string data)
	{
		if (!PS_PlayersHelper.IsAdminOrServer())
			return;
		
		PlayerController playerController = GetGame().GetPlayerController();
		PS_PlayableControllerComponent playableController = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));
		if (!playableController)
			return;
		
		if (GetState() != SCR_EGameModeState.GAME || IsFreezeTimeEnd())
			return;
		
		playableController.FreezeTimerAdvance(data.ToInt());
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void FreezeTimerAdvance_Notify()
	{
		SCR_ChatPanelManager chatPanelManager = SCR_ChatPanelManager.GetInstance();
		ChatCommandInvoker invoker = chatPanelManager.GetCommandInvoker("smsg");
		invoker.Invoke(null, "#PS-Freeze_time_advanced");
	}
	
	void FreezeTimerEnd_Callback(SCR_ChatPanel panel, string data)
	{
		if (!PS_PlayersHelper.IsAdminOrServer())
			return;
		
		PlayerController playerController = GetGame().GetPlayerController();
		PS_PlayableControllerComponent playableController = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));
		if (!playableController)
			return;
		
		if (GetState() != SCR_EGameModeState.GAME || IsFreezeTimeEnd())
			return;
		
		playableController.FreezeTimerEnd();
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void FreezeTimerEnd_Notify()
	{
		SCR_ChatPanelManager chatPanelManager = SCR_ChatPanelManager.GetInstance();
		ChatCommandInvoker invoker = chatPanelManager.GetCommandInvoker("smsg");
		invoker.Invoke(null, "#PS-Freeze_time_force_end");
	}

	void SpawnPosition_Callback(SCR_ChatPanel panel, string data)
	{
		PlayerController playerController = GetGame().GetPlayerController();
		PS_PlayableControllerComponent playableController = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));
		if (!playableController)
			return;
		
		array<string> outTokens = {};
		data.Split(" ", outTokens, true);
		string positionStr = outTokens[0];
		positionStr.Replace("|", " ");
		positionStr.Replace("<", " ");
		positionStr.Replace(">", " ");
		positionStr.Replace(",", " ");
		vector position = positionStr.ToVector();
		
		playableController.SpawnPrefab(data, position);
	}
	
	void SpawnInit_Callback(SCR_ChatPanel panel, string data)
	{
		PlayerController playerController = GetGame().GetPlayerController();
		PS_PlayableControllerComponent playableController = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));
		if (!playableController)
			return;

		playableController.SpawnPrefab(data, "0 0 0");
	}

	void ForceUnconsious_Callback(SCR_ChatPanel panel, string data)
	{
		SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(SCR_PlayerController.GetLocalControlledEntity());
		if (!character)
			return;
		CharacterControllerComponent characterControllerComponent = character.GetCharacterController();
		if (characterControllerComponent.IsUnconscious())
			return;
		characterControllerComponent.SetUnconscious(true);
		GetGame().GetCallqueue().CallLater(ResetUnconsious, 400, false, characterControllerComponent);
	}
	void ResetUnconsious(CharacterControllerComponent characterControllerComponent)
	{
		characterControllerComponent.SetUnconscious(false);
	}

	void Respawn_Callback(SCR_ChatPanel panel, string data)
	{
		PlayerController playerController = GetGame().GetPlayerController();
		PS_PlayableControllerComponent playableController = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));
		if (!playableController)
			return;

		playableController.ForceRespawnPlayer();
	}

	void RespawnInit_Callback(SCR_ChatPanel panel, string data)
	{
		PlayerController playerController = GetGame().GetPlayerController();
		PS_PlayableControllerComponent playableController = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));
		if (!playableController)
			return;

		playableController.ForceRespawnPlayer(true);
	}

	void Test_Callback(SCR_ChatPanel panel, string data)
	{
		MemoryStatsSnapshot snapshot = new MemoryStatsSnapshot();
		int statsCount = MemoryStatsSnapshot.GetStatsCount();
		for (int i = 0; i < statsCount; i++)
		{
			Print(MemoryStatsSnapshot.GetStatName(i));
			Print(snapshot.GetStatValue(i));
		}
	}

	void ExportMissionData_Callback(SCR_ChatPanel panel, string data)
	{
		PS_MissionDataManager.GetInstance().WriteToFile();
	}

	void LoadMap_Callback(SCR_ChatPanel panel, string data)
	{
		PlayerController playerController = GetGame().GetPlayerController();
		PS_PlayableControllerComponent playableController = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));

		PlayerManager playerManager = GetGame().GetPlayerManager();
		EPlayerRole playerRole = playerManager.GetPlayerRoles(playerController.GetPlayerId());
		if (!PS_PlayersHelper.IsAdminOrServer()) return;

		playableController.LoadMission(data);
	}

	void AdvanceStage_Callback(SCR_ChatPanel panel, string data)
	{
		PlayerController playerController = GetGame().GetPlayerController();
		PS_PlayableControllerComponent playableController = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));

		PlayerManager playerManager = GetGame().GetPlayerManager();
		EPlayerRole playerRole = playerManager.GetPlayerRoles(playerController.GetPlayerId());
		if (!PS_PlayersHelper.IsAdminOrServer()) return;

		playableController.AdvanceGameState(SCR_EGameModeState.NULL);
	}

	void PlayGameConfig_Callback(SCR_ChatPanel panel, string data)
	{
		if (data == "")
			return;
		Resource resource = BaseContainerTools.LoadContainer(data);
		if (!resource)
			return;
		GameStateTransitions.RequestScenarioChangeTransition(data, "", "");
	}

	void removeRestrictedZones()
	{
		BaseGameMode gamemode = GetGame().GetGameMode();
		SCR_PlayersRestrictionZoneManagerComponent restrictionZoneManager = SCR_PlayersRestrictionZoneManagerComponent.Cast(gamemode.FindComponent(SCR_PlayersRestrictionZoneManagerComponent));
		set<SCR_EditorRestrictionZoneEntity> zones = restrictionZoneManager.GetZones();

		SCR_ChatPanelManager chatPanelManager = SCR_ChatPanelManager.GetInstance();
		ChatCommandInvoker invoker = chatPanelManager.GetCommandInvoker("smsg");
		invoker.Invoke(null, "#PS-Freeze_End");

		array<int> playerIds = new array<int>();
		GetGame().GetPlayerManager().GetPlayers(playerIds);
		foreach (int playerId : playerIds)
		{
			restrictionZoneManager.ResetPlayerZoneData(playerId);
		}

		for (int i = 0; i < zones.Count(); i++)
		{
			SCR_EditorRestrictionZoneEntity zone = zones.Get(i);
			SCR_EntityHelper.DeleteEntityAndChildren(zone);
		}
	}

	protected PS_FactionRespawnCount GetFactionRespawnCount(FactionKey factionKey)
	{
		if (m_mFactionRespawnCount.Contains(factionKey))
		{
			return m_mFactionRespawnCount[factionKey];
		}
		return null;
	}

	protected override void OnPlayerConnected(int playerId)
	{
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		string name = GetGame().GetPlayerManager().GetPlayerName(playerId);
		playableManager.SetPlayerName(playerId, name);

		// TODO: remove CallLater
		#ifdef WORKBENCH
		GetGame().GetCallqueue().CallLater(SpawnInitialEntity, 500, false, playerId);
		#else
		GetGame().GetCallqueue().CallLater(SpawnInitialEntity, 100, false, playerId);
		#endif

		// Reforger Lobby Conflict Edition: groups are generated once at server boot, so virtually every player is a
		// late-joiner. Re-send the groupID -> preset mapping once the manager is replicated to them.
		GetGame().GetCallqueue().CallLater(playableManager.SendDeploymentGroupsToPlayer, 1000, false, playerId);

		m_OnPlayerConnected.Invoke(playerId);
	}

	protected override bool HandlePlayerKilled(int playerId, IEntity playerEntity, IEntity killerEntity, notnull Instigator killer)
	{
		m_OnHandlePlayerKilled.Invoke(playerId, playerEntity, killerEntity, killer);

		return super.HandlePlayerKilled(playerId, playerEntity, killerEntity, killer);
	}

	// Reforger Lobby Conflict Edition (persistent PvE): on disconnect we DELETE the deployed body. The legacy flow
	// kept it alive for reconnect, but on our 24/7 server that body lingered in the world and could even
	// be persisted across a session save / restart (the player sees their old character on reconnect).
	// We delete the deployed body (server only, never the lobby camera) and clear the playable link so a
	// reconnecting player redeploys fresh from the lobby.
	protected override void OnPlayerDisconnected(int playerId, KickCauseCode cause, int timeout)
	{
		PlayerManager playerManager = GetGame().GetPlayerManager();
		SCR_PlayerController playerController = SCR_PlayerController.Cast(playerManager.GetPlayerController(playerId));
		PS_PlayableControllerComponent playableController = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));

		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		playableManager.SetPlayerState(playerId, PS_EPlayableControllerState.Disconected);
		if (m_iReconnectTime > 0) GetGame().GetCallqueue().CallLater(RemoveDisconnectedPlayer, m_iReconnectTime, false, playerId);

		IEntity controlledEntity = playerController.GetControlledEntity();
		IEntity initialEntity;
		if (playableController)
			initialEntity = playableController.GetInitialEntity();

		// Server-authoritative: unlink the playable and destroy the deployed body so nothing lingers
		// (or gets persisted). Never touch the lobby camera (initial entity).
		if (IsMaster())
		{
			playableManager.SetPlayerPlayable(playerId, RplId.Invalid());
			if (controlledEntity && controlledEntity != initialEntity)
			{
				Print("[PVE DISCONNECT] deleting deployed body for player=" + playerId, LogLevel.NORMAL);
				GetGame().GetCallqueue().Call(SCR_EntityHelper.DeleteEntityAndChildren, controlledEntity);
				controlledEntity = null; // body gone -> skip the reconnect-keep block below
			}
		}

		m_OnPlayerDisconnected.Invoke(playerId, cause, timeout);
		foreach (SCR_BaseGameModeComponent comp : m_aAdditionalGamemodeComponents)
		{
			comp.OnPlayerDisconnected(playerId, cause, timeout);
		}

		m_OnPostCompPlayerDisconnected.Invoke(playerId, cause, timeout);

		// RespawnSystemComponent is not a SCR_BaseGameModeComponent, so for now we have to
		// propagate these events manually.
		if (IsMaster())
			m_pRespawnSystemComponent.OnPlayerDisconnected_S(playerId, cause, timeout);

		foreach (SCR_BaseGameModeComponent comp : m_aAdditionalGamemodeComponents)
		{
			comp.OnPlayerDisconnected(playerId, cause, timeout);
		}

		m_OnPostCompPlayerDisconnected.Invoke(playerId, cause, timeout);

		if (IsMaster())
		{
			if (controlledEntity)
			{
				if (SCR_ReconnectComponent.GetInstance())
				{
					if (SCR_ReconnectComponent.GetInstance().HandlePlayerDisconnect(playerId, cause))	// if conditions to allow reconnect pass, skip the entity delete
					{
						CharacterControllerComponent charController = CharacterControllerComponent.Cast(controlledEntity.FindComponent(CharacterControllerComponent));
						if (charController)
						{
							charController.SetMovement(0, vector.Forward);
						}

						CompartmentAccessComponent compAccess = CompartmentAccessComponent.Cast(controlledEntity.FindComponent(CompartmentAccessComponent)); // TODO nullcheck
						if (compAccess)
						{
							BaseCompartmentSlot compartment = compAccess.GetCompartment();
							if (compartment)
							{
								CarControllerComponent carController = CarControllerComponent.Cast(compartment.GetVehicle().FindComponent(CarControllerComponent));
								if (carController)
								{
									carController.Shutdown();
									carController.StopEngine(false);
								}
							}
						}

						return;
					}
				}
			}
		}
	}

	bool CanJoinFaction(FactionKey factionKeyPlayer, FactionKey currentFaction)
	{
		if (m_iFactionsBalance == -1)
			return true;
		if (factionKeyPlayer == currentFaction)
			return true;

		map<FactionKey, int> players = new map<FactionKey, int>();
		map<FactionKey, int> playables = new map<FactionKey, int>();
		array<PS_PlayableContainer> playableComponents = m_playableManager.GetPlayablesSorted();
		foreach (PS_PlayableContainer playable : playableComponents)
		{
			FactionKey factionKey = playable.GetFactionKey();

			if (!players.Contains(factionKey))
				players[factionKey] = 0;
			if (!playables.Contains(factionKey))
				playables[factionKey] = 0;

			playables[factionKey] = playables[factionKey] + 1;
			int playerId = m_playableManager.GetPlayerByPlayable(playable.GetRplId());
			if (playerId > 0)
				players[factionKey] = players[factionKey] + 1;
		}
		if (currentFaction != "")
			players[currentFaction] = players[currentFaction] - 1;

		float maxFaction = 0;
		foreach (FactionKey factionKey, int count : playables)
			if (maxFaction < count)
				maxFaction = count;

		// Scale
		int minFaction = 999;
		foreach (FactionKey factionKey, int count : players)
		{
			int scaledCount = players[factionKey] * (maxFaction / playables[factionKey]);
			if (minFaction > scaledCount)
				minFaction = scaledCount;
		}

		int currentCount = players[factionKeyPlayer];
		int diff = currentCount - minFaction;

		return diff <= m_iFactionsBalance;
	}

	// ------------------------------------------ Actions ------------------------------------------
	// Open lobby in game
	void Action_OpenLobby()
	{
		PlayerController playerController = GetGame().GetPlayerController();
		PlayerManager playerManager = GetGame().GetPlayerManager();
		PS_PlayableControllerComponent playableController = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));
		EPlayerRole playerRole = playerManager.GetPlayerRoles(playerController.GetPlayerId());
		
		if (!m_bTeamSwitch && !PS_PlayersHelper.IsAdminOrServer()) return;

		MenuBase lobbyMenu = GetGame().GetMenuManager().FindMenuByPreset(ChimeraMenuPreset.CoopLobby);
		if (!lobbyMenu)
			GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.CoopLobby);
	}


	// Force open current game state menu
	void OpenCurrentMenuOnClients()
	{
		if (RplSession.Mode() != RplMode.Dedicated) RPC_OpenCurrentMenu(GetState());
		Rpc(RPC_OpenCurrentMenu, GetState());
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RPC_OpenCurrentMenu(SCR_EGameModeState state)
	{
		PlayerController playerController = GetGame().GetPlayerController();
		if (!playerController) return;
		if (playerController.GetPlayerId() == 0) return;
		PS_PlayableControllerComponent playableController = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));
		playableController.SwitchToMenu(state);
	}

	// Reforger Lobby Conflict Edition: per-player position of the lobby camera entity (InitialPlayer). The legacy
	// y=100000 "off-map" position triggers the engine assertion "Entity out of world bounds" on
	// smaller/custom worlds and crashes. We place players on a small grid near the map origin, just
	// above the terrain surface (guaranteed in-bounds). Used by both SpawnInitialEntity and
	// PS_PlayableControllerComponent.UpdatePosition so they never disagree (which would re-teleport
	// the entity back out of bounds). Raise the +2 offset if your world's vertical bounds allow it.
	static vector GetLobbyEntityPosition(int playerId)
	{
		// Per-player horizontal spread so multiple lobby characters never overlap.
		float gx = 1000 * Math.Mod(playerId, 10);
		float gz = 1000 * Math.Floor(Math.Mod(playerId, 100) / 10) + 5000 * Math.Floor(playerId / 100);
		// Reforger Lobby Conflict Edition: park the invisible lobby character well ABOVE the terrain/sea so the 3D
		// view stays empty (the menu background shows) and no ambient water sound plays when the
		// surface under the spawn grid is below sea level. The original mod used a fixed y=100000,
		// but that crashes ("Entity out of world bounds") on smaller/custom maps, so we stay a
		// bounded distance above the surface and clamp to the world's vertical ceiling.
		float gy = 1000;
		BaseWorld world = GetGame().GetWorld();
		if (world)
		{
			float surfaceY = world.GetSurfaceY(gx, gz);
			gy = surfaceY + 1000.0;

			vector mins;
			vector maxs;
			world.GetBoundBox(mins, maxs);
			gx = Math.Clamp(gx, mins[0] + 10.0, maxs[0] - 10.0);
			gz = Math.Clamp(gz, mins[2] + 10.0, maxs[2] - 10.0);

			float ceiling = maxs[1] - 10.0;
			if (gy > ceiling)
				gy = ceiling;
			// Never end up below the surface (clipping / underwater) if the ceiling is very low.
			if (gy < surfaceY + 5.0)
				gy = surfaceY + 5.0;
		}
		return Vector(gx, gy, gz);
	}

	void SpawnInitialEntity(int playerId)
	{
		#ifdef WORKBENCH
		IEntity WBCharacter = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
		if (WBCharacter)
			return;
		#endif

		PS_VoNRoomsManager VoNRoomsManager = PS_VoNRoomsManager.GetInstance();
		Resource resource = Resource.Load("{ADDE38E4119816AB}Prefabs/InitialPlayer_Version2.et");
		EntitySpawnParams params = new EntitySpawnParams();
		GetTransform(params.Transform);
		vector position = GetLobbyEntityPosition(playerId);
		params.Transform[3] = position;
		IEntity initialEntity = GetGame().SpawnEntityPrefab(resource, GetGame().GetWorld(), params);
		PlayerManager playerManager = GetGame().GetPlayerManager();
		SCR_PlayerController playerController = SCR_PlayerController.Cast(playerManager.GetPlayerController(playerId));
		PS_PlayableControllerComponent playableController = PS_PlayableControllerComponent.Cast(playerController.FindComponent(PS_PlayableControllerComponent));
		playableController.SetInitialEntity(initialEntity);
		playerController.SetInitialMainEntity(initialEntity);
		VoNRoomsManager.RestoreRoom(playerId);
	}

	// Reforger Lobby Conflict Edition: players who explicitly requested a return to the lobby (pause-menu / Respawn
	// Menu). A Game Master who merely deletes/loses their controlled character must NOT be dropped to
	// the lobby automatically (player = "dead" but kept in the editor); they only leave the editor for
	// the lobby through this explicit request. Consumed by the death pipeline in TryRespawn.
	protected ref array<int> m_aExplicitLobbyReturnPlayers = {};

	void MarkExplicitLobbyReturn(int playerId)
	{
		if (playerId > 0 && !m_aExplicitLobbyReturnPlayers.Contains(playerId))
			m_aExplicitLobbyReturnPlayers.Insert(playerId);
	}

	protected bool ConsumeExplicitLobbyReturn(int playerId)
	{
		int index = m_aExplicitLobbyReturnPlayers.Find(playerId);
		if (index < 0)
			return false;
		m_aExplicitLobbyReturnPlayers.Remove(index);
		return true;
	}

	// True when the player is currently in the full Game Master editor (not the limited player editor).
	// Used to keep a Game Master in the editor instead of force-dropping them to the deployment lobby
	// when their controlled character is deleted or dies.
	bool IsPlayerInGameMaster(int playerId)
	{
		if (playerId <= 0)
			return false;
		SCR_EditorManagerCore editorManagerCore = SCR_EditorManagerCore.Cast(SCR_EditorManagerCore.GetInstance(SCR_EditorManagerCore));
		if (!editorManagerCore)
			return false;
		SCR_EditorManagerEntity editorManager = editorManagerCore.GetEditorManager(playerId);
		if (!editorManager)
			return false;
		return editorManager.IsOpened() && !editorManager.IsLimited();
	}

	// Reforger Lobby Conflict Edition (PROJECT.md #4): release the player's current playable, drop them back on the
	// lobby camera and re-open the deployment lobby (CoopLobby). Used both on death and from the
	// pause-menu "Respawn". Server-only.
	void ReturnPlayerToLobby(int playerId)
	{
		if (!Replication.IsServer() || playerId <= 0)
			return;

		SCR_PlayerController playerController = SCR_PlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));
		PS_PlayableControllerComponent playableController;
		if (playerController)
			playableController = playerController.PS_GetPLayableComponent();

		// Kill the currently deployed body (authoritative) so no live, unpossessed character lingers
		// in the world after the player drops back to the lobby. NEVER touch the lobby/initial entity
		// (this method can be re-entered by the death->TryRespawn path once the body actually dies).
		if (playerController)
		{
			IEntity controlled = playerController.GetControlledEntity();
			IEntity initialEntity;
			if (playableController)
				initialEntity = playableController.GetInitialEntity();
			if (controlled && controlled != initialEntity)
			{
				CharacterControllerComponent characterController = CharacterControllerComponent.Cast(controlled.FindComponent(CharacterControllerComponent));
				if (characterController && !characterController.IsDead())
					characterController.ForceDeath();
			}
		}

		// Release the playable -> lobby camera.
		SwitchToInitialEntity(playerId);
		// Re-open the deployment lobby on the player's client.
		if (playableController)
			playableController.SwitchToMenuServer(SCR_EGameModeState.SLOTSELECTION);
	}

	void TryRespawn(RplId playableId, int playerId)
	{
		// Reforger Lobby Conflict Edition (PROJECT.md #4): a dead player always returns to the
		// deployment lobby to redeploy. No mission auto-respawn / faction respawn-count.
		// Exception: a Game Master who deletes/loses their controlled character stays in the editor
		// (player = "dead" but kept in Game Master). They only drop to the lobby when they explicitly
		// press Respawn (which marks an explicit return), consumed here. We still undeploy them (clear
		// playable + park on the lobby camera) so closing the editor / pressing Respawn menu later
		// cleanly opens the lobby instead of re-possessing the dead body.
		if (!ConsumeExplicitLobbyReturn(playerId) && IsPlayerInGameMaster(playerId))
		{
			PrepareGameMasterReturnToLobby(playerId);
			return;
		}

		ReturnPlayerToLobby(playerId);

		/* Legacy mission-respawn path — disabled for the persistent PvE lobby (kept for reference).
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		if (playableManager && playableId != RplId.Invalid() && playableManager.GetPlayableById(playableId))
		{
			PS_PlayableComponent playableComponent = playableManager.GetPlayableById(playableId).GetPlayableComponent();
			if (!playableComponent)
				return;

			FactionAffiliationComponent factionAffiliationComponent = playableComponent.GetFactionAffiliationComponent();
			Faction faction = factionAffiliationComponent.GetDefaultAffiliatedFaction();
			FactionKey factionKey = faction.GetFactionKey();
			PS_FactionRespawnCount factionRespawns = GetFactionRespawnCount(factionKey);
			if (!factionRespawns || factionRespawns.m_iCount == 0)
			{
				SwitchToInitialEntity(playerId);
				return;
			}
			ResourceName prefabToSpawn = playableComponent.GetNextRespawn(factionRespawns.m_iCount == -1);
			if (factionRespawns.m_iCount > 0)
				factionRespawns.m_iCount--;
			if (prefabToSpawn != "")
			{
				int time = factionRespawns.m_iTime;
				if (factionRespawns.m_bWaveMode)
					time = factionRespawns.m_iTime - Math.Mod(GetGame().GetWorld().GetWorldTime(), time);
				if (playerId > 0)
					playableComponent.OpenRespawnMenu(time);

				PS_RespawnData respawnData = new PS_RespawnData(playableComponent, prefabToSpawn);
				GetGame().GetCallqueue().CallLater(Respawn, time, false, playerId, respawnData);
				return;
			}
		}

		SwitchToInitialEntity(playerId);
		*/
	}

	void Respawn(int playerId, PS_RespawnData respawnData)
	{
		Resource resource = Resource.Load(respawnData.m_sPrefabName);
		EntitySpawnParams params = new EntitySpawnParams();
		Math3D.MatrixCopy(respawnData.m_aSpawnTransform, params.Transform);
		IEntity entity = GetGame().SpawnEntityPrefab(resource, GetGame().GetWorld(), params);
		SCR_AIGroup aiGroup = m_playableManager.GetPlayerGroupByPlayable(respawnData.m_Id);
		SCR_AIGroup playabelGroup = aiGroup.GetSlave();
		playabelGroup.AddAIEntityToGroup(entity);

		PS_PlayableComponent playableComponentNew = PS_PlayableComponent.Cast(entity.FindComponent(PS_PlayableComponent));
		playableComponentNew.SetPlayable(true);

		GetGame().GetCallqueue().Call(SwitchToSpawnedEntity, playerId, respawnData, entity, 4);
	}

	void SwitchToSpawnedEntity(int playerId, PS_RespawnData respawnData, IEntity entity, int frameCounter)
	{
		if (frameCounter > 0) // Await four frames
		{
			GetGame().GetCallqueue().Call(SwitchToSpawnedEntity, playerId, respawnData, entity, frameCounter - 1);
			return;
		}

		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();

		PS_PlayableComponent playableComponent = PS_PlayableComponent.Cast(entity.FindComponent(PS_PlayableComponent));
		RplId playableId = playableComponent.GetRplId();

		playableComponent.CopyState(respawnData);
		if (playerId > 0)
		{
			playableManager.SetPlayerPlayable(playerId, playableId);
			playableManager.ForceSwitch(playerId);
		}
	}

	void SwitchToInitialEntity(int playerId)
	{
		if (playerId <= 0)
			return;
		PlayerManager playerManager = GetGame().GetPlayerManager();
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		playableManager.SetPlayerPlayable(playerId, RplId.Invalid());
		playableManager.ApplyPlayable(playerId);
	}

	// Persistent PvE: a Game Master deleted/lost their controlled character but must STAY in the editor
	// (PROJECT.md #4 exception). We still "undeploy" them server-side — clear the stale playable
	// assignment and park them on the lobby camera — WITHOUT switching their menu. This way, when they
	// later close the editor or press the Respawn menu, EditorClosed -> SwitchToMenu(GAME) sees no
	// playable and cleanly opens the deployment lobby instead of trying to re-possess the deleted body.
	void PrepareGameMasterReturnToLobby(int playerId)
	{
		if (!Replication.IsServer() || playerId <= 0)
			return;
		SwitchToInitialEntity(playerId);
	}

	// --------------------------------------------------------------------------------------------
	// Reforger Lobby Conflict Edition deploy: spawn the player's chosen loadout prefab at the chosen spawn point,
	// attach it to the chosen deployment group, and possess the requesting player.
	// Server-only. Deploys ONLY the requesting player (PROJECT.md #2) and never changes global state.
	void RequestDeploy(int playerId)
	{
		if (!Replication.IsServer())
			return;
		if (playerId <= 0)
			return;

		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		if (!playableManager)
			return;

		int groupID = playableManager.GetPlayerSelectedGroup(playerId);
		ResourceName loadout = playableManager.GetPlayerSelectedLoadout(playerId);
		RplId spawnId = playableManager.GetPlayerSelectedSpawn(playerId);

		int spawnValid = 0;
		if (spawnId.IsValid())
			spawnValid = 1;
		if (groupID <= 0 || loadout == "" || spawnValid == 0)
		{
			Print("[PVE DEPLOY] player=" + playerId + " incomplete selection group=" + groupID + " loadout=" + loadout + " spawnValid=" + spawnValid, LogLevel.WARNING);
			return;
		}

		SCR_SpawnPoint spawnPoint = SCR_SpawnPoint.GetSpawnPointByRplId(spawnId);
		if (!spawnPoint)
		{
			Print("[PVE DEPLOY] spawn point not found id=" + spawnId, LogLevel.WARNING);
			return;
		}

		// Group is OPTIONAL for deployment: it's only used to attach the spawned character to a
		// squad slave group. If it was culled (or never created), still deploy the player solo
		// rather than blocking them in the lobby.
		SCR_GroupsManagerComponent groupsManager = SCR_GroupsManagerComponent.GetInstance();
		SCR_AIGroup group;
		if (groupsManager)
			group = groupsManager.FindGroup(groupID);
		if (!group)
			Print("[PVE DEPLOY] group not found id=" + groupID + " (deploying solo)", LogLevel.WARNING);

		Resource resource = Resource.Load(loadout);
		if (!resource.IsValid())
		{
			Print("[PVE DEPLOY] invalid loadout resource=" + loadout, LogLevel.WARNING);
			return;
		}

		EntitySpawnParams params = new EntitySpawnParams();
		spawnPoint.GetWorldTransform(params.Transform);
		IEntity entity = GetGame().SpawnEntityPrefab(resource, GetGame().GetWorld(), params);
		if (!entity)
		{
			Print("[PVE DEPLOY] SpawnEntityPrefab failed loadout=" + loadout, LogLevel.WARNING);
			return;
		}

		PS_PlayableComponent playableComponent = PS_PlayableComponent.Cast(entity.FindComponent(PS_PlayableComponent));
		if (!playableComponent)
		{
			// The loadout prefab must be a playable character (have PS_PlayableComponent) so the
			// player can possess it. Vanilla Character_* prefabs without it cannot be deployed.
			Print("[PVE DEPLOY] loadout prefab has no PS_PlayableComponent (cannot possess): " + loadout, LogLevel.WARNING);
			SCR_EntityHelper.DeleteEntityAndChildren(entity);
			return;
		}

		// Reforger Lobby Conflict Edition: stamp the chosen deployment group on the playable. RegisterPlayable() reads
		// this to bind the player to the named lobby group instead of spawning a fresh, unnamed group
		// ("Atlas Green 1"). We do NOT rely on the bots/slave linkage here: the freshly spawned loadout
		// does not reliably land in our group's slave, so the player-group binding is forced by id.
		playableComponent.PS_SetDeployGroupID(groupID);

		if (group)
		{
			SCR_AIGroup slave = group.GetSlave();
			if (slave)
				slave.AddAIEntityToGroup(entity);
		}

		playableComponent.SetPlayable(true);

		Print("[PVE DEPLOY] player=" + playerId + " deployed groupID=" + groupID + " loadout=" + loadout, LogLevel.NORMAL);
		GetGame().GetCallqueue().Call(SwitchToDeployedEntity, playerId, entity, 30);
	}

	void SwitchToDeployedEntity(int playerId, IEntity entity, int frameCounter)
	{
		if (!entity)
			return;

		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		if (!playableManager)
			return;
		PS_PlayableComponent playableComponent = PS_PlayableComponent.Cast(entity.FindComponent(PS_PlayableComponent));
		if (!playableComponent)
			return;

		// The character was just spawned: its RplId and the manager registration (RegisterPlayable
		// runs through a couple of deferred frames) may not be ready yet. If we link the player to an
		// invalid/unregistered playable, ApplyPlayable falls back to the off-map lobby camera and the
		// player is stuck on the "Get Back to the battlefield!" out-of-bounds warning. Wait until both
		// the RplId is valid AND the playable is registered before transferring control.
		RplId playableId = playableComponent.GetRplId();
		int rplValid = 0;
		if (playableId.IsValid())
			rplValid = 1;
		int registered = 0;
		if (playableManager.GetPlayableById(playableId))
			registered = 1;
		if (rplValid == 0 || registered == 0)
		{
			if (frameCounter > 0)
			{
				GetGame().GetCallqueue().Call(SwitchToDeployedEntity, playerId, entity, frameCounter - 1);
				return;
			}
			Print("[PVE DEPLOY] playable not ready for possession rplValid=" + rplValid + " registered=" + registered, LogLevel.WARNING);
			return;
		}

		Print("[PVE DEPLOY] possessing player=" + playerId + " playableId=" + playableId, LogLevel.NORMAL);

		// Backstop: force the playable -> CHOSEN deployment group mapping now that the playable is
		// registered. RegisterPlayable() runs across deferred frames and may have bound the playable to
		// a fresh, unnamed junk group ("Atlas Green 1") instead of the picked one. We override the map
		// BEFORE ApplyPlayable() (which schedules ChangeGroup) so the player joins the named group with
		// its preset name + flag. Runs late = no spawn/registration timing races.
		int chosenGroupId = playableComponent.PS_GetDeployGroupID();
		if (chosenGroupId > 0)
		{
			SCR_GroupsManagerComponent groupsManager = SCR_GroupsManagerComponent.GetInstance();
			SCR_AIGroup chosenGroup;
			if (groupsManager)
				chosenGroup = groupsManager.FindGroup(chosenGroupId);
			if (chosenGroup)
			{
				playableManager.SetPlayablePlayerGroupId(playableId, chosenGroupId);
				Print("[PVE GROUP] backstop bound playable=" + playableId + " -> chosen group id=" + chosenGroupId + " name=" + chosenGroup.GetCustomName(), LogLevel.NORMAL);
			}
			else
			{
				Print("[PVE GROUP] backstop FindGroup FAILED for chosen id=" + chosenGroupId, LogLevel.WARNING);
			}
		}
		else
		{
			Print("[PVE GROUP] backstop: no chosen group id stamped on playable=" + playableId, LogLevel.WARNING);
		}

		playableManager.SetPlayerPlayable(playerId, playableId);
		// Possess on the server directly (authoritative). Do NOT rely solely on the ForceSwitch
		// client roundtrip — if the client doesn't process it, the player is left on the off-map
		// lobby camera ("Get Back to the battlefield!").
		playableManager.ApplyPlayable(playerId);
		// Switch the client's menu out of the lobby and fade into the game.
		playableManager.ForceSwitch(playerId);
	}

	// If after m_iReconnectTime player still disconnected release playable
	void RemoveDisconnectedPlayer(int playerId)
	{
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		PS_EPlayableControllerState state = playableManager.GetPlayerState(playerId);
		if (state == PS_EPlayableControllerState.Disconected)
		{
			playableManager.SetPlayerPlayable(playerId, RplId.Invalid());
		}
	}

	// Reforger Lobby Conflict Edition (Strategy B): build playable groups from each playable faction's
	// SCR_GroupRolePresetConfig list. One playable group per preset; capacity and the
	// loadout palette come from the preset (m_iGroupSize / m_aLoadoutResources).
	// - Execute ONLY on server (called once from OnGameStart).
	void GenerateDeploymentGroups()
	{
		if (!Replication.IsServer())
			return;

		SCR_FactionManager factionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		if (!factionManager)
			return;
		SCR_GroupsManagerComponent groupsManager = SCR_GroupsManagerComponent.GetInstance();
		if (!groupsManager)
			return;
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		if (!playableManager)
			return;

		array<Faction> factions = {};
		factionManager.GetFactionsList(factions);
		int isCampaignFM = 0;
		if (SCR_CampaignFactionManager.Cast(factionManager))
			isCampaignFM = 1;
		Print("[PVE GEN] isCampaign=" + isCampaignFM + " factions=" + factions.Count(), LogLevel.NORMAL);
		foreach (Faction faction : factions)
		{
			SCR_Faction scrFaction = SCR_Faction.Cast(faction);
			if (!scrFaction)
				continue;

			array<SCR_GroupRolePresetConfig> presets = {};
			scrFaction.GetGroupRolePresetConfigs(presets);
			int playableInt = 0;
			if (scrFaction.IsPlayable())
				playableInt = 1;
			Print("[PVE GEN] faction=" + scrFaction.GetFactionKey() + " playable=" + playableInt + " presets=" + presets.Count(), LogLevel.NORMAL);

			if (!scrFaction.IsPlayable())
				continue;

			for (int i = 0, count = presets.Count(); i < count; i++)
			{
				SCR_GroupRolePresetConfig preset = presets[i];
				if (!preset || !preset.IsEnabled())
					continue;

				SCR_AIGroup group = groupsManager.CreateNewPlayableGroup(faction, preset.GetGroupRole());
				if (!group)
				{
					Print("[PVE GEN] CreateNewPlayableGroup FAILED for preset=" + preset.GetGroupName() + " (check Default Group Prefab)", LogLevel.WARNING);
					continue;
				}

				// Reforger Lobby Conflict Edition: these lobby groups must survive while empty (no player has
				// deployed into them yet). SCR_AIGroup has TWO independent auto-delete flags;
				// setting only one still lets the empty group be culled, so disable both.
				group.SetCanDeleteIfNoPlayer(false);
				group.SetDeleteWhenEmpty(false);
				preset.SetupGroup(group);
				preset.SetupGroupFlag(group, scrFaction);

				// Reforger Lobby Conflict Edition: force the preset name to be authored by the server (-1). SCR_AIGroup
				// .GetCustomName() blanks any name authored by a real player (UGC filtering), so a name
				// set with author 0 by SetupGroup would not display on clients. Author -1 always shows.
				group.SetCustomName(preset.GetGroupName(), -1);

				Print("[PVE GEN] created groupID=" + group.GetGroupID() + " preset=" + preset.GetGroupName() + " size=" + preset.GetGroupSize() + " loadouts=" + preset.GetLoadouts().Count(), LogLevel.NORMAL);
				playableManager.RegisterDeploymentGroup(group.GetGroupID(), scrFaction.GetFactionKey(), i);
			}
		}
	}

	// Reforger Lobby Conflict Edition (PROJECT.md #1 & #3): the vanilla Conflict base (SCR_GameModeCampaign)
	// funnels every match-ending event (faction victory, full base sweep, scenario end, admin
	// end-game vote, etc.) through EndGameMode(). Swallow them all so the server stays in GAME
	// 24/7 and never transitions to DEBRIEFING / POSTGAME. Objectives/base captures keep running
	// for gameplay but can no longer stop the session.
	override void EndGameMode(SCR_GameModeEndData endData)
	{
		Print("[PVE] EndGameMode suppressed - persistent 24/7 server never ends", LogLevel.WARNING);
	}

	override void OnGameStateChanged()
	{
		super.OnGameStateChanged();

		PS_VoNRoomsManager VoNRoomsManager = PS_VoNRoomsManager.GetInstance();
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		array<int> playerIds = new array<int>();
		GetGame().GetPlayerManager().GetPlayers(playerIds);

		SCR_EGameModeState state = GetState();
		m_OnGameStateChange.Invoke(state);
		switch (state)
		{
			case SCR_EGameModeState.BRIEFING: // Force move to voice rooms
				foreach (int playerId : playerIds)
				{
					RplId playableId = playableManager.GetPlayableByPlayer(playerId);
					if (playableId == RplId.Invalid())
					{
						playableManager.SetPlayerFactionKey(playerId, "");
						VoNRoomsManager.MoveToRoom(playerId, "", "#PS-VoNRoom_Global");
					}else{
						if (playableManager.IsPlayerGroupLeader(playerId) || m_bPublicCommandBriefing)
						{
							VoNRoomsManager.MoveToRoom(playerId, playableManager.GetPlayerFactionKey(playerId), "#PS-VoNRoom_Command");
						} else {
							string groupName = playableManager.GetGroupCallsignByPlayable(playableId).ToString();
							VoNRoomsManager.MoveToRoom(playerId, playableManager.GetPlayerFactionKey(playerId), groupName);
						}
					}
				}
				if (m_bHolsterWeapon)
					playableManager.HolsterWeapons();
				break;
		}
	}

	// Switch to next game state
	void AdvanceGameState(SCR_EGameModeState oldState)
	{
		if (!Replication.IsServer())
			return;

		SCR_EGameModeState state = GetState();
		if (oldState != SCR_EGameModeState.NULL && oldState != state) return;
		switch (state)
		{
			case SCR_EGameModeState.PREGAME:
				SetGameModeState(SCR_EGameModeState.SLOTSELECTION);
				break;
			case SCR_EGameModeState.SLOTSELECTION:
				if (m_bShowCutscene)
				{
					SetGameModeState(SCR_EGameModeState.CUTSCENE);
					GetGame().GetCallqueue().CallLater(AdvanceGameState, m_CutsceneManager.GetCutsceneTime() + 400, false, SCR_EGameModeState.CUTSCENE);
					if (RplSession.Mode() == RplMode.Dedicated)
						PS_CutsceneManager.GetInstance().RunCutscene(0);
				}
				else
					SetGameModeState(SCR_EGameModeState.BRIEFING);
				break;
			case SCR_EGameModeState.CUTSCENE:
				SetGameModeState(SCR_EGameModeState.BRIEFING);
				break;
			case SCR_EGameModeState.BRIEFING:
				StartGame();
				break;
			// Reforger Lobby Conflict Edition server (see PROJECT.md): once in GAME the mission runs
			// 24/7. Never auto-advance to DEBRIEFING/POSTGAME. This also neutralizes
			// objective-completion ending the match (PS_Objective calls
			// AdvanceGameState(GAME), which now no-ops here).
			case SCR_EGameModeState.GAME:
				break;
			case SCR_EGameModeState.DEBRIEFING: // unreachable, kept for enum completeness
				break;
			case SCR_EGameModeState.POSTGAME:
				break;
		}
		OpenCurrentMenuOnClients();
	}

	void StartGame()
	{
		m_iReconnectTime = m_iReconnectTimeAfterBriefing;
		if (m_bReserveSlots)
			ReserveSlots();
		PS_PlayableManager.GetInstance().RemoveRedundantUnits();
		restrictedZonesTimer(m_iFreezeTime);
		StartGameMode();
	}

	void ReserveSlots()
	{
		if (!Replication.IsServer())
			return;

		PS_SlotsReserver slotsReserver = PS_SlotsReserver.Cast(FindComponent(PS_SlotsReserver));
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();

		array<string> GUIDs = {};
		map<RplId, ref PS_PlayableContainer> playables = playableManager.GetPlayables();
		foreach (RplId id, PS_PlayableContainer playable : playables)
		{
			int playerId = playableManager.GetPlayerByPlayable(id);
			if (playerId <= 0)
				continue;

			string GUID = GetGame().GetBackendApi().GetPlayerIdentityId(playerId);
			GUIDs.Insert(GUID);
		}

		slotsReserver.AddGUIDs(GUIDs);
		slotsReserver.SetEnabled(true);
	}

	// TODO: move it to component
	void restrictedZonesTimer(int freezeTime)
	{
		// reduce time by second
		int time = 1000;
		if (freezeTime < time) time = freezeTime;
		freezeTime -= time;

		m_fCurrentFreezeTime = freezeTime;
		Replication.BumpMe();

		// Show timer on clients synced to server
		if (RplSession.Mode() != RplMode.Dedicated) RPC_restrictedZonesTimer(freezeTime);
		Rpc(RPC_restrictedZonesTimer, freezeTime);

		// next second or end
		if (freezeTime <= 0)
		{
			m_fGameStartTime = GetGame().GetWorld().GetWorldTime();
			m_fGameStartElapsedTime = GetElapsedTime();
			Replication.BumpMe();
			removeRestrictedZones();
			if (m_bDisableBuildingModeAfterFreezeTime)
				DisableBuildingMode();
		}
		else
			GetGame().GetCallqueue().CallLater(restrictedZonesTimer, time, false, freezeTime);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RPC_restrictedZonesTimer(int freezeTime)
	{
		if (freezeTime <= 0)
		{
			if (m_hFreezeTimeCounter)
			{
				m_hFreezeTimeCounter.GetRootWidget().RemoveFromHierarchy();
			}
			return;
		}

		if (m_hFreezeTimeCounter == null)
		{
			Widget widget = GetGame().GetWorkspace().CreateWidgets("{EC8A548C3F53BE4F}UI/layouts/FreezeTime/FreezeTimeCounter.layout");
			m_hFreezeTimeCounter = PS_FreezeTimeCounter.Cast(widget.FindHandler(PS_FreezeTimeCounter));
		}

		m_hFreezeTimeCounter.SetTime(freezeTime);
	}
	void DisableBuildingMode()
	{
		SCR_EditorManagerCore editorManagerCore = SCR_EditorManagerCore.Cast(SCR_EditorManagerCore.GetInstance(SCR_EditorManagerCore));
		array<int> outPlayers = {};
		GetGame().GetPlayerManager().GetAllPlayers(outPlayers);
		foreach (int player : outPlayers)
		{
			SCR_EditorManagerEntity editorManager = editorManagerCore.GetEditorManager(player);
			if (editorManager)
				editorManager.SetCanOpen(false, EEditorCanOpen.ALIVE);
		}
	}
	PS_FreezeTimeCounter m_hFreezeTimeCounter;

	// ------------------------------------------ Global flags ------------------------------------------
	bool IsFreezeTimeEnd()
	{
		return m_fCurrentFreezeTime <= 0;
	}
	
	bool IsDisableTimeEnd()
	{
		return m_fCurrentFreezeTime <= (m_iFreezeTime - m_iDisableTime) && m_fCurrentFreezeTime != 1;
	}
	
	bool IsFreezeTimeShootingForbiden()
	{
		return m_bFreezeTimeShootingForbiden;
	}

	bool IsAdminMode()
	{
		return m_bAdminMode;
	}

	bool GetFriendliesSpectatorOnly()
	{
		if (!GetGame().GetPlayerController()) return true;
		if (SCR_Global.IsAdmin(GetGame().GetPlayerController().GetPlayerId())) return false;
		return m_bFriendliesSpectatorOnly;
	}

	bool GetDisablePlayablesStreaming()
	{
		return m_bDisablePlayablesStreaming;
	}

	bool IsChatDisabled()
	{
		return m_bDisableChat;
	}

	bool IsFactionLockMode()
	{
		return m_bFactionLock;
	}
	
	bool IsArmaVisionDisabled()
	{
		return m_bDisableArmaVision;
	}
	
	bool GetDisableBuildingModeAfterFreezeTime()
	{
		return m_bDisableBuildingModeAfterFreezeTime;
	}

	bool GetMarkersOnlyOnBriefing()
	{
		return m_bMarkersOnlyOnBriefing;
	}
	void SetMarkersOnlyOnBriefing(bool markersOnlyOnBriefing)
	{
		RPC_SetMarkersOnlyOnBriefing(markersOnlyOnBriefing);
		Rpc(RPC_SetMarkersOnlyOnBriefing, markersOnlyOnBriefing);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RPC_SetMarkersOnlyOnBriefing(bool markersOnlyOnBriefing)
	{
		m_bMarkersOnlyOnBriefing = markersOnlyOnBriefing;
	}

	bool GetDisableLeaderSquadMarkers()
	{
		return m_bRemoveSquadMarkers;
	}
	void SetDisableLeaderSquadMarkers(bool disableLeaderSquadMarkers)
	{
		RPC_SetDisableLeaderSquadMarkers(disableLeaderSquadMarkers);
		Rpc(RPC_SetDisableLeaderSquadMarkers, disableLeaderSquadMarkers);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RPC_SetDisableLeaderSquadMarkers(bool disableLeaderSquadMarkers)
	{
		m_bRemoveSquadMarkers = disableLeaderSquadMarkers;
	}

	// Global flags set
	void FactionLockSwitch()
	{
		m_bFactionLock = !m_bFactionLock;
		Rpc(RPC_SetFactionLock, m_bFactionLock);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RPC_SetFactionLock(bool factionLock)
	{
		m_bFactionLock = factionLock;
	}

	// ------------------------------------------ Global variables ------------------------------------------
	int GetFreezeTime()
	{
		return m_iFreezeTime;
	}
	void SetFreezeTime(int freezeTime)
	{
		RPC_SetFreezeTime(freezeTime);
		Rpc(RPC_SetFreezeTime, freezeTime);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RPC_SetFreezeTime(int freezeTime)
	{
		m_iFreezeTime = freezeTime;
	}
	
	int GetDisableTime()
	{
		return m_iDisableTime;
	}
	
	float GetGameStartTime()
	{
		return m_fGameStartTime;
	}

	float GetGameStartElapsedTime()
	{
		return m_fGameStartElapsedTime;
	}

	int GetReconnectTime()
	{
		return m_iReconnectTime;
	}
	void SetReconnectTime(int availableReconnectTime)
	{
		RPC_SetReconnectTime(availableReconnectTime);
		Rpc(RPC_SetReconnectTime, availableReconnectTime);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RPC_SetReconnectTime(int availableReconnectTime)
	{
		m_iReconnectTime = availableReconnectTime;
	}

	bool GetRemoveRedundantUnits()
	{
		return m_bRemoveRedundantUnits;
	}
	void SetRemoveRedundantUnits(bool killRedundantUnits)
	{
		RPC_SetRemoveRedundantUnits(killRedundantUnits);
		Rpc(RPC_SetRemoveRedundantUnits, killRedundantUnits);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RPC_SetRemoveRedundantUnits(bool killRedundantUnits)
	{
		m_bRemoveRedundantUnits = killRedundantUnits;
	}

	bool GetCanOpenLobbyInGame()
	{
		return m_bTeamSwitch;
	}
	void SetCanOpenLobbyInGame(bool canOpenLobbyInGame)
	{
		RPC_SetCanOpenLobbyInGame(canOpenLobbyInGame);
		Rpc(RPC_SetCanOpenLobbyInGame, canOpenLobbyInGame);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RPC_SetCanOpenLobbyInGame(bool canOpenLobbyInGame)
	{
		m_bTeamSwitch = canOpenLobbyInGame;
	}

	// ------------------------------------------ JIP Replication ------------------------------------------
	override bool RplSave(ScriptBitWriter writer)
	{
		writer.WriteBool(m_bFactionLock);
		writer.WriteInt(m_iFreezeTime);
		writer.WriteInt(m_iReconnectTime);

		return true;
	}

	override bool RplLoad(ScriptBitReader reader)
	{
		reader.ReadBool(m_bFactionLock);
		reader.ReadInt(m_iFreezeTime);
		reader.ReadInt(m_iReconnectTime);

		return true;
	}
}

[BaseContainerProps()]
class PS_FactionRespawnCount
{
	[Attribute()]
	FactionKey m_sFactionKey;
	[Attribute()]
	int m_iCount;
	[Attribute()]
	int m_iTime;
	[Attribute()]
	bool m_bWaveMode;
}
