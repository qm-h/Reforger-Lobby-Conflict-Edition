modded class SCR_PingEditorComponent
{
	override protected void ReceivePing(int reporterID, bool reporterInEditor, SCR_EditableEntityComponent reporterEntity, bool unlimitedOnly, vector position, RplId targetID)
	{
		super.ReceivePing(reporterID, reporterInEditor, reporterEntity, unlimitedOnly, position, targetID);
		
		SCR_EditorManagerEntity manager = GetManager();
		if (!SCR_Global.IsAdmin(manager.GetPlayerID()))
			return;
		
		Rpc(ReceivePingObserver, reporterID, reporterInEditor, unlimitedOnly, position, targetID);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void ReceivePingObserver(int reporterID, bool reporterInEditor, bool unlimitedOnly, vector position, RplId targetID)
	{
		PS_SpectatorMenu spectatorMenu = PS_SpectatorMenu.Cast(GetGame().GetMenuManager().FindMenuByPreset(ChimeraMenuPreset.SpectatorMenu));
		if (!spectatorMenu)
			return;
		
		spectatorMenu.ReceivePing(reporterID, position, targetID);
	}
	
	override void SendPing(bool unlimitedOnly = false, vector position = vector.Zero, SCR_EditableEntityComponent target = null)
	{
		// Reforger Lobby Conflict Edition: ping (et sa popup de message) désactivé dans le lobby de déploiement.
		return;
	}
	
	void PS_SendPing_Custom(bool unlimitedOnly = false, vector position = vector.Zero, SCR_EditableEntityComponent target = null, string customText = "")
	{
		//~ Check if on cooldown. If true send notification informing player
		if (IsPingOnCooldown())
		{
			SCR_NotificationsComponent.SendLocal(ENotification.ACTION_ON_COOLDOWN, m_fCurrentCooldownTime * 100);
			return;
		}
			
		SCR_EditorManagerEntity manager = GetManager();
		if (!manager) 
			return;
		
		//~ If Unlimited only ping
		if (unlimitedOnly)
		{
			//~ Limited editor so cannot send GM only ping
			if (manager.IsLimited())
			{
				SCR_NotificationsComponent.SendLocal(ENotification.EDITOR_GM_ONLY_PING_LIMITED_RIGHTS);
				return;
			}
			//~ Never send editor only ping if editor is not opened
			else if (!manager.IsOpened())
			{
				return;
			}
		}
			
		//~ If no GM do not send out ping
		SCR_PlayerDelegateEditorComponent playerDelegateManager = SCR_PlayerDelegateEditorComponent.Cast(SCR_PlayerDelegateEditorComponent.GetInstance(SCR_PlayerDelegateEditorComponent));
		if (playerDelegateManager && !playerDelegateManager.HasPlayerWithUnlimitedEditor())
		{
			SCR_NotificationsComponent.SendLocal(ENotification.EDITOR_PING_NO_GM_TO_PING);
			return;
		}
		
		int reporterID = manager.GetPlayerID();
		bool reporterInEditor = manager.IsOpened() && !manager.IsLimited();
		
		PS_CustomTextNotificationComponent.GetInstance().SendTextDataInstance(reporterID, customText);
		
		if (target) target.GetPos(position);
		CallEvents(manager, false, reporterID, reporterInEditor, unlimitedOnly, position, target);

		//--- Send the ping to server
		Rpc(PS_SendPingServer_CustomText, unlimitedOnly, position, Replication.FindId(target), customText);
		
		//~ Ping cooldown to prevent spamming
		ActivateCooldown();
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void PS_SendPingServer_CustomText(bool unlimitedOnly, vector position, RplId targetID, string customText)
	{
		SCR_EditorManagerEntity manager = GetManager();
		PS_CustomTextNotificationComponent.SendTextData(manager.GetPlayerID(), customText);
		
		SendPingServer(unlimitedOnly, position, targetID);
	}
}