// Reforger Lobby Conflict Edition: add a simple, directly-editable display-name field on spawn points so admins
// can label them in the deployment spawn list without having to configure a full SCR_UIInfo
// "Info" object. Read by PS_CoopLobby.AddSpawnPointsSection (with fallbacks).
modded class SCR_SpawnPoint : SCR_Position
{
	[Attribute("", UIWidgets.EditBox, "PvE lobby: custom display name shown in the deployment spawn list.")]
	protected string m_sPS_LobbyDisplayName;

	string PS_GetLobbyDisplayName()
	{
		return m_sPS_LobbyDisplayName;
	}
}
