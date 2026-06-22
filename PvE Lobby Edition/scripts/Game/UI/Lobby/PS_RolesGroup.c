// Widget displays info about group witch contains any playable character.
// Contains PS_CharacterSelector widgets for each playable members of group.
// Path: {3F761F63F1DF29D1}UI/Lobby/CharacterSelector.layout
// Part of Lobby menu PS_CoopLobby ({9DECCA625D345B35}UI/Lobby/CoopLobby.layout)

class PS_RolesGroup : SCR_ScriptedWidgetComponent
{
	// Const
	protected ResourceName m_sCharacterSelectorPrefab = "{3F761F63F1DF29D1}UI/Lobby/CharacterSelector.layout";
	protected ResourceName m_sVehicleSelectorPrefab = "{1B4B23FBA92940C9}UI/Lobby/VehicleSelector.layout";
	protected ResourceName m_sMembersCounterPrefab = "{6C1B127A14DD2C58}UI/Lobby/MemberCounterLabel.layout";
	protected ResourceName m_sImageSet = "{2EFEA2AF1F38E7F0}UI/Textures/Icons/icons_wrapperUI-64.imageset";
	protected ResourceName m_sImageSetPS = "{F3A9B47F55BE8D2B}UI/Textures/Icons/PS_Atlas_x64.imageset";
	
	// Cache global
	protected PS_GameModeCoop m_GameModeCoop;
	protected PS_PlayableManager m_PlayableManager;
	protected WorkspaceWidget m_wWorkspaceWidget;
	protected PlayerController m_PlayerController;
	protected PS_PlayableControllerComponent m_PlayableControllerComponent;
	protected int m_iCurrentPlayerId;
	
	// Widgets
	protected VerticalLayoutWidget m_wCharactersList;
	protected HorizontalLayoutWidget m_wVehiclesList;
	protected ButtonWidget m_wLockButton;
	protected ImageWidget m_wLockImage;
	protected ButtonWidget m_wVoiceJoinButton;
	protected TextWidget m_wRolesGroupName;
	protected TextWidget m_wRolesGroupNameCustom;
	protected ImageWidget m_wGroupFactionColor;
	protected ImageWidget m_wGroupFlagImage;
	protected VerticalLayoutWidget m_wList;
	protected ButtonWidget m_wRolesGroupButton;
	protected HorizontalLayoutWidget m_wMembersHorizontalLayout;
	
	// Handlers
	SCR_ButtonBaseComponent m_LockButtonBaseComponent;
	SCR_ButtonBaseComponent m_VoiceJoinButton;
	PS_ButtonComponent m_RolesGroupButton;
	
	// Vars
	protected PS_CoopLobby m_CoopLobby;
	protected SCR_AIGroup m_AIGroup;
	protected int m_iGroupCallsign;
	protected string m_sGroupCallsign;
	protected FactionKey m_sFactionKey;
	protected bool m_bInnerButtonClicked;
	
	protected int m_iCharactersCount;
	protected int m_iLockedCount;
	protected int m_iPlayersCount;

	// Reforger Lobby Conflict Edition deploy mode: this widget represents a generated group (capacity + loadout palette).
	protected bool m_bDeployMode;
	protected int m_iGroupID;
	protected int m_iCapacity;
	protected string m_sGroupDisplayName;
	protected string m_sGroupDescription;
	protected SCR_Faction m_DeployFaction;
	// Reforger Lobby Conflict Edition: this widget is a spawn-point section (buttons = world spawn points).
	protected bool m_bSpawnSection;
	ref array<PS_CharacterSelector> m_aLoadoutSelectors = {};
	
	ref map<PS_PlayableContainer, PS_CharacterSelector> m_mCharacters = new map<PS_PlayableContainer, PS_CharacterSelector>();
	ref map<ResourceName, PS_MembersCounter> m_mMembers = new map<ResourceName, PS_MembersCounter>();
	ref map<PS_PlayableVehicleContainer, PS_VehicleSelector> m_mVehicles = new map<PS_PlayableVehicleContainer, PS_VehicleSelector>();
		
	// --------------------------------------------------------------------------------------------------------------------------------
	// Init
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		
		if (!GetGame().InPlayMode())
			return;
		
