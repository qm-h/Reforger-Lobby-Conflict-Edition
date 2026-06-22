// Insert new menu to global pressets enum
// Don't forge modify config {C747AFB6B750CE9A}Configs/System/chimeraMenus.conf, if you do the same.
modded enum ChimeraMenuPreset : ScriptMenuPresetEnum
{
	CoopLobby
}

/*
	Script generate widgets tree:
	Lobby 							- Our lobby widget
	├── FactionList 				- Contains all factions that have playable
	│ └── FactionWidget			- (m_sFactionSelectorPrefab) Set m_fCurrentFaction And update CharactersList
	├── CharactersList 				- Contains all playables from current selected faction (m_fCurrentFaction)
	│ └── GroupList 				- (m_sRolesGroupPrefab) Has no functional just group playable by group name
	│ └── CharacterWidget		- (m_sCharacterSelectorPrefab) Select playable for player
	└── PlayersList 				- Contains all CONNECTED players
		└── PlayerWidget			- (m_sPlayerSelectorPrefab) Kick or mute players.
*/

// Main lobby widget
// Menu preset: ChimeraMenuPreset.CoopLobby
// Path: {9DECCA625D345B35}UI/Lobby/CoopLobby.layout
class PS_CoopLobby : MenuBase
{
	// Const
	const static ResourceName IMAGESET_PS = "{F3A9B47F55BE8D2B}UI/Textures/Icons/PS_Atlas_x64.imageset";
	
	protected ResourceName m_sRolesGroupPrefab = "{B45A0FA6883A7A0E}UI/Lobby/RolesGroup.layout"; // Handler: PS_RolesGroup
	protected ResourceName m_sCharacterSelectorPrefab = "{3F761F63F1DF29D1}UI/Lobby/CharacterSelector.layout"; // Handler: PS_CharacterSelector
	protected ResourceName m_sFactionSelectorPrefab = "{DA22ED7112FA8028}UI/Lobby/FactionSelector.layout"; // Handler: PS_FactionSelector
	
	// Cache global
	protected InputManager m_InputManager;
	protected PS_GameModeCoop m_GameModeCoop;
	protected PS_PlayableManager m_PlayableManager;
	protected PlayerManager m_PlayerManager;
	protected PlayerController m_PlayerController;
	protected SCR_FactionManager m_FactionManager;
	protected WorkspaceWidget m_wWorkspaceWidget;
	protected PS_PlayableControllerComponent m_PlayableControllerComponent;
	protected int m_iPlayerId;

	// Widgets
	protected Widget m_wRoot;
	protected FrameWidget m_wGameModeHeader;
	protected OverlayWidget m_wChatPanel;
	protected VerticalLayoutWidget m_wFactionList;
	protected VerticalLayoutWidget m_wRolesList;
	// Reforger Lobby Conflict Edition: dedicated column listing the faction's world spawn points.
	protected VerticalLayoutWidget m_wSpawnList;
	protected VerticalLayoutWidget m_wPlayersSearch;
	protected ScrollLayoutWidget m_wPlayersScroll;
	protected OverlayWidget m_wPlayersSearchBox;
	protected VerticalLayoutWidget m_wPlayersList;
	protected ButtonWidget m_wPreviewHideButton;
	protected TextWidget m_wPlayersCounter;
	protected FrameWidget m_wVoiceChatFrame;
	protected OverlayWidget m_wLobbyLittleInventoryItemInfo;
	protected ButtonWidget m_wPlayersSwitch;
	protected ButtonWidget m_wVoiceSwitch;
	protected VerticalLayoutWidget m_wMainLoadoutPreview;
	protected OverlayWidget m_wLoadoutPreviewBody;
	protected OverlayWidget m_wPlayersBody;
	protected ItemPreviewWidget m_wPreview;
	protected ButtonWidget m_wNavigationStart;
	protected ButtonWidget m_wNavigationChat;
	protected ButtonWidget m_wNavigationClose;
	protected ScrollLayoutWidget m_wRolesScroll;
	protected OverlayWidget m_wOverlayCounter;
	protected TextWidget m_wTextCounter;
	protected ButtonWidget m_wScreenButton;
	protected ButtonWidget m_wRolesFoldButton;
	protected ImageWidget m_wRolesFoldButtonImage;
	
	// Handlers
	protected SCR_ButtonBaseComponent m_PlayersSwitchButtonComponent;
	protected SCR_ButtonBaseComponent m_VoiceSwitchButtonComponent;
	protected SCR_ButtonBaseComponent m_PreviewHideButtonComponent;
	protected PS_GameModeHeader m_GameModeHeader;
	protected SCR_ChatPanel m_ChatPanel;
	protected PS_LobbyLoadoutPreview m_LobbyLoadoutPreview;
	protected PS_PlayersList m_PlayersList;
	protected SCR_InputButtonComponent m_NavigationStart;
	protected SCR_InputButtonComponent m_NavigationChat;
	protected SCR_InputButtonComponent m_NavigationClose;
	protected PS_VoiceChatList m_VoiceChatList;
	protected SCR_EditBoxComponent m_PlayersSearchBox;
	protected SCR_ButtonBaseComponent m_RolesFoldButtonComponent;
	
	// Vars
	protected ref map<SCR_Faction, PS_FactionSelector> m_mFactions = new map<SCR_Faction, PS_FactionSelector>();
	protected ref map<SCR_AIGroup, PS_RolesGroup> m_mGroups = new map<SCR_AIGroup, PS_RolesGroup>();
	// Reforger Lobby Conflict Edition deployment group rows (built from preset, not keyed by a live SCR_AIGroup).
	protected ref array<PS_RolesGroup> m_aDeployGroups = {};
	protected SCR_Faction m_CurrentFaction = null;
	protected int m_iSelectedPlayer;
	// Reforger Lobby Conflict Edition: deployment groups are generated at server boot and replicated to late-joining
	// clients via RPC. The lobby may build before they arrive; this coalesces a single rebuild.
	protected bool m_bRebuildScheduled = false;
	
