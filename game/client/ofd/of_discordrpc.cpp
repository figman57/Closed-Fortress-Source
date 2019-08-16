//=============================================================================
//
// Purpose: Discord and Steam Rich Presence support.
//
//=============================================================================

#include "cbase.h"
#include "of_discordrpc.h"
#include "c_team_objectiveresource.h"
#include "tf_gamerules.h"
#include "c_tf_team.h"
#include "c_tf_player.h"
#include "achievementmgr.h"
#include "baseachievement.h"
#include "c_tf_playerresource.h"
#include <inetchannelinfo.h>
#include "discord_rpc.h"
#include "discord_register.h"
#include "tf_gamerules.h"
#include <ctime>
#include "steam/isteammatchmaking.h"
#include "steam/isteamgameserver.h"
#include "steam/isteamfriends.h"
#include "steam/steam_api.h"
#include "tier0/icommandline.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar cl_richpresence_printmsg( "cl_richpresence_printmsg", "0", FCVAR_ARCHIVE, "" );

ConVar of_enable_rpc("of_enable_rpc", "1", FCVAR_ARCHIVE, "Enables/Disables Discord Rich Presence. Requires a game restart.");

//#define DISCORD_LIBRARY_DLL "discord-rpc.dll"
#define DISCORD_APP_ID	"577632370602278912"

// update once every 10 seconds. discord has an internal rate limiter of 15 seconds as well
#define DISCORD_UPDATE_RATE 10.0f

// placeholder code SUCKS i go to BED.
#define MAP_COUNT 63

// TODO give these better fitting names and move them to .h
const char *g_aClassImage[] =
{
	"undefined_ffa",
	"scout_ffa",
	"sniper_ffa",
	"soldier_ffa",
	"demo_ffa",
	"medic_ffa",
	"heavy_ffa",
	"pyro_ffa",
	"spy_ffa",
	"engineer_ffa",
	"merc_ffa",
	"civilian_ffa"
};

const char *g_aGameTypeNames_NonLocalized[] = // Move me?
{
	"Undefined",
	"Deathmatch",
	"Team Deathmatch",
	"Capture the Flag",
	"Control Point",
	"Escort",
	"Zombie Survival",
	"Gun Game",
	"Arena"
};

const char *g_aMapList[] =
{
	"dm_2fort",
	"dm_aerowalk",
	"dm_backfort",
	"dm_blockfort",
	"dm_bloodrun",
	"dm_boxy",
	"dm_bricks",
	"dm_chthon",
	"dm_congo",
	"dm_corpseyard",
	"dm_cs16_mansion",
	"dm_darkzone",
	"dm_deadsimple",
	"dm_degrootkeep",
	"dm_dmc_dm2",
	"dm_entryway",
	"dm_framework",
	"dm_greenback",
	"dm_hangar",
	"dm_hardcore",
	"dm_harvest",
	"dm_hl2dm_runoff",
	"dm_johnny",
	"dm_junkyard",
	"dm_longestyard",
	"dm_lumberyard",
	"dm_minecraft",
	"dm_office",
	"dm_skate",
	"dm_thebadplace",
	"dm_tvland",
	"dm_watergate",
	"dm_wiseau",
	"dm_wiseau_classic",
	"esc_tfc_hunted_test",
	"mctf_johnny",
	"mctf_longestyard",
	"mctf_redplanet",
	"mctf_greenback",
	"mctf_backfort",
	"mctf_congo",
	"dm_offblast",
	"dm_grain",
	"dm_sawdust",
	"dm_legacy",
	"dm_moonbase",
	"dm_cargo",
	"dm_bailey",
	"mctf_splashdown",
	"dm_dev_itemtest",
	"dm_deadlock_a1",
	"dm_pandora",
	"dm_knoxx",
	"dm_lobstershore",
	"dm_watchtower",
	"mctf_xpress3",
    "mctf_2fort",
    "mctf_turbine",
    "dm_overkill",
    "dm_coaltown",
	"ctf_push",
	"dm_bloodcovenant",
	"dm_badworks"
};