		// Cache global
		m_GameModeCoop = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		m_PlayableManager = PS_PlayableManager.GetInstance();
		m_wWorkspaceWidget = GetGame().GetWorkspace();
		m_PlayerController = GetGame().GetPlayerController();
		m_PlayableControllerComponent = PS_PlayableControllerComponent.Cast(m_PlayerController.FindComponent(PS_PlayableControllerComponent));
		m_iCurrentPlayerId = m_PlayerController.GetPlayerId();
		
		// Widgets
		m_wCharactersList = VerticalLayoutWidget.Cast(w.FindAnyWidget("CharactersList"));
		m_wVehiclesList = HorizontalLayoutWidget.Cast(w.FindAnyWidget("VehiclesList"));
		m_wLockButton = ButtonWidget.Cast(w.FindAnyWidget("LockButton"));
		m_wLockImage = ImageWidget.Cast(w.FindAnyWidget("LockImage"));
		m_wVoiceJoinButton = ButtonWidget.Cast(w.FindAnyWidget("VoiceJoinButton"));
		m_wRolesGroupName = TextWidget.Cast(w.FindAnyWidget("RolesGroupName"));
		m_wRolesGroupNameCustom = TextWidget.Cast(w.FindAnyWidget("RolesGroupNameCustom"));
		m_wGroupFactionColor = ImageWidget.Cast(w.FindAnyWidget("GroupFactionColor"));
		m_wGroupFlagImage = ImageWidget.Cast(w.FindAnyWidget("GroupFlagImage"));
		// Hidden by default; shown only when a group preset provides a flag (see SetDeploymentGroup).
		if (m_wGroupFlagImage)
			m_wGroupFlagImage.SetVisible(false);
		m_wList = VerticalLayoutWidget.Cast(w.FindAnyWidget("List"));
		m_wRolesGroupButton = ButtonWidget.Cast(w.FindAnyWidget("RolesGroupButton"));
		m_wMembersHorizontalLayout = HorizontalLayoutWidget	.Cast(w.FindAnyWidget("MembersHorizontalLayout"));
		
		// Handlers
		m_LockButtonBaseComponent = SCR_ButtonBaseComponent.Cast(m_wLockButton.FindHandler(SCR_ButtonBaseComponent));
		m_VoiceJoinButton = SCR_ButtonBaseComponent.Cast(m_wVoiceJoinButton.FindHandler(SCR_ButtonBaseComponent));
		m_RolesGroupButton = PS_ButtonComponent.Cast(m_wRolesGroupButton.FindHandler(PS_ButtonComponent));
		