	override void OnMenuOpen()
	{
		Print("[PVE UI] PS_CoopLobby.OnMenuOpen ENTER (waitEnded=" + PS_WaitScreen.m_bWaitEnded + " rplMode=" + RplSession.Mode() + ")", LogLevel.NORMAL);
		if (!PS_WaitScreen.m_bWaitEnded) {
			Print("[PVE UI] OnMenuOpen ABORT: WaitScreen not ended -> Close()", LogLevel.WARNING);
			Close();
			return;
		}
			
		if (RplSession.Mode() == RplMode.Dedicated) {
			Print("[PVE UI] OnMenuOpen ABORT: Dedicated mode -> Close()", LogLevel.NORMAL);
			Close();
			return;
		}
		
		// Cache global
		m_InputManager = GetGame().GetInputManager();
		m_GameModeCoop = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		m_PlayableManager = PS_PlayableManager.GetInstance();
		m_PlayerManager = GetGame().GetPlayerManager();
		m_PlayerController = GetGame().GetPlayerController();
		m_FactionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		m_wWorkspaceWidget = GetGame().GetWorkspace();
		Print("[PVE UI] OnMenuOpen managers: gameMode=" + (m_GameModeCoop != null) + " playableMgr=" + (m_PlayableManager != null) + " playerMgr=" + (m_PlayerManager != null) + " playerCtrl=" + (m_PlayerController != null) + " factionMgr=" + (m_FactionManager != null), LogLevel.NORMAL);
		if (!m_PlayerController)
		{
			Print("[PVE UI] OnMenuOpen ABORT: PlayerController is NULL", LogLevel.ERROR);
			Close();
			return;
		}
		m_iPlayerId = m_PlayerController.GetPlayerId();
		m_PlayableControllerComponent = PS_PlayableControllerComponent.Cast(m_PlayerController.FindComponent(PS_PlayableControllerComponent));
		m_iSelectedPlayer = m_iPlayerId;

		// Widgets
		m_wRoot = GetRootWidget();
		m_wGameModeHeader = FrameWidget.Cast(m_wRoot.FindAnyWidget("GameModeHeader"));
		m_wChatPanel = OverlayWidget.Cast(m_wRoot.FindAnyWidget("ChatPanel"));
		m_wFactionList = VerticalLayoutWidget.Cast(m_wRoot.FindAnyWidget("FactionList"));
		m_wRolesList = VerticalLayoutWidget.Cast(m_wRoot.FindAnyWidget("RolesList"));
		m_wSpawnList = VerticalLayoutWidget.Cast(m_wRoot.FindAnyWidget("SpawnList"));
		m_wPlayersSearch = VerticalLayoutWidget.Cast(m_wRoot.FindAnyWidget("PlayersSearch"));
		m_wPlayersScroll = ScrollLayoutWidget.Cast(m_wRoot.FindAnyWidget("PlayersScroll"));
		m_wPlayersSearchBox = OverlayWidget.Cast(m_wRoot.FindAnyWidget("PlayersSearchBox"));
		m_wPreviewHideButton = ButtonWidget.Cast(m_wRoot.FindAnyWidget("PreviewHideButton"));
		m_wPlayersCounter = TextWidget.Cast(m_wRoot.FindAnyWidget("PlayersCounter"));
		m_wVoiceChatFrame = FrameWidget.Cast(m_wRoot.FindAnyWidget("VoiceChatFrame"));
		m_wLobbyLittleInventoryItemInfo = OverlayWidget.Cast(m_wRoot.FindAnyWidget("LobbyLittleInventoryItemInfo"));
		m_wPlayersSwitch = ButtonWidget.Cast(m_wRoot.FindAnyWidget("PlayersSwitch"));
		m_wVoiceSwitch = ButtonWidget.Cast(m_wRoot.FindAnyWidget("VoiceSwitch"));
		m_wMainLoadoutPreview = VerticalLayoutWidget.Cast(m_wRoot.FindAnyWidget("MainLoadoutPreview"));
		m_wLoadoutPreviewBody = OverlayWidget.Cast(m_wRoot.FindAnyWidget("LoadoutPreviewBody"));
		m_wPlayersBody = OverlayWidget.Cast(m_wRoot.FindAnyWidget("PlayersBody"));
		m_wPreview = ItemPreviewWidget.Cast(m_wRoot.FindAnyWidget("Preview"));
		m_wNavigationStart = ButtonWidget.Cast(m_wRoot.FindAnyWidget("NavigationStart"));
		m_wRolesScroll = ScrollLayoutWidget.Cast(m_wRoot.FindAnyWidget("RolesScroll"));
		m_wNavigationChat = ButtonWidget.Cast(m_wRoot.FindAnyWidget("NavigationChat"));
		m_wNavigationClose = ButtonWidget.Cast(m_wRoot.FindAnyWidget("NavigationClose"));
		m_wOverlayCounter = OverlayWidget.Cast(m_wRoot.FindAnyWidget("OverlayCounter"));
		m_wTextCounter = TextWidget.Cast(m_wRoot.FindAnyWidget("TextCounter"));
		m_wScreenButton = ButtonWidget.Cast(m_wRoot.FindAnyWidget("ScreenButton"));
		m_wRolesFoldButton = ButtonWidget.Cast(m_wRoot.FindAnyWidget("RolesFoldButton"));
		m_wRolesFoldButtonImage = ImageWidget.Cast(m_wRoot.FindAnyWidget("RolesFoldButtonImage"));
		
		// Handlers
		m_GameModeHeader = PS_GameModeHeader.Cast(m_wGameModeHeader.FindHandler(PS_GameModeHeader));
		m_ChatPanel = SCR_ChatPanel.Cast(m_wChatPanel.FindHandler(SCR_ChatPanel));
		m_PreviewHideButtonComponent = SCR_ButtonBaseComponent.Cast(m_wPreviewHideButton.FindHandler(SCR_ButtonBaseComponent));
		m_PlayersSwitchButtonComponent = SCR_ButtonBaseComponent.Cast(m_wPlayersSwitch.FindHandler(SCR_ButtonBaseComponent));
		m_VoiceSwitchButtonComponent = SCR_ButtonBaseComponent.Cast(m_wVoiceSwitch.FindHandler(SCR_ButtonBaseComponent));
		m_NavigationChat = SCR_InputButtonComponent.Cast(m_wNavigationChat.FindHandler(SCR_InputButtonComponent));
		m_NavigationClose = SCR_InputButtonComponent.Cast(m_wNavigationClose.FindHandler(SCR_InputButtonComponent));
		m_LobbyLoadoutPreview = PS_LobbyLoadoutPreview.Cast(m_wMainLoadoutPreview.FindHandler(PS_LobbyLoadoutPreview));
		m_PlayersList = PS_PlayersList.Cast(m_wPlayersBody.FindHandler(PS_PlayersList));
		m_NavigationStart = SCR_InputButtonComponent.Cast(m_wNavigationStart.FindHandler(SCR_InputButtonComponent));
		m_VoiceChatList = PS_VoiceChatList.Cast(m_wVoiceChatFrame.FindHandler(PS_VoiceChatList));
		m_PlayersSearchBox = SCR_EditBoxComponent.Cast(m_wPlayersSearchBox.FindHandler(SCR_EditBoxComponent));
		m_RolesFoldButtonComponent = SCR_ButtonBaseComponent.Cast(m_wRolesFoldButton.FindHandler(SCR_ButtonBaseComponent));
		
		FactionKey factionKey = m_PlayableManager.GetPlayerFactionKey(m_iPlayerId);
		// Reforger Lobby Conflict Edition: after death the player is sent back to the lobby and ApplyPlayable() clears the
		// active faction key (""), so m_CurrentFaction would be null and the player had to re-click their
		// faction even though their loadout/spawn/group selections were still remembered. Fall back to the
		// last non-null faction (GetPlayerFactionKeyRemembered) so the faction is pre-selected on return.
		if (factionKey == "")
			factionKey = m_PlayableManager.GetPlayerFactionKeyRemembered(m_iPlayerId);
		m_CurrentFaction = SCR_Faction.Cast(m_FactionManager.GetFactionByKey(factionKey));
		m_VoiceChatList.SwitchFaction(factionKey);
		
		// Buttons
		m_PreviewHideButtonComponent.m_OnClicked.Insert(OnClickedPreviewHide);
		m_PlayersSwitchButtonComponent.m_OnClicked.Insert(OnClickedPlayersSwitch);
		m_VoiceSwitchButtonComponent.m_OnClicked.Insert(OnClickedVoiceSwitch);
		m_NavigationStart.m_OnActivated.Insert(Action_Ready);
		m_NavigationChat.m_OnActivated.Insert(Action_ChatOpen);
		m_NavigationClose.m_OnActivated.Insert(Action_Exit);
		m_RolesFoldButtonComponent.m_OnClicked.Insert(OnClickedRolesFold);
		
		// Events
		m_PlayableManager.GetOnFactionChange().Insert(UpdatePlayerFaction);
		m_PlayableManager.GetOnStartTimerCounterChanged().Insert(OnStartTimerCounterChanged);
		m_PlayableManager.GetOnPlayerConnected().Insert(OnPlayerConnected);
		m_PlayableManager.GetOnPlayerDisconnected().Insert(OnPlayerDisconnected);
		m_PlayersSearchBox.m_OnChanged.Insert(OnPlayersSearchChanged);
		m_PlayersSearchBox.m_OnWriteModeEnter.Insert(OnPlayersSearchWriteModeEnter);
		m_PlayersSearchBox.m_OnWriteModeLeave.Insert(OnPlayersSearchWriteModeLeave);
		
		// Actions
		if (m_GameModeCoop.GetState() == SCR_EGameModeState.SLOTSELECTION)
		{
			m_InputManager.AddActionListener("VONDirect", EActionTrigger.DOWN, Action_LobbyVoNOn);
			m_InputManager.AddActionListener("VONDirect", EActionTrigger.UP, Action_LobbyVoNOff);
			//m_InputManager.AddActionListener("VONChannel", EActionTrigger.DOWN, Action_LobbyVoNChannelOn);
			m_InputManager.AddActionListener("VONChannel", EActionTrigger.UP, Action_LobbyVoNChannelOff);
		}
		
		m_LobbyLoadoutPreview.SetItemInfoWidget(m_wLobbyLittleInventoryItemInfo);

		// Reforger Lobby Conflict Edition: voice chat disabled. Hide the VoN list (it overlapped the loadout
		// preview) and the voice/players toggle buttons; keep the players list visible.
		if (m_wVoiceChatFrame)
			m_wVoiceChatFrame.SetVisible(false);
		if (m_wVoiceSwitch)
			m_wVoiceSwitch.SetVisible(false);
		if (m_wPlayersSwitch)
			m_wPlayersSwitch.SetVisible(false);
		if (m_wPlayersSearch)
			m_wPlayersSearch.SetVisible(true);
		
		// Init
		Init();

		// CEAF: play the custom deploy/respawn music while the lobby (spawn screen) is shown.
		PS_PlayDeployMusic();
		// CEAF: silence world ambience (wind/water/environment, the SFX bus) coming from the lobby
		// camera so only the menu music is heard. Restored in OnMenuClose.
		PS_MuteWorldSounds(true);
	}
	