CTFDiscordRPC g_discordrpc;

CTFDiscordRPC::CTFDiscordRPC()
{
	Q_memset(m_szLatchedMapname, 0, MAX_MAP_NAME);
	m_bInitializeRequested = false;
}

CTFDiscordRPC::~CTFDiscordRPC()
{
}

void CTFDiscordRPC::Shutdown()
{
	Discord_Shutdown();

	if ( steamapicontext->SteamFriends() )
		steamapicontext->SteamFriends()->ClearRichPresence();
}

void CTFDiscordRPC::Init()
{
	InitializeDiscord();
	m_bInitializeRequested = true;

	// make sure to call this after game system initialized
	ListenForGameEvent( "server_spawn" );
}

void CTFDiscordRPC::RunFrame()
{
	if (m_bErrored)
		return;

	// NOTE: we want to run this even if they have use_discord off, so we can clear
	// any previous state that may have already been sent
	UpdateRichPresence();
	Discord_RunCallbacks();

	// always run this, otherwise we will chicken & egg waiting for ready
	//if (Discord_RunCallbacks)
	//	Discord_RunCallbacks();
}

void CTFDiscordRPC::OnReady( const DiscordUser* user )
{
	if (!of_enable_rpc.GetBool())
	{
		Discord_Shutdown();

		if ( steamapicontext->SteamFriends() )
			steamapicontext->SteamFriends()->ClearRichPresence();

		return;
	}

	ConColorMsg( Color( 114, 137, 218, 255 ), "[Rich Presence] Ready!\n" );
	ConColorMsg( Color( 114, 137, 218, 255 ), "[Rich Presence] User %s#%s - %s\n", user->username, user->discriminator, user->userId );
	
	g_discordrpc.Reset();
}

void CTFDiscordRPC::OnDiscordError(int errorCode, const char *szMessage)
{
	g_discordrpc.m_bErrored = true;
	char buff[1024];
	Q_snprintf(buff, 1024, "[Rich Presence] Init failed. code %d - error: %s\n", errorCode, szMessage);
	Warning(buff);
}


void CTFDiscordRPC::OnJoinGame( const char *joinSecret )
{
	ConColorMsg( Color( 114, 137, 218, 255 ), "[Rich Presence] Join Game: %s\n", joinSecret );

	char szCommand[128];
	Q_snprintf( szCommand, sizeof( szCommand ), "connect %s\n", joinSecret );
	engine->ExecuteClientCmd( szCommand );
}

void CTFDiscordRPC::OnSpectateGame( const char *spectateSecret )
{
	ConColorMsg( Color( 114, 137, 218, 255 ), "[Rich Presence] Spectate Game: %s\n", spectateSecret );
}

void CTFDiscordRPC::OnJoinRequest( const DiscordUser *joinRequest )
{
	ConColorMsg( Color( 114, 137, 218, 255 ), "[Rich Presence] Join Request: %s#%s\n", joinRequest->username, joinRequest->discriminator );
	ConColorMsg( Color( 114, 137, 218, 255 ), "[Rich Presence] Join Request Accepted\n" );
	Discord_Respond( joinRequest->userId, DISCORD_REPLY_YES );
}