		m_RolesGroupButton.GetOnClick_PS().Insert(OnClicked);
	}
	
	// --------------------------------------------------------------------------------------------------------------------------------
	// Set
	void SetAIGroup(SCR_AIGroup aiGroup)
	{
		m_AIGroup = aiGroup;
		if (!aiGroup)
			return;
		m_iGroupCallsign = aiGroup.GetCallsignNum();
		m_sGroupCallsign = m_iGroupCallsign.ToString();
		SCR_Faction faction = SCR_Faction.Cast(aiGroup.GetFaction());
		m_sFactionKey = faction.GetFactionKey();
		
		 m_wRoot.SetZOrder(aiGroup.GetCallsignNum());
		
		// Init
		m_wRolesGroupName.SetText(PS_GroupHelper.GetGroupName(m_AIGroup));
		UpdateCustomName();
		
		m_wGroupFactionColor.SetColor(faction.GetFactionColor());
	}
	
	// --------------------------------------------------------------------------------------------------------------------------------
	// Reforger Lobby Conflict Edition deploy mode
	// Built purely from the authored preset + faction (no live SCR_AIGroup entity needed,
	// avoids any dependency on group-entity replication timing in the lobby).
	void SetDeploymentGroup(int groupID, SCR_Faction faction, int capacity, string groupName, string description, ResourceName groupFlag)
	{
		m_bDeployMode = true;
		m_iGroupID = groupID;
		m_iCapacity = capacity;
		m_sGroupDisplayName = groupName;
		m_sGroupDescription = description;
		m_DeployFaction = faction;
		if (faction)
		{
			m_sFactionKey = faction.GetFactionKey();
			m_wGroupFactionColor.SetColor(faction.GetFactionColor());
		}
		m_wRolesGroupName.SetText(groupName);
		// Reforger Lobby Conflict Edition: show the authored group flag patch next to the name when present.
		if (m_wGroupFlagImage)
		{
			if (!groupFlag.IsEmpty())
			{
				m_wGroupFlagImage.LoadImageTexture(0, groupFlag);
				m_wGroupFlagImage.SetVisible(true);
			}
			else
				m_wGroupFlagImage.SetVisible(false);
		}
		UpdateDeployCounter();
	}

	int GetGroupID()
	{
		return m_iGroupID;
	}

	SCR_Faction GetDeployFaction()
	{
		return m_DeployFaction;
	}

	bool IsSpawnSection()
	{
		return m_bSpawnSection;
	}

	// Reforger Lobby Conflict Edition: header-only section listing the faction's world spawn points.
	void SetSpawnSection(SCR_Faction faction, string title)
	{
		m_bDeployMode = true;
		m_bSpawnSection = true;
		m_DeployFaction = faction;
		if (faction)
		{
			m_sFactionKey = faction.GetFactionKey();
			m_wGroupFactionColor.SetColor(faction.GetFactionColor());
		}
		m_wRolesGroupName.SetText(title);
		m_wRolesGroupNameCustom.SetText("");
		// The dedicated Spawn Points column has its own header, so hide this row's foldable
		// header rectangle (the empty open/close button above the spawn list).
		if (m_wRolesGroupButton)
			m_wRolesGroupButton.SetVisible(false);
	}

	void InsertLoadout(int groupID, ResourceName loadoutResource, SCR_Faction faction, string displayName, ResourceName iconResource)
	{
		Widget characterSelectorRoot = m_wWorkspaceWidget.CreateWidgets(m_sCharacterSelectorPrefab, m_wCharactersList);
		PS_CharacterSelector characterSelector = PS_CharacterSelector.Cast(characterSelectorRoot.FindHandler(PS_CharacterSelector));
		characterSelector.SetLobbyMenu(m_CoopLobby);
		characterSelector.SetRolesGroup(this);
		characterSelector.SetLoadout(groupID, loadoutResource, faction, displayName, iconResource);
		m_aLoadoutSelectors.Insert(characterSelector);
	}

	void InsertSpawnPoint(RplId spawnRplId, SCR_Faction faction, string displayName)
	{
		Widget characterSelectorRoot = m_wWorkspaceWidget.CreateWidgets(m_sCharacterSelectorPrefab, m_wCharactersList);
		PS_CharacterSelector characterSelector = PS_CharacterSelector.Cast(characterSelectorRoot.FindHandler(PS_CharacterSelector));
		characterSelector.SetLobbyMenu(m_CoopLobby);
		characterSelector.SetRolesGroup(this);
		characterSelector.SetSpawnPoint(spawnRplId, faction, displayName);
		m_aLoadoutSelectors.Insert(characterSelector);
	}

	// Reforger Lobby Conflict Edition: refresh the selected-highlight of all loadout/spawn buttons in this row,
	// and the group occupancy counter "(x/y)" (players who currently have this group selected).
	void RefreshDeploySelection()
	{
		UpdateDeployCounter();
		foreach (PS_CharacterSelector selector : m_aLoadoutSelectors)
		{
			if (selector)
				selector.RefreshDeploySelection();
		}
	}

	void UpdateDeployCounter()
	{
		if (m_bSpawnSection)
			return;
		int selected = m_PlayableManager.GetGroupSelectedCount(m_iGroupID);
		string counter;
		if (m_iCapacity <= 0) // Group Size 0 = unlimited
			counter = "(" + selected + ")";
		else
			counter = "(" + selected + "/" + m_iCapacity + ")";
		// Second line under the group name shows the authored group description + occupancy.
		string line = m_sGroupDescription;
		if (line != "")
			line = line + " " + counter;
		else
			line = counter;
		m_wRolesGroupNameCustom.SetText(line);
	}

	// --------------------------------------------------------------------------------------------------------------------------------
	void UpdateCustomName()
	{
		string customName = PS_GroupHelper.GetGroupNameCustom(m_AIGroup);
		if (customName == "")
			customName = "#PS_Lobby_DefaultSquadCustom";
		if (m_iCharactersCount - m_iLockedCount == 0)
			customName += " (#PS_Lobby_Locked)";
		else
			customName += " (" + (m_iPlayersCount) + "/" + (m_iCharactersCount - m_iLockedCount) + ")";
		m_wRolesGroupNameCustom.SetText(customName);
	}
	
	// --------------------------------------------------------------------------------------------------------------------------------
	void SetLobbyMenu(PS_CoopLobby coopLobby)
	{
		m_CoopLobby = coopLobby;
	}
	
	// --------------------------------------------------------------------------------------------------------------------------------
	// Update
	void UpdateLockedState(bool add)
	{
		if (add)
			m_iLockedCount++;
		else
			m_iLockedCount--;
		
		if (m_mCharacters.Count() == m_iLockedCount)
			m_wLockImage.LoadImageFromSet(0, m_sImageSet, "server-locked");
		else
			m_wLockImage.LoadImageFromSet(0, m_sImageSet, "server-unlocked");
		
		UpdateCustomName();
	}
	
	// --------------------------------------------------------------------------------------------------------------------------------
	// Add
	void InsertVehicle(PS_PlayableVehicleContainer vehicle)
	{
		Widget vehicleSelectorRoot = m_wWorkspaceWidget.CreateWidgets(m_sVehicleSelectorPrefab, m_wVehiclesList);
		PS_VehicleSelector vehicleSelector = PS_VehicleSelector.Cast(vehicleSelectorRoot.FindHandler(PS_VehicleSelector));
		vehicleSelector.SetLobbyMenu(m_CoopLobby);
		vehicleSelector.SetRolesGroup(this);
		vehicleSelector.SetMembersCounter(AddMember(vehicle.GetIconPath()));
		vehicleSelector.SetVehicle(vehicle);
		m_mVehicles.Insert(vehicle, vehicleSelector);
	}
	
	void InsertPlayable(PS_PlayableContainer playable)
	{
		Widget characterSelectorRoot = m_wWorkspaceWidget.CreateWidgets(m_sCharacterSelectorPrefab, m_wCharactersList);
		PS_CharacterSelector characterSelector = PS_CharacterSelector.Cast(characterSelectorRoot.FindHandler(PS_CharacterSelector));
		characterSelector.SetLobbyMenu(m_CoopLobby);
		characterSelector.SetRolesGroup(this);
		characterSelector.SetPlayable(playable);
		m_mCharacters.Insert(playable, characterSelector);
		
		//AddMember("{66C8FC843E6E3F49}UI/Textures/Icons/Infantry.edds");
		
		if (m_PlayableManager.GetPlayerByPlayable(playable.GetRplId()) == -2)
			m_iLockedCount++;
		else if (m_PlayableManager.GetPlayerByPlayable(playable.GetRplId()) > 0)
			m_iPlayersCount++;
		if (m_mCharacters.Count() == m_iLockedCount)
			m_wLockImage.LoadImageFromSet(0, m_sImageSet, "server-locked");
		else
			m_wLockImage.LoadImageFromSet(0, m_sImageSet, "server-unlocked");
		
		playable.GetOnPlayerChange().Insert(OnPlayablePlayerChange);
		m_iCharactersCount++;
		UpdateCustomName();
	}
	
	PS_MembersCounter AddMember(ResourceName memberIcon)
	{
		if (!m_mMembers.Contains(memberIcon))
		{
			Widget membersWidget = m_wWorkspaceWidget.CreateWidgets(m_sMembersCounterPrefab, m_wMembersHorizontalLayout);
			PS_MembersCounter membersCounter = PS_MembersCounter.Cast(membersWidget.FindHandler(PS_MembersCounter));
			membersCounter.SetIcon(memberIcon);
			m_mMembers[memberIcon] = membersCounter;
		}
		return m_mMembers[memberIcon];
	}
	
	// --------------------------------------------------------------------------------------------------------------------------------
	bool IsFolded()
	{
		return !m_wList.IsVisible();
	}
	void SetFolded(bool folded)
	{
		m_wList.SetVisible(!folded);
		m_CoopLobby.OnRolesFold(this);
	}
	
	// --------------------------------------------------------------------------------------------------------------------------------
	void OnPlayablePlayerChange(int oldPlayerId, int playerId)
	{
		if (playerId <= 0 && oldPlayerId > 0)
			m_iPlayersCount--;
		else if (playerId > 0 && oldPlayerId <= 0)
			m_iPlayersCount++;
		UpdateCustomName();
	}
	
	// --------------------------------------------------------------------------------------------------------------------------------
	// Buttons
	void OnClicked(PS_ButtonComponent buttonComponent, int x, int y, int button)
	{
		if (button == 1)
		{
			OpenContext();
			return;
		}
		if (button != 0)
			return;
		
		OnClickedLeft();
		return;
	}
	void OpenContext()
	{
		PS_ContextMenu contextMenu = PS_ContextMenu.CreateContextMenuOnMousePosition(m_CoopLobby.GetRootWidget(), m_wRolesGroupName.GetText());
		
		contextMenu.ActionJoinVoice().Insert(OnClickedVoiceJoin);
		if (PS_PlayersHelper.IsAdminOrServer())
		{
			if (m_iLockedCount != m_mCharacters.Count())
				contextMenu.ActionLock(0).Insert(OnClickedLock);
			else
				contextMenu.ActionUnlock(1).Insert(OnClickedLock);
		}
	}
	
	// --------------------------------------------------------------------------------------------------------------------------------
	// Buttons
	void OnClickedLock(PS_ContextAction contextAction, PS_ContextActionDataPlayable contextActionDataPlayable)
	{
		m_bInnerButtonClicked = true;
		if (!PS_PlayersHelper.IsAdminOrServer())
			return;
		
		bool unlock = contextActionDataPlayable.GetPlayableId();
		
		foreach (PS_PlayableContainer playable, PS_CharacterSelector characterSelector : m_mCharacters)
		{
			int playerId = characterSelector.GetPlayerId();
			if (unlock)
			{
				if (playerId == -2)
				{
					m_PlayableControllerComponent.SetPlayablePlayer(playable.GetRplId(), -1);
				}
			}
			else
			{
				if (playerId != -2)
				{
					PS_EPlayableControllerState state = m_PlayableManager.GetPlayerState(playerId);
					if (state == PS_EPlayableControllerState.Ready)
						m_PlayableControllerComponent.SetPlayerState(playerId, PS_EPlayableControllerState.NotReady);
					m_PlayableControllerComponent.SetPlayablePlayer(playable.GetRplId(), -2);
				}
			}
		}
		
		foreach (PS_PlayableVehicleContainer vehicle, PS_VehicleSelector vehicleSelector : m_mVehicles)
		{
			if (unlock)
				vehicleSelector.UnlockVehicle(null, new PS_ContextActionDataPlayable(vehicle.GetRplId()));
			else
				vehicleSelector.LockVehicle(null, new PS_ContextActionDataPlayable(vehicle.GetRplId()));
		}
		
		if (unlock) {
			m_wLockImage.LoadImageFromSet(0, m_sImageSet, "server-unlocked");
			m_iLockedCount = 0;
			SCR_UISoundEntity.SoundEvent("SOUND_FE_BUTTON_FILTER_OFF");
		} else {
			m_wLockImage.LoadImageFromSet(0, m_sImageSet, "server-locked");
			m_iLockedCount = m_mCharacters.Count();
			SCR_UISoundEntity.SoundEvent("SOUND_FE_BUTTON_FILTER_ON");
		}
	}
	
	// --------------------------------------------------------------------------------------------------------------------------------
	void OnClickedVoiceJoin(PS_ContextAction contextAction, PS_ContextActionData contextActionData)
	{
		m_bInnerButtonClicked = true;
		SCR_EGameModeState gameState = m_GameModeCoop.GetState();
		if (gameState == SCR_EGameModeState.SLOTSELECTION)
			m_PlayableControllerComponent.MoveToVoNRoom(m_iCurrentPlayerId, m_sFactionKey, m_sGroupCallsign);
	}
	
	// --------------------------------------------------------------------------------------------------------------------------------
	void OnClickedLeft()
	{
		if (m_bInnerButtonClicked)
		{
			m_bInnerButtonClicked = false;
			return;
		}
		SetFolded(!IsFolded());
	}
	
	// --------------------------------------------------------------------------------------------------------------------------------
	// Removed
	void OnPlayableRemoved(PS_PlayableContainer playable)
	{
		m_mCharacters.Remove(playable);
		if (!m_wCharactersList.GetChildren())
		{
			m_wRoot.RemoveFromHierarchy();
			m_CoopLobby.OnRolesGroupRemoved(this);
		}
		playable.GetOnPlayerChange().Remove(OnPlayablePlayerChange);
		
		m_iCharactersCount--;
		UpdateCustomName();
	}
}




