	override void OnMenuUpdate(float tDelta)
	{
		m_ChatPanel.OnUpdateChat(tDelta);
		
		// Force refreash
		//if (m_wLoadoutPreviewBody.IsVisible())
		//	m_wPreview.SetRefresh(1, 1);
		
		m_GameModeHeader.TryUpdate();

		// CEAF: keep the world SFX bus paused while the lobby is open. The engine can re-enable it
		// when the lobby camera / listener updates, so we re-assert the pause every frame.
		if (m_bCEAFWorldMuted)
			AudioSystem.Pause(1 << AudioSystem.SFX);
	};

	override void OnMenuClose()
	{
		// CEAF: stop the deploy/respawn music when leaving the lobby (deployed / closed).
		PS_StopDeployMusic();
		// CEAF: restore world ambience (SFX bus) now that the player leaves the lobby.
		PS_MuteWorldSounds(false);

		if (m_InputManager)
		{
			m_InputManager.RemoveActionListener("VONDirect", EActionTrigger.DOWN, Action_LobbyVoNOn);
			m_InputManager.RemoveActionListener("VONDirect", EActionTrigger.UP, Action_LobbyVoNOff);
			//m_InputManager.RemoveActionListener("VONChannel", EActionTrigger.DOWN, Action_LobbyVoNChannelOn);
			m_InputManager.RemoveActionListener("VONChannel", EActionTrigger.UP, Action_LobbyVoNChannelOff);
		}
		if (m_PlayableManager)
		{
			m_PlayableManager.GetOnFactionChange().Remove(UpdatePlayerFaction);
			m_PlayableManager.GetOnStartTimerCounterChanged().Remove(OnStartTimerCounterChanged);
			m_PlayableManager.GetOnPlayerConnected().Remove(OnPlayerConnected);
			m_PlayableManager.GetOnPlayerDisconnected().Remove(OnPlayerDisconnected);
			m_PlayableManager.GetOnPlayerSelectionChange().Remove(OnSelectionChanged);
			m_PlayableManager.GetOnDeploymentGroupRegistered().Remove(OnDeploymentGroupRegistered);
		}
	}