void CTFDiscordRPC::SetLogo( void )
{
	const char *pszGameType = "";
	const char *pszImageLarge = "ico";
	const char *pMapIcon = "missing";
	//string for setting the picture of the class
	//you should name the small picture affter the class itself ex: Scout.jpg, Soldier.jpg, Pyro.jpg ...
	//you get it
	//-Nbc66
	const char *pszImageSmall = "";
	const char *pszImageText = "";
	C_TFPlayer *pTFPlayer = ToTFPlayer(C_BasePlayer::GetLocalPlayer());

	//checks if player is connected and sets the map name
	//image should be named after the map name
	//ex: dm_wiseau.jpg, dm_2fort.jpg..
	//case sensitive
	//-Nbc66
	if (engine->IsConnected())
	{
		if (pszImageLarge != m_szLatchedMapname)
		{
			for (int i=0; i<MAP_COUNT; i++)
			{
				if ( V_strcmp( g_aMapList[i], m_szLatchedMapname ) == 0 )
				{
					pMapIcon = m_szLatchedMapname;
					break;
				}
			}
		}
		//steam rpc setts the steam status display to show what map you are playing on
		steamapicontext->SteamFriends()->SetRichPresence("steam_display", m_szLatchedMapname);
		//pszImageLarge is used for discord rpc to set the name of the icon it should call in rpc
		pszImageLarge = pMapIcon;
	}

	//checks the players class
	if ( pTFPlayer )
	{
		int iClass = pTFPlayer->GetPlayerClass()->GetClassIndex();

		if ( pTFPlayer->GetTeamNumber() != TEAM_SPECTATOR )
		{
			pszImageSmall = g_aClassImage[iClass];
			pszImageText = g_aPlayerClassNames_NonLocalized[iClass];
		}
		else
		{
			pszImageSmall = "spectator";
			pszImageText = "Spectating";
		}
	}
	
	// Game Mode
	if ( TFGameRules( ) && engine->IsConnected() )
	{
		for ( int i = 0; i < TF_GAMETYPE_LAST; i++ )
		{
			if ( TFGameRules( )->InGametype(i) )
			{
				pszGameType = g_aGameTypeNames_NonLocalized[i];
			}
				
		}
	}
	
	//strings that set the the discord rpc icons and text
	m_sDiscordRichPresence.largeImageKey = pszImageLarge;
	m_sDiscordRichPresence.largeImageText = pszGameType;
	m_sDiscordRichPresence.smallImageKey = pszImageSmall;
	m_sDiscordRichPresence.smallImageText = pszImageText;
}

void CTFDiscordRPC::InitializeDiscord()
{
	DiscordEventHandlers handlers;
	Q_memset(&handlers, 0, sizeof(handlers));
	handlers.ready			= &CTFDiscordRPC::OnReady;
	handlers.errored		= &CTFDiscordRPC::OnDiscordError;
	handlers.joinGame		= &CTFDiscordRPC::OnJoinGame;
	//handlers.spectateGame = &CTFDiscordRPC::OnSpectateGame;
	handlers.joinRequest	= &CTFDiscordRPC::OnJoinRequest;

	char command[512];
	V_snprintf( command, sizeof( command ), "%s -game \"%s\" -novid -steam\n", CommandLine()->GetParm( 0 ), CommandLine()->ParmValue( "-game" ) );
	Discord_Register( DISCORD_APP_ID, command );
	Discord_Initialize( DISCORD_APP_ID, &handlers, false, "" );
	Reset();
}

bool CTFDiscordRPC::NeedToUpdate()
{
	if ( m_bErrored || m_szLatchedMapname[0] == '\0')
		return false;

	return gpGlobals->realtime >= m_flLastUpdatedTime + DISCORD_UPDATE_RATE;
}

void CTFDiscordRPC::Reset()
{
	Q_memset( &m_sDiscordRichPresence, 0, sizeof( m_sDiscordRichPresence ) );
	m_sDiscordRichPresence.details = "Main Menu";
	const char *pszState = "";
	
	m_sDiscordRichPresence.state = pszState;

	m_sDiscordRichPresence.endTimestamp;

	if ( steamapicontext->SteamFriends() )
	{
		steamapicontext->SteamFriends()->SetRichPresence( "status", "Main Menu" );
		steamapicontext->SteamFriends()->SetRichPresence( "connect", NULL );
		steamapicontext->SteamFriends()->SetRichPresence( "steam_display", "Main Menu" );
		steamapicontext->SteamFriends()->SetRichPresence( "steam_player_group", NULL );
		steamapicontext->SteamFriends()->SetRichPresence( "steam_player_group_size", NULL );
	}

	SetLogo();
	Discord_UpdatePresence( &m_sDiscordRichPresence );
}

