modded class SCR_RespawnSystemComponentClass : RespawnSystemComponentClass
{
}

//! Scripted implementation that handles spawning and respawning of players.
//! Should be attached to a GameMode entity.
[ComponentEditorProps(icon: HYBRID_COMPONENT_ICON)]
modded class SCR_RespawnSystemComponent : RespawnSystemComponent
{
	[Attribute(category: "Respawn System")]
	protected ref SCR_SpawnLogic m_SpawnLogic;
	
	[Attribute("1", uiwidget: UIWidgets.CheckBox, category: "Respawn System")]
	protected bool m_bEnableRespawn;
	
	[Attribute("1", desc: "Handles visibility of the Respawn button in Pause menu.", category: "Respawn System")]
	protected bool m_bEnablePauseMenuRespawn;

	[Attribute("{A1CE9D1EC16DA9BE}UI/layouts/Menus/MainMenu/SplashScreen.layout", desc: "Optional: Layout shown during world load completion and respawn menu opening or automatically completing.", category: "Respawn System")]
	protected ResourceName m_sLoadingLayout;

	[Attribute("{A39BE59EB6F41125}Configs/Respawn/SpawnPointRequestResultInfoConfig.conf", desc: "Holds a config of all reasons why a specific spawnpoint can be disabled", category: "Respawn System")]
	protected ResourceName m_sSpawnPointRequestResultInfoHolder;

	protected ref SCR_SpawnPointRequestResultInfoConfig m_SpawnPointRequestResultInfoHolder;

	// Instance of this component
	private static SCR_RespawnSystemComponent s_Instance = null;

	// The parent of this entity which should be a gamemode
	protected SCR_BaseGameMode m_pGameMode;

	// Parent entity's rpl component
	protected RplComponent m_RplComponent;

	// Preload
	protected ref SimplePreload m_Preload;
	protected ref ScriptInvoker Event_OnRespawnEnabledChanged;
	
	protected Widget m_wLoadingPlaceholder;
	protected SCR_LoadingSpinner m_LoadingSpinner;

	//------------------------------------------------------------------------------------------------
	//! \param[in] requestComponent
	//! \param[in] response
	//! \param[in] data
	//! \return
	override SCR_BaseSpawnPointRequestResultInfo GetSpawnPointRequestResultInfo(SCR_SpawnRequestComponent requestComponent, SCR_ESpawnResult response, SCR_SpawnData data){	return null;	} 
	// Reforger Lobby Conflict Edition: the pause-menu "Respawn" must work so a deployed player can send themselves
	// back to the deployment lobby (PROJECT.md #4). The vanilla pause menu gates that button on
	// GetInstance() + IsPauseMenuRespawnEnabled(); returning null/false here made the button dead
	// ("clicking Respawn -> Confirm does nothing"). Restore the lookup and enable the flags. The
	// actual respawn is a suicide (SCR_RespawnComponent.RequestPlayerSuicide -> ForceDeath) which
	// trips PS_PlayableComponent.OnDamageStateChange -> PS_GameModeCoop.TryRespawn -> reopen lobby.
	// The vanilla spawn/deploy pipeline stays neutered: its menus are suppressed elsewhere
	// (PS_M_SCR_PlayerDeployMenuHandlerComponent + PS_M_SCR_WelcomeScreenMenu) and the spawn-logic
	// entry points below remain no-ops.
	static override SCR_RespawnSystemComponent GetInstance()
	{
		if (!s_Instance)
		{
			BaseGameMode pGameMode = GetGame().GetGameMode();
			if (pGameMode)
				s_Instance = SCR_RespawnSystemComponent.Cast(pGameMode.FindComponent(SCR_RespawnSystemComponent));
		}
		return s_Instance;
	}
	override RplComponent GetRplComponent()	{return null;}
	static override MenuBase OpenRespawnMenu()	{return null;} 
	static override void CloseRespawnMenu()	{return;} 
	override void ServerSetEnableRespawn(bool enableSpawning){return;}  
	override bool IsRespawnEnabled()	{return true;} 
	override bool IsPauseMenuRespawnEnabled() {return true;} 
	override bool IsFactionChangeAllowed() {return false;} 
	// Return a valid (empty) invoker: now that GetInstance() is restored the pause menu may subscribe
	// to respawn-enabled changes, and inserting into a null invoker would crash.
	override ScriptInvoker GetOnRespawnEnabledChanged()
	{
		if (!Event_OnRespawnEnabledChanged)
			Event_OnRespawnEnabledChanged = new ScriptInvoker();
		return Event_OnRespawnEnabledChanged;
	}
	override void OnPlayerRegistered_S(int playerId) {return;}	
	override void OnInit(IEntity owner) {return;}
	override void OnPlayerAuditSuccess_S(int playerId)	{return;}
	// Reforger Lobby Conflict Edition: respawn/spawn-logic is neutered, so m_SpawnLogic is null. The vanilla
	// OnPlayerDisconnected_S forwards to m_SpawnLogic and crashes (NULL pointer) on disconnect.
	override void OnPlayerDisconnected_S(int playerId, KickCauseCode cause, int timeout) {return;}
}