	// --------------------------------------------------------------------------------------------------------------------------------
	// CEAF deploy/respawn music: this mod disables the vanilla deploy menu handler, so SCR_RespawnMusic
	// never receives its OnMenuOpen/OnMenuClosed events. We drive the music manually from the lobby
	// lifecycle instead. The track is the SOUND_RESPAWNMENU sound event provided by the world's
	// MusicManager acp (see MusicManager entity -> SCR_RespawnMusic -> Acp). No-op if not present.
	protected MusicManager PS_GetMusicManager()
	{
		ChimeraWorld world = GetGame().GetWorld();
		if (!world)
			return null;
		return world.GetMusicManager();
	}

	void PS_PlayDeployMusic()
	{
		MusicManager musicManager = PS_GetMusicManager();
		if (!musicManager)
			return;
		musicManager.Play(SCR_SoundEvent.SOUND_RESPAWNMENU);
	}

	void PS_StopDeployMusic()
	{
		MusicManager musicManager = PS_GetMusicManager();
		if (!musicManager)
			return;
		musicManager.Stop(SCR_SoundEvent.SOUND_RESPAWNMENU);
	}

	// CEAF: tracks whether we currently have the SFX bus paused for this lobby.
	protected bool m_bCEAFWorldMuted = false;

	// CEAF: mute/unmute the world ambience while the lobby is shown. The SFX channel carries the
	// environment loops (wind, water, ambient) produced at the lobby camera position. SetMasterVolume
	// gets reset by the engine when the listener updates, so we PAUSE the whole SFX bus instead -
	// the exact approach BI uses behind the respawn loading placeholder (SCR_RespawnSystemComponent).
	// VoiceChat / Music live on other buses and keep working.
	void PS_MuteWorldSounds(bool mute)
	{
		if (mute)
		{
			AudioSystem.Pause(1 << AudioSystem.SFX);
			m_bCEAFWorldMuted = true;
		}
		else if (m_bCEAFWorldMuted)
		{
			AudioSystem.Resume(1 << AudioSystem.SFX);
			m_bCEAFWorldMuted = false;
		}
	}

	// --------------------------------------------------------------------------------------------------------------------------------
	// Init
	void Init()
	{
		// Reforger Lobby Conflict Edition: rebuild the deployment columns whenever group mappings arrive from the
		// server (they are generated at boot, so they usually replicate AFTER this menu opens).
		if (m_PlayableManager)
		{
			m_PlayableManager.GetOnDeploymentGroupRegistered().Remove(OnDeploymentGroupRegistered);
			m_PlayableManager.GetOnDeploymentGroupRegistered().Insert(OnDeploymentGroupRegistered);
		}
		Print("[PVE UI] Init() calling InitDeployment", LogLevel.NORMAL);
		InitDeployment();
		InitPlayers();
		
		m_wPlayersCounter.SetTextFormat("%1/%2", m_PlayerManager.GetPlayerCount(), m_PlayableManager.GetMaxPlayers());
	}

	// Reforger Lobby Conflict Edition: build the lobby from generated deployment groups (faction -> group preset
	// -> loadout palette) instead of hand-placed playable entities. Rows are built entirely from
	// the replicated groupID->preset mapping; we do NOT need the live SCR_AIGroup entity.
	void InitDeployment()
	{
		Print("[PVE UI] InitDeployment ENTER (factionMgr=" + (m_FactionManager != null) + " playableMgr=" + (m_PlayableManager != null) + ")", LogLevel.NORMAL);
		if (!m_FactionManager)
		{
			Print("[PVE UI] InitDeployment ABORT: FactionManager is NULL", LogLevel.ERROR);
			return;
		}
		if (!m_PlayableManager)
		{
			Print("[PVE UI] InitDeployment ABORT: PlayableManager is NULL", LogLevel.ERROR);
			return;
		}
		array<Faction> factions = {};
		m_FactionManager.GetFactionsList(factions);
		Print("[PVE UI] factions=" + factions.Count(), LogLevel.NORMAL);
		foreach (Faction faction : factions)
		{
			SCR_Faction scrFaction = SCR_Faction.Cast(faction);
			if (!scrFaction || !scrFaction.IsPlayable())
				continue;

			array<int> groupIDs = {};
			m_PlayableManager.GetDeploymentGroupsForFaction(scrFaction.GetFactionKey(), groupIDs);
			Print("[PVE UI] faction=" + scrFaction.GetFactionKey() + " groupIDs=" + groupIDs.Count(), LogLevel.NORMAL);
			if (groupIDs.IsEmpty())
				continue;

			int capacityTotal = 0;
			bool factionUnlimited = false;
			foreach (int groupID : groupIDs)
			{
				SCR_GroupRolePresetConfig preset = m_PlayableManager.GetGroupPreset(groupID);
				if (!preset)
					continue;
				int groupSize = preset.GetGroupSize();
				if (groupSize <= 0) // Group Size 0 = unlimited
					factionUnlimited = true;
				else
					capacityTotal += groupSize;
			}

			AddFaction(scrFaction, 0, capacityTotal, 0);
			if (factionUnlimited && m_mFactions.Get(scrFaction))
				m_mFactions.Get(scrFaction).SetUnlimited(true);
			foreach (int groupID : groupIDs)
				AddDeploymentGroup(scrFaction, groupID);
			AddSpawnPointsSection(scrFaction);
		}

		m_PlayableManager.GetOnPlayerSelectionChange().Remove(OnSelectionChanged);
		m_PlayableManager.GetOnPlayerSelectionChange().Insert(OnSelectionChanged);
		RefreshCounters();
		RefreshDeploySelections();
	}

	// Reforger Lobby Conflict Edition: a group mapping arrived from the server. Coalesce a single rebuild so a burst
	// of RPCs (one per group) only triggers one widget rebuild.
	void OnDeploymentGroupRegistered(int groupID)
	{
		if (m_bRebuildScheduled)
			return;
		m_bRebuildScheduled = true;
		GetGame().GetCallqueue().CallLater(RebuildDeployment, 50, false);
	}