void CTFDiscordRPC::UpdatePlayerInfo()
{
	C_TF_PlayerResource *pResource = GetTFPlayerResource();
	if ( !pResource )
		return;

	int maxPlayers = gpGlobals->maxClients;
	int curPlayers = 0;

	const char *pzePlayerName = NULL;

	m_sDiscordRichPresence.details = m_szLatchedMapname;
	m_sDiscordRichPresence.startTimestamp;

	for (int i = 1; i < maxPlayers; i++)
	{
		if ( pResource->IsConnected( i ) )
		{
			
			curPlayers++;
			if ( pResource->IsLocalPlayer( i ) )
			{
				pzePlayerName = pResource->GetPlayerName( i );
			}
		}
	}

	//int iTimeLeft = TFGameRules()->GetTimeLeft();

	if ( m_szLatchedHostname[0] != '\0' )
	{
		if ( cl_richpresence_printmsg.GetBool() )
		{
			ConColorMsg( Color( 114, 137, 218, 255 ), "[Discord] sending details of\n '%s'\n", m_szServerInfo );
		}
		m_sDiscordRichPresence.partySize = curPlayers;
		m_sDiscordRichPresence.partyMax = maxPlayers;
		m_sDiscordRichPresence.state = m_szLatchedHostname;
	}

	if ( steamapicontext->SteamFriends() )
	{
		//steamapicontext->SteamFriends()->SetRichPresence( "connect", pSteamGameServer->GetSteamID() );
		steamapicontext->SteamFriends()->SetRichPresence( "connect", NULL );
		steamapicontext->SteamFriends()->SetRichPresence( "steam_player_group", NULL );
		steamapicontext->SteamFriends()->SetRichPresence( "steam_player_group_size", NULL );
		steamapicontext->SteamFriends()->SetRichPresence( "status", m_szServerInfo );
		steamapicontext->SteamFriends()->SetRichPresence( "steam_display", "In-Game" );
	}
}

void CTFDiscordRPC::FireGameEvent( IGameEvent *event )
{
	const char * type = event->GetName();

	if ( !Q_strcmp( type, "server_spawn" ) )
	{
		Q_strncpy( m_szLatchedHostname, event->GetString( "hostname" ), 255 );
	}
}

void CTFDiscordRPC::UpdateRichPresence()
{
	//The elapsed timer function using <ctime>
	//this is for setting up the time when the player joins a server
	//-Nbc66
	time_t iSysTime;
	time(&iSysTime);
	struct tm *tStartTime = NULL;
	tStartTime = localtime(&iSysTime);
	tStartTime->tm_sec += 0 - gpGlobals->curtime;
	

	if (!NeedToUpdate())
		return;

	m_flLastUpdatedTime = gpGlobals->realtime;

	if ( engine->IsConnected() )
	{
		UpdatePlayerInfo();
		UpdateNetworkInfo();
		//starts the elapsed timer for discord rpc
		//-Nbc66
		m_sDiscordRichPresence.startTimestamp = mktime(tStartTime);
	}

	//checks if the loading bar is being drawn
	//and sets the discord status to "Currently is loading..."
	//-Nbc66
	if (engine->IsDrawingLoadingImage() == true)
	{
		m_sDiscordRichPresence.state = "";
		m_sDiscordRichPresence.details = "Currently is loading...";
	}
	
	SetLogo();

	Discord_UpdatePresence(&m_sDiscordRichPresence);
}


void CTFDiscordRPC::UpdateNetworkInfo()
{
	INetChannelInfo *ni = engine->GetNetChannelInfo();

	char partyId[128];
	sprintf( partyId, "%s-party", ni->GetAddress()); // adding -party here because secrets cannot match the party id

	m_sDiscordRichPresence.partyId = partyId;

	m_sDiscordRichPresence.joinSecret = ni->GetAddress();
	m_sDiscordRichPresence.spectateSecret = "Spectate";
}

void CTFDiscordRPC::LevelInit( const char *szMapname )
{
	Reset();
	// we cant update our presence here, because if its the first map a client loaded,
	// discord api may not yet be loaded, so latch
	Q_strcpy(m_szLatchedMapname, szMapname);
	// important, clear last update time as well
	m_flLastUpdatedTime = max(0, gpGlobals->realtime - DISCORD_UPDATE_RATE);
}