	void RebuildDeployment()
	{
		m_bRebuildScheduled = false;
		if (!m_PlayableManager || !m_FactionManager)
			return;
		Print("[PVE UI] RebuildDeployment (group mappings arrived)", LogLevel.NORMAL);
		ClearChildren(m_wFactionList);
		ClearChildren(m_wRolesList);
		ClearChildren(m_wSpawnList);
		m_mFactions.Clear();
		m_mGroups.Clear();
		m_aDeployGroups.Clear();
		InitDeployment();
	}

	void ClearChildren(Widget parent)
	{
		if (!parent)
			return;
		Widget child = parent.GetChildren();
		while (child)
		{
			Widget next = child.GetSibling();
			child.RemoveFromHierarchy();
			child = next;
		}
	}

	void AddDeploymentGroup(SCR_Faction faction, int groupID)
	{
		SCR_GroupRolePresetConfig preset = m_PlayableManager.GetGroupPreset(groupID);
		if (!preset)
		{
			Print("[PVE UI] AddDeploymentGroup: GetGroupPreset(" + groupID + ") NULL", LogLevel.WARNING);
			return;
		}

		Widget rolesGroupRoot = m_wWorkspaceWidget.CreateWidgets(m_sRolesGroupPrefab, m_wRolesList);
		PS_RolesGroup rolesGroup = PS_RolesGroup.Cast(rolesGroupRoot.FindHandler(PS_RolesGroup));
		rolesGroup.SetLobbyMenu(this);
		rolesGroup.SetDeploymentGroup(groupID, faction, preset.GetGroupSize(), preset.GetGroupName(), preset.GetGroupDescription(), preset.GetGroupFlag());
		rolesGroupRoot.SetVisible(m_CurrentFaction == faction);
		m_aDeployGroups.Insert(rolesGroup);

		array<ResourceName> loadouts = preset.GetLoadouts();
		Print("[PVE UI] groupID=" + groupID + " row created, loadouts=" + loadouts.Count(), LogLevel.NORMAL);
		foreach (ResourceName loadoutResource : loadouts)
		{
			// Reforger Lobby Conflict Edition: the palette entries are character prefabs authored in the
			// SCR_GroupRolePresetConfig (Loadout Resources). We resolve a readable name from
			// the prefab path directly (no dependency on SCR_LoadoutManager registration).
			string displayName = PrettyLoadoutName(loadoutResource);
			rolesGroup.InsertLoadout(groupID, loadoutResource, faction, displayName, ResourceName.Empty);
		}
	}

	// Build a readable label from a prefab ResourceName, e.g.
	// "{GUID}Prefabs/.../Character_US_MG.et" -> "US MG".
	static string PrettyLoadoutName(ResourceName resource)
	{
		string s = "" + resource;
		int brace = s.IndexOf("}");
		if (brace >= 0)
			s = s.Substring(brace + 1, s.Length() - brace - 1);
		int slash = -1;
		for (int i = 0, len = s.Length(); i < len; i++)
		{
			if (s.Get(i) == "/")
				slash = i;
		}
		if (slash >= 0)
			s = s.Substring(slash + 1, s.Length() - slash - 1);
		int dot = s.IndexOf(".");
		if (dot >= 0)
			s = s.Substring(0, dot);
		// Vanilla Conflict character prefabs are named "Campaign_<Faction>_Player_<Role>". Keep only
		// the role part so the lobby shows "SL Thompson" instead of "Campaign US Player SL Thompson".
		int playerTok = s.IndexOf("_Player_");
		if (playerTok >= 0)
			s = s.Substring(playerTok + 8, s.Length() - playerTok - 8);
		s.Replace("Character_", "");
		s.Replace("_", " ");
		if (s == "")
			s = "Loadout";
		return s;
	}

	// Reforger Lobby Conflict Edition: append a "Spawn Points" section under the faction, listing the faction's
	// deploy spawns from TWO sources:
	//   1. Conflict military bases currently OWNED by this faction (their SCR_CampaignSpawnPointGroup,
	//      exposed by SCR_CampaignMilitaryBaseComponent.GetSpawnPoint()), named by the base callsign.
	//   2. Standalone world-placed SCR_SpawnPoint / SCR_PlayerSpawnPoint affiliated to the faction.
	// Base-owned spawn points have their faction key set dynamically (HandleSpawnPointFaction), so
	// they can ALSO surface in GetSpawnPointsForFaction(): we track them to avoid duplicate rows.
	void AddSpawnPointsSection(SCR_Faction faction)
	{
		array<SCR_SpawnPoint> spawns = {};
		array<string> spawnNames = {};
		set<SCR_SpawnPoint> baseSpawnSet = new set<SCR_SpawnPoint>();

		// --- Source 1: military bases owned by this faction (US / USSR / FIA). ---
		SCR_GameModeCampaign campaign = SCR_GameModeCampaign.GetInstance();
		if (campaign)
		{
			SCR_CampaignMilitaryBaseManager baseManager = campaign.GetBaseManager();
			if (baseManager)
			{
				array<SCR_CampaignMilitaryBaseComponent> bases = {};
				baseManager.GetBases(bases, faction);
				int basesWithSpawn = 0;
				foreach (SCR_CampaignMilitaryBaseComponent base : bases)
				{
					if (!base)
						continue;
					SCR_SpawnPoint baseSpawn = base.GetSpawnPoint();
					if (!baseSpawn)
						continue;

					basesWithSpawn++;
					baseSpawnSet.Insert(baseSpawn);
					string baseName = base.GetCallsignDisplayName();
					if (baseName == "")
						baseName = baseSpawn.PS_GetLobbyDisplayName();
					if (baseName == "")
						baseName = baseSpawn.GetSpawnPointName();
					if (baseName == "")
						baseName = "Base";
					spawns.Insert(baseSpawn);
					spawnNames.Insert(baseName + " — Military Base");
				}
				Print("[PVE UI] faction=" + faction.GetFactionKey() + " ownedBases=" + bases.Count() + " basesWithSpawn=" + basesWithSpawn, LogLevel.NORMAL);
			}
		}

		// --- Source 2: standalone world spawn points (skip base-owned duplicates). ---
		array<SCR_SpawnPoint> standalone = SCR_SpawnPoint.GetSpawnPointsForFaction(faction.GetFactionKey());
		foreach (SCR_SpawnPoint spawnPoint : standalone)
		{
			if (!spawnPoint || baseSpawnSet.Contains(spawnPoint))
				continue;
			// Name priority: our custom "Lobby Display Name" field -> vanilla spawn point name
			// (m_sSpawnPointName / Info) -> generic fallback.
			string spName = spawnPoint.PS_GetLobbyDisplayName();
			if (spName == "")
				spName = spawnPoint.GetSpawnPointName();
			if (spName == "")
				spName = "Spawn";
			spawns.Insert(spawnPoint);
			spawnNames.Insert(spName + " — Spawn Point");
		}

		Print("[PVE UI] faction=" + faction.GetFactionKey() + " baseSpawns=" + baseSpawnSet.Count() + " totalSpawns=" + spawns.Count(), LogLevel.NORMAL);
		if (spawns.IsEmpty())
			return;

		// Render into the dedicated Spawn Points column (falls back to the Roles column if the
		// layout doesn't define a SpawnList container).
		Widget spawnParent = m_wSpawnList;
		if (!spawnParent)
			spawnParent = m_wRolesList;
		Widget rolesGroupRoot = m_wWorkspaceWidget.CreateWidgets(m_sRolesGroupPrefab, spawnParent);
		PS_RolesGroup rolesGroup = PS_RolesGroup.Cast(rolesGroupRoot.FindHandler(PS_RolesGroup));
		rolesGroup.SetLobbyMenu(this);
		// Empty in-row header: the dedicated column already has its own "Spawn Points" header.
		rolesGroup.SetSpawnSection(faction, "");
		rolesGroupRoot.SetVisible(m_CurrentFaction == faction);
		m_aDeployGroups.Insert(rolesGroup);

		for (int i = 0, count = spawns.Count(); i < count; i++)
			rolesGroup.InsertSpawnPoint(spawns[i].GetRplId(), faction, spawnNames[i]);
	}

	void OnSelectionChanged(int playerId)
	{
		RefreshCounters();
		RefreshDeploySelections();
	}

	// Reforger Lobby Conflict Edition: keep every loadout/spawn button's highlight in sync with the player's
	// current selection (independent highlights for loadout and spawn).
	void RefreshDeploySelections()
	{
		foreach (PS_RolesGroup rolesGroup : m_aDeployGroups)
		{
			if (rolesGroup)
				rolesGroup.RefreshDeploySelection();
		}
	}

	void RefreshCounters()
	{
		map<SCR_Faction, int> selectedByFaction = new map<SCR_Faction, int>();
		foreach (PS_RolesGroup rolesGroup : m_aDeployGroups)
		{
			if (!rolesGroup)
				continue;
			if (rolesGroup.IsSpawnSection())
				continue;
			rolesGroup.UpdateDeployCounter();
			SCR_Faction groupFaction = rolesGroup.GetDeployFaction();
			int selected = m_PlayableManager.GetGroupSelectedCount(rolesGroup.GetGroupID());
			int current = 0;
			selectedByFaction.Find(groupFaction, current);
			selectedByFaction.Set(groupFaction, current + selected);
		}
		foreach (SCR_Faction faction, PS_FactionSelector factionSelector : m_mFactions)
		{
			int selected = 0;
			selectedByFaction.Find(faction, selected);
			factionSelector.SetCount(selected);
		}
	}
	
	void InitPlayables()
	{
		array<PS_PlayableContainer> playables = m_PlayableManager.GetPlayablesSorted();
		map<RplId, ref PS_PlayableVehicleContainer> playableVehicles = m_PlayableManager.GetPlayableVehicles();
		map<SCR_Faction, ref Tuple3<int, int, int>> factions = new map<SCR_Faction, ref Tuple3<int, int, int>>();
		
		foreach (PS_PlayableContainer playable : playables)
		{
			AddPlayable(playable);
			
			int playerId = m_PlayableManager.GetPlayerByPlayable(playable.GetRplId());
			int playerAdded = 0;
			int playerAddedMax = 1;
			int playerAddedLocked = 0;
			if (playerId >= 0)
				playerAdded = 1;
			if (playerId == -2)
				playerAddedLocked = 1;
			
			SCR_Faction faction = playable.GetFaction();
			if (!factions.Contains(faction))
				//DRG_BUG
				factions.Insert(faction, new Tuple3<int, int, int>(playerAdded, playerAddedMax, playerAddedLocked));
			else
			{
				Tuple3<int, int, int> tuple = factions.Get(faction);
				tuple.param1 += playerAdded;
				tuple.param2 += playerAddedMax;
				tuple.param3 += playerAddedLocked;
			}
		}
		
		foreach (RplId rplId, PS_PlayableVehicleContainer playableVehicleContainer : playableVehicles)
		{
			AddPlayableVehicle(playableVehicleContainer);
		}
		
		foreach (SCR_Faction faction, Tuple3<int, int, int> count : factions)
		{
			AddFaction(faction, count.param1, count.param2, count.param3);
		}
		
		// Added in runtime
		m_PlayableManager.GetOnPlayableRegistered().Remove(OnPlayableRegistered);
		m_PlayableManager.GetOnPlayableRegistered().Insert(OnPlayableRegistered);
	}
	
	void InitPlayers()
	{
		m_PlayersList.SetLobbyMenu(this);
		m_PlayersList.InitPlayers();
	}
	
	// --------------------------------------------------------------------------------------------------------------------------------
	// Add
	void AddFaction(SCR_Faction faction, int count, int maxCount, int lockedCount)
	{
		Widget factionSelectorRoot = m_wWorkspaceWidget.CreateWidgets(m_sFactionSelectorPrefab, m_wFactionList);
		PS_FactionSelector factionSelector = PS_FactionSelector.Cast(factionSelectorRoot.FindHandler(PS_FactionSelector));
		factionSelector.SetFaction(faction);
		factionSelector.SetCount(count);
		factionSelector.SetMaxCount(maxCount);
		factionSelector.SetLockedCount(lockedCount);
		factionSelector.SetCoopLobby(this);
		factionSelector.SetToggled(m_CurrentFaction == faction);
		m_mFactions.Insert(faction, factionSelector);
	}
	
	void AddPlayableVehicle(PS_PlayableVehicleContainer playableVehicleContainer)
	{
		SCR_AIGroup playableGroup = m_PlayableManager.GetPlayerGroupByVehicle(playableVehicleContainer);
		PS_RolesGroup rolesGroup;
		if (!m_mGroups.Contains(playableGroup))
		{
			return;
		}
		else rolesGroup = m_mGroups.Get(playableGroup);
		rolesGroup.InsertVehicle(playableVehicleContainer);
	}
	
	void AddPlayable(PS_PlayableContainer playable)
	{
		SCR_AIGroup playableGroup = m_PlayableManager.GetPlayerGroupByPlayable(playable.GetRplId());
		PS_RolesGroup rolesGroup;
		if (!m_mGroups.Contains(playableGroup))
		{
			Widget rolesGroupRoot = m_wWorkspaceWidget.CreateWidgets(m_sRolesGroupPrefab, m_wRolesList);
			rolesGroup = PS_RolesGroup.Cast(rolesGroupRoot.FindHandler(PS_RolesGroup));
			rolesGroup.SetLobbyMenu(this);
			rolesGroup.SetAIGroup(playableGroup);
			m_mGroups.Insert(playableGroup, rolesGroup);
		
			SCR_Faction faction = playable.GetFaction();
			rolesGroupRoot.SetVisible(m_CurrentFaction == faction);
		}
		else rolesGroup = m_mGroups.Get(playableGroup);
		rolesGroup.InsertPlayable(playable);
	}
	
	void AddPlayer(int playerId)
	{
		
	}
	
	void AddFactionCount(SCR_Faction faction, int added, int addedMax, int addedLocked = 0)
	{
		if (!m_mFactions.Contains(faction))
			AddFaction(faction, 0, 0, 0);
		PS_FactionSelector factionSelector = m_mFactions.Get(faction);
		int count = factionSelector.GetCount();
		int maxCount = factionSelector.GetMaxCount();
		int lockedCount = factionSelector.GetLockedCount();
		factionSelector.SetCount(count + added);
		factionSelector.SetMaxCount(maxCount + addedMax);
		factionSelector.SetLockedCount(lockedCount + addedLocked);
		if ((maxCount + addedMax) < 1)
		{
			factionSelector.GetRootWidget().RemoveFromHierarchy();
			m_mFactions.Remove(faction);
		}
	}
	
	// --------------------------------------------------------------------------------------------------------------------------------
	// Buttons
	void OnClickedPreviewHide(SCR_ButtonBaseComponent factionButton)
	{
		m_wLoadoutPreviewBody.SetVisible(!m_wLoadoutPreviewBody.IsVisible());
	}
	
	void OnClickedPlayersSwitch(SCR_ButtonBaseComponent factionButton)
	{
		m_wVoiceChatFrame.SetVisible(false);
		m_wPlayersSearch.SetVisible(true);
		m_VoiceSwitchButtonComponent.SetToggled(false);
	}
	
	void OnClickedVoiceSwitch(SCR_ButtonBaseComponent factionButton)
	{
		m_wVoiceChatFrame.SetVisible(true);
		m_wPlayersSearch.SetVisible(false);
		m_PlayersSwitchButtonComponent.SetToggled(false);
	}
	
	void OnClickedRolesFold(SCR_ButtonBaseComponent rolesFold)
	{
		bool fold = false;
		foreach (PS_RolesGroup rolesGroup : m_aDeployGroups)
		{
			if (!rolesGroup || !rolesGroup.GetRootWidget().IsVisible())
				continue;
			if (!rolesGroup.IsFolded())
			{
				fold = true;
				break;
			}
		}
		foreach (PS_RolesGroup rolesGroup : m_aDeployGroups)
		{
			if (!rolesGroup || !rolesGroup.GetRootWidget().IsVisible())
				continue;
			rolesGroup.SetFolded(fold);
		}
	}
	void OnRolesFold(PS_RolesGroup _rolesGroup)
	{
		bool fold = false;
		foreach (PS_RolesGroup rolesGroup : m_aDeployGroups)
		{
			if (!rolesGroup || !rolesGroup.GetRootWidget().IsVisible())
				continue;
			if (!rolesGroup.IsFolded())
			{
				fold = true;
				break;
			}
		}
		if (fold)
			m_wRolesFoldButtonImage.LoadImageFromSet(0, IMAGESET_PS, "Fold");
		else
			m_wRolesFoldButtonImage.LoadImageFromSet(0, IMAGESET_PS, "Unfold");
	}
	
	void OnPlayableRegistered(RplId playableId, PS_PlayableContainer playable)
	{
		SCR_Faction faction = playable.GetFaction();
		if (!m_mFactions.Contains(faction))
		{
			AddFaction(faction, 0, 1, 0);
		}
		else
			AddFactionCount(faction, 0, 1, 0);
		AddPlayable(playable);
	}
	
	void OnRolesGroupRemoved(PS_RolesGroup rolesGroup)
	{
		m_mGroups.Remove(m_mGroups.GetKeyByValue(rolesGroup));
	}
	
	void OnPlayableRemoved(PS_PlayableContainer playable)
	{
		SCR_Faction faction = playable.GetFaction();
		
		int playerId = m_PlayableManager.GetPlayerByPlayable(playable.GetRplId());
		int playerAdded = 0;
		int lockedAdded = 0;
		if (playerId >= 0)
			playerAdded = -1;
		if (playerId == -2)
			lockedAdded = -1;
		
		AddFactionCount(faction, playerAdded, -1, lockedAdded);
	}
	
	void OnPlayerRemoved(int playerId)
	{
		if (playerId != m_iSelectedPlayer)
			return;
		
		m_iSelectedPlayer = m_iPlayerId;
		m_PlayersList.SetSelectedPlayer(m_iPlayerId);
	}
	
	void OnPlayersSearchChanged(SCR_EditBoxComponent editBoxComponent, string text)
	{
		m_PlayersList.SetSearchText(text);
	}
	
	void OnPlayersSearchWriteModeEnter()
	{
		m_wScreenButton.SetVisible(true);
	}
	
	void OnPlayersSearchWriteModeLeave(string searchText)
	{
		m_wScreenButton.SetVisible(false);
	}
	
	
	void SwitchCurrentFaction(SCR_Faction faction)
	{
		if (m_CurrentFaction != faction)
		{
			if (m_CurrentFaction && m_mFactions.Get(m_CurrentFaction))
				m_mFactions.Get(m_CurrentFaction).SetToggled(false);
			m_CurrentFaction = faction;
		}
		else
			m_CurrentFaction = null;
		
		foreach (SCR_AIGroup aiGroup, PS_RolesGroup rolesGroup : m_mGroups)
		{
			if (!aiGroup)
				continue;
			SCR_Faction groupFaction = SCR_Faction.Cast(aiGroup.GetFaction());
			bool factionSelected = m_CurrentFaction == groupFaction;
			rolesGroup.GetRootWidget().SetVisible(factionSelected);
		}
		foreach (PS_RolesGroup deployGroup : m_aDeployGroups)
		{
			if (!deployGroup)
				continue;
			deployGroup.GetRootWidget().SetVisible(m_CurrentFaction == deployGroup.GetDeployFaction());
		}
		
		m_wRolesScroll.SetSliderPos(0, 0);
		OnRolesFold(null);
	}
	
	void SetPreviewPlayableVehicle(PS_PlayableVehicleContainer playableVehicleContainer, bool openInventory)
	{
		m_LobbyLoadoutPreview.SetPreviewPlayableVehicle(playableVehicleContainer, openInventory);
	}
	// Reforger Lobby Conflict Edition: preview a loadout-palette entry directly from its character prefab.
	void SetPreviewLoadout(ResourceName prefabName, string displayName)
	{
		m_LobbyLoadoutPreview.SetPreviewLoadout(prefabName, displayName);
	}
	void SetPreviewPlayable(RplId playableId, bool openInventory)
	{
		if (!playableId.IsValid())
		{
			playableId = m_PlayableManager.GetPlayableByPlayer(m_iPlayerId);
			if (playableId == RplId.Invalid())
				return;
		}
		string prefabName = m_PlayableManager.GetPlayablePrefab(playableId);
		m_LobbyLoadoutPreview.SetPreviewPlayable(playableId, prefabName, openInventory);
	}
	
	void UpdatePlayerFaction(int playerId, FactionKey factionKey, FactionKey factionKeyOld)
	{
		if (playerId != m_iPlayerId)
			return;
		
		SwitchVoiceChatFaction(factionKey);
	}
	
	void SwitchVoiceChatFaction(FactionKey factionKey)
	{
		m_VoiceChatList.SwitchFaction(factionKey);
	}
	
	
	void OnPlayerConnected(int playerId)
	{
		m_wPlayersCounter.SetTextFormat("%1/%2", m_PlayerManager.GetPlayerCount(), m_PlayableManager.GetMaxPlayers());
	}
	
	void OnPlayerDisconnected(int playerId)
	{
		m_wPlayersCounter.SetTextFormat("%1/%2", m_PlayerManager.GetPlayerCount(), m_PlayableManager.GetMaxPlayers());
	}
	
	void OnStartTimerCounterChanged(int timer)
	{
		if (timer == -1)
		{
			m_wOverlayCounter.SetVisible(false);
		} else {
			m_wOverlayCounter.SetVisible(true);
			m_wTextCounter.SetText(timer.ToString());
			SCR_UISoundEntity.SoundEvent("SOUND_RADIO_FREQUENCY_CYCLE");
		}
	}
	
	// --------------------------------------------------------------------------------------------------------------------------------
	// Set
	void SetSelectedPlayer(int playerId)
	{
		m_iSelectedPlayer = playerId;
		m_PlayersList.SetSelectedPlayer(playerId);
		m_VoiceChatList.SetSelectedPlayer(playerId);
	}
	
	// --------------------------------------------------------------------------------------------------------------------------------
	// Get
	int GetSelectedPlayer()
	{
		return m_iSelectedPlayer;
	}
	
	// --------------------------------------------------------------------------------------------------------------------------------
	// Actions
	void Action_Ready()
	{
		// Reforger Lobby Conflict Edition: the global game-mode state is permanently GAME, so we key off whether
		// THIS player is already deployed (has a playable) rather than the global state.
		RplId playableId = m_PlayableManager.GetPlayableByPlayer(m_iPlayerId);
		if (playableId != RplId.Invalid())
		{
			// Already deployed -> just return to the controlled character.
			m_PlayableControllerComponent.SwitchToMenu(SCR_EGameModeState.GAME);
			return;
		}

		// Not deployed -> deploy the requesting player at their selected group + loadout + spawn.
		// The server validates the selection (PS_GameModeCoop.RequestDeploy).
		m_PlayableControllerComponent.RequestDeploy();
	}
	
	// Direct
	void Action_LobbyVoNOn()
	{
		m_PlayableControllerComponent.LobbyVoNEnable();
	}
	void Action_LobbyVoNOff()
	{
		m_PlayableControllerComponent.LobbyVoNDisable();
	}
	// Channel
	void Action_LobbyVoNChannelOn()
	{
		m_PlayableControllerComponent.LobbyVoNRadioEnable();
	}
	void Action_LobbyVoNChannelOff()
	{
		m_PlayableControllerComponent.LobbyVoNDisable();
	}
	
	void Action_ChatOpen()
	{
		// Delay is esential, if any character already controlled
		GetGame().GetCallqueue().CallLater(ChatWrap, 0);
	}
	void ChatWrap()
	{
		SCR_ChatPanelManager.GetInstance().OpenChatPanel(m_ChatPanel);
	}
	
	void Action_Exit()
	{
		// For some strange reason players all the time accidentally exit game, maybe jus open pause menu
		//GameStateTransitions.RequestGameplayEndTransition();  
		//Close();
		
		// Reforger Lobby Conflict Edition: on the Conflict base the global state is permanently GAME, so the old
		// "state == GAME -> Close()" check closed the lobby on every ESC. The vanilla pause menu then
		// opened over the bare observer camera and pressing Continue left the player with no lobby.
		// The lobby is only shown to UNDEPLOYED players, so ESC must open the pause menu ON TOP of the
		// lobby and leave it intact. Only close (hand control back to the world) when the player has a
		// deployed playable.
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		if (playableManager && m_PlayerController
			&& playableManager.GetPlayableByPlayer(m_PlayerController.GetPlayerId()) != RplId.Invalid())
		{
			Close();
			return;
		}
		
		GetGame().GetCallqueue().CallLater(OpenPauseMenuWrap, 0); //  Else menu auto close itself
	}
	void OpenPauseMenuWrap()
	{
		ArmaReforgerScripted.OpenPauseMenu();
	}
}





























