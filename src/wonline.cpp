/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2025 Electronic Arts Inc.
 * Copyright 2026 OpenTS contributors
 *
 * Contains material derived from Electronic Arts source code.
 * Modified by OpenTS contributors, 2026.
 * EA's GPLv3 Section 7 additional terms and supplemental warranty
 * disclaimers apply; see LICENSE.md.
 ******************************************************************************/

// RA2: the first part of wonline.cpp (0x0073C040-0x00741EA0): the lobby helpers, the chat text senders, the channel list requests, the idle callback, the reset of the globals, the lobby, game and user lists, the user list, the start of the chat services and the leave of a user; Shutdown_Chat, Get_WDT_State_Silent, Init_Wonline (0x00742080-0x007424C0) follow as functions placed by their addresses
#include "always.h"

#include "_keyboar.h"
#include "_map.h"
#include "_rules.h"
#include "arraylist.h"
#include "dict.h" // RA2: the lobby dictionaries
#include "conquer.h"
#include "data.h"
#include "dbgprint.h"
#include "globals.h"
#include "goptions.h"
#include "ini.h" // RA2: the includes this file needs
#include "rcptr.h" // RA2: the profile pointer
#include "rules.h" // RA2: the CRCs of the rules
#include "init.h" // RA2: Title_Screen_Restore
#include "msgloop.h" // RA2: Peek_Windows_Message, MessageLoopCount
#include "ipxmgr.h"
#include "language\language.h"
#include "netshare.h"
#include "ownrdraw.h"
#include "playerprofile.h" // RA2: the profile of the player (wolpersonainformation.cpp)
#include "scenario.h"
#include "srfcache.h" // RA2: the images of the game list
#include "session.h"
#include "theme.h"
#include "widewstring.h" // RA2: the lobby text is wide
#include "windlg.h"
#include "winstub.h"
#include "wolapi.h" // RA2: the includes this file needs
#include "wonline.h"
#include "wstring.h"

#include <atlbase.h> // RA2: the chat services are COM objects
extern CComModule _Module; // RA2: required between atlbase.h and atlcom.h, defined with the rest of the file
#include <atlcom.h>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <shellapi.h>
#include <windows.h>
#undef LoadString // RA2: windows.h maps it to LoadStringA
#include "stringtable.h" // RA2: the texts are loaded from the string table by label

// RA2: the per user lobby record. It is 0x94 bytes (as TibSun's), the player's accepted state is at +0x20 (0x0073D150 stores it)
struct ChannelUserInfo
{
	char name[20];
	int color;
	int house;
	int Unknown_1C; // RA2: as above
	int accepted;
	char pad[0x70];
};

// RA2: globals and functions of the rest of wonline.cpp and of other files, as the executable's callers use them
extern ChannelUserInfo g_UserInfo[]; // RA2: 0x00B29CF0, one record per user of the channel
extern int g_RuleCRC; // RA2: the three CRCs 0x0073CFD0 xors (0x00B2AC18 to 0x00B2AC20)
extern int g_ArtCRC; // RA2: as above
extern int g_AICRC; // RA2: as above
extern ArrayList<unsigned int> g_ChanListQueue; // RA2: 0x00B29390, the channel list requests that wait for an answer
extern HANDLE g_WaitEventHandles[18]; // RA2: 0x00B264F8, EV_COUNT events
extern unsigned int g_ActiveWaitFlags; // RA2: 0x00B2ABE0, the events the wait dialog (0xC4) waits for
extern wchar_t g_LastErrorMessage[]; // RA2: 0x00B2A8EC, wide
extern int g_IsChannelCreator; // RA2: 0x00B2AC58, 1 for the host of the channel
extern bool g_GameStartRequested; // RA2: 0x00B2AC04
extern int g_MatchState; // RA2: 0x00B26898, set with the next three by Set_Match_State
extern int g_MatchTimeout; // RA2: 0x00B2AB88
extern int g_MatchResult; // RA2: 0x00B2AC2C
extern long g_MatchStartTime; // RA2: 0x00B2A8A4
extern ArrayList<User> g_UserList; // RA2: 0x00B2A728
extern Channel g_CurrentChannel; // RA2: 0x00B299C0
extern int g_AllowPage; // RA2: the lobby settings, 0x00B290A0
extern int g_AllowFind; // RA2: 0x00B2A818
extern int g_LangFilter; // RA2: 0x00B2A7E4
extern int g_LobbyMusic; // RA2: 0x00B297C8
extern int g_ShowAllChannels; // RA2: 0x00B2A634
extern int g_DisplayAsian; // RA2: 0x00B2A804, the two settings TibSun does not have
extern int g_DisplayLatin; // RA2: 0x00B2A7E8
extern int g_LanguageCode; // RA2: 0x00B2AC5C
extern int Online_Session_State; // RA2: as in ownrdraw.cpp
extern ArrayList<Wstring> g_WDTTeamLookupQueue; // RA2: 0x00B2A648, the names whose team is asked for (0x0073E100 sends them in batches)
extern ArrayList<Wstring> g_UserLocaleLookupQueue; // RA2: 0x00B29468, the names whose locale is asked for
extern unsigned g_LastWDTUserTeamRequestTime; // RA2: 0x00B28FF8
extern unsigned g_LastChannelLocaleRequestTime; // RA2: 0x00B28FF4
extern int g_NextWDTStatePollTime; // RA2: 0x00B292DC
extern int WolOptionsLocked; // RA2: 0x00B2AC40, as in netshare.cpp (2 once the host started the game)
extern bool g_StartGameReceived; // RA2: 0x00B2AC45, the start of the game was announced, Start_Game_Now runs it
extern bool g_StartGameRunning; // RA2: 0x00B2AC44, Start_Game_Now is running (it sets it first and clears it at its end)
void Start_Game_Now(void); // RA2: 0x0074F300, the executable's own debug text names it so
extern rc_ptr<PlayerProfile> g_PlayerProfile; // RA2: 0x00B28CC0, set by Reset_WOL_Globals
extern ArrayList<Channel> g_UserChannels; // RA2: 0x00B26C88
extern ArrayList<Channel> g_GameChannelList; // RA2: 0x00B29990
extern Dictionary<Wstring, int> g_WDTTeams; // RA2: 0x00B294B8
extern Dictionary<Wstring, int> g_UserLocales; // RA2: 0x00B293A0
extern Dictionary<Wstring, Ladder> g_BattleClansLadderDict; // RA2: 0x00B296F8
extern Dictionary<Wstring, Ladder> g_WDTLadderDict; // RA2: 0x00B26970
extern Dictionary<Wstring, Ladder> g_GeneralLadderDict; // RA2: 0x00B2AB98
extern Dictionary<Wstring, Channel> g_BuddyChannels; // RA2: 0x00B29478, the channel a buddy is in, by name
extern ArrayList<Wstring> g_BuddyList; // RA2: 0x00B26920, the names of the buddies
extern ArrayList<Wstring> g_BuddyRequests; // RA2: 0x00B29838, the buddies whose status waits to be asked for
extern Dictionary<Wstring, unsigned int> g_BuddyStatus; // RA2: 0x00B29848, the status of a buddy by name
extern Dictionary<Wstring, Ladder> * g_ActiveLadder; // RA2: 0x00B2AC24
extern int g_InstalledLadderSKU; // RA2: 0x00B2A7F0
extern int g_SelectedLadderSKU; // RA2: 0x00B26564
extern int g_GameSKU; // RA2: 0x007FD370, 0x2100
extern Channel g_PendingChannel; // RA2: 0x00B26A60, cleared with the current channel
extern Server g_GameServer; // RA2: 0x00B28CD0
extern Server g_WDTServer; // RA2: 0x00B29880
extern int g_CurrentServerIndex; // RA2: 0x00B292D8
extern int g_TargetServerIndex; // RA2: 0x00B2A7F4
extern bool g_IgnoreNetStatus; // RA2: 0x00B2ABD8
extern bool g_ShowMOTD; // RA2: 0x007FD2FC, true at first
extern Wstring g_Motd; // RA2: 0x00B26954, the message of the day
extern bool g_PlayingNetGame; // RA2: 0x00B2ABD9, as windlg.cpp
extern bool g_ConnectionLost; // RA2: 0x00B2ABDA
extern int g_ServerType; // RA2: 0x007FD300, -1 at first
extern int g_LastChatListTime; // RA2: 0x00B2ABD0
extern int g_LastGameListTime; // RA2: 0x00B2ABD4
extern unsigned short const * g_Lobbies[8]; // RA2: 0x00B26C94, the eight lobby names, wide
extern int WDT_TERRITORY_LOBBY_INDEXES[8]; // RA2: 0x00B26540, eight indexes
extern DWORD g_dwChatAdvise; // RA2: 0x00B2AC60
extern DWORD g_dwNetUtilAdvise; // RA2: 0x00B2AC64
extern DWORD g_dwDownloadAdvise; // RA2: 0x00B2AC68
extern int g_SquadInfoRequests; // RA2: 0x00B2ABF4
extern int g_RequestingOwnSquadInfo; // RA2: 0x00B2ABF8
extern int g_OwnSquadID; // RA2: 0x00B2ABFC
extern char g_OwnSquadName[41]; // RA2: 0x00B2A8B0
extern int g_ChannelCount; // RA2: 0x00B2ABE8
extern int g_LadderPos; // RA2: 0x00B29CE8
extern int g_JoinLobbyNow; // RA2: 0x00B2ABF0
extern int g_UserAge; // RA2: 0x00B2AC6C
extern int g_UserConsent; // RA2: 0x00B2AC70
extern int ManglerIndex; // RA2: 0x00B2AC38, the mangler server picked for the port probes (FirewallHelperClass::Get_Next_Mangled_Source_Port reads it)
extern int g_Col_ChanName; // RA2: 0x00B29CCC, the column of the channel names in the lobby lists
extern int g_Col_GameName; // RA2: 0x00B2A8A8, the columns of the game list (TibSun's g_Col_GameName, g_Col_GameIcon, g_Col_GamePrivate, g_Col_GamePingTime and the observer column RA2 adds)
extern int g_Col_GameIcon; // RA2: 0x00B2A6F8
extern int g_Col_GamePrivate; // RA2: 0x00B2A6F4
extern int g_Col_GameObserver; // RA2: 0x00B299A0
extern int g_Col_GamePingTime; // RA2: 0x00B292A8
extern bool g_GameListRefreshing; // RA2: 0x00B2AC80, set while Refresh_Game_List fills the list
extern int ObserverMode; // RA2: 0x00A72C68, as in init.cpp
void __fastcall Clear_Listbox(HWND list); // RA2: 0x00600DB0, removes every row of an owner drawn list box
void __fastcall Get_Window_Item_Text(HWND list, int row, Wstring & text); // RA2: 0x00600EB0, the text of a row
#define OD_INSERTSTRING (WM_USER + 199) // RA2: 0x4C7, inserts the string at the row of wParam and returns the new row (name guessed from its use)
#define OD_FINDSTRING (WM_USER + 197) // RA2: 0x4C5, the row whose text is lParam, -1 if none (name guessed from its use)
WideWstring WideWstring_FromInt(int value); // RA2: 0x007710C0, the number as wide text ("%d")
WideWstring operator+(wchar_t const * first, WideWstring const & second); // RA2: 0x00771380

int Count_AI_Slots(void); // RA2: 0x005C9400, the slots holding an easy, medium or hard computer player
bool Is_Observer_Slot_Occupied(void); // RA2: 0x005C94B0, the observer slot holds a seated entry whose team is -1
bool Is_Observer_Slot_Full(void); // RA2: 0x005C9490, defined in netdlg2.cpp
bool Is_Real_Player_Entry(NodeNameType * entry); // RA2: 0x005C9320, the entry is a player (not open, closed, waiting or a computer player)
Wstring Lobby_Entry_Name(NodeNameType * entry); // RA2: 0x005CD0E0, the lobby's name for an open, closed, waiting or computer player entry
void Load_Observer_Mode(void); // RA2: 0x005CD380, reads MultiPlayer ObserverMode from RA2.INI (the counterpart of Save_Observer_Mode)
void Save_Observer_Mode(void); // RA2: 0x005CD3A0, defined in netdlg2.cpp
void Update_Observer_Buttons(HWND dialog); // RA2: 0x005CD540, defined in netdlg2.cpp
void Enable_Player_Combos(HWND dialog, bool value); // RA2: 0x005CD030, defined in netdlg2.cpp
int Player_Side_Combo_ID(int player); // RA2: 0x004D1350, defined in netdlg2.cpp (as the next eight)
int Player_Color_Combo_ID(int player); // RA2: 0x004D1CB0
int Side_Combo_Data(HWND dialog, int id, int index); // RA2: 0x004D1C70
void Fill_Observer_Combo(HWND dialog, int id); // RA2: 0x004D1640
void Fill_Random_Color_Combo(HWND dialog, int id); // RA2: 0x004D2250
void Select_Color_Combo(HWND dialog, int id, int color); // RA2: 0x004D2480
void Fill_Side_Combo(HWND dialog, int id); // RA2: 0x004D1560
void Fill_Color_Combo(HWND dialog, int id); // RA2: 0x004D2080
int Get_Slot_Control_Id(int slot); // RA2: 0x005C97A0
char * Wide_To_Narrow(wchar_t const * text); // RA2: 0x006FA120, static buffer
Wstring Wstring_FromWide(WideWstring const & text); // RA2: 0x007715D0, keeps the 7 bit characters
Wstring Wstring_Narrow(WideWstring const & text); // RA2: 0x00771280, narrows each character of a wide string (the counterpart of Wstring_ToWide)
void Draw_Player_List(int keep_selection, bool force); // RA2: 0x0073FEE0, one more argument than TibSun's
#define OD_INSERTSTRINGW (WM_USER + 204) // RA2: 0x4CC, inserts the wide string of lParam at the row of wParam (-1 appends) and returns the new row (name guessed from its use)
#define OD_SETTEXT (WM_USER + 180) // RA2: 0x4B4, sets the (narrow) text of a control of the dialog (name guessed from its use)
#define APP_REG_KEY "SOFTWARE\\Westwood\\Red Alert 2" // RA2: the registry key of the game (the literal at 0x007E6AD8)
void __fastcall Refresh_Slot_Settings(HWND dialog, bool force); // RA2: 0x005CA000, redraws the slot list of the game lobby
void __fastcall Fill_Buddy_List(HWND dialog); // RA2: 0x00755180, fills the buddy dialog (0x116)
void Char_To_Wide(wchar_t * wide, char const * narrow, int length); // RA2: 0x006FA0C0
wchar_t * Narrow_To_Wide(char const * narrow); // RA2: 0x006FA180
extern int g_Col_Username; // RA2: 0x00B2A81C, the name column of the user list (lobby and game channels)
extern int g_Col_Rank; // RA2: 0x00B292A4
extern int g_Col_Chanop; // RA2: 0x00B2657C
extern int g_Col_Gameinfo; // RA2: 0x00B2AB80
extern int g_Col_House; // RA2: 0x00B292E0
extern int g_Col_Accept; // RA2: 0x00B297B8
extern int g_Col_GamePing; // RA2: 0x00B26958
void Show_Wait_Window(unsigned int event, WideWstring const & text, bool block); // RA2: 0x0074D4D0, the wait dialog with a wide text
void Reset_Wait_Events(void); // RA2: 0x00752930, resets every wait event
BOOL CALLBACK WOL_Find_Page_Dialog_Proc(HWND win, UINT uMsg, WPARAM wParam, LPARAM lParam); // RA2: 0x007485D0
bool Wait_Callback_Thunk(void); // RA2: 0x0073C120, defined below
bool Is_Channel_Flag_8(Channel const * channel); // RA2: 0x0073C3C0, defined below

void Spaces_To_Underscores(char * text) // RA2: 0x0073C040, turns every space of the text into an underscore, nothing calls it
{
	for (char * space = strchr(text, ' '); space != NULL; space = strchr(space, ' ')) { // RA2: as above
		*space = '_';
	}
}

bool Is_User_Message_Shown(User const * user) // RA2: 0x0073C070, a message from this user shows when the user is an operator, a lobby dialog is open or Latin text is allowed
{
	if (!user) return(false); // RA2: as above
	if (user->flags & 1) return(true); // RA2: as above
	if (WS_Find_Dialog(0xC2)) return(true); // RA2: as above
	if (WS_Find_Dialog(0xC9)) return(true); // RA2: as above
	return g_DisplayLatin != 0; // RA2: as above
}

bool Is_Text_Shown(unsigned char const * flags, WideWstring const & text) // RA2: 0x0073C0B0, as Is_User_Message_Shown but with the first byte of a user record, and the Asian setting for text above 0xFF
{
	if (flags == NULL) { // RA2: as above
		return(false);
	}
	if (*flags & 1) { // RA2: as above
		return(true);
	}
	if (WS_Find_Dialog(0xC2) != NULL) { // RA2: as above
		return(true);
	}
	if (WS_Find_Dialog(0xC9) != NULL) { // RA2: as above
		return(true);
	}
	if (text.Is_Narrow()) { // RA2: as above
		return(g_DisplayLatin != 0); // RA2: as above
	}
	return(g_DisplayAsian != 0); // RA2: as above
}

bool Wait_Callback_Thunk(void) // RA2: 0x0073C120, a jump to WOL_Wait_Callback (the message box callback)
{
	return(WOL_Wait_Callback()); // RA2: as above
}

//	SKU, reported to WOLAPI for the purpose of finding patches.
IPXAddressClass User_To_IPX_Address(User const * user) // RA2: 0x0073C130, the address of a user from its IP: net 0, the IP in the first four node bytes
{
	NetNumType net; // RA2: as above
	NetNodeType node; // RA2: as above
	memset(net, 0, sizeof(net)); // RA2: as above
	memset(node, 0, sizeof(node)); // RA2: as above
	unsigned long ip = ntohl(user->ipaddr); // RA2: as above
	node[3] = (unsigned char)ip; // RA2: as above
	node[2] = (unsigned char)(ip >> 8); // RA2: as above
	node[1] = (unsigned char)(ip >> 16); // RA2: as above
	node[0] = (unsigned char)(ip >> 24); // RA2: as above
	IPXAddressClass address(net, node, 0); // RA2: as above
	return(address); // RA2: as above
}

void Set_Lobby_Music(bool online) // RA2: 0x0073C1B0, restarts the lobby score (theme -2) or queues theme -3
{
	if (online) { // RA2: as above
		if (!Theme.Still_Playing()) { // RA2: as above
			Theme.Stop();
			Theme.Queue_Song((ThemeType)-2); // RA2: as above
			return;
		}
		if (Theme.What_Is_Playing() == Theme.From_Name("INTRO")) { // RA2: as above
			Theme.Stop();
			Theme.Queue_Song((ThemeType)-2); // RA2: as above
		}
	} else {
		Theme.Queue_Song((ThemeType)-3); // RA2: as above
	}
}

// RA2: the four values of the match state are set together (0x0073D150 and the quick match start 0x00759E20 inline this)
inline void Set_Match_State(int state, int timeout, int result) // RA2: as above
{
	g_MatchResult = result; // RA2: as above
	g_MatchState = state; // RA2: as above
	g_MatchTimeout = timeout; // RA2: as above
	g_MatchStartTime = time(0); // RA2: as above
}

/// <summary>
/// Opens the Find Page dialog, waits for it to close, then restores the input-ignore flag
/// and forces the tactical map to fully redraw.
/// </summary>
void DoFindPage(void) // RA2: 0x0073C230, dialog 0xCF is opened by OwnerDraw::Begin_Dialog and the lobby is redrawn after it
{
	HWND dlg = OwnerDraw::Begin_Dialog(0xCF, (DLGPROC)WOL_Find_Page_Dialog_Proc, 0); // RA2: as above
	IgnoreInput = true;
	Keyboard->Clear();
	ShowWindow(dlg, SW_NORMAL);
	WS_Wait_Dialog(dlg, OwnerDraw::Dialog_Message_Handler, false, true, true); // RA2: as above
	Keyboard->Clear();
	IgnoreInput = Scen->IsInputLocked;
	Map.Flag_To_Redraw(GS_REDRAW_ALL);
}

/// <summary>
/// Searches the global user list for an entry whose name matches the given name
/// case-insensitively.
/// </summary>
/// <param name="name">Name to search for.</param>
/// <returns>Index of the matching user in g_UserList, or -1 if no user was found.</returns>
void Open_Tournament_Page(void) // RA2: 0x0073C2A0, a jump to ViewHTML with the tournament page
{
	ViewHTML("http://www.westwood.com/westwoodonline/tournaments/redalert2/", 0); // RA2: as above
}

void Open_Registration_Page(void) // RA2: 0x0073C2B0, the registration page for the language code
{
	char url[256];
	sprintf(url, "http://games2.westwood.com/cgi-bin/cgiclient?register&request=reg_menu&LANGCODE=%d", g_LanguageCode); // RA2: as above
	ViewHTML(url, 0);
}

int Player_Name_To_Index(Wstring const & name) // RA2: 0x0073C2E0, the name is a Wstring
{
	int i;
	User * user = 0;
	for (i = 0; i < g_UserList.length(); i++) {
		g_UserList.getPointer(&user, i);
		if (stricmp(name.text(), (char *)user->name) == 0) { // RA2: as above
			return(i);
		}
	}
	return(-1);
}

bool Is_Channel_Tournament(Channel const * channel) // RA2: 0x0073C340, only in an Internet game: the tournament flag or channel flag 8
{
	if (Session.Type != GAME_INTERNET) return(false); // RA2: as above
	return(channel->tournament || Is_Channel_Flag_8(channel)); // RA2: as above
}

bool Is_Current_Channel_Tournament(void) // RA2: 0x0073C370, the same test of the current channel
{
	return(Is_Channel_Tournament(&g_CurrentChannel)); // RA2: as above
}

bool Is_Current_Channel_Flag_8(void) // RA2: 0x0073C3A0, channel flag 8 of the current channel
{
	return(Is_Channel_Flag_8(&g_CurrentChannel)); // RA2: as above
}

bool Is_Channel_Flag_8(Channel const * channel) // RA2: 0x0073C3C0, only in an Internet game: bit 8 of the channel's reserved word
{
	if (Session.Type != GAME_INTERNET) return(false); // RA2: as above
	return((channel->reserved >> 8) & 1); // RA2: as above
}

bool Is_Channel_Flag_11(Channel const * channel) // RA2: 0x0073C3E0, as above for bit 11
{
	if (Session.Type != GAME_INTERNET) return(false); // RA2: as above
	return((channel->reserved >> 11) & 1); // RA2: as above
}

bool Is_Channel_Flag_12(Channel const * channel) // RA2: 0x0073C400, as above for bit 12
{
	if (Session.Type != GAME_INTERNET) return(false); // RA2: as above
	return((channel->reserved >> 12) & 1); // RA2: as above
}

Wstring g_NickName; // RA2: a Wstring, the persona of the login (0x0073C420 constructs it)
Wstring g_LoginPassword; // RA2: a Wstring, the password of the login (0x0073C450 constructs it)

void Send_Player_List(void) // RA2: 0x0073C480, sends the names of the eight slots (the lobby entry names for open and AI slots) as game option L
{
	char text[256];
	strcpy(text, "L"); // RA2: as above
	for (int i = 0; i < 8; ++i) { // RA2: as above
		NodeNameType * entry = Session.Slots[i]; // RA2: as above
		Wstring name;
		if (Is_Real_Player_Entry(entry)) { // RA2: as above
			name = Wstring_Narrow(entry->Name); // RA2: as above
		} else {
			name = Lobby_Entry_Name(entry); // RA2: as above
		}
		strncat(text, name.get(), 256); // RA2: as above
		strncat(text, ",", 256); // RA2: as above
	}
	g_pChat->RequestPublicGameOptions(text); // RA2: as above
}

/// <summary>
/// Displays a web page in the player's own browser.
/// This routine hands the page to whatever program the system has registered for HTML and
/// then waits on it, pumping the wait callback throughout so the Westwood Online connection
/// does not lapse while the player reads. The game window is brought back to the front once
/// the browser is finished with. If no browser is installed the player is told so.
/// </summary>
/// <param name="name">URL or page name passed to the browser as the "[open]" argument.</param>
/// <param name="no_ask">Should the browser be launched without asking the player first?</param>
void ViewHTML(const char * name, int no_ask) // RA2: 0x0073C580, the launch and the missing browser texts come from the string table
{
	/*
	 * Create a throwaway HTML file so FindExecutable can tell us which browser handles it.
	 */
	char filename[160];
	tmpnam(filename);
	strcat(filename, ".html");

	FILE * file = fopen(filename, "w");
	fprintf(file, "<title>Hi there.</title>");
	fclose(file);

	char exeName[MAX_PATH + 1];
	int result = (int)FindExecutable(filename, NULL, exeName);

	_unlink(filename);

	/// Ask the user for confirmation (unless no_ask forces an immediate launch), then spawn the browser.
	if (result > 32) { // RA2: as above
		if (!no_ask && ODMessageBox(StringTable::LoadString("TXT_LAUNCHBROWSER", NULL, "D:\\Ra2\\wonline.cpp", 1172), MB_YESNO, Wait_Callback_Thunk, false) != IDYES) { // RA2: as above
			return;
		}

		char commandLine[MAX_PATH + 10];
		sprintf(commandLine, "[open] %s", name);
		
		STARTUPINFO startupInfo;
		memset(&startupInfo, 0, sizeof(startupInfo));
		startupInfo.cb = sizeof(startupInfo);

		PROCESS_INFORMATION processInfo;

		BOOL createSuccess = CreateProcess(exeName, commandLine, NULL, NULL, FALSE, 0, NULL, NULL, &startupInfo, &processInfo); // RA2: as above

		if (createSuccess) {
			HANDLE process = processInfo.hProcess;
			if (process) {
				int done_waiting = FALSE;

				WOL_Wait_Callback();

				WaitForInputIdle(process, 5000);

				/// Wait for the browser to exit, or for the game window to regain focus.
				do {
					WOL_Wait_Callback();
					Sleep(500);

					DWORD exitCode;
					GetExitCodeProcess(process, &exitCode);
					if (exitCode != STILL_ACTIVE) {
						done_waiting = TRUE;
					}

					if (GameInFocus || GetTopWindow(NULL) == MainWindow || GetForegroundWindow() == MainWindow) {
						done_waiting = TRUE;
					}
					WOL_Wait_Callback();
				} while (!done_waiting); // RA2: as above

				/*
				 * Bring the game window back to the foreground now that the browser is done.
				 */
				WOL_Wait_Callback();
	
				if (GetTopWindow(NULL) != MainWindow) {
					WOL_Wait_Callback();
					SetForegroundWindow(MainWindow);
					ShowWindow(MainWindow, SW_RESTORE);
					WOL_Wait_Callback();
				}

				int waits = 10;
				while (waits) {
					Sleep(100);
					WOL_Wait_Callback();
					waits--;
				}
			}
		}
	} else {
		/*
		 * No handler found for this file type -- tell the user there's no browser installed.
		 */
		ODMessageBox(StringTable::LoadString("TXT_NOBROWSER", NULL, "D:\\Ra2\\wonline.cpp", 1240), MB_OK, Wait_Callback_Thunk, false); // RA2: as above
	}
}

/// <summary>
/// Determines the sort order of two channel users.
/// Channel owners come first, then users with voice, then everyone else; equals are listed
/// alphabetically. This is the order the user list box is kept in.
/// </summary>
/// <returns>Returns with a negative value if the u1 user sorts first, a positive value if
/// the u2 one does, and zero if the two cannot be told apart.</returns>
void Read_WOL_Settings(void) // RA2: 0x0073C7C0, two more settings than TibSun (DisplayAsian, DisplayLatin) and the observer mode
{
	g_AllowPage = ConfigINI.Get_Int("WOnline", "AllowPage", TRUE);
	g_AllowFind = ConfigINI.Get_Int("WOnline", "AllowFind", TRUE);
	g_LangFilter = ConfigINI.Get_Int("WOnline", "LangFilter", TRUE);
	g_LobbyMusic = ConfigINI.Get_Int("WOnline", "LobMusic", TRUE);
	g_ShowAllChannels = ConfigINI.Get_Int("WOnline", "ShowAll", TRUE);
	g_DisplayAsian = ConfigINI.Get_Int("WOnline", "DisplayAsian", TRUE); // RA2: as above
	g_DisplayLatin = ConfigINI.Get_Int("WOnline", "DisplayLatin", TRUE); // RA2: as above
	Load_Observer_Mode(); // RA2: as above
}

/// <summary>
/// Writes the current WOL lobby settings (paging, finding, language filter, lobby music,
/// show-all) to the config INI and saves it to disk.
/// </summary>
void Write_WOL_Settings(void) // RA2: 0x0073C890, the file is RA2.INI, as above
{
	RawFileClass file("RA2.INI"); // RA2: as above
	ConfigINI.Put_Int("WOnline", "AllowPage", g_AllowPage);
	ConfigINI.Put_Int("WOnline", "AllowFind", g_AllowFind);
	ConfigINI.Put_Int("WOnline", "LangFilter", g_LangFilter);
	ConfigINI.Put_Int("WOnline", "LobMusic", g_LobbyMusic);
	ConfigINI.Put_Int("WOnline", "ShowAll", g_ShowAllChannels);
	ConfigINI.Put_Int("WOnline", "DisplayAsian", g_DisplayAsian); // RA2: as above
	ConfigINI.Put_Int("WOnline", "DisplayLatin", g_DisplayLatin); // RA2: as above
	ConfigINI.Save(file, false);
	Save_Observer_Mode(); // RA2: as above
}

/// <summary>
/// Applies the current WOL settings to the active chat session: pushes the language filter
/// and find/page flags to the chat client, and starts, continues, or fades out the lobby
/// music depending on the LobMusic setting.
/// </summary>
void Apply_WOL_Settings(void) // RA2: 0x0073C990, the observer buttons are updated and the lobby music is queued as in Set_Lobby_Music
{
	if (g_pChat) {
		g_pChat->SetLangFilter(g_LangFilter);
		g_pChat->SetFindPage(g_AllowFind, g_AllowPage);
	}
	Update_Observer_Buttons(NULL); // RA2: as above

		/// Manage the lobby music according to the LobMusic setting.
	bool music = g_LobbyMusic != 0; // RA2: as above
	if (music && !Theme.Still_Playing()) { // RA2: as above
		Theme.Stop();
		Theme.Queue_Song((ThemeType)-2); // RA2: as above
	} else if (music) { // RA2: as above
		if (Theme.What_Is_Playing() == Theme.From_Name("INTRO")) { // RA2: as above
			Theme.Stop();
			Theme.Queue_Song((ThemeType)-2); // RA2: as above
		}
	} else if (!music) { // RA2: as above
		Theme.Queue_Song((ThemeType)-3); // RA2: as above
	}
}

/// <summary>
/// Sends a chat message on the player's behalf.
/// The message goes privately to whoever is selected in the user list box, or out to the
/// whole channel when nothing is selected.
/// </summary>
/// <param name="msg">Text of the message to send.</param>
/// <returns>Returns with 0 if the message went to the whole channel, or 1 if it went
/// privately to the selected users.</returns>
int Send_Chat_Message(WideWstring const & msg) // RA2: 0x0073CA50, the text is wide and goes through the wide IChat calls
{
	static int sel_user_indexes[128];
	HWND users = GetDlgItem(WS_Top_Window(), IDC_USERS);
	int count = ListBox_GetSelItems(users, (WPARAM)32, (LPARAM)sel_user_indexes);

	if (count == 0) {
		/*
		 * No users selected -- broadcast to the whole channel.
		 */
		g_pChat->RequestPublicMessageWide((unsigned short const *)msg.text()); // RA2: as above
		return(0);
	} else {
		/*
		 * Build a linked list of the selected users and send them a private message.
		 */
		User * last = NULL;
		User * head = NULL;
		User * item = NULL;
		User source;
		memset(&source, 0, sizeof(source));
		for (int i = 0; i < count; i++) {
			g_UserList.get(source, sel_user_indexes[i]);
			item = new User;
			*item = source;
			item->next = NULL;
			if (i == 0) head = item;
			if (last) last->next = item;
			last = item;
		}

		if (head) g_pChat->RequestPrivateMessageWide(head, (unsigned short const *)msg.text()); // RA2: as above

		/// Free the temporary user list.
		while (head) {
			item = head;
			head = head->next;
			delete (item);
		}
		return(1);
	}
}

/// <summary>
/// Sends a chat action on the player's behalf.
/// The action goes privately to whoever is selected in the user list box, or out to the
/// whole channel when nothing is selected.
/// </summary>
/// <param name="msg">Text of the action to send.</param>
/// <returns>Returns with 0 if the action went to the whole channel, or 1 if it went
/// privately to the selected users.</returns>
int Send_Chat_Action(WideWstring const & msg) // RA2: 0x0073CB80, as above
{
	static int sel_user_indexes[128];
	HWND users = GetDlgItem(WS_Top_Window(), IDC_USERS);
	int count = ListBox_GetSelItems(users, (WPARAM)32, (LPARAM)sel_user_indexes);

	if (count == 0) {
		/*
		 * No users selected -- broadcast to the whole channel.
		 */
		g_pChat->RequestPublicActionWide((unsigned short const *)msg.text()); // RA2: as above
		return(0);
	} else {
		/*
		 * Build a linked list of the selected users and send them a private action.
		 */
		User * last = NULL;
		User * head = NULL;
		User * item = NULL;
		User source;
		memset(&source, 0, sizeof(source));
		for (int i = 0; i < count; i++) {
			g_UserList.get(source, sel_user_indexes[i]);
			item = new User;
			*item = source;
			item->next = NULL;
			if (i == 0) head = item;
			if (last) last->next = item;
			last = item;
		}

		if (head) g_pChat->RequestPrivateActionWide(head, (unsigned short const *)msg.text()); // RA2: as above

		/// Free the temporary user list.
		while (head) {
			item = head;
			head = head->next;
			delete (item);
		}
		return(1);
	}
}

static inline bool Same_Player_Name(wchar_t const * first, wchar_t const * second) // RA2: the inline compare of two lobby names (case insensitive, as narrow text)
{
	char * name = Wide_To_Narrow(first); // RA2: as above
	return(stricmp(name, Wide_To_Narrow(second)) == 0); // RA2: as above
}

/// <summary>
/// Encodes the current game options and scenario info, followed by each user's name,
/// house, and color, into a single comma-separated string for transmission over WOL.
/// </summary>
/// <param name="out">Buffer that receives the encoded options string.</param>
void Encode_Game_Options(char * out) // RA2: 0x0073CCB0, RA2's option text: seed, speed, credits, units, switches, mode, map name, scenario, then the players and the computer players
{
	static char useroptions[512];

	Wstring name = Session.Options.ScenarioDescription ? Wstring_FromWide(Session.Options.ScenarioDescription) : ""; // RA2: only the 7 bit characters of the map name are sent

	/// Encode the game options and scenario info.
	sprintf(out, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%s,%d,%d,%d,%s,%s:", // RA2: as above
			Seed, Options.GameSpeed, Session.Options.GameSpeed, Session.Options.Credits, Session.Options.UnitCount, // RA2: as above
			Session.Options.ShortGame, Session.Options.SWAllowed, Session.Options.BuildOffAlly, Session.Options.MCVRedeploy, // RA2: as above
			Session.Options.Goodies, Session.Options.MPModeIndex, name.get(), Session.ScenarioMaxPlayers, Session.ScenarioIsOfficial, // RA2: as above
			Session.ScenarioFileLength, Session.ScenarioFileName, Session.ScenarioDigest); // RA2: as above

	/*
	 * Append each user's name, house, and color.
	 * Append each user's name, country, color, address, port and ping.
	 */
	memset(useroptions, 0, sizeof(useroptions));
	for (int i = 0, count = Session.Players.Count(); i < count; i++) { // RA2: as above
		NodeNameType * entry = Session.Players[i]; // RA2: as above
		int country = entry->Get_Country(); // RA2: as above
		int color = entry->Get_Color(); // RA2: as above
		int address = 0; // RA2: as above
		int port = 0; // RA2: as above
		int ping = 0; // RA2: as above
		for (int j = 0; j < 8; j++) { // RA2: as above
			if (Same_Player_Name(Session.Players[i]->Name, Session.Slots[j]->Name)) { // RA2: as above
				NetNumType net; // RA2: as above
				NetNodeType node; // RA2: as above
				unsigned short socket; // RA2: as above
				Session.SlotAddresses[j].Get_Address(net, node, &socket); // RA2: as above
				address = *(int *)node; // RA2: as above
				port = Session.WolPlayerState[j]; // RA2: as above
				ping = Session.PlayerPortBase[j]; // RA2: as above
				break;
			}
		}
		Wstring text(Wstring_Narrow(Session.Players[i]->Name)); // RA2: as above
		sprintf(useroptions + strlen(useroptions), "%s,%d,%d,%x,%x,%x,", text.get(), country, color, address, port, ping); // RA2: as above
	}
	useroptions[strlen(useroptions) - 1] = ':'; // RA2: as above
	strcat(useroptions, "@:"); // RA2: as above

	/// Append the computer players.
	for (int k = 0; k < 8; k++) { // RA2: as above
		sprintf(useroptions + strlen(useroptions), "%d,%d,%d,", Session.Options.AISlots.Difficulties[k], Session.Options.AISlots.Countries[k], Session.Options.AISlots.Colors[k]); // RA2: as above
	}

	strcat(out, useroptions);
}

/// <summary>
/// Combines the rules, AI, and art CRCs into a single hash used to verify that all players
/// are running matching game data.
/// </summary>
/// <returns>Combined CRC hash.</returns>
int GetINIHash(void) // RA2: 0x0073CFD0
{
	return(g_RuleCRC ^ g_AICRC ^ g_ArtCRC);
}

/// <summary>
/// Encodes extended game/session info -- build number, data hash, options, and a flag
/// describing which house(s) still have room -- into a comma-separated string for
/// transmission over WOL.
/// </summary>
/// <param name="out">Buffer that receives the encoded info string.</param>
void Encode_Channel_ExInfo(char * out) // RA2: 0x0073CFF0, RA2's info text: build, hash, computer players, observer slot state, scenario file
{
	int i = 0;
	int house1_count = 0;
	int house0_count = 0;

	/// Count how many users are currently on each house.
	for (int j = 0; j < (int)g_CurrentChannel.currentUsers; j++) {
		int house = g_UserInfo[j].house;
		if (house == 0) {
			house0_count++;
		} else if (house == 1) { // RA2: as above
			house1_count++;
		}
	}

	/// Flag which house(s) still have free slots.
	if (house1_count < (int)g_CurrentChannel.maxUsers / 2) {
		i = 2;
	}

	if (house0_count < (int)g_CurrentChannel.maxUsers / 2) {
		i++;
	}

	sprintf(out, "%d,%d,%d,%d,%d,%s", Build_Number(), GetINIHash(), Count_AI_Slots(), Is_Observer_Slot_Occupied(), !Is_Observer_Slot_Full(), Session.ScenarioFileName); // RA2: as above
}

/// <summary>
/// Records whether the given user (or the local player if who is NULL) has accepted an
/// invitation/request, then refreshes the user list display.
/// </summary>
/// <param name="who">Name of the user to update, or NULL for the local player.</param>
void Parse_Channel_ExInfo(char * text, int * build, int * hash, int * ai_count, int * observer, int * not_full, Wstring * scenario) // RA2: 0x0073D070, the counterpart of Encode_Channel_ExInfo
{
	char * token = strtok(text, ","); // RA2: as above
	*build = token != NULL ? atol(token) : 0;
	token = strtok(NULL, ",");
	*hash = token != NULL ? atol(token) : 0;
	token = strtok(NULL, ",");
	*ai_count = token != NULL ? atol(token) : 0;
	token = strtok(NULL, ",");
	*observer = token != NULL ? atol(token) : 0;
	token = strtok(NULL, ",");
	*not_full = token != NULL ? atol(token) : 0;
	*scenario = strtok(NULL, ",");
}



/// <summary>
/// Records whether the given user (or the local player if who is empty) has accepted the game
/// options, then refreshes the user list display. When the host is waiting for the only other
/// player of a two player game and that player has accepted, the game is started.
/// </summary>
/// <param name="who">Name of the user to update, empty for the local player.</param>
/// <param name="status">New accepted status to store.</param>
void SetPlayerAccepted(Wstring const & who, int status) // RA2: 0x0073D150, the name is a Wstring and the host starts a two player game when the guest accepts
{
	Wstring const nick(who.length() > 0 ? who : g_NickName); // RA2: as above
	if (nick == g_NickName && status == 0) { // RA2: as above
		HWND dialog = WS_Find_Dialog(201); // RA2: as above
		Enable_Player_Combos(dialog, true); // RA2: as above
	}

	User * user = 0;
	for (int index = 0; index < g_UserList.length(); ++index) { // RA2: as above
		g_UserList.getPointer(&user, index); // RA2: as above
		if (stricmp(nick.text(), (char *)user->name) == 0) { // RA2: as above
			if (index != -1) {
				g_UserInfo[index].accepted = status; // RA2: as above
				Draw_Player_List(0, false); // RA2: as above
			}
			break;
		}
	}

	bool local = g_IsChannelCreator == 1; // RA2: as above
	Wstring const lower = nick.toLower(); // RA2: as above
	int player_index; // RA2: as above
	for (player_index = 0; player_index < Session.Players.Count(); ++player_index) { // RA2: as above
		Wstring const player = Wstring_Narrow(Session.Players[player_index]->Name); // RA2: as above
		if (player.toLower() == lower) break; // RA2: as above
	}
	if (player_index >= Session.Players.Count()) return; // RA2: as above
	Session.Players[player_index]->Slot.Unknown77 = status; // RA2: as above

	if (local && (Online_Session_State == 2 || Online_Session_State == 1) && !g_GameStartRequested && player_index == 1 && g_UserList.length() == 2) { // RA2: as above
		User * node; // RA2: as above
		User * head = 0; // RA2: as above
		User copy; // RA2: as above
		memset(&copy, 0, sizeof(copy)); // RA2: as above
		for (int user_index = 0; user_index < g_UserList.length(); ++user_index) { // RA2: as above
			g_UserList.get(copy, user_index); // RA2: as above
			node = new User; // RA2: as above
			*node = copy;
			node->next = head; // RA2: as above
			head = node; // RA2: as above
		}
		Set_Match_State(9, 90, 0); // RA2: as above
		g_pChat->RequestGameStart(head);
		g_GameStartRequested = true;
		node = head; // RA2: as above
		while (node) { // RA2: as above
			head = node->next; // RA2: as above
			delete node; // RA2: as above
			node = head; // RA2: as above
		}
	}
	Draw_Player_List(0, false); // RA2: as above
}

/// <summary>
/// Looks up whether the given user (or the local player if who is NULL) has accepted an
/// invitation/request.
/// </summary>
/// <param name="who">Name of the user to query, or NULL for the local player.</param>
/// <returns>The user's accepted status, or -1 if the user could not be found.</returns>
int GetPlayerAccepted(Wstring const & who) // RA2: 0x0073D3F0, the name is a Wstring and the answer is the player slot's status
{
	Wstring const nick(who.length() > 0 ? who : g_NickName); // RA2: as above
	Wstring const lower = nick.toLower(); // RA2: as above
	if (Wstring_ToWide(lower) == static_cast<WideWstring const &>(WideWstring(Session.GameName)).toLower()) { // RA2: as above
		return(1);
	}
	int found = -1; // RA2: as above
	for (int index = 0; index < Session.Players.Count(); index++) { // RA2: as above
		Wstring const player = Wstring_Narrow(Session.Players[index]->Name); // RA2: as above
		if (player.toLower() == lower) found = index; // RA2: as above
	}
	if (found != -1) { // RA2: as above
		return(Session.Players[found]->Slot.Unknown77); // RA2: as above
	}
	return(-1);
}

/// <summary>
/// Leaves the current channel (waiting for the leave to complete) and joins the lobby channel
/// selected by Pick_Lobby.
/// </summary>
/// <returns>The index of the joined lobby channel, or -1 if the leave timed out, the join
/// failed, or no lobby was available.</returns>
/// <param name="who">Name of the player whose country/color changed.</param>
/// <param name="house">New country selection.</param>
/// <param name="color">New color selection.</param>
/// <returns>1 if the player's country actually changed, 0 otherwise.</returns>
int Assign_House_And_Color(Wstring const & who, int house, int color) // RA2: 0x0073D550, works on the lobby slots and fills the side and color boxes for the observer
{
	int found = -1; // RA2: as above
	Wstring const lower = who.toLower(); // RA2: as above
	for (int index = 0; index < 8; ++index) { // RA2: as above
		Wstring const player_name = Wstring_Narrow(Session.Slots[index]->Name); // RA2: as above
		if (lower == player_name.toLower()) { // RA2: as above
			found = index; // RA2: as above
			break;
		}
	}
	int changed = 0; // RA2: as above
	if (found == -1) return(changed); // RA2: as above

	HWND window = WS_Find_Dialog(0xC2); // RA2: as above
	if (!window) window = WS_Find_Dialog(0xC9); // RA2: as above
	NodeNameType * player = Session.Slots[found]; // RA2: as above
	if (player->Get_Country() != house) { // RA2: as above
		changed = 1; // RA2: as above
		player->Set_Country(house); // RA2: as above
		int side = Player_Side_Combo_ID(found); // RA2: as above
		int previous = Side_Combo_Data(window, side, -1); // RA2: as above
		if (house == -3) { // RA2: as above
			if (previous != -3) { // RA2: as above
				Fill_Observer_Combo(window, side); // RA2: as above
				int observer_control = Get_Slot_Control_Id(found); // RA2: as above
				SendMessage(GetDlgItem(window, observer_control), 0x4F1, 0, 1); // RA2: as above
				int color_control = Player_Color_Combo_ID(found); // RA2: as above
				Fill_Random_Color_Combo(window, color_control); // RA2: as above
				Select_Color_Combo(window, color_control, 8); // RA2: as above
			}
		} else if (previous == -3) { // RA2: as above
			Fill_Side_Combo(window, side); // RA2: as above
			int observer_control = Get_Slot_Control_Id(found); // RA2: as above
			SendMessage(GetDlgItem(window, observer_control), 0x4F1, 0, 0); // RA2: as above
			int color_control = Player_Color_Combo_ID(found); // RA2: as above
			Fill_Color_Combo(window, color_control); // RA2: as above
			Select_Color_Combo(window, color_control, -2); // RA2: as above
		}
		if (player->Get_Country() == -3) player->Slot.SpectatorFlag = -1; // RA2: as above
	}
	if (Session.Slots[found]->Get_Color() != color) player->Set_Color(color); // RA2: as above
	if (Session.Slots[found] == Session.Players[0]) Session.PrefColor = color; // RA2: as above
	Draw_Player_List(0, false); // RA2: as above
	return(changed); // RA2: as above
}

/// <summary>
/// Fills a login dialog in from a remembered login.
/// This routine is used to spare the player from retyping credentials. If the nickname is
/// one Westwood Online has kept, its password and locale are recalled into the dialog's
/// controls along with it; if it is not, the dialog is left as it was.
/// </summary>
/// <param name="name">Nickname to search for among the stored logins, or NULL to skip
/// the lookup.</param>
/// Most names are exposed by debug prints in RA2, rest guessed
enum {
	EV_ABORT,
	EV_GOT_SERVERS,
	EV_CONNECTED,
	EV_CHAN_LIST,
	EV_CHAN_JOINED,
	EV_ERROR,
	EV_EXIT,
	EV_STARTGAME,
	EV_QUITGAME,
	EV_NETIGNORE,
	EV_CHAN_JOIN_FAIL,
	EV_11, // continue waiting?
	EV_12,
	EV_13,
	EV_LOCALE,
	EV_SKIRMISH,
	EV_16, // RA2: as above
	EV_17, // RA2: as above
	EV_COUNT,
};

/// <summary>
/// Saves a login's nickname, password, and locale into one of the 32 stored login slots.
/// Reuses whichever slot already stores this nickname if one exists, otherwise the first
/// empty slot, otherwise cycles to the slot after the session's last-used nickname slot.
/// </summary>
/// <param name="loginName">Nickname to store.</param>
/// <param name="password">Password to store alongside the nickname.</param>
/// <param name="passwordPlaintext">Whether the password is stored/sent as plaintext.</param>
/// <param name="locale">Locale value to associate with this login.</param>
/// <returns>The result of setting the nick's locale (from SetNickLocale), or the computed
/// slot number if no slot ended up being written to.</returns>
/// <param name="channel_type">0 for the chat channels, 0x21 for the game channels.</param>
/// <param name="autoping">Should the server ping the channels?</param>
/// <returns>True if the request was sent.</returns>
bool Request_Channel_List(int channel_type, unsigned char autoping) // RA2: 0x0073D760, not in TibSun: the request and its queue entry in one place
{
	unsigned int type = channel_type; // RA2: the queue entry is made before the request, where Run_Lobby_Session expands this (0x00743BA4 stores it on the stack before the debug text)
	DebugString("Requesting channel list type = %d, ping = %d\n", channel_type, autoping); // RA2: as above
	if (g_pChat->RequestChannelList(type, autoping) != S_OK) { // RA2: as above
		return(false);
	}
	g_ChanListQueue.addTail(type);
	return(true);
}

/// <summary>
/// Dialog procedure for the "New Game" creation dialog. Handles owner-draw
/// painting, sets up the player-count controls (a slider, or WDT 1on1/2on2
/// radio buttons for World Domination Tour games), enforces tournament and
/// battle-clan slider interlocks, and stores the result on OK.
/// </summary>
/// <param name="win">Dialog window handle.</param>
/// <param name="uMsg">Window message identifier.</param>
/// <param name="wParam">Message-specific first parameter.</param>
/// <param name="lParam">Message-specific second parameter.</param>
/// <returns>TRUE if the message was handled, FALSE otherwise.</returns>
/// <returns>True if the channel list arrived.</returns>
bool Wait_Channel_List(void) // RA2: 0x0073D8F0, not in TibSun: waits for the answer, 30 seconds at most
{
	{
		WideWstring title(StringTable::LoadString("TXT_REQ_CHANLIST", NULL, "D:\\Ra2\\wonline.cpp", 2042)); // RA2: as above
		Show_Wait_Window(1, title, false); // RA2: as above
	}
	int elapsed = 0; // RA2: as above
	for (;;) { // RA2: as above
		DWORD event = WaitForMultipleObjects(18, g_WaitEventHandles, FALSE, 100); // RA2: as above
		while (event == WAIT_TIMEOUT) { // RA2: as above
			elapsed += 100; // RA2: as above
			if (elapsed > 30000) { // RA2: as above
				swprintf(g_LastErrorMessage, StringTable::LoadString("TXT_CANT_CONNECT", NULL, "D:\\Ra2\\wonline.cpp", 2051)); // RA2: as above
				SetEvent(g_WaitEventHandles[EV_ABORT]);
				return(false);
			}
			WOL_Wait_Callback();
			event = WaitForMultipleObjects(18, g_WaitEventHandles, FALSE, 100); // RA2: as above
		}
		if (event == WAIT_FAILED) { // RA2: as above
			Close_Wait_Window(1); // RA2: as above
			DebugString("WaitForMultipleObjects() failed in Get_Channel_List\n"); // RA2: as above
			return(false);
		}
		ResetEvent(g_WaitEventHandles[event]); // RA2: as above
		if (event == EV_ERROR) { // RA2: as above
			Close_Wait_Window(1); // RA2: as above
			return(false);
		}
		if (event == EV_ABORT) { // RA2: as above
			Close_Wait_Window(1); // RA2: as above
			break;
		}
		if (event == EV_CHAN_LIST) { // RA2: as above
			Close_Wait_Window(1); // RA2: as above
			return(true);
		}
	}
	return(false);
}

/// <summary>
/// Nudges the host with a NET_PING packet while waiting in a live game channel as a guest.
/// Sets up the session's player/host address from the channel's first user, then sends a
/// single ping packet to the host address.
/// </summary>
bool Queue_Channel_List_Request(int channel_type, unsigned char autoping, bool wait) // RA2: 0x0073DB20, not in TibSun
{
	if (g_ChanListQueue.length()) { // RA2: as above
		bool result = Wait_Channel_List(); // RA2: as above
		if (!result) return(result); // RA2: as above
	}
	Reset_Wait_Events(); // RA2: as above
	bool requested = Request_Channel_List(channel_type, autoping); // RA2: as above
	if (!requested) { // RA2: as above
		SetEvent(g_WaitEventHandles[EV_ABORT]);
		return(false);
	}
	if (wait) { // RA2: as above
		bool result = Wait_Channel_List(); // RA2: as above
		return(result);
	}
	return(true);
}

/// <summary>
/// Shows the World Domination Tour "available servers" dialog and waits for it to close.
/// </summary>
/// <returns>Result code returned by the dialog (see WS_Wait_Dialog/WS_Destroy_Dialog).</returns>
bool Request_Chat_Channel_List(bool wait) // RA2: 0x0073DD20, not in TibSun
{
	if (g_ChanListQueue.length()) { // RA2: as above
		bool result = Wait_Channel_List(); // RA2: as above
		if (!result) return(result); // RA2: as above
	}
	for (int i = 0; i < 18; ++i) ResetEvent(g_WaitEventHandles[i]); // RA2: as above
	if (!Request_Channel_List(0, 0)) { // RA2: as above
		SetEvent(g_WaitEventHandles[EV_ABORT]);
		return(false);
	}
	if (wait) return(Wait_Channel_List()); // RA2: as above
	return(true);
}

/// <summary>
/// Dialog procedure for the World Domination Tour "available servers" list. Populates the
/// list box on OD_SUBCLASSED (stripping any "region:" prefix from each server name and
/// pre-selecting the player's preferred server), and on OK stores the chosen server as the
/// new preferred server and saves the multiplayer settings.
/// </summary>
/// <param name="win">Handle of the dialog window.</param>
/// <param name="uMsg">Window message being processed.</param>
/// <param name="wParam">Message-specific parameter.</param>
/// <param name="lParam">Message-specific parameter.</param>
/// <returns>TRUE if the message was handled, FALSE otherwise.</returns>
bool Request_Game_Channel_List(unsigned char autoping, bool wait) // RA2: 0x0073DF10, not in TibSun
{
	if (g_ChanListQueue.length()) { // RA2: as above
		bool result = Wait_Channel_List(); // RA2: as above
		if (!result) return(result); // RA2: as above
	}
	for (int i = 0; i < 18; ++i) ResetEvent(g_WaitEventHandles[i]); // RA2: as above
	if (!Request_Channel_List(0x21, autoping)) { // RA2: as above
		SetEvent(g_WaitEventHandles[EV_ABORT]);
		return(false);
	}
	if (wait) return(Wait_Channel_List()); // RA2: as above
	return(true);
}

inline bool Is_Tournament_Session_Inline(void) // RA2: 0x0073BDB0 is this test, inlined here
{
	return(Online_Session_State == 2 || Online_Session_State == 1);
}

bool WOL_Wait_Callback(void) // RA2: 0x0073E100, no channel list requests (TibSun's are gone), the match timeout and the start of the game are RA2's
{
	Title_Screen_Restore(false); // RA2: as above

	static int prev_exinfo = 0;
	static int prev_ping = 0;

	MSG msg;
	int counter = 0;
	MessageLoopCount++; // RA2: as above
	while (Peek_Windows_Message(&msg, NULL, 0, 0, 1)) { // RA2: as above
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		counter++;
		if (counter == 100) break;
	}

	// RA2: a tournament match that waited longer than its timeout is cancelled (states 2 to 5) or given 30 seconds more
	if (Is_Tournament_Session_Inline() && g_MatchTimeout != -1 && g_MatchState != 1 && g_MatchState != 0 && g_MatchState != 11 && (unsigned)(time(NULL) - g_MatchStartTime) > (unsigned)g_MatchTimeout) {
		if (g_MatchState >= 2 && g_MatchState <= 5) {
			Set_Match_State(0, -1, 2);
		} else {
			Lobby_Chat_Printf(0x20BFFF, StringTable::LoadString("WOL:MatchTimeout", NULL, "D:\\Ra2\\wonline.cpp", 2155));
			Set_Match_State(2, 30, 0);
		}
		SetEvent(g_WaitEventHandles[EV_ERROR]);
	}

	WS_Top_Window_ID(); // RA2: the result is not used

	if (g_WDTTeamLookupQueue.length() > 200) {
		g_WDTTeamLookupQueue.clear();
	}

	if (g_LastWDTUserTeamRequestTime < (unsigned)time(0) - 2 && g_WDTTeamLookupQueue.length() > 65 ||
		g_LastWDTUserTeamRequestTime < (unsigned)time(0) - 3 && g_WDTTeamLookupQueue.length() > 10 ||
		g_LastWDTUserTeamRequestTime < (unsigned)time(0) - 15 && g_WDTTeamLookupQueue.length() > 0) {

		User * list = NULL;
		User * last_user = NULL;
		for (int i = 0; i < 10; i++) {
			if (g_WDTTeamLookupQueue.length() > 0) {
				User * new_user = new User;
				memset(new_user, 0, sizeof(User));
				Wstring name;
				g_WDTTeamLookupQueue.removeTail(name);
				char * rawName = name.get();
				if (strlen(rawName) > 9) {
					rawName[9] = '\0';
				}
				strcpy((char *)new_user->name, rawName);
				if (last_user == NULL) {
					list = new_user;
				} else {
					last_user->next = new_user;
				}
				last_user = new_user;
			} else {
				break;
			}
		}

		if (g_pChat != NULL) {
			g_pChat->RequestUserTeam(list);
		}

		User * d = list;
		while (d != NULL) {
			User * next = d->next;
			delete d;
			d = next;
		}

		g_LastWDTUserTeamRequestTime = time(0);
	}

	if (g_UserLocaleLookupQueue.length() > 200) {
		g_UserLocaleLookupQueue.clear();
	}

	if (g_LastChannelLocaleRequestTime < (unsigned)time(0) - 2 && g_UserLocaleLookupQueue.length() > 65 ||
		g_LastChannelLocaleRequestTime < (unsigned)time(0) - 3 && g_UserLocaleLookupQueue.length() > 10 ||
		g_LastChannelLocaleRequestTime < (unsigned)time(0) - 15 && g_UserLocaleLookupQueue.length() > 0) {

		User * list = NULL;
		User * last_user = NULL;
		for (int i = 0; i < 10; i++) {
			if (g_UserLocaleLookupQueue.length() > 0) {
				User * new_user = new User;
				memset(new_user, 0, sizeof(User));
				Wstring name;
				g_UserLocaleLookupQueue.removeTail(name);
				char * rawName = name.get();
				if (strlen(rawName) > 9) {
					rawName[9] = '\0';
				}
				strcpy((char *)new_user->name, rawName);
				if (last_user == NULL) {
					list = new_user;
				} else {
					last_user->next = new_user;
				}
				last_user = new_user;
			} else {
				break;
			}
		}

		if (g_pChat != NULL) {
			g_pChat->RequestUserLocale(list);
		}

		User * d = list;
		while (d != NULL) {
			User * next = d->next;
			delete d;
			d = next;
		}

		g_LastChannelLocaleRequestTime = time(0);
	}

	if (Session.Type == GAME_INTERNET && Online_Session_State == 2 && time(0) > g_NextWDTStatePollTime) { // RA2: an Internet tournament session, not TibSun's Session.IsWDT
		g_NextWDTStatePollTime = time(0) + 30;
		Get_WDT_State_Silent();
	}

	if ((g_pChat) && (g_SuspendChatPump == 0)) g_pChat->PumpMessages();
	if (g_pNetUtil) g_pNetUtil->PumpMessages();
	if (g_pDownload) g_pDownload->PumpMessages();

	Call_Back();

	if ((g_CurrentChannel.name[0]) && (g_CurrentChannel.type == 0x21) && (g_IsChannelCreator)) Send_Lobby_Options(false, false); // RA2: for PumpGameopts(false)

	if ((g_pChat != NULL) && (g_CurrentChannel.type != 0) && (g_IsChannelCreator)) {
		if ((time(NULL) - prev_exinfo) > 10) {
			char exinfo_out[80];
			memset(exinfo_out, 0, 80);
			Encode_Channel_ExInfo(exinfo_out);
			exinfo_out[40] = 0;
			g_pChat->SetChannelExInfo(exinfo_out);
			prev_exinfo = time(NULL);
		}
	}

	if (WolOptionsLocked != 2) { // RA2: no pings once the game is locked in
		if ((g_pChat != NULL) && (g_CurrentChannel.type != 0) && ((time(NULL) - prev_ping) > 10)) {
			User user;
			Wstring nick = ((Wstring const &)g_NickName).toLower(); // RA2: the names are compared as Wstrings, not with stricmp; the const member returns the lower case copy
			int handle;
			int i;
			in_addr addr;

			if (!Is_Tournament_Session_Inline()) { // RA2: as above
				for (i = 0; i < g_UserList.length(); i++) {
					g_UserList.get(user, i);
					Wstring const user_name((char *)user.name); // RA2: as above
					if (nick == user_name.toLower()) continue; // RA2: as above
					addr.s_addr = user.ipaddr;
					if (user.ipaddr != 0) g_pNetUtil->RequestPing(inet_ntoa(addr), 1000, &handle);
				}
			}
			prev_ping = time(NULL);
		}
	}

	if (WolOptionsLocked == 2) { // RA2: the host announced the start, the game is started from here
		if (g_StartGameReceived && !g_StartGameRunning) {
			Start_Game_Now();
		}
	}

	Handle_Preview_Download();
	return(false);
}

void Reset_WOL_Globals(void) // RA2: 0x0073E8D0, loads the default profile, clears the buddy lists and sets the SKU of the tournament ladder
{
	g_PlayerProfile = PlayerProfile::Load_Default(); // RA2: as above

	g_UserChannels.clear();
	g_GameChannelList.clear();
	g_UserList.clear();

	g_WDTTeamLookupQueue.clear();
	g_WDTTeams.clear();

	g_UserLocaleLookupQueue.clear();
	g_UserLocales.clear();

	g_NextWDTStatePollTime = time(0) + 86400;
	g_LastChannelLocaleRequestTime = 0;
	g_LastWDTUserTeamRequestTime = 0;

	g_BattleClansLadderDict.clear();
	g_WDTLadderDict.clear();
	g_BuddyChannels.clear(); // RA2: as above
	g_BuddyList.clear(); // RA2: as above
	g_BuddyRequests.clear(); // RA2: as above
	g_BuddyStatus.clear(); // RA2: as above

	if (Session.Type == GAME_INTERNET && Online_Session_State == 2) { // RA2: an Internet tournament session, not TibSun's Addon_Enabled(ADDON_FIRESTORM) && Session.IsWDT
		int sku = 0x2300; // RA2: the SKU of the tournament ladder
		g_InstalledLadderSKU = sku;
		g_SelectedLadderSKU = sku;
		g_ActiveLadder = &g_WDTLadderDict;
	} else {
		int sku = g_GameSKU;
		g_InstalledLadderSKU = sku;
		g_SelectedLadderSKU = sku;
		g_ActiveLadder = &g_GeneralLadderDict;
	}

	memset(&g_CurrentChannel, 0, sizeof(g_CurrentChannel));
	memset(&g_PendingChannel, 0, sizeof(g_PendingChannel)); // RA2: as above
	memset(&g_GameServer, 0, sizeof(g_GameServer));
	memset(&g_WDTServer, 0, sizeof(g_WDTServer));

	g_LastErrorMessage[0] = 0;

	g_MatchResult = 0;
	g_MatchState = 1;
	g_MatchTimeout = -1;
	g_MatchStartTime = time(0);

	g_CurrentServerIndex = 0;
	g_TargetServerIndex = 0;

	g_IsChannelCreator = 0;
	g_ActiveWaitFlags = 0;

	g_GameStartRequested = false;

	g_IgnoreNetStatus = false;
	g_ShowMOTD = true;
	g_Motd.clear(); // RA2: as above

	memset(&g_WaitEventHandles, 0, sizeof(g_WaitEventHandles));

	g_PlayingNetGame = false;
	g_ConnectionLost = false;

	g_ServerType = -1;

	g_SuspendChatPump = 0;

	g_LastChatListTime = 0xFFFFFFFF;
	g_LastGameListTime = 0xFFFFFFFF;

	g_Lobbies[0] = StringTable::LoadString("TXT_LOB_1", NULL, "D:\\Ra2\\wonline.cpp", 2485); // RA2: as above
	g_Lobbies[1] = StringTable::LoadString("TXT_LOB_2", NULL, "D:\\Ra2\\wonline.cpp", 2486); // RA2: as above
	g_Lobbies[2] = StringTable::LoadString("TXT_LOB_3", NULL, "D:\\Ra2\\wonline.cpp", 2487); // RA2: as above
	g_Lobbies[3] = StringTable::LoadString("TXT_LOB_4", NULL, "D:\\Ra2\\wonline.cpp", 2488); // RA2: as above
	g_Lobbies[4] = StringTable::LoadString("TXT_LOB_5", NULL, "D:\\Ra2\\wonline.cpp", 2489); // RA2: as above
	g_Lobbies[5] = StringTable::LoadString("TXT_LOB_6", NULL, "D:\\Ra2\\wonline.cpp", 2490); // RA2: as above
	g_Lobbies[6] = StringTable::LoadString("TXT_LOB_7", NULL, "D:\\Ra2\\wonline.cpp", 2491); // RA2: as above
	g_Lobbies[7] = StringTable::LoadString("TXT_LOB_8", NULL, "D:\\Ra2\\wonline.cpp", 2492); // RA2: as above

	memset(WDT_TERRITORY_LOBBY_INDEXES, 0xFFFFFFFF, sizeof(WDT_TERRITORY_LOBBY_INDEXES));

	g_pChat = NULL;
	g_pNetUtil = NULL;
	g_pDownload = NULL;

	g_dwChatAdvise = 0;
	g_dwNetUtilAdvise = 0;
	g_dwDownloadAdvise = 0;

	g_SquadInfoRequests = 0;
	g_RequestingOwnSquadInfo = 0;
	g_OwnSquadID = 0;
	memset(g_OwnSquadName, 0, 41); // RA2: as above

	g_ChannelCount = 0;

	g_LadderPos = 1;

	g_JoinLobbyNow = 0;

	g_ChanListQueue.setEmpty();

	WolOptionsLocked = 0; // RA2: TibSun's g_GameStarted

	g_UserAge = 0;
	g_UserConsent = 0;

	g_RuleCRC = RulesClass::Get_Rule_Unique_ID();
	RulesClass::Load_Art_INI();
	g_ArtCRC = RulesClass::Get_Art_Unique_ID();
	g_AICRC = RulesClass::Get_AI_Unique_ID();

	ManglerIndex = 0; // RA2: as above
}

void Refresh_Lobby_List(void) // RA2: 0x0073EE80, fills the lobby list of dialog 0x113 with the "Lob_33_" channels in number order, the current one selected
{
	HWND dialog = WS_Find_Dialog(0x113);
	if (!dialog) return; // RA2: as above
	HWND list = GetDlgItem(dialog, 0x53D);
	if (!list) return; // RA2: as above

	Wstring selected; // RA2: the text of the first visible row
	int top_row = SendMessageA(list, LB_GETTOPINDEX, 0, 0); // RA2: the first visible row is kept, not the selected one
	if (top_row != -1) Get_Window_Item_Text(list, top_row, selected);
	SendMessageA(list, OD_DISABLEPAINT, 0, 1);
	Clear_Listbox(list);

	int count = g_UserChannels.length();
	for (int i = 0; i < count; ++i) {
		Channel channel;
		g_UserChannels.get(channel, i);
		if (strncmp((char *)channel.name, "Lob_33_", strlen("Lob_33_")) != 0) continue;
		int number = atol((char *)channel.name + strlen("Lob_33_"));
		bool current = strcmp((char *)channel.name, (char *)g_CurrentChannel.name) == 0;
		wchar_t title[80];
		int page = number / 8;
		int name_index = number % 8;
		if (page == 0) {
			swprintf(title, L"%s  (%d)", (wchar_t const *)g_Lobbies[name_index], channel.currentUsers);
		} else {
			swprintf(title, L"%s_%d  (%d)", (wchar_t const *)g_Lobbies[name_index], page + 1, channel.currentUsers);
		}
		int rows = SendMessageA(list, LB_GETCOUNT, 0, 0);
		int at = 0;
		Channel other;
		for (; at < rows; ++at) {
			int index = SendMessageA(list, LB_GETITEMDATA, at, 0);
			g_UserChannels.get(other, index);
			if (atol((char *)other.name + strlen("Lob_33_")) > number) break;
		}
		int row = SendMessageA(list, OD_INSERTSTRING, at, (LPARAM)channel.name);
		SendMessageA(list, LB_SETITEMDATA, row, i);
		OwnerDraw::CellData cell;
		cell.type = OwnerDraw::CellData::TEXT;
		cell.string.set((unsigned short *)title);
		cell.hint.set((unsigned short *)L"");
		SendMessageA(list, OD_SETCELL, MAKEWPARAM(g_Col_ChanName, row), (LPARAM)&cell);
		if (current) SendMessageA(list, LB_SETCURSEL, row, 0);
	}

	int restored = SendMessageA(list, OD_FINDSTRING, 0, (LPARAM)selected.text());
	if (restored != -1) SendMessageA(list, LB_SETTOPINDEX, restored, 0);
	SendMessageA(list, OD_DISABLEPAINT, 0, 0);
	InvalidateRect(list, NULL, false);
}

WideWstring Get_Locale_Name(int locale) // RA2: 0x0073F1C0, the localized name of a locale number, from the string "WOL:Locale" and the number in two digits
{
	Wstring label = "WOL:Locale" + Wstring_FromInt(locale, 2); // RA2: as above
	WideWstring text(StringTable::LoadString(label.get(), NULL, "D:\\Ra2\\wonline.cpp", 2644)); // RA2: as above
	if (locale < 37) { // RA2: both ways return the same text, the test is kept
		return(text);
	} else {
		return(text);
	}
}

WideWstring User_Tooltip(User const & user) // RA2: 0x0073F250, the text of the tooltip of a lobby user: name, rank in the active ladder, squad and locale
{
	Wstring name((char *)user.name); // RA2: as above
	WideWstring text = Wstring_ToWide(name) + L" : "; // RA2: as above
	name.toLower();
	Ladder ladder;
	memset(&ladder, 0, sizeof(ladder));
	if (g_ActiveLadder && g_ActiveLadder->getValue(name, ladder)) { // RA2: as above
		text += WideWstring(StringTable::LoadString("TXT_RANK", NULL, "D:\\Ra2\\wonline.cpp", 2672)) + WideWstring(L" ") + WideWstring_FromInt(ladder.rung); // RA2: as above
	} else {
		text += StringTable::LoadString("TXT_UNRANKED", NULL, "D:\\Ra2\\wonline.cpp", 2674); // RA2: as above
	}
	Wstring squad((char *)user.squadname); // RA2: as above
	if (squad.length() == 0) squad = (char *)user.squadabbrev; // RA2: the abbreviation stands for a squad without a name
	if (squad.length() > 0) {
		text += L": " + Wstring_ToWide(squad); // RA2: as above
	}
	if (g_UserLocales.contains(name)) { // RA2: as above
		int * locale = 0;
		if (g_pChat && g_UserLocales.getPointer(name, &locale) && locale && *locale) { // RA2: as above
			text += L" : " + Get_Locale_Name(*locale); // RA2: as above
		}
	}
	return(text);
}

WideWstring Get_Mission_Description(Wstring const & filename) // RA2: 0x0073F550, the description of the scenario of a file name, a fixed text for the random map and none for an unknown file
{
	if (stricmp(filename.text(), "RandMap.Sed") == 0) { // RA2: as above
		return(WideWstring(StringTable::LoadString("TXT_RANDOM_MAP_DESCRIPTION", NULL, "D:\\Ra2\\wonline.cpp", 2705))); // RA2: as above
	}
	for (int index = 0; index < Session.Scenarios.Count(); index++) { // RA2: as above
		MultiMission * mission = Session.Scenarios[index]; // RA2: as above
		if (stricmp(filename.text(), mission->Get_Filename()) == 0) { // RA2: as above
			return(WideWstring(mission->Description())); // RA2: as above
		}
	}
	return(WideWstring(L"")); // RA2: as above
}

WideWstring Game_List_Tooltip(HWND list, int row) // RA2: 0x0073F5F0, the tooltip of a row of the game list: the host as User_Tooltip makes it, the ping and the description of the scenario
{
	if (row == -1) { // RA2: as above
		return(WideWstring(StringTable::LoadString("STT:LobbyListGames", NULL, "D:\\Ra2\\wonline.cpp", 2722))); // RA2: as above
	}
	Channel channel;
	memset(&channel, 0, sizeof(channel));
	int index = SendMessageA(list, LB_GETITEMDATA, row, 0);
	if (index >= 0 && index < g_GameChannelList.length()) { // RA2: as above
		g_GameChannelList.get(channel, index);
		Wstring host((char *)channel.name);
		host.truncate('\''); // RA2: the host name ends before the first quote
		User user;
		memset(&user, 0, sizeof(user));
		strncpy((char *)user.name, host.get(), 10);
		WideWstring tooltip = User_Tooltip(user);
		tooltip += WideWstring(L", ");
		wchar_t ping_text[64];
		if (channel.latency != -1) {
			swprintf(ping_text, StringTable::LoadString("MSG:PingInfo", NULL, "D:\\Ra2\\wonline.cpp", 2756), channel.latency);
			tooltip += ping_text;
		} else {
			tooltip += StringTable::LoadString("TXT_UNKNOWN_PING", NULL, "D:\\Ra2\\wonline.cpp", 2759);
		}
		char options[132];
		strncpy(options, (char *)channel.exInfo, 128);
		int build = 0;
		int hash = 0;
		int ai_count = 0;
		int observer = 0;
		int not_full = 0;
		Wstring scenario;
		Parse_Channel_ExInfo(options, &build, &hash, &ai_count, &observer, &not_full, &scenario);
		if (scenario.length() > 0) {
			WideWstring description = Get_Mission_Description(scenario);
			if (description.length() > 0) {
				tooltip += L", ";
				tooltip += description;
			}
		}
		return(tooltip);
	}
	return(WideWstring(StringTable::LoadString("STT:LobbyListGames", NULL, "D:\\Ra2\\wonline.cpp", 2729))); // RA2: as above
}

WideWstring User_List_Tooltip(HWND list, int row) // RA2: 0x0073F850, the tooltip of a row of the user list: User_Tooltip of the user
{
	if (row == -1) { // RA2: as above
		return(WideWstring(StringTable::LoadString("STT:LobbyListUsers", NULL, "D:\\Ra2\\wonline.cpp", 2785))); // RA2: as above
	}
	int index = SendMessageA(list, LB_GETITEMDATA, row, 0);
	if (index >= 0 && index < g_UserList.length()) { // RA2: as above
		User user;
		memset(&user, 0, sizeof(user));
		g_UserList.get(user, index);
		return(User_Tooltip(user));
	}
	return(WideWstring(StringTable::LoadString("STT:LobbyListUsers", NULL, "D:\\Ra2\\wonline.cpp", 2790))); // RA2: as above
}

void Refresh_Game_List(void) // RA2: 0x0073F8F0, fills the game list of dialog 0x113 from the game channels: name with the players, kind, password, observer and ping cells, the selection and the first visible row kept
{
	HWND dialog = WS_Find_Dialog(0x113);
	if (!dialog) return; // RA2: as above
	HWND list = GetDlgItem(dialog, 0x6EE);
	if (!list) return; // RA2: as above

	Wstring selected;
	int selected_row = SendMessageA(list, LB_GETCURSEL, 0, 0);
	if (selected_row != -1) Get_Window_Item_Text(list, selected_row, selected);
	Wstring top;
	int top_row = SendMessageA(list, LB_GETTOPINDEX, 0, 0);
	if (top_row != -1) Get_Window_Item_Text(list, top_row, top);
	SendMessageA(list, OD_DISABLEPAINT, 0, 1);
	g_GameListRefreshing = true; // RA2: as above
	SendMessageA(list, LB_RESETCONTENT, 0, 0);

	int count = g_GameChannelList.length();
	for (int i = 0; i < count; ++i) {
		Channel channel;
		memset(&channel, 0, sizeof(channel));
		g_GameChannelList.get(channel, i);
		if (Is_Channel_Flag_11(&channel) || Is_Channel_Flag_12(&channel)) continue; // RA2: as above
		char options[132];
		strncpy(options, (char *)channel.exInfo, 128);
		int build = 0;
		int hash = 0;
		int ai_count = 0;
		int observer = 0;
		int not_full = 0;
		Wstring scenario;
		Parse_Channel_ExInfo(options, &build, &hash, &ai_count, &observer, &not_full, &scenario);
		int participants = channel.currentUsers - observer + ai_count;
		int capacity = channel.maxUsers - ((observer || not_full) ? 1 : 0);
		wchar_t title[80];
		swprintf(title, L"%hs %d/%d", channel.name, participants, capacity); // RA2: as above
		int row = SendMessageA(list, OD_INSERTSTRING, -1, (LPARAM)channel.name);
		SendMessageA(list, LB_SETITEMDATA, row, i);

		OwnerDraw::CellData cell;
		cell.type = OwnerDraw::CellData::TEXT;
		cell.string.set((unsigned short *)title);
		cell.hint.set((unsigned short *)L"");
		cell.color = ColorSystem;
		if (ObserverMode) { // RA2: a game that takes no observer, or has one, cannot be joined as an observer
			if (!not_full || observer) cell.color = ColorNoJoin;
		} else if (participants == capacity) {
			cell.color = ColorNoJoin;
		}
		SendMessageA(list, OD_SETCELL, MAKEWPARAM(g_Col_GameName, row), (LPARAM)&cell);

		cell.type = OwnerDraw::CellData::INVALID;
		cell.hint.set((unsigned short *)L"");
		cell.type = OwnerDraw::CellData::SURFACE;
		cell.hint.set((unsigned short *)L"");
		if (Is_Channel_Flag_8(&channel)) { // RA2: a battle clan game
			cell.surf = SurfaceCache.GetSurface("wolclan.pcx", false);
			cell.hint.set(StringTable::LoadString("TXT_BATTLECLAN_GAME", NULL, "D:\\Ra2\\wonline.cpp", 2903));
		} else if (Is_Channel_Tournament(&channel)) {
			cell.surf = SurfaceCache.GetSurface("woltrny.pcx", false);
			cell.hint.set(StringTable::LoadString("TXT_TOURNAMENT_GAME", NULL, "D:\\Ra2\\wonline.cpp", 2908));
		} else {
			cell.surf = SurfaceCache.GetSurface("gt18.pcx", false);
			cell.hint.set((unsigned short *)L"");
		}
		SendMessageA(list, OD_SETCELL, MAKEWPARAM(g_Col_GameIcon, row), (LPARAM)&cell);

		cell.type = OwnerDraw::CellData::SURFACE;
		cell.hint.set((unsigned short *)L"");
		if (channel.flags & 0x100) {
			cell.surf = SurfaceCache.GetSurface("wolpriv.pcx", false);
			cell.hint.set(StringTable::LoadString("TXT_GAME_PASSWORD", NULL, "D:\\Ra2\\wonline.cpp", 2922));
		} else {
			cell.type = OwnerDraw::CellData::INVALID;
			cell.hint.set((unsigned short *)L"");
		}
		SendMessageA(list, OD_SETCELL, MAKEWPARAM(g_Col_GamePrivate, row), (LPARAM)&cell);

		cell.type = OwnerDraw::CellData::INVALID;
		cell.string.set((unsigned short *)L"");
		cell.hint.set((unsigned short *)L"");
		if (channel.reserved & 0x600) {
			if (!observer && not_full) {
				cell.type = OwnerDraw::CellData::SURFACE;
				cell.surf = SurfaceCache.GetSurface("wolrslkob.pcx", false);
			} else {
				cell.type = OwnerDraw::CellData::SURFACE;
				cell.surf = SurfaceCache.GetSurface("wolreslk.pcx", false);
			}
		} else if (!observer && not_full) {
			cell.type = OwnerDraw::CellData::SURFACE;
			cell.surf = SurfaceCache.GetSurface("wolob.pcx", false);
		}
		SendMessageA(list, OD_SETCELL, MAKEWPARAM(g_Col_GameObserver, row), (LPARAM)&cell);

		if (channel.latency == -1) {
			cell.type = OwnerDraw::CellData::TEXT;
			cell.string.set((unsigned short *)L"???");
		} else {
			cell.pingtime = channel.latency;
			cell.type = OwnerDraw::CellData::PING;
		}
		swprintf(title, StringTable::LoadString("WOL:GamePing", NULL, "D:\\Ra2\\wonline.cpp", 2962), channel.latency);
		cell.hint.set((unsigned short *)title);
		SendMessageA(list, OD_SETCELL, MAKEWPARAM(g_Col_GamePingTime, row), (LPARAM)&cell);
	}

	int restored = SendMessageA(list, OD_FINDSTRING, 0, (LPARAM)selected.text());
	SendMessageA(list, LB_SETCURSEL, restored, 0);
	int restored_top = SendMessageA(list, OD_FINDSTRING, 0, (LPARAM)top.text());
	if (restored_top != -1) SendMessageA(list, LB_SETTOPINDEX, restored_top, 0);
	SendMessageA(list, OD_DISABLEPAINT, 0, 0);
	InvalidateRect(list, NULL, false);
	if (dialog == WS_Top_Window()) SetFocus(GetDlgItem(dialog, 0x540));
}

void Draw_Player_List(int keep_selection, bool force) // RA2: 0x0073FEE0, fills the user list of the lobby (0x113), of the buddy dialog (0x116) or of the game lobby's slot list (Refresh_Slot_Settings)
{
	HWND win = GameoptWindow(); // RA2: the game lobby shows its players in its own slot list, redrawn by 0x005CA000
	HWND userwin; // RA2: as above
	int i;
	if (win) { // RA2: as above
		Refresh_Slot_Settings(win, force); // RA2: as above
		return;
	}
	if (WS_Find_Dialog(0x6B)) return; // RA2: as above
	win = WS_Find_Dialog(0x116); // RA2: the buddy dialog (0x116) is filled first, TibSun has no such dialog
	if (win) { // RA2: as above
		Fill_Buddy_List(win); // RA2: as above
	} else {
		win = WS_Find_Dialog(0x113); // RA2: the lobby dialog is 0x113
	}
	userwin = GetDlgItem(win, 0x53E); // RA2: as above
	if (!win || !userwin) return; // RA2: as above

	Dictionary<WideWstring, bool> lbdict(Wstring_Hash); // RA2: as above
	LBSaveSelections(userwin, lbdict);
	int topindex = SendDlgItemMessageA(win, 0x53E, LB_GETTOPINDEX, 0, 0); // RA2: as above
	SendDlgItemMessageA(win, 0x53E, OD_DISABLEPAINT, 0, 1); // RA2: as above
	SendDlgItemMessageA(win, 0x53E, LB_RESETCONTENT, 0, 0); // RA2: as above

	Wstring ladder_name;
	Ladder ladder;
	memset(&ladder, 0, sizeof(ladder));
	OwnerDraw::CellData thecell;
	Wstring const & nick = g_NickName; // RA2: the const toLower (0x0076EE70) is called on the nickname
	Wstring local_name = nick.toLower(); // RA2: as above
	wchar_t info[80]; // RA2: as above
	wchar_t rank_string[80]; // RA2: as above
	char count_text[20]; // RA2: as above
	User user;
	static wchar_t localestring[64]; // RA2: 0x00B29738

	for (i = 0; i < g_UserList.length(); i++) {
		g_UserList.get(user, i);
		Wstring const user_name((char *)user.name); // RA2: the names are compared as lower case Wstrings
		if (local_name == user_name.toLower()) { // RA2: as above
			HWND hwnd = GetDlgItem(win, 0x555); // RA2: as above
			if (hwnd) {
				LONG style = GetWindowLongA(hwnd, GWL_STYLE); // RA2: as above
				if (user.flags & 1) { // RA2: as above
					style &= ~WS_DISABLED;
				} else {
					style |= WS_DISABLED;
				}
				SetWindowLongA(hwnd, GWL_STYLE, style); // RA2: as above
				InvalidateRect(hwnd, NULL, 0);
			}
			hwnd = GetDlgItem(win, 0x587); // RA2: as above
			if (hwnd) {
				LONG style = GetWindowLongA(hwnd, GWL_STYLE); // RA2: as above
				if (user.flags & 1) { // RA2: as above
					style &= ~WS_DISABLED;
				} else {
					style |= WS_DISABLED;
				}
				SetWindowLongA(hwnd, GWL_STYLE, style); // RA2: as above
				InvalidateRect(hwnd, NULL, 0);
			}
		}

		if (g_CurrentChannel.type == 0) { // RA2: a chat channel
			swprintf(info, L"%hs", user.name); // RA2: the row text is wide
			int row = SendDlgItemMessageA(win, 0x53E, OD_INSERTSTRINGW, (WPARAM)-1, (LPARAM)info); // RA2: message 0x4CC inserts the wide text and returns the row
			thecell.type = OwnerDraw::CellData::TEXT;
			thecell.hint.set(StringTable::LoadString("TXT_UNRANKED", NULL, "D:\\Ra2\\wonline.cpp", 3096)); // RA2: as above
			Char_To_Wide(info, (char *)user.name, -1); // RA2: the cell text is wide
			thecell.string.set(info);
			if (Session.Type == GAME_INTERNET && Online_Session_State == 2) { // RA2: as above
				if (!(user.flags & 1)) { // RA2: as above
					Wstring team_name = Wstring_Narrow(thecell.string.get()); // RA2: the team is looked up by the narrowed name
					if (g_WDTTeams.contains(team_name)) { // RA2: as above
						int color;
						int * value = 0; // RA2: as above
						if (g_WDTTeams.getPointer(team_name, &value) && value) color = *value; // RA2: as above
						WideWstring converted_team(Narrow_To_Wide(team_name.get())); // RA2: as above
						thecell.string = converted_team; // RA2: as above
						thecell.color = PlayerColorTable[color];
					} else {
						thecell.color = ColorSystem;
					}
				} else {
					thecell.color = ColorSystem;
				}
			} else {
				thecell.color = ColorSystem;
			}

			ladder_name.set((char *)user.name);
			ladder_name.toLower();
			memset(localestring, 0, sizeof(localestring));
			int locale = 0;
			if (g_UserLocales.contains(ladder_name) && g_pChat) { // RA2: as above
				int * value = 0; // RA2: as above
				if (g_UserLocales.getPointer(ladder_name, &value) && value) locale = *value; // RA2: as above
				if (locale) {
					WideWstring locale_name = Get_Locale_Name(locale); // RA2: as above
					swprintf(localestring, L"(%hs)", locale_name.get()); // RA2: as above
				}
			}
			if (g_ActiveLadder->getValue(ladder_name, ladder)) {
				swprintf(rank_string, L"%s %d", StringTable::LoadString("TXT_RANK", NULL, "D:\\Ra2\\wonline.cpp", 3162), ladder.rung); // RA2: as above
				thecell.hint.set(rank_string);
			}
			if (g_UserLocales.contains(ladder_name)) {
				if (locale) thecell.hint.insert(L" ", 0); // RA2: as above
				thecell.hint.insert(localestring, 0);
			}
			SendMessageA(GetDlgItem(win, 0x53E), LB_SETITEMDATA, row, i); // RA2: the row remembers its index in the user list
			SendDlgItemMessageA(win, 0x53E, OD_SETCELL, MAKEWPARAM(g_Col_Username, i), (LPARAM)&thecell); // RA2: as above

			thecell.type = OwnerDraw::CellData::TEXT;
			ladder_name.set((char *)user.name);
			ladder_name.toLower();
			rank_string[0] = 0;
			if (strlen((char *)user.squadabbrev)) {
				swprintf(rank_string, L"[%hs]", user.squadabbrev); // RA2: as above
				wchar_t squad_name[200]; // RA2: as above
				Char_To_Wide(squad_name, (char *)user.squadname, -1); // RA2: as above
				thecell.hint.set(squad_name); // RA2: as above
			} else if (g_ActiveLadder->getValue(ladder_name, ladder)) {
				swprintf(rank_string, L"(%s %d)", StringTable::LoadString("TXT_RANK", NULL, "D:\\Ra2\\wonline.cpp", 3204), ladder.rung); // RA2: as above
				thecell.hint.set(L""); // RA2: as above
			}
			thecell.string.set(rank_string);
			SendDlgItemMessageA(win, 0x53E, OD_SETCELL, MAKEWPARAM(g_Col_Rank, i), (LPARAM)&thecell); // RA2: as above

			thecell.type = OwnerDraw::CellData::TEXT;
			if (user.flags & 1) { // RA2: as above
				thecell.type = OwnerDraw::CellData::SURFACE;
				thecell.surf = SurfaceCache.GetSurface("woloper.pcx", false); // RA2: as above
				thecell.hint.set(StringTable::LoadString("TXT_OPER", NULL, "D:\\Ra2\\wonline.cpp", 3216)); // RA2: as above
			} else if (user.flags & 2) { // RA2: as above
				thecell.type = OwnerDraw::CellData::SURFACE;
				thecell.surf = SurfaceCache.GetSurface("wolvoice.pcx", false); // RA2: as above
				thecell.hint.set(StringTable::LoadString("TXT_VOICE", NULL, "D:\\Ra2\\wonline.cpp", 3222)); // RA2: as above
			} else {
				thecell.string.set(L""); // RA2: as above
				thecell.hint.set(L""); // RA2: as above
			}
			SendDlgItemMessageA(win, 0x53E, OD_SETCELL, MAKEWPARAM(g_Col_Chanop, i), (LPARAM)&thecell); // RA2: as above
		} else {
			if (HouseTypes.Count() <= 1) return;
			int slot_index;
			{
				Wstring lookup_name((char *)user.name); // RA2: as above
				slot_index = Player_Name_To_Index(lookup_name); // RA2: as above
			}
			int house = 0; // RA2: as above
			int star = 0; // RA2: as above
			if (slot_index != -1) {
				house = g_UserInfo[slot_index].house;
				if (g_UserInfo[slot_index].accepted) star = 1;
				if (slot_index == 0) {
					g_UserInfo[slot_index].accepted = 1;
					star = 2;
				}
			}
			house = house > 0 ? house : 0; // RA2: nine countries (usai.pcx .. rusi.pcx) instead of GDI and Nod

			ladder_name.set((char *)user.name);
			ladder_name.toLower();
			swprintf(info, L"%hs", user.name); // RA2: the row text is wide
			for (int j = 0; j < (int)wcslen(info); j++) { // RA2: as above
				info[j] = (wchar_t)tolower(info[j]); // RA2: as above
			}
			SendDlgItemMessageA(win, 0x53E, OD_INSERTSTRINGW, (WPARAM)-1, (LPARAM)info); // RA2: as above

			thecell.type = OwnerDraw::CellData::PRIMARY;
			thecell.color = PlayerColorTable[g_UserInfo[slot_index].color];
			thecell.hint.set(StringTable::LoadString("TXT_UNRANKED", NULL, "D:\\Ra2\\wonline.cpp", 3297)); // RA2: as above
			if (g_ActiveLadder->getValue(ladder_name, ladder)) {
				swprintf(rank_string, L"%s %d", StringTable::LoadString("TXT_RANK", NULL, "D:\\Ra2\\wonline.cpp", 3300), ladder.rung); // RA2: as above
				thecell.hint.set(rank_string);
			}
			SendDlgItemMessageA(win, 0x53E, OD_SETCELL, MAKEWPARAM(g_Col_Username, i), (LPARAM)&thecell); // RA2: as above

			thecell.type = OwnerDraw::CellData::TEXT;
			if (strlen((char *)user.squadabbrev)) {
				Char_To_Wide(info, (char *)user.squadname, -1); // RA2: as above
				thecell.hint.set(info);
				Char_To_Wide(info, (char *)user.squadabbrev, -1); // RA2: as above
			} else {
				if (g_ActiveLadder->getValue(ladder_name, ladder)) {
					swprintf(info, L"(%s %d)", StringTable::LoadString("TXT_RANK", NULL, "D:\\Ra2\\wonline.cpp", 3319), ladder.rung); // RA2: as above
					thecell.hint.set(L""); // RA2: as above
				} else {
					swprintf(info, L""); // RA2: as above
					thecell.hint.set(L""); // RA2: as above
				}
			}
			thecell.string.set(info);
			SendDlgItemMessageA(win, 0x53E, OD_SETCELL, MAKEWPARAM(g_Col_Gameinfo, i), (LPARAM)&thecell); // RA2: as above

			thecell.type = OwnerDraw::CellData::SURFACE;
			switch (house) { // RA2: the country icon and its side name (TXT_GDI for the first five, TXT_NOD for the rest)
			case 0:
				thecell.surf = SurfaceCache.GetSurface("usai.pcx", false); // RA2: as above
				swprintf(info, L"%s", StringTable::LoadString("TXT_GDI", NULL, "D:\\Ra2\\wonline.cpp", 3355)); // RA2: as above
				break;
			case 1:
				thecell.surf = SurfaceCache.GetSurface("japi.pcx", false); // RA2: as above
				swprintf(info, L"%s", StringTable::LoadString("TXT_GDI", NULL, "D:\\Ra2\\wonline.cpp", 3359)); // RA2: as above
				break;
			case 2:
				thecell.surf = SurfaceCache.GetSurface("frai.pcx", false); // RA2: as above
				swprintf(info, L"%s", StringTable::LoadString("TXT_GDI", NULL, "D:\\Ra2\\wonline.cpp", 3363)); // RA2: as above
				break;
			case 3:
				thecell.surf = SurfaceCache.GetSurface("geri.pcx", false); // RA2: as above
				swprintf(info, L"%s", StringTable::LoadString("TXT_GDI", NULL, "D:\\Ra2\\wonline.cpp", 3367)); // RA2: as above
				break;
			case 4:
				thecell.surf = SurfaceCache.GetSurface("gbri.pcx", false); // RA2: as above
				swprintf(info, L"%s", StringTable::LoadString("TXT_GDI", NULL, "D:\\Ra2\\wonline.cpp", 3371)); // RA2: as above
				break;
			case 5:
				thecell.surf = SurfaceCache.GetSurface("djbi.pcx", false); // RA2: as above
				swprintf(info, L"%s", StringTable::LoadString("TXT_NOD", NULL, "D:\\Ra2\\wonline.cpp", 3375)); // RA2: as above
				break;
			case 6: // RA2: as above
				thecell.surf = SurfaceCache.GetSurface("arbi.pcx", false); // RA2: as above
				swprintf(info, L"%s", StringTable::LoadString("TXT_NOD", NULL, "D:\\Ra2\\wonline.cpp", 3379)); // RA2: as above
				break;
			case 7: // RA2: as above
				thecell.surf = SurfaceCache.GetSurface("lati.pcx", false); // RA2: as above
				swprintf(info, L"%s", StringTable::LoadString("TXT_NOD", NULL, "D:\\Ra2\\wonline.cpp", 3383)); // RA2: as above
				break;
			case 8: // RA2: as above
				thecell.surf = SurfaceCache.GetSurface("rusi.pcx", false); // RA2: as above
				swprintf(info, L"%s", StringTable::LoadString("TXT_NOD", NULL, "D:\\Ra2\\wonline.cpp", 3387)); // RA2: as above
				break;
			}
			thecell.hint.set(info);
			SendDlgItemMessageA(win, 0x53E, OD_SETCELL, MAKEWPARAM(g_Col_House, i), (LPARAM)&thecell); // RA2: as above

			thecell.type = OwnerDraw::CellData::SURFACE;
			if (star) {
				if (star == 2) {
					thecell.surf = SurfaceCache.GetSurface("wolhost.pcx", false); // RA2: as above
				} else {
					thecell.surf = SurfaceCache.GetSurface("wolacpt.pcx", false); // RA2: as above
				}
				thecell.hint.set(StringTable::LoadString("TXT_ACCEPTED", NULL, "D:\\Ra2\\wonline.cpp", 3401)); // RA2: as above
			} else {
				thecell.type = OwnerDraw::CellData::INVALID;
				thecell.hint.set(L""); // RA2: as above
			}
			SendDlgItemMessageA(win, 0x53E, OD_SETCELL, MAKEWPARAM(g_Col_Accept, i), (LPARAM)&thecell); // RA2: as above

			int average_ping = 0; // RA2: as above
			g_pNetUtil->GetAvgPing(user.ipaddr, &average_ping);
			if (average_ping < 0) average_ping = 0;
			thecell.type = OwnerDraw::CellData::PING;
			thecell.pingtime = average_ping;
			wchar_t ping_text[200]; // RA2: as above
			swprintf(ping_text, StringTable::LoadString("MSG:PingInfo", NULL, "D:\\Ra2\\wonline.cpp", 3417), average_ping); // RA2: as above
			if (average_ping == 0) {
				swprintf(ping_text, StringTable::LoadString("TXT_UNKNOWN_PING", NULL, "D:\\Ra2\\wonline.cpp", 3419)); // RA2: as above
			}
			thecell.hint.set(ping_text); // RA2: as above
			Wstring const ping_name((char *)user.name); // RA2: as above
			if (local_name == ping_name.toLower()) { // RA2: as above
				thecell.type = OwnerDraw::CellData::INVALID;
			}
			SendDlgItemMessageA(win, 0x53E, OD_SETCELL, MAKEWPARAM(g_Col_GamePing, i), (LPARAM)&thecell); // RA2: as above
		}
	}

	if (keep_selection == 0) LBRestoreSelections(userwin, lbdict);
	int sel_count = SendDlgItemMessageA(win, 0x53E, LB_GETSELCOUNT, 0, 0); // RA2: as above
	int user_count = g_UserList.length();
	if (sel_count > 0) {
		sprintf(count_text, "%d  (%d)", user_count, sel_count); // RA2: as above
	} else {
		sprintf(count_text, "%d", user_count); // RA2: as above
	}
	SendDlgItemMessageA(win, 0x60B, OD_SETTEXT, 0, (LPARAM)count_text); // RA2: message 0x4B4 sets the text of the count label
	SendDlgItemMessageA(win, 0x53E, LB_SETTOPINDEX, (WPARAM)topindex, 0); // RA2: as above
	SendDlgItemMessageA(win, 0x53E, OD_DISABLEPAINT, 0, 0); // RA2: as above
	InvalidateRect(GetDlgItem(win, 0x53E), NULL, 0); // RA2: as above
}


// RA2: the event sinks of the chat, net utility and download services. Their handlers are defined with the rest of the file (they are not part of this part); the classes are declared here as the executable creates them in Startup_Chat (0x00741300): each is 32 bytes, the vtable pointer, the reference count and the critical section of CComObjectRoot
class CChatEventSink : public CComObjectRoot, public IChatEvent // RA2: the sink the executable creates in Startup_Chat
{
	public:
		CChatEventSink(void) {}
		BEGIN_COM_MAP(CChatEventSink)
		COM_INTERFACE_ENTRY(IChatEvent)
		END_COM_MAP()
		STDMETHOD(OnServerList)(HRESULT res, Server *servers); // RA2: a handler of the interface, defined with the rest of the file
		STDMETHOD(OnUpdateList)(HRESULT res, Update *updates); // RA2: a handler of the interface, defined with the rest of the file
		STDMETHOD(OnServerError)(HRESULT res, LPCSTR ircmsg); // RA2: a handler of the interface, defined with the rest of the file
		STDMETHOD(OnConnection)(HRESULT res, LPCSTR motd); // RA2: a handler of the interface, defined with the rest of the file
		STDMETHOD(OnMessageOfTheDay)(HRESULT res, LPCSTR motd); // RA2: a handler of the interface, defined with the rest of the file
		STDMETHOD(OnChannelList)(HRESULT res, Channel *channels); // RA2: a handler of the interface, defined with the rest of the file
		STDMETHOD(OnChannelCreate)(HRESULT res, Channel *channel); // RA2: a handler of the interface, defined with the rest of the file
		STDMETHOD(OnChannelJoin)(HRESULT res, Channel *channel, User *user); // RA2: a handler of the interface, defined with the rest of the file
		STDMETHOD(OnChannelLeave)(HRESULT res, Channel *channel, User *user); // RA2: a handler of the interface, defined with the rest of the file
		STDMETHOD(OnChannelTopic)(HRESULT res, Channel *channel, LPCSTR topic); // RA2: a handler of the interface, defined with the rest of the file
		STDMETHOD(OnPrivateAction)(HRESULT res, User *user, LPCSTR action); // RA2: a handler of the interface, defined with the rest of the file
		STDMETHOD(OnPublicAction)(HRESULT res, Channel *channel, User *user, LPCSTR action); // RA2: a handler of the interface, defined with the rest of the file
		STDMETHOD(OnUserList)(HRESULT res, Channel *channel, User *users); // RA2: a handler of the interface, defined with the rest of the file
		STDMETHOD(OnPublicMessage)(HRESULT res, Channel *channel, User *user, LPCSTR message); // RA2: a handler of the interface, defined with the rest of the file
		STDMETHOD(OnPrivateMessage)(HRESULT res, User *user, LPCSTR message); // RA2: a handler of the interface, defined with the rest of the file
		STDMETHOD(OnSystemMessage)(HRESULT res, LPCSTR message); // RA2: a handler of the interface, defined with the rest of the file
		STDMETHOD(OnNetStatus)(HRESULT res); // RA2: a handler of the interface, defined with the rest of the file
		STDMETHOD(OnLogout)(HRESULT status, User *user); // RA2: a handler of the interface, defined with the rest of the file
		STDMETHOD(OnPrivateGameOptions)(HRESULT res, User *user, LPCSTR options); // RA2: a handler of the interface, defined with the rest of the file
		STDMETHOD(OnPublicGameOptions)(HRESULT res, Channel *channel, User *user, LPCSTR options); // RA2: a handler of the interface, defined with the rest of the file
		STDMETHOD(OnGameStart)(HRESULT res, Channel *channel, User *users, int gameid); // RA2: a handler of the interface, defined with the rest of the file
		STDMETHOD(OnUserKick)(HRESULT res, Channel *channel, User *kicked, User *kicker); // RA2: a handler of the interface, defined with the rest of the file
		STDMETHOD(OnUserIP)(HRESULT res, User *user); // RA2: a handler of the interface, defined with the rest of the file
		STDMETHOD(OnFind)(HRESULT res, Channel *chan); // RA2: a handler of the interface, defined with the rest of the file
		STDMETHOD(OnPageSend)(HRESULT res); // RA2: a handler of the interface, defined with the rest of the file
		STDMETHOD(OnPaged)(HRESULT res, User *user, LPCSTR message); // RA2: a handler of the interface, defined with the rest of the file
		STDMETHOD(OnServerBannedYou)(HRESULT res, time_t bannedTill); // RA2: a handler of the interface, defined with the rest of the file
		STDMETHOD(OnUserFlags)(HRESULT res, LPCSTR name, unsigned int flags, unsigned int mask); // RA2: a handler of the interface, defined with the rest of the file
		STDMETHOD(OnChannelBan)(HRESULT res, LPCSTR name, int banned); // RA2: a handler of the interface, defined with the rest of the file
		STDMETHOD(OnSquadInfo)(HRESULT res, unsigned long id, Squad *squad); // RA2: a handler of the interface, defined with the rest of the file
		STDMETHOD(OnUserLocale)(HRESULT res, User *users); // RA2: a handler of the interface, defined with the rest of the file
		STDMETHOD(OnUserTeam)(HRESULT res, User *users); // RA2: a handler of the interface, defined with the rest of the file
		STDMETHOD(OnSetLocale)(HRESULT res, Locale newlocale); // RA2: a handler of the interface, defined with the rest of the file
		STDMETHOD(OnSetTeam)(HRESULT res, int newteam); // RA2: a handler of the interface, defined with the rest of the file
};

class CNetUtilEventSink : public CComObjectRoot, public INetUtilEvent // RA2: the sink the executable creates in Startup_Chat
{
	public:
		CNetUtilEventSink(void) {}
		BEGIN_COM_MAP(CNetUtilEventSink)
		COM_INTERFACE_ENTRY(INetUtilEvent)
		END_COM_MAP()
		STDMETHOD(OnPing)(HRESULT res, int time, unsigned long ip, int handle); // RA2: a handler of the interface, defined with the rest of the file
		STDMETHOD(OnLadderList)(HRESULT res, Ladder *list, int totalCount, long timeStamp, int keyRung); // RA2: a handler of the interface, defined with the rest of the file
		STDMETHOD(OnGameresSent)(HRESULT res); // RA2: a handler of the interface, defined with the rest of the file
		STDMETHOD(OnNewNick)(HRESULT res, LPCSTR message, LPCSTR nick, LPCSTR pass); // RA2: a handler of the interface, defined with the rest of the file
		STDMETHOD(OnAgeCheck)(HRESULT res, int years, int consent); // RA2: a handler of the interface, defined with the rest of the file
		STDMETHOD(OnWDTState)(HRESULT res, unsigned char *state, int length); // RA2: a handler of the interface, defined with the rest of the file
};

class CDownloadEventSink : public CComObjectRoot, public IDownloadEvent // RA2: the sink the executable creates in Startup_Chat
{
	public:
		CDownloadEventSink(void) {}
		BEGIN_COM_MAP(CDownloadEventSink)
		COM_INTERFACE_ENTRY(IDownloadEvent)
		END_COM_MAP()
		STDMETHOD(OnEnd)(void); // RA2: a handler of the interface, defined with the rest of the file
		STDMETHOD(OnError)(int error); // RA2: a handler of the interface, defined with the rest of the file
		STDMETHOD(OnProgressUpdate)(int bytesread, int totalsize, int timetaken, int timeleft); // RA2: a handler of the interface, defined with the rest of the file
		STDMETHOD(OnQueryResume)(void); // RA2: a handler of the interface, defined with the rest of the file
		STDMETHOD(OnStatusUpdate)(int status); // RA2: a handler of the interface, defined with the rest of the file
};

extern CComObject<CChatEventSink> * g_pChatSink; // RA2: 0x00B2A720
extern CComObject<CNetUtilEventSink> * g_pNetUtilSink; // RA2: 0x00B26560
extern CComObject<CDownloadEventSink> * g_pDownloadSink; // RA2: 0x00B28DE0

void Redraw_Player_List(void) // RA2: 0x007412F0, Draw_Player_List with the default arguments (a jump to it)
{
	Draw_Player_List(0, false); // RA2: as above
}

extern CLSID const CLSID_Chat; // RA2: 0x007AF640, the class ids of the three services
extern CLSID const CLSID_NetUtil; // RA2: 0x007AF660
extern CLSID const CLSID_Download; // RA2: 0x007AF650
extern IID const IID_IChat; // RA2: 0x007AF5A0, the interface ids
extern IID const IID_IChatEvent; // RA2: 0x007AF5B0
extern IID const IID_INetUtil; // RA2: 0x007AF5E0
extern IID const IID_INetUtilEvent; // RA2: 0x007AF5F0
extern IID const IID_IDownload; // RA2: 0x007AF5C0
extern IID const IID_IDownloadEvent; // RA2: 0x007AF5D0

int Startup_Chat(HINSTANCE hInstance) // RA2: 0x00741300, creates the chat, net utility and download services with their event sinks, checks the version of the chat API and tells it the registry key and the SKU; 1 when all is well, -2 when a service is missing or too old
{
	HRESULT hresult = NULL;
	_Module.Init(NULL, hInstance);

	g_pChat = NULL;
	g_pNetUtil = NULL;
	g_pDownload = NULL;
	g_dwChatAdvise = 0;
	g_dwNetUtilAdvise = 0;
	g_dwDownloadAdvise = 0;

	CoCreateInstance(CLSID_Chat, NULL, CLSCTX_INPROC_SERVER, IID_IChat, (void **)&g_pChat);
	if (g_pChat) {
		CComObject<CChatEventSink>::CreateInstance(&g_pChatSink);
		hresult = AtlAdvise(g_pChat, g_pChatSink->GetUnknown(), IID_IChatEvent, &g_dwChatAdvise);
	}
	if ((g_pChat == NULL) || (hresult != S_OK)) {
		ODMessageBox(StringTable::LoadString("TXT_APIMISSING", NULL, "D:\\Ra2\\wonline.cpp", 7881), 0, NULL, false); // RA2: as above
		return(-2); // RA2: as above
	}

	CoCreateInstance(CLSID_NetUtil, NULL, CLSCTX_INPROC_SERVER, IID_INetUtil, (void **)&g_pNetUtil);
	if (g_pNetUtil) {
		CComObject<CNetUtilEventSink>::CreateInstance(&g_pNetUtilSink);
		hresult = AtlAdvise(g_pNetUtil, g_pNetUtilSink->GetUnknown(), IID_INetUtilEvent, &g_dwNetUtilAdvise);
	}
	if ((g_pNetUtil == NULL) || (hresult != S_OK)) {
		ODMessageBox(StringTable::LoadString("TXT_APIMISSING", NULL, "D:\\Ra2\\wonline.cpp", 7897), 0, NULL, false); // RA2: as above
		return(-2); // RA2: as above
	}

	CoCreateInstance(CLSID_Download, NULL, CLSCTX_INPROC_SERVER, IID_IDownload, (void **)&g_pDownload);
	if (g_pDownload) {
		CComObject<CDownloadEventSink>::CreateInstance(&g_pDownloadSink);
		hresult = AtlAdvise(g_pDownload, g_pDownloadSink->GetUnknown(), IID_IDownloadEvent, &g_dwDownloadAdvise);
	}
	if ((g_pDownload == NULL) || (hresult != S_OK)) {
		ODMessageBox(StringTable::LoadString("TXT_APIMISSING", NULL, "D:\\Ra2\\wonline.cpp", 7913), 0, NULL, false); // RA2: as above
		return(-2); // RA2: as above
	}

	unsigned long version;
	g_pChat->GetVersion(&version);
	if (version < 0x1000E) { // RA2: the API of RA2 is newer than TibSun's
		ODMessageBox(StringTable::LoadString("TXT_APIWRONGVERSION", NULL, "D:\\Ra2\\wonline.cpp", 7925), 0, NULL, false); // RA2: as above
		return(-2); // RA2: as above
	}

	g_pChat->SetAttributeValue("RegPath", APP_REG_KEY);

	HKEY rKey;
	char buf[256];
	if (APP_REG_KEY != NULL && strlen(APP_REG_KEY)) {
		strcpy(buf, APP_REG_KEY);
		if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, buf, 0, KEY_READ, &rKey) == ERROR_SUCCESS) {
			DWORD type;
			DWORD value;
			DWORD valuesize = sizeof(value);
			if (RegQueryValueEx(rKey, "SKU", NULL, &type, (LPBYTE)&value, &valuesize) == ERROR_SUCCESS) {
				g_pChat->SetProductSKU(value);
			}
			RegCloseKey(rKey);
		}
	}

	return(1); // RA2: as above
}

/// <summary>
/// Determines the sort order of two channel users.
/// Channel owners come first, then users with voice, then everyone else; equals are listed
/// alphabetically. This is the order the user list box is kept in.
/// </summary>
/// <returns>Returns with a negative value if the u1 user sorts first, a positive value if
/// the u2 one does, and zero if the two cannot be told apart.</returns>
int Compare_Users(User * u1, User * u2) // RA2: 0x007416D0, the same as TibSun's
{
	User * users[2];
	int value[2];
	users[0] = u1;
	users[1] = u2;
	value[0] = 0;
	value[1] = 0;

	for (int i = 0; i < 2; i++) {
		if (users[i]->flags & 1) value[i] += 4; // RA2: as above
		if (users[i]->flags & 2) value[i] += 2; // RA2: as above
	}
	if (value[0] < value[1]) {
		return(1);
	} else if (value[0] > value[1]) {
		return(-1);
	}
	return(stricmp((char *)u1->name, (char *)u2->name));
}

#include "multiplayer.h" // RA2: the game mode of the session, the handlers of the end of the game

namespace OwnerDraw { BOOL __stdcall Set_Paint_Disabled(HWND window, LPARAM lparam); } // RA2: 0x005ECF40, defined in ownrdraw.cpp
void __fastcall Set_Player_Ping(HWND dialog, int player, int ping); // RA2: 0x004D1060, defined in gdlgsupp.cpp
void __fastcall Enable_Start_Button(void); // RA2: 0x005C9700, defined in netshare.cpp
void __fastcall Release_Slot_By_Name(char const * name); // RA2: 0x005C9270, defined in netshare.cpp
int Join_Selected_Lobby(bool join); // RA2: 0x00744930, defined later in the file
int Pick_Lobby(bool require_room); // RA2: 0x00744090, the index of the lobby channel to join, -1 when none
void __fastcall Store_User_Ping(char const * name, int ping); // RA2: 0x0075B110, a user's ping (WOL), defined later in the file
extern int g_PlayerPings[8]; // RA2: 0x00B292AC, the ping of each slot of the game lobby

/// <summary>
/// Handles a user leaving the current channel. If the departing user is ourselves, resets all
/// local channel/session state and returns to the lobby (closing any open guest or game-options
/// dialogs); otherwise removes the given user from our local user list and updates the
/// game-options acceptance state before refreshing the user list display.
/// </summary>
/// <param name="user">The user who left the channel.</param>
void Handle_User_Leave(User & user) // RA2: 0x00741750
{
	stricmp(g_NickName.get(), (char *)user.name); // RA2: the result is not used

	if (g_CurrentChannel.type != 0) VocClass::PlayGlobal(Rule->PlayerLeft, 1.0f, 0x2000, 0); // RA2: as above

	if (user.flags & 0x8000) { // RA2: as above
		DebugString("Local user leaving channel\n"); // RA2: as above
		if (WolOptionsLocked != 0) DebugString("Local user leaving channel after game start\n"); // RA2: as above
		wchar_t name[64]; // RA2: as above
		Char_To_Wide(name, (char *)g_CurrentChannel.name, -1); // RA2: the name is wide
		if (strncmp((char *)g_CurrentChannel.name, "Lob_33_", 7) == 0) { // RA2: as above
			int num = atol(((char *)g_CurrentChannel.name) + strlen("Lob_33_")); // RA2: as above
			int num2 = num / 8; // RA2: as above
			int num1 = num % 8; // RA2: as above
			if (num2 == 0) {
				swprintf(name, L"%s", g_Lobbies[num1]); // RA2: as above
			} else {
				swprintf(name, L"%s %d", g_Lobbies[num1], num2 + 1); // RA2: as above
			}
		}
		if (Online_Session_State != 2 && Online_Session_State != 1) { // RA2: no message in a LAN or WDT game
			Lobby_Chat_Printf(ColorSystem, StringTable::LoadString("TXT_YOULEFT", NULL, "D:\\Ra2\\wonline.cpp", 7993), name); // RA2: as above
		}
		g_ChannelCount = 0;
		DebugString("Channel Leave \n"); // RA2: as above
		memset(&g_CurrentChannel, 0, sizeof(Channel));
		memset(g_UserInfo, 0, sizeof(ChannelUserInfo) * 16); // RA2: sixteen users
		g_UserList.clear();
		g_IsChannelCreator = 0;
		HWND wait_dlg = WS_Find_Dialog(0xC4); // RA2: Close_Wait_Window inline, for the channel leave event
		if (wait_dlg) { // RA2: as above
			g_ActiveWaitFlags &= ~0x40; // RA2: as above
			if (g_ActiveWaitFlags == 0) {
				WS_Destroy_Dialog(wait_dlg, 0); // RA2: as above
			}
		}
		HWND top = WS_Top_Window(); // RA2: as above
		HWND control = GetDlgItem(top, 0x540); // RA2: as above
		if (control) { // RA2: as above
			SetFocus(control); // RA2: as above
		}

		++Session.Suspended; // RA2: a count that is raised while the dialogs are closed, +0x2824 of the session
		Session.Options.ScenarioDescription[0] = 0;
		Session.ScenarioFileLength = 0;
		Session.ScenarioDigest[0] = 0;
		HWND mainwin = WS_Find_Dialog(0x113); // RA2: as above
		if (mainwin && g_PlayingNetGame) { // RA2: as above
			ShowWindow(mainwin, SW_HIDE); // RA2: as above
			OwnerDraw::Set_Paint_Disabled(mainwin, 1); // RA2: as above
		}
		HWND guest_dlg = WS_Find_Dialog(0xC9); // RA2: as above
		if (guest_dlg) {
			WS_Destroy_Dialog(guest_dlg, 2); // RA2: as above
			ShowWindow(guest_dlg, SW_HIDE); // RA2: as above
			OwnerDraw::Set_Paint_Disabled(guest_dlg, 1); // RA2: as above
			if (Online_Session_State == 3) { // RA2: as above
				if (g_PlayingNetGame) {
					if (mainwin) {
						ShowWindow(mainwin, SW_HIDE); // RA2: as above
						OwnerDraw::Set_Paint_Disabled(mainwin, 1); // RA2: as above
					}
				} else {
					Join_Selected_Lobby(false); // RA2: as above
					if (mainwin) PostMessageA(mainwin, 0x593, 0, 0); // RA2: as above
				}
			} else if (Online_Session_State == 4) { // RA2: as above
				DebugString("Setting g_JoinLobbyNow, Guest\n"); // RA2: as above
				g_JoinLobbyNow = 1;
			}
		}
		HWND gamewin = WS_Find_Dialog(0xC2); // RA2: as above
		if (gamewin) {
			WS_Destroy_Dialog(gamewin, 2); // RA2: as above
			ShowWindow(gamewin, SW_HIDE); // RA2: as above
			OwnerDraw::Set_Paint_Disabled(gamewin, 1); // RA2: as above
			if (Online_Session_State == 3) { // RA2: as above
				if (g_PlayingNetGame) {
					if (mainwin) {
						ShowWindow(mainwin, SW_HIDE); // RA2: as above
						OwnerDraw::Set_Paint_Disabled(mainwin, 1); // RA2: as above
					}
				} else {
					Join_Selected_Lobby(false); // RA2: as above
					if (mainwin) PostMessageA(mainwin, 0x593, 0, 0); // RA2: as above
				}
			} else if (Online_Session_State == 4) { // RA2: as above
				DebugString("Setting g_JoinLobbyNow, Host\n"); // RA2: as above
			}
		}
		if (mainwin) {
			if (!g_PlayingNetGame) { // RA2: as above
				ShowWindow(mainwin, SW_SHOW);
				OwnerDraw::Set_Paint_Disabled(mainwin, 0); // RA2: as above
			}
			Refresh_Game_List(); // RA2: the game list is redrawn, there is no channel list
		}
		--Session.Suspended; // RA2: as above
		return;
	}

	if (WolOptionsLocked == 2) { // RA2: the game was started, nothing else to do for another player
		DebugString("other player leaving channel after Game Started\n"); // RA2: as above
		return;
	}

	if (&user) { // RA2: the slot of the leaving player shows no ping
		for (int slot = 0; slot < 8; slot++) { // RA2: the pings of the slots go to the user list
			if (Session.Slots[slot] && !strcmp(Wide_To_Narrow(Session.Slots[slot]->Name), (char *)user.name)) { // RA2: the player's slot gets no ping
				HWND lobby = GameoptWindow(); // RA2: as above
				if (lobby) Set_Player_Ping(lobby, slot, -1); // RA2: as above
				break;
			}
		}
	}

	User temp;
	for (int i = 0; i < g_UserList.length(); i++) {
		g_UserList.get(temp, i);
		if (stricmp((char *)temp.name, (char *)user.name) == 0) { // RA2: as above
			g_UserList.remove(i);
			if (g_CurrentChannel.type != 0) { // RA2: as above
				memmove(g_UserInfo + i, g_UserInfo + i + 1, sizeof(ChannelUserInfo) * (15 - i)); // RA2: sixteen users
				if (WolOptionsLocked != 2) { // RA2: as above
					for (int * accepted = &g_UserInfo[1].accepted; accepted < &g_UserInfo[16].accepted; accepted += sizeof(ChannelUserInfo) / sizeof(int)) { // RA2: every user but the host loses the accept status
						*accepted = 0;
						EnableWindow(GetDlgItem(GameoptWindow(), 0x59F), 1); // RA2: as above
					}
					if (!g_IsChannelCreator) { // RA2: as above
						HWND guest = WS_Find_Dialog(0xC9); // RA2: as above
						Enable_Player_Combos(guest, true); // RA2: a guest gets the combos back
					}
					Enable_Start_Button(); // RA2: as above
				}
			}
			if (i == 0 && g_CurrentChannel.type != 0 && g_IsChannelCreator == 0) { // RA2: as above
				g_pChat->RequestChannelLeave();
			}
			break;
		}
	}

	if (g_CurrentChannel.type != 0) { // RA2: as above
		Release_Slot_By_Name((char *)user.name); // RA2: frees the slot of the player
		for (int player = 0; player < Session.Players.Count(); player++) { // RA2: as above
			if (stricmp(Wide_To_Narrow(Session.Players[player]->Name), (char *)user.name) == 0) { // RA2: the player is dropped from the list
				NodeNameType * entry = Session.Players[player]; // RA2: as above
				Session.Players.Delete_Index(player); // RA2: as above
				Session.Remove_Player_Port(player); // RA2: as above
				delete entry; // RA2: as above
				break;
			}
		}
		for (int slot = 0; slot < 8; slot++) { // RA2: the pings of the slots go to the user list
			if (!Session.Slots[slot] || !Is_Real_Player_Entry(Session.Slots[slot])) g_PlayerPings[slot] = -1; // RA2: as above
			char * slot_name = Wide_To_Narrow(Session.Slots[slot]->Name); // RA2: as above
			Store_User_Ping(slot_name, g_PlayerPings[slot]); // RA2: as above
		}
		if (g_IsChannelCreator) { // RA2: as above
			Enable_Start_Button(); // RA2: as above
			Send_Player_List(); // RA2: as above
			Send_Lobby_Options(true, true); // RA2: PumpGameopts(true, true)
			if (WS_Top_Window_ID() == 0x6B) { // RA2: in the game list dialog the game mode of the selected row is told
				HWND window = WS_Top_Window(); // RA2: as above
				int index = SendDlgItemMessageA(window, 0x6EB, LB_GETCURSEL, 0, 0); // RA2: as above
				if (index != -1) {
					MultiplayerGameMode * mode = (MultiplayerGameMode *)SendDlgItemMessageA(window, 0x6EB, LB_GETITEMDATA, index, 0); // RA2: as above
					if (mode) { // RA2: as above
						mode->vt_entry_78(); // RA2: as above
						Draw_Player_List(0, false); // RA2: as above
						return;
					}
				}
			} else if (Session.MPGameMode) { // RA2: as above
				Session.MPGameMode->vt_entry_78(); // RA2: as above
			}
		}
	}
	Draw_Player_List(0, false); // RA2: as above
}

extern bool g_WOLInitialized; // RA2: 0x00B2AC81, set once the chat services are up, tested by Shutdown_Chat
void __fastcall Free_Lobby_Entries(void); // RA2: 0x005C8950, defined in netshare.cpp

static inline void Reset_Wait_Event_At(int index) { ResetEvent(g_WaitEventHandles[index]); }
static inline void Reset_All_Wait_Events(void) { for (int i = 0; i < EV_COUNT; i++) { Reset_Wait_Event_At(i); } }
/// <summary>
/// Logs the local player out of the WOL chat session. Sends a logout request to the Chat
/// interface and waits (with timeout retries) for it to complete or be aborted, then, for
/// World Domination Tour sessions, also closes the event handles and fully shuts down chat.
/// </summary>
void Logout_WOnline(void) // RA2: 0x00741EA0
{
	int i;
	DWORD wait_result;

	g_IgnoreNetStatus = TRUE;

	if (Session.Type == GAME_INTERNET && Online_Session_State == 2) { // RA2: the team lookups of the WDT are dropped
		g_WDTTeams.clear();
	}

	if (g_pChat != NULL) {
		Reset_All_Wait_Events();
		WOL_Wait_Callback();
		if (g_pChat->RequestLogout() == S_OK) {
			{
				WideWstring title(StringTable::LoadString("TXT_WOL_LOGGING_OUT", NULL, "D:\\Ra2\\wonline.cpp", 8220)); // RA2: the wait dialog takes a wide text
				Show_Wait_Window(0x20, title, false);
			}
			for (i = 0; i < 5; i++) {
				while ((wait_result = WaitForMultipleObjects(EV_COUNT, g_WaitEventHandles, FALSE, 100)) == WAIT_TIMEOUT) {
					WOL_Wait_Callback();
				}
				ResetEvent(g_WaitEventHandles[wait_result]);
				if (wait_result == EV_ABORT) {
					break;
				}
				if (wait_result == EV_NETIGNORE) {
					break;
				}
			}
		}

		HWND profile = WS_Find_Dialog(0x11E); // RA2: the profile dialog is closed first
		if (profile) WS_Destroy_Dialog(profile, 0);
		Close_Wait_Window(0x20); // RA2: expanded here
	}
}

/// <summary>
/// Tears down the WOL COM interfaces: unadvises each event sink, releases the Chat, NetUtil,
/// and Download interfaces, clears the global pointers, and terminates the ATL module.
/// </summary>
void Shutdown_Chat(void) // RA2: 0x00742080
{
	if (MultiplayerMapPreview) { // RA2: the preview of the map is dropped
		delete MultiplayerMapPreview;
		MultiplayerMapPreview = NULL;
	}
	HWND profile = WS_Find_Dialog(0x11E); // RA2: the profile dialog is closed first
	if (profile) WS_Destroy_Dialog(profile, 0); // RA2: as above
	if (g_WOLInitialized) { // RA2: only after Startup_Chat
		Free_Lobby_Entries(); // RA2: the fixed lobby entries are dropped
		if (g_pChat) {
			AtlUnadvise(g_pChat, IID_IChatEvent, g_dwChatAdvise);
			g_pChat->Release();
		}
		if (g_pNetUtil) {
			AtlUnadvise(g_pNetUtil, IID_INetUtilEvent, g_dwNetUtilAdvise);
			g_pNetUtil->Release();
		}
		if (g_pDownload) {
			AtlUnadvise(g_pDownload, IID_IDownloadEvent, g_dwDownloadAdvise);
			g_pDownload->Release();
		}
		g_pChat = NULL;
		g_pNetUtil = NULL;
		g_pDownload = NULL;
		for (int i = 0; i < EV_COUNT; i++) {
			CloseHandle(g_WaitEventHandles[i]);
		}
		_Module.Term();
		g_WOLInitialized = false; // RA2: as above
	}
}

extern char g_WDTHost[]; // RA2: 0x00B269AC, the host of the WDT server (Request_WDT_Server_List sets it)
extern int g_WDTPort; // RA2: 0x00B299BC

/// <summary>
/// Requests the World Domination Tour state.
/// This routine puts up no wait window of its own, which is what lets the wait callback
/// poll the tour state in the background while the player is doing something else. The
/// language code is refreshed from the installed product's SKU on the way through.
/// </summary>
/// <returns>Returns with true if a request was issued, or false if there is no NetUtil connection
/// to issue it on.</returns>
bool Get_WDT_State_Silent(void) // RA2: 0x00742270, returns a bool
{
	DWORD value = 0;

	HKEY rKey;
	char buf[256];
	if (APP_REG_KEY != NULL && strlen(APP_REG_KEY)) {
		strcpy(buf, APP_REG_KEY);
		if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, buf, 0, KEY_READ, &rKey) == ERROR_SUCCESS) {
			DWORD type;
			DWORD valuesize = sizeof(value);
			RegQueryValueEx(rKey, "SKU", NULL, &type, (LPBYTE)&value, &valuesize);
			RegCloseKey(rKey);
		}
	}

	g_LanguageCode = value & 0xFF;
	if (g_pChat != NULL) {
		g_pChat->SetProductSKU(value);
	}

	if (g_pNetUtil != NULL) {
		DebugString("Get_WDT_State_Silent: WDTHost is %s, port is %d\n", g_WDTHost, g_WDTPort);
		g_pNetUtil->RequestWDTState(g_WDTHost, g_WDTPort, 3 /* RA2: everything, as WDT_REQ_STATE_EVERYTHING of wdtnet.h */); // RA2: 3 asks for everything (WDT_REQ_STATE_EVERYTHING of wdtnet.h, which this file does not include)
		return(true); // RA2: as above
	}
	return(false); // RA2: as above
}

void __fastcall Init_Lobby_Entries(void); // RA2: 0x005C86E0, defined in netshare.cpp
void __fastcall Clear_Slot_Lists(void); // RA2: 0x005C8A70, as above
void __fastcall Reset_Slots(void); // RA2: 0x005C8A90, as above

int Init_Wonline(void) // RA2: 0x00742390, not in TibSun: brings the chat services up once (the work Login_WOL did itself in TibSun), 1 when they already are, else the result of Startup_Chat
{
	if (g_WOLInitialized) return(1); // RA2: as above
	g_WOLInitialized = true; // RA2: set before the services come up
	Init_Lobby_Entries(); // RA2: the fixed lobby entries (open, closed, computers)
	Clear_Slot_Lists(); // RA2: as above
	Reset_Slots(); // RA2: as above

	DWORD value = 0;
	HKEY rKey;
	char regkey[256];
	if (APP_REG_KEY != NULL && strlen(APP_REG_KEY)) {
		strcpy(regkey, APP_REG_KEY);
		if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, regkey, 0, KEY_READ, &rKey) == ERROR_SUCCESS) {
			DWORD type;
			DWORD valuesize = sizeof(value);
			RegQueryValueEx(rKey, "SKU", NULL, &type, (LPBYTE)&value, &valuesize);
			RegCloseKey(rKey);
		}
	}

	value &= 0xFF; // RA2: as above
	g_LanguageCode = value; // RA2: as above
	g_GameSKU = value + 0x2100; // RA2: the SKU of Red Alert 2 plus the language

	Reset_WOL_Globals();
	g_IgnoreNetStatus = true;
	for (int i = 0; i < EV_COUNT; i++) {
		g_WaitEventHandles[i] = CreateEvent(NULL, FALSE, FALSE, NULL);
	}
	return(Startup_Chat(ProgramInstance)); // RA2: 1 on success, -2 when a service is missing
}

#include "wolopt.h" // RA2: the options of Westwood Online

extern char g_LadderAddress[]; // RA2: 0x00B29018, the address of the ladder server (empty when there is none)
extern int g_LadderPort; // RA2: 0x00B2A8AC, the port of the ladder server
extern bool g_PlaintextPassword; // RA2: 0x007FD374, TibSun's name; RequestConnection takes its low byte
namespace OwnerDraw {
class DialogControl { // RA2: the base of the controls dialogs report to (vtable 0x007A7B28); the same class as in ownrdraw.cpp, whose header does not exist yet
public:
	DialogControl(void); // RA2: 0x00601030
	virtual ~DialogControl(void); // RA2: as above
	virtual void Set_Window(unsigned int key, HWND window) = 0; // RA2: as above
	virtual bool Is_Done(void) = 0; // RA2: as above
	virtual HWND Get_Window(void) = 0; // RA2: as above
	virtual bool Is_Cancelled(void) = 0; // RA2: as above
};
class SimpleDialogControl : public DialogControl { // RA2: vtable 0x007A7B40
public:
	SimpleDialogControl(void); // RA2: 0x006010C0, defined in another file
	virtual ~SimpleDialogControl(void); // RA2: 0x006010E0, as above
	virtual void Set_Window(unsigned int key, HWND window); // RA2: 0x006010F0
	virtual bool Is_Done(void); // RA2: 0x00601170, reads the byte at +8
	virtual HWND Get_Window(void); // RA2: 0x00601180, returns the dword at +4 (the dialogs store their answer there)
	virtual bool Is_Cancelled(void); // RA2: 0x00601190
	HWND window; // RA2: +0x04
	bool IsDone; // RA2: +0x08
};
int __fastcall Controlled_Dialog(int resource, DLGPROC procedure, DialogControl * control); // RA2: 0x006011A0, runs the dialog until its control reports; defined in ownrdraw.cpp
}
class SimpleWonlineDialogControl : public OwnerDraw::SimpleDialogControl { // RA2: vtable 0x007AF6F4
public:
	SimpleWonlineDialogControl(void) {} // RA2: 0x0073C000 (inline: the base constructor and the vtable)
	virtual ~SimpleWonlineDialogControl(void); // RA2: 0x0073C020, called out of line by Connect_To_Server (stores the vtable, jumps to the base destructor); declared only, its copy lies before the module. Login_Wonline expands it (0x00742FD0 shows the vtable store and the base destructor call): the executable decides differently there, see the note at Login_Wonline
};
BOOL CALLBACK Locale_Dialog_Proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam); // RA2: 0x00751190, the country dialog (0xFE)

Wstring Server_Name_Without_Prefix(Server const * server); // RA2: 0x00745450, defined below

// RA2: the executable calls Dictionary<Wstring, int>::add 0x00766070, ::remove(key, value) 0x007675E0 and ::getPointer 0x00767570 out of line from Connect_To_Server (meastes keeps them as non-inline members of the name set, defined in other files); these explicit specializations carry the template's own statements and are never expanded (auto_inline off), so the executable's copies are emitted here and match; replaces the Update_Locale layers and the Get_Locale_Pointer wrapper
template<> char Dictionary<Wstring, int>::add(Wstring const & key, int & value); // RA2: defined at the end of the file (0x00766070 lies behind the file's other functions)
template<> char Dictionary<Wstring, int>::remove(Wstring const & key, int & value); // RA2: as above (0x007675E0)



static inline int Ask_Locale(void) // RA2: the dialog (0xFE, Locale_Dialog_Proc 0x00751190) that asks for the country of the persona; the control is destroyed on each way out (debt: its real name and form are not known, two returns give the executable's two copies of the destructor call)
{
	SimpleWonlineDialogControl control; // RA2: vtable 0x007AF6F4, constructed by 0x006010C0, destroyed by 0x0073C020
	int answer = OwnerDraw::Controlled_Dialog(0xFE, Locale_Dialog_Proc, &control); // RA2: as above
	if (answer < 0) { // RA2: cancelled
		return(0);
	}
	return(answer);
}


static inline char Get_Locale_Pointer(Wstring const & name, int ** value) { return g_UserLocales.getPointer(name, value); } // RA2: a wrapper layer (debt: its real name is not known); the executable calls Dictionary::getPointer 0x00767570 out of line here; an explicit specialization of getPointer (auto_inline off) does that too but also changes User_Tooltip 0x0073F250 and Draw_Player_List 0x0073FEE0, which expand it

bool Connect_To_Server(Server * server) // RA2: 0x007424C0, not in TibSun (its Login_WOL did this): logs in to the chat server with the persona and password, waits for the answer (30 seconds, 20 rounds), reads the profile of the persona, tells the ladder server about it and settles the locale of the persona; matches (2192 bytes) with the out-of-line derived destructor and the out-of-line Dictionary calls above
{
	strcpy((char *)server->login, g_NickName.get()); // RA2: the persona and the password go into the server entry
	strcpy((char *)server->password, g_LoginPassword.get()); // RA2: as above
	for (int i = 0; i < EV_COUNT; i++) { // RA2: all wait events are cleared first
		ResetEvent(g_WaitEventHandles[i]);
	}
	{
		WideWstring connecting(StringTable::LoadString("TXT_CONNECTING_SERVER", NULL, "D:\\Ra2\\wonline.cpp", 8397)); // RA2: the wait dialog takes a wide text
		Show_Wait_Window(0x10, connecting, false); // RA2: as above
	}
	g_pChat->RequestConnection(server, 20, ((int &)g_PlaintextPassword) & 0xFF); // RA2: virtual 0x14

	int attempts = 20; // RA2: rounds of the wait
	for (;;) { // RA2: as above
		int remaining = 30000; // RA2: 30 seconds in steps of 100 ms
		DWORD event = WaitForMultipleObjects(EV_COUNT, g_WaitEventHandles, FALSE, 100); // RA2: as above
		while (event == WAIT_TIMEOUT) { // RA2: as above
			remaining -= 100; // RA2: as above
			if (remaining <= 0) { // RA2: as above
				goto connect_failed; // RA2: the executable shares one Close_Wait_Window block between this timeout and the event 18 test below (8 copies of it in the function); no goto-free form gave that (they give 10 copies and a different block order)
			}
			WOL_Wait_Callback(); // RA2: as above
			event = WaitForMultipleObjects(EV_COUNT, g_WaitEventHandles, FALSE, 100); // RA2: as above
		}
		{ // RA2: the event that ended the wait is cleared again
			ResetEvent(g_WaitEventHandles[event]); // RA2: as above
			if (event == EV_ABORT) { // RA2: as above
				Close_Wait_Window(0xFFFFFFFF); // RA2: as above
				return(false);
			}
			if (event == EV_ERROR) { // RA2: as above
				ODMessageBox(StringTable::LoadString("TXT_CANT_CONNECT", NULL, "D:\\Ra2\\wonline.cpp", 8410), 0, Wait_Callback_Thunk); // RA2: as above
				Close_Wait_Window(0xFFFFFFFF); // RA2: as above
				return(false);
			}
		}
		if (event == EV_COUNT) { // RA2: as above
connect_failed: // RA2: label of the shared block described at the goto above
			Close_Wait_Window(0xFFFFFFFF); // RA2: as above
			return(false);
		}
		if (attempts-- < 0) { // RA2: as above
			ODMessageBox(StringTable::LoadString("TXT_CANT_CONNECT", NULL, "D:\\Ra2\\wonline.cpp", 8417), 0, Wait_Callback_Thunk); // RA2: as above
			Close_Wait_Window(0xFFFFFFFF); // RA2: as above
			return(false);
		}
		if (event == EV_CONNECTED) { // RA2: as above
			break;
		}
	}
	Close_Wait_Window(0xFFFFFFFF); // RA2: as above
	g_pChat->SetClientVersion(0x1000E); // RA2: virtual 0xD4
	g_IgnoreNetStatus = false; // RA2: as above
	Session.Read_MultiPlayer_Settings(); // RA2: as above

	WOLOptionsClass options; // RA2: the constructor of wolopt.h clears both members
	options.Load(); // RA2: then Load is called, as 0x004FF200 shows
	rc_ptr<PlayerProfile> profile; // RA2: as above
	if (options.Is_Bit1_Clear()) { // RA2: as above
		profile = PlayerProfile::Load(g_NickName); // RA2: the persona's own profile
	} else {
		profile = g_PlayerProfile.Get(); // RA2: the profile of Reset_WOL_Globals
	}
	if (Online_Session_State == 3 || Online_Session_State == 4) { // RA2: as above
		{
			Wstring name(Server_Name_Without_Prefix(server)); // RA2: as above
			profile->SetName(&name); // RA2: the server's name is kept in the profile
		}
		if (options.Is_Bit1_Clear()) { // RA2: as above
			profile->Save(&g_NickName); // RA2: as above
		}
	}
	strcpy(Session.Handle, g_NickName.get()); // RA2: as above
	Session.Set_Side(profile->Country, false); // RA2: as above
	Session.Set_Color(profile->Color, false); // RA2: as above
	Session.Write_MultiPlayer_Settings(); // RA2: as above
	WOL_Set_Last_Valid_Persona(g_NickName); // RA2: as above
	if (g_pNetUtil && strlen(g_LadderAddress)) { // RA2: as above
		g_pNetUtil->RequestLadderList(g_LadderAddress, g_LadderPort, g_NickName.get(), g_GameSKU & 0xFFFF00, -1, 0, 0); // RA2: virtual 0x14
	}
	for (int j = 0; j < EV_COUNT; j++) { // RA2: as above
		ResetEvent(g_WaitEventHandles[j]);
	}
	User user;
	memset(&user, 0, sizeof(user)); // RA2: as above
	strcpy((char *)user.name, g_NickName.get()); // RA2: as above
	g_pChat->RequestUserLocale(&user); // RA2: virtual 0xBC
	for (;;) { // RA2: the second wait, for the locale
		int remaining = 30000; // RA2: as above
		DWORD event = WaitForMultipleObjects(EV_COUNT, g_WaitEventHandles, FALSE, 100); // RA2: as above
		while (event == WAIT_TIMEOUT) { // RA2: as above
			remaining -= 100; // RA2: as above
			if (remaining <= 0) { // RA2: as above
				goto locale_wait_failed; // RA2: as the goto above, for the three blocks of this wait
			}
			WOL_Wait_Callback(); // RA2: as above
			event = WaitForMultipleObjects(EV_COUNT, g_WaitEventHandles, FALSE, 100); // RA2: as above
		}
		{ // RA2: as above
			ResetEvent(g_WaitEventHandles[event]); // RA2: as above
			if (event == EV_ABORT) { // RA2: as above
				Close_Wait_Window(0xFFFFFFFF); // RA2: as above
				break;
			}
			if (event == EV_ERROR) { // RA2: as above
				ODMessageBox(StringTable::LoadString("TXT_WDT_NET_ERR", NULL, "D:\\Ra2\\wonline.cpp", 8501), 0, Wait_Callback_Thunk); // RA2: as above
				Close_Wait_Window(0xFFFFFFFF); // RA2: as above
				break;
			}
		}
		if (event == EV_COUNT) { // RA2: as above
locale_wait_failed: // RA2: label of the shared block of the second wait
			Close_Wait_Window(0xFFFFFFFF); // RA2: as above
			break;
		}
		if (event == EV_LOCALE) { // RA2: as above
			break;
		}
	}

	Wstring const & nick_source = g_NickName; // RA2: the const overload of toLower
	Wstring nick = nick_source.toLower(); // RA2: the locales are kept by lower case name
	if (g_UserLocales.contains(nick)) { // RA2: the server answered for the persona
		int locale = 0; // RA2: as above
		{
			int * value = NULL; // RA2: as above
			if (Get_Locale_Pointer(nick, &value) && value != NULL) { // RA2: as above
				locale = *value; // RA2: as above
			}
		}
		if (locale != 0) { // RA2: the server already knows a locale: make the profile agree with it and tell the server the profile's
			if (profile.Get() != NULL && profile->Locale != locale) { // RA2: as above
				if (profile->Locale == 0) { // RA2: the profile had none yet
					profile->SetLocale(locale, 2); // RA2: as above
					profile->Save(&g_NickName); // RA2: as above
				}
				int current = profile->Locale; // RA2: read once into a register (the executable uses esi)
				g_pChat->RequestSetLocale((Locale)current); // RA2: virtual 0xB8
				g_pChat->RequestUserLocale(&user); // RA2: virtual 0xBC
				locale = current; // RA2: the executable keeps the new locale in esi and stores it only here, for the update
				g_UserLocales.updateValue(nick, locale); // RA2: Dictionary::remove then ::add (calls 0x007675E0 and 0x00766070)
			}
			return(true);
		}
	}
	int chosen = 0; // RA2: the locale of the second path
	if (profile.Get() == NULL || (chosen = profile->Locale) == 0) { // RA2: no locale known anywhere: ask the player
		chosen = Ask_Locale(); // RA2: as above
		if (profile.Get() != NULL) { // RA2: as above
			profile->SetLocale(chosen, 3); // RA2: as above
			profile->Save(&g_NickName); // RA2: as above
		}
	}
	if (chosen != 0) { // RA2: as above
		g_pChat->RequestSetLocale((Locale)chosen); // RA2: as above
		g_pChat->RequestUserLocale(&user); // RA2: as above
		int stored = chosen; // RA2: Dictionary::updateValue takes a reference: the value goes through a variable of its own
		g_UserLocales.updateValue(nick, stored); // RA2: as above
	}
	return(true);
}

extern ArrayList<Server> g_Servers; // RA2: 0x00B2A700, the chat and game servers of the server list (248 bytes each)
extern HRESULT g_OnConnectionResult; // RA2: 0x00B2AC10, the result of the last connection (the chat event sink stores it)

bool Connect_To_Selected_Server(void) // RA2: 0x00742D50, not in TibSun: connects to the server of the list the player picked, false when the index is not in the list
{
	if (g_CurrentServerIndex >= 1 && g_CurrentServerIndex < g_Servers.length()) { // RA2: as above
		Server * server = NULL; // RA2: as above
		g_Servers.getPointer(&server, g_CurrentServerIndex); // RA2: as above
		return(Connect_To_Server(server)); // RA2: a jump to Connect_To_Server
	}
	return(false); // RA2: as above
}

bool Recall_Stored_Login(Wstring const & name) // RA2: 0x00742D80, not in TibSun: looks the persona up among the 32 logins the chat API remembers; keeps its nickname and password when it is there, else clears the nickname
{
	g_LoginPassword.clear(); // RA2: as above
	for (int index = 1; index <= 32; index++) { // RA2: as above
		char const * nickname; // RA2: not cleared, the chat API sets both when it succeeds
		char const * password; // RA2: as above
		if (g_pChat->GetNick(index, &nickname, &password) == 0 && nickname && strlen(nickname) && !stricmp(name.text(), nickname)) { // RA2: as above
			g_NickName = nickname; // RA2: as above
			g_LoginPassword = password; // RA2: as above
			g_PlaintextPassword = false; // RA2: as above
			return(true); // RA2: as above
		}
	}
	g_NickName.clear(); // RA2: as above
	return(false); // RA2: as above
}

int Find_Server_Suffix(Wstring const & name) // RA2: 0x00742E30, not in TibSun: the index of the server whose name is "<prefix>:" followed by this name, -1 when there is none
{
	if (name.length() != 0) { // RA2: as above
		for (int i = 1; i < g_Servers.length(); i++) { // RA2: as above
			Server * server = NULL; // RA2: as above
			g_Servers.getPointer(&server, i); // RA2: as above
			char * separator = strchr((char *)server->name, ':'); // RA2: as above
			if (separator && *separator && name == (separator + 1)) { // RA2: as above
				return(i);
			}
		}
	}
	return(-1); // RA2: as above
}

static const HRESULT CHAT_E_BANNED = (HRESULT)0x80040072; // RA2: the answers of the chat server that have a support page; the page names say which is which (banned_, disabled_, serialbanned_, serialdup_, serialunknown_, skuserialmismatch_)
static const HRESULT CHAT_E_DISABLED = (HRESULT)0x80040074; // RA2: as above
static const HRESULT CHAT_E_SERIALBANNED = (HRESULT)0x80040075; // RA2: as above
static const HRESULT CHAT_E_SERIALDUP = (HRESULT)0x80040076; // RA2: as above
static const HRESULT CHAT_E_SERIALUNKNOWN = (HRESULT)0x80040077; // RA2: as above
static const HRESULT CHAT_E_SKUSERIALMISMATCH = (HRESULT)0x80040078; // RA2: as above

bool Handle_Connection_Error(void) // RA2: 0x00742EB0, not in TibSun: shows the error text of the failed connection; for the answers that have a support page it asks whether to open it and does so (4 is the yes/no kind). True when the answer was one of those
{
	bool handled = false; // RA2: as above
	char url[260]; // RA2: as above
	if (g_OnConnectionResult == CHAT_E_SERIALUNKNOWN || g_OnConnectionResult == CHAT_E_SKUSERIALMISMATCH || g_OnConnectionResult == CHAT_E_BANNED || g_OnConnectionResult == CHAT_E_DISABLED || g_OnConnectionResult == CHAT_E_SERIALBANNED || g_OnConnectionResult == CHAT_E_SERIALDUP) { // RA2: as above
		handled = true; // RA2: as above
		if (ODMessageBox(g_LastErrorMessage, 4, Wait_Callback_Thunk, false) == 6) { // RA2: IDYES
			int language = g_LanguageCode; // RA2: the support pages exist per language
			switch (g_OnConnectionResult) { // RA2: as above
			case CHAT_E_SERIALUNKNOWN: sprintf(url, "http://apiregister.westwood.com/support/serialunknown_%d.html", language); break; // RA2: as above
			case CHAT_E_SKUSERIALMISMATCH: sprintf(url, "http://apiregister.westwood.com/support/skuserialmismatch_%d.html", language); break; // RA2: as above
			case CHAT_E_BANNED: sprintf(url, "http://apiregister.westwood.com/support/banned_%d.html", language); break; // RA2: as above
			case CHAT_E_DISABLED: sprintf(url, "http://apiregister.westwood.com/support/disabled_%d.html", language); break; // RA2: as above
			case CHAT_E_SERIALBANNED: sprintf(url, "http://apiregister.westwood.com/support/serialbanned_%d.html", language); break; // RA2: as above
			case CHAT_E_SERIALDUP: sprintf(url, "http://apiregister.westwood.com/support/serialdup_%d.html", language); break; // RA2: as above
			}
			ViewHTML(url, 1); // RA2: as above
		}
	} else {
		ODMessageBox(g_LastErrorMessage, 0, Wait_Callback_Thunk, false); // RA2: as above
	}
	return(handled); // RA2: as above
}

int __fastcall Request_And_Wait_Server_List(unsigned int flags); // RA2: 0x00751C30, asks for the list of chat servers and waits for it (0 done, 1 failed, 2 quit), not yet defined
LRESULT CALLBACK Wonline_Account_Dialog_Proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam); // RA2: 0x007540A0, the dialog proc of the login dialog (0x10F, 0x11D)
LRESULT CALLBACK Wonline_Autologin_Dialog_Proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam); // RA2: 0x00755070, the dialog proc of the dialog (0x112) after a login
extern bool g_RememberLogin; // RA2: 0x00B2AC74, the login is to be kept in the chat service's list of logins (set by the login dialog)
int Save_Nickname(char const * name, char const * password, bool mangle, int locale); // RA2: 0x00745320, defined below
void Darken_Surface(Surface * surface, bool update); // RA2: 0x005159B0, defined in init.cpp

static inline void Set_Login_Focus(HWND window) // RA2: the login dialog puts the focus on the password box (0x72C) when a nickname is known, else on the name box (0x628); the executable has both SetFocus calls with a merged tail
{
	if (g_NickName.length() > 0 && g_LoginPassword.length() == 0) { // RA2: as above
		SetFocus(GetDlgItem(window, 0x72C)); // RA2: as above
	} else {
		SetFocus(GetDlgItem(window, 0x628)); // RA2: as above
	}
}

static inline int Fetch_Server_List(void) // RA2: shows "fetching the server list" and requests the list for the product (0x2300 in a tournament session, else 0x2100)
{
	{
		WideWstring fetching(StringTable::LoadString("TXT_FETCHING_SERVLIST", NULL, "D:\\Ra2\\wonline.cpp", 22220)); // RA2: as above
		Show_Wait_Window(16, fetching, false); // RA2: as above
	}
	int sku = 0x2300; // RA2: as above
	if (Online_Session_State != 2) { // RA2: as above
		sku = 0x2100; // RA2: as above
	}
	return(Request_And_Wait_Server_List(sku)); // RA2: as above
}

int __fastcall Login_Wonline(void) // RA2: 0x00742FD0, not in TibSun: logs in to Westwood Online: starts the chat services, fetches the server list, tries the stored login of the auto login persona, else opens the login dialog (the executable's debug text says "Creating login dlg") and connects to the server the persona's profile names; 1 when connected, 0 when not, negative when the server list could not be fetched; NOT matching (unplaced, 1904 bytes against the executable's 1857): the executable calls ArrayList::getPointer 0x00765E60 out of line in the three early expansions of Find_Server_Suffix / Connect_To_Selected_Server (the last two expansions are inline) which our build expands everywhere (about 16 bytes each; wrapper layers around the early calls make the expansions themselves vanish, so the inline budget is not what is wrong); it keeps the zero in ebp and the rc_ptr vtable in ebx where we use ebx; and it expands the derived control destructor (see SimpleWonlineDialogControl)
{
	bool failed = false; // RA2: as above
	g_OnConnectionResult = 0; // RA2: as above
	int result = Init_Wonline(); // RA2: as above
	if (result != 1) { // RA2: as above
		return(result); // RA2: as above
	}
	{ // RA2: the lists of the last session are cleared before the login
	memset(&g_CurrentChannel, 0, sizeof(g_CurrentChannel)); // RA2: as above
	memset(&g_PendingChannel, 0, sizeof(g_PendingChannel)); // RA2: as above
	g_UserChannels.clear(); // RA2: as above
	g_GameChannelList.clear(); // RA2: as above
	g_UserList.clear(); // RA2: as above
	g_ChannelCount = 0; // RA2: as above
	result = 0; // RA2: as above
	int error = Fetch_Server_List(); // RA2: as above
	switch (error) { // RA2: as above
	case 0: // RA2: as above
		break; // RA2: as above
	case 2: // RA2: as above
		failed = true; // RA2: as above
		result = -1; // RA2: as above
		break; // RA2: as above
	default: // RA2: as above
		failed = true; // RA2: as above
		result = -2; // RA2: as above
		break; // RA2: as above
	}
	WOLOptionsClass options; // RA2: as above
	options.Load(); // RA2: as above
	if (!failed) { // RA2: as above
		if (options.Is_Bit0_Clear()) { // RA2: the auto login is on
			Wstring persona = WOL_Auto_Login_Persona(); // RA2: as above
			if (persona.length() > 0) { // RA2: as above
				if (Recall_Stored_Login(persona)) { // RA2: expanded here
					int index = -1; // RA2: as above
					if (Is_Tournament_Session_Inline()) { // RA2: an Internet game
						rc_ptr<PlayerProfile> profile = PlayerProfile::Load(persona); // RA2: as above
						if (profile.Get()) { // RA2: the server of the persona's profile (Find_Server_Suffix tests the empty name itself)
							index = Find_Server_Suffix(profile->Name); // RA2: expanded here
						}
						if (index == -1) { // RA2: as above
							index = g_CurrentServerIndex; // RA2: as above
						}
					} else {
						rc_ptr<PlayerProfile> profile = PlayerProfile::Load(persona); // RA2: as above
						if (profile.Get()) { // RA2: as above
							if (profile->Name.length() == 0) { // RA2: as above
								index = g_CurrentServerIndex; // RA2: as above
							} else {
								index = Find_Server_Suffix(profile->Name); // RA2: expanded here
							}
						}
					}
					if (index != -1) { // RA2: as above
						g_CurrentServerIndex = index; // RA2: as above
						if (Connect_To_Selected_Server()) { // RA2: expanded here
							return(1); // RA2: as above
						}
						failed = Handle_Connection_Error(); // RA2: as above
					}
				}
			}
		}
		if (g_NickName.length() == 0 && options.Is_Bit1_Clear()) { // RA2: as above
			g_NickName = WOL_Last_Valid_Persona(); // RA2: as above
		}
		{
			Wstring empty(""); // RA2: as above
			WOL_Set_Auto_Login_Persona(empty); // RA2: the auto login is used once
		}
		if (failed) { // RA2: as above
			return(0); // RA2: as above
		}
		DebugString("Creating login dlg\n"); // RA2: as above
		SimpleWonlineDialogControl control; // RA2: as above
		int dialog_id = options.Is_Bit1_Clear() ? 0x10F : 0x11D; // RA2: as above
		Darken_Surface(NULL, true); // RA2: as above
		HWND window = OwnerDraw::Begin_Dialog(dialog_id, (DLGPROC)Wonline_Account_Dialog_Proc, (LPARAM)&control); // RA2: as above
		OwnerDraw::Display_Dialog(window); // RA2: as above
		Set_Login_Focus(window); // RA2: as above
		while (!control.Is_Done() && !WOL_Wait_Callback() && !OwnerDraw::Dialog_Message_Handler()) { // RA2: as above
			Title_Screen_Restore(false); // RA2: as above
		}
		OwnerDraw::End_Dialog(window); // RA2: as above
		if ((int)control.Get_Window() == 2) { // RA2: the dialog was cancelled
			failed = true; // RA2: as above
		} else {
			WOL_Wait_Callback(); // RA2: as above
			g_IgnoreNetStatus = false; // RA2: as above
			WOLOptionsClass saved; // RA2: as above
			saved.Load(); // RA2: as above
			rc_ptr<PlayerProfile> profile; // RA2: as above
			if (saved.Is_Bit1_Clear()) { // RA2: as above
				profile = PlayerProfile::Load(g_NickName); // RA2: as above
			} else {
				profile = g_PlayerProfile.Get(); // RA2: as above
			}
			int found = Find_Server_Suffix(profile->Name); // RA2: expanded here
			if (found != -1) { // RA2: as above
				g_CurrentServerIndex = found; // RA2: as above
			}
			if (g_CurrentServerIndex >= g_Servers.length()) { // RA2: as above
				g_CurrentServerIndex = -1; // RA2: as above
			}
			if (g_CurrentServerIndex == -1 && g_Servers.length() > 1) { // RA2: as above
				g_CurrentServerIndex = 1; // RA2: as above
			}
			if (Connect_To_Selected_Server()) { // RA2: expanded here
				if (g_RememberLogin && saved.Is_Bit1_Clear()) { // RA2: the login is kept for the next time
					int locale = profile->Locale; // RA2: read before the nickname is saved (0x00742FD0)
					Save_Nickname(g_NickName.get(), g_LoginPassword.get(), g_PlaintextPassword, locale); // RA2: as above
					Session.Write_MultiPlayer_Settings(); // RA2: as above
				}
				if (!saved.Is_Bit0_Clear() || !WOL_Get_Auto_Login()) { // RA2: no auto-login question unless the profile allows it and auto-login is on
					return(1); // RA2: as above
				}
				Darken_Surface(NULL, true); // RA2: as above
				SimpleWonlineDialogControl next; // RA2: as above
				OwnerDraw::Controlled_Dialog(0x112, (DLGPROC)Wonline_Autologin_Dialog_Proc, &next); // RA2: as above
				return(1); // RA2: as above
			}
		}
	}
	Close_Wait_Window(0xFFFFFFFF); // RA2: expanded here
	if (!failed) { // RA2: as above
		Handle_Connection_Error(); // RA2: as above
	}
	g_IgnoreNetStatus = true; // RA2: as above
	} // RA2: as above
	return(result); // RA2: as above
}

// RA2: declarations for the functions below (placed after Draw_Player_List: declarations above it change its code)
#include "ccfile.h" // RA2: WDTMAP.INI is read through the game's file system by Pick_Lobby
#include "mixfile.h" // RA2: and WDT.MIX is opened
#include "_pk.h" // RA2: FastKey
#include "wdtnet.h" // RA2: the state of the tour
extern int g_LobbyNumber; // RA2: 0x00B2ABEC, the number of the lobby channel ("Lob_33_<number>") joined last
extern wchar_t * g_TerritoryName; // RA2: 0x00B28CC8, the name of the WDT territory of this player ("A territory" when the map INI has none), a copy made by _wcsdup
rc_ptr<WDTState> const & __fastcall Current_WDT_State(void); // RA2: 0x00769510, defined in worlddom.cpp

// RA2: declarations for Run_Lobby_Session (before Pick_Lobby: the number of declarations in front of it decides how Pick_Lobby is compiled, see allmatch)
extern bool g_HasJoinedLobby; // RA2: 0x007FD354, a lobby was joined before (the next join asks for a lobby with room)
extern bool g_WDTSoundsInitialized; // RA2: 0x00B2AC28, Initialize_WDT_Sounds ran
extern ArrayList<Wstring> g_WDTWinSounds0; // RA2: 0x00B26570, the sounds "00-W022.V00", "00-W024.V00", "00-W026.V00" (Initialize_WDT_Sounds 0x00752210 fills it with the seventh to ninth name)
extern ArrayList<Wstring> g_WDTWinSounds1; // RA2: 0x00B26A50, the sounds "01-W024.V01", "01-W026.V01", "01-W028.V01" (the first to third name)
extern ArrayList<Wstring> g_WDTLoseSounds0; // RA2: 0x00B29380, the sounds "00-W017.V00", "00-W018.V00", "00-W020.V00" (the tenth to twelfth name)
void Initialize_WDT_Sounds(void); // RA2: 0x00752210, fills the lists of the tour's sounds, once
void Discard_WDT_Sound_List(ArrayList<Wstring> sounds); // RA2: 0x00752890, takes a list of sounds by value and does nothing with it
int Join_Selected_Lobby(bool join); // RA2: 0x00744930, defined below
int CALLBACK Wonline_Main_Dialog_Proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam); // RA2: 0x007469C0, the dialog proc of the lobby (0x113)
void Set_Wait_Window_Text(LPARAM text); // RA2: 0x0074D750, defined below
bool Start_Network_Session(void); // RA2: 0x00514EE0, defined in init.cpp
inline Wstring Narrow_Text(wchar_t const * text) // RA2: the executable narrows the text through a named wide string that dies before the result is used (0x00743720, 0x00743A40 below)
{
	WideWstring wide(text); // RA2: as above
	return Wstring_Narrow(wide); // RA2: as above
}
inline WideWstring Wide_Text(char const * text) // RA2: the counterpart: a named narrow string that dies after the widening
{
	Wstring narrow(text); // RA2: as above
	return Wstring_ToWide(narrow); // RA2: as above
}

inline void Print_Message_Of_The_Day(void) // RA2: the message of the day
{
	if (g_Motd.length() > 0) { // RA2: the message of the day, printed line by line
		char * text = new char[g_Motd.length() + 1]; // RA2: as above
		if (text) { // RA2: as above
			strcpy(text, g_Motd.get()); // RA2: as above
			char * token = strtok(text, "\n"); // RA2: as above
			while (token != NULL) { // RA2: as above
				WideWstring wide_line = Wide_Text(token); // RA2: as above
				Lobby_Chat_Printf(ColorSystem, wide_line.get()); // RA2: as above
				token = strtok(NULL, "\n"); // RA2: the next token is taken before the wide line is destroyed
			}
			Lobby_Chat_Printf(ColorSystem, L"\n"); // RA2: as above
			g_Motd.clear(); // RA2: as above
			delete text; // RA2: as above
		}
	}
}

int __fastcall Run_Lobby_Session(HWND window, bool first_time) // RA2: 0x00743720, not in TibSun: the lobby session of Westwood Online: ends the finished game (the tour's victory and defeat sounds), logs in again when the connection was lost, opens the lobby dialog (0x113) with the message of the day, requests the channel list, joins a lobby and waits for the events of the chat: 1 when a game starts, 2 when the skirmish event ended it, 0 otherwise (the dialog, the chat and the wait dialogs are closed again)
{
	int result = 0; // RA2: as above
	if (g_PlayingNetGame && Session.Type == GAME_INTERNET && Online_Session_State == 2 && Session.Winner != -1 && Session.Winner < 8) { // RA2: a tour game just ended
		Wstring winner = Narrow_Text(Session.Score[Session.Winner].Name); // RA2: the name of the winner
		if (_strnicmp(winner.get(), Session.Handle, 20) == 0) { // RA2: the local player won
			if (Session.House == 0) { // RA2: as above
				Discard_WDT_Sound_List(g_WDTWinSounds0); // RA2: as above
			} else if (Session.House == 1) { // RA2: as above
				Discard_WDT_Sound_List(g_WDTWinSounds1); // RA2: as above
			}
		} else if (Session.House == 0) { // RA2: the local player lost
			Discard_WDT_Sound_List(g_WDTLoseSounds0); // RA2: as above
		}
	}
	g_PlayingNetGame = false; // RA2: as above
	WolOptionsLocked = 0; // RA2: as above
	if (Session.Type == GAME_INTERNET && Online_Session_State == 2) { // RA2: a tour game
		g_pChat->RequestSetTeam(Session.House); // RA2: virtual 0xB4
		if (!g_WDTSoundsInitialized) { // RA2: as above
			Initialize_WDT_Sounds(); // RA2: as above
		}
	}
	Start_Network_Session(); // RA2: as above
	if (g_ConnectionLost) { // RA2: as above
		int login = Login_Wonline(); // RA2: as above
		if (login != 1) { // RA2: as above
			return(login); // RA2: as above
		}
	}
	for (int i = 0; i < EV_COUNT; i++) { // RA2: as above
		ResetEvent(g_WaitEventHandles[i]); // RA2: as above
	}
	if (window == NULL) { // RA2: as above
		Darken_Surface(NULL, true); // RA2: as above
		window = OwnerDraw::Begin_Dialog(0x113, (DLGPROC)Wonline_Main_Dialog_Proc, 0); // RA2: the lobby dialog
		Print_Message_Of_The_Day(); // RA2: as above
		{
			WideWstring title(StringTable::LoadString("GUI:LoginComplete", NULL, "D:\\Ra2\\wonline.cpp", 9075)); // RA2: the wait text is wide
			Show_Wait_Window(0x10, title, false); // RA2: as above
		}
	}
	g_SquadInfoRequests++; // RA2: as above
	g_RequestingOwnSquadInfo = 1; // RA2: as above
	g_pChat->RequestSquadInfo(0); // RA2: virtual 0xB0
	g_ServerType = 0x21; // RA2: as above
	Set_Wait_Window_Text((LPARAM)StringTable::LoadString("TXT_REQ_CHANLIST", NULL, "D:\\Ra2\\wonline.cpp", 9092)); // RA2: expanded here
	if (!Request_Chat_Channel_List(true)) { // RA2: expanded here
		goto cleanup; // RA2: as above
	}
	Set_Wait_Window_Text((LPARAM)StringTable::LoadString("TXT_JOINLOB", NULL, "D:\\Ra2\\wonline.cpp", 9099)); // RA2: as above
	{
		int joined = Join_Selected_Lobby(g_HasJoinedLobby && !first_time); // RA2: as above
		g_HasJoinedLobby = true; // RA2: as above
		if (joined == -1) { // RA2: as above
			goto cleanup; // RA2: as above
		}
	}
	{
		DWORD event; // RA2: as above
		while ((event = WaitForMultipleObjects(EV_COUNT, g_WaitEventHandles, FALSE, 100)) == WAIT_TIMEOUT) { // RA2: as above
			WOL_Wait_Callback(); // RA2: as above
		}
		ResetEvent(g_WaitEventHandles[event]); // RA2: as above
		if (event == EV_ABORT || event == EV_ERROR) { // RA2: as above
			goto logout; // RA2: as above
		}
	}
	g_JoinLobbyNow = 0; // RA2: as above
	PostMessage(window, 0x593, 0, 0); // RA2: as above
	Close_Wait_Window(0xFFFFFFFF); // RA2: expanded here
	SetFocus(MainWindow); // RA2: as above
	ShowWindow(window, SW_SHOWNORMAL); // RA2: as above
	SetFocus(window); // RA2: as above
	SetFocus(GetDlgItem(window, 0x540)); // RA2: as above
	g_LastGameListTime = 0; // RA2: as above
	g_LastChatListTime = time(0); // RA2: as above
	for (;;) { // RA2: as above
		DWORD event; // RA2: as above
		while ((event = WaitForMultipleObjects(EV_COUNT, g_WaitEventHandles, FALSE, 20)) == WAIT_TIMEOUT) { // RA2: as above
			WOL_Wait_Callback(); // RA2: as above
			if (g_JoinLobbyNow) { // RA2: the lobby is to be left and another one joined
				EnableWindow(GetDlgItem(window, 0x53D), FALSE); // RA2: as above
				EnableWindow(GetDlgItem(window, 0x6EE), FALSE); // RA2: as above
				EnableWindow(GetDlgItem(window, 0x62D), FALSE); // RA2: as above
				EnableWindow(GetDlgItem(window, 0x62B), FALSE); // RA2: as above
				EnableWindow(GetDlgItem(window, 0x689), FALSE); // RA2: as above
				EnableWindow(GetDlgItem(window, 0x558), FALSE); // RA2: as above
				EnableWindow(GetDlgItem(window, 0x61F), FALSE); // RA2: as above
				EnableWindow(GetDlgItem(window, 0x686), FALSE); // RA2: as above
				if (Join_Selected_Lobby(false) == -1) { // RA2: as above
					EnableWindow(GetDlgItem(window, 0x686), TRUE); // RA2: as above
					SetEvent(g_WaitEventHandles[EV_ERROR]); // RA2: as above
				} else {
					PostMessage(window, 0x593, 0, 0); // RA2: as above
				}
				g_JoinLobbyNow = 0; // RA2: as above
			}
		}
		ResetEvent(g_WaitEventHandles[event]); // RA2: as above
		if (event == EV_ABORT) { // RA2: as above
			goto cleanup; // RA2: as above
		}
		if (event != EV_EXIT) { // RA2: as above
			if (event == EV_SKIRMISH) { // RA2: as above
				result = 2; // RA2: as above
				break; // RA2: as above
			}
			if (event == EV_ERROR) { // RA2: as above
				goto cleanup; // RA2: as above
			}
			if (event == EV_STARTGAME) { // RA2: the game starts
				goto started; // RA2: as above
			}
		} else {
			break; // RA2: as above
		}
	}
	g_LastErrorMessage[0] = 0; // RA2: as above
logout: // RA2: as above
	WS_Destroy_Dialog(window, 0); // RA2: as above
	WOL_Wait_Callback(); // RA2: as above
	window = NULL; // RA2: as above
	if (g_pChat) { // RA2: as above
		g_pChat->RequestChannelLeave(); // RA2: virtual 0x24
	}
cleanup: // RA2: as above
	Close_Wait_Window(0xFFFFFFFF); // RA2: expanded here
	ODMessageBox(g_LastErrorMessage, 0, Wait_Callback_Thunk, false); // RA2: as above
	if (window) { // RA2: as above
		WS_Destroy_Dialog(window, 0); // RA2: as above
	}
	while (WS_Destroy_Dialog(NULL, 0)) { // RA2: as above
	}
	if (result == 1) { // RA2: as above
		g_PlayingNetGame = true; // RA2: as above
	}
	if (Session.Type == GAME_INTERNET && Online_Session_State == 2) { // RA2: as above
		g_WDTTeams.clear(); // RA2: as above
	}
	return(result); // RA2: as above
started: // RA2: as above
	result = 1; // RA2: as above
	g_pChat->RequestChannelLeave(); // RA2: virtual 0x24
	g_GameStartRequested = false; // RA2: as above
	g_LastErrorMessage[0] = 0; // RA2: as above
	goto cleanup; // RA2: as above
}


int Pick_Lobby(bool require_room) // RA2: 0x00744090, not in TibSun: chooses the lobby channel to join. In a World Domination Tour game it is the one that carries the player's territory; else the lobby that was joined last, else the lobby with the fewest players that still has room; the index in the channel list, -1 when there is none
{
	int count = g_UserChannels.length(); // RA2: as above
	Channel channel; // RA2: as above
	memset(&channel, 0, sizeof(channel)); // RA2: as above
	char section[32]; // RA2: the INI sections of the territory
	char entry[32]; // RA2: as above
	char fallback[32]; // RA2: as above
	char name[32]; // RA2: as above
	wchar_t territory[32]; // RA2: the name of the territory, wide
	if (Session.Type == GAME_INTERNET && Online_Session_State == 2) { // RA2: a World Domination Tour game
		memset(section, 0, sizeof(section)); // RA2: as above
		memset(territory, 0, 32); // RA2: only the first 32 bytes
		memset(entry, 0, sizeof(entry)); // RA2: as above
		memset(fallback, 0, sizeof(fallback)); // RA2: as above
		MixFileClass * archive = new MixFileClass("WDT.MIX", &FastKey); // RA2: the tour's archive is open while the INI is read
		{
			INIClass ini; // RA2: as above
			CCFileClass file("WDTMAP.INI"); // RA2: as above
			ini.Load(file, false); // RA2: as above
			sprintf(entry, "Territory%02d", Session.TournamentTerritory); // RA2: as above
			strcpy(fallback, entry); // RA2: as above
			if (Current_WDT_State()->MapID == 0) { // RA2: the map decides which of the two sections names the territory
				ini.Get_String("NorthAmerica", entry, fallback, section, sizeof(section)); // RA2: as above
			} else {
				ini.Get_String("Europe", entry, fallback, section, sizeof(section)); // RA2: as above
			}
			ini.Get_String(section, "Name", "A territory", name, sizeof(name)); // RA2: as above
			Char_To_Wide(territory, name, -1); // RA2: as above
			g_TerritoryName = _wcsdup(territory); // RA2: as above
			delete archive; // RA2: as above
		}
	}
	Wstring lobby_name = Wstring("Lob_33_") + Wstring_FromInt(g_LobbyNumber, 1); // RA2: the lobby that was joined last
	int i; // RA2: as above
	for (i = 0; i < count; i++) { // RA2: the territory's lobby or the one joined last
		g_UserChannels.get(channel, i); // RA2: as above
		if (Session.Type == GAME_INTERNET && Online_Session_State == 2) { // RA2: as above
			if (wcscmp((wchar_t const *)g_Lobbies[i], territory) != 0) { // RA2: as above
				continue; // RA2: as above
			}
			if (!require_room || (int)channel.currentUsers <= 25) { // RA2: 25 players per lobby
				return(i); // RA2: as above
			}
		}
		Wstring channel_name((char const *)channel.name); // RA2: as above
		if (channel_name == lobby_name) { // RA2: as above
			if (!require_room || (int)channel.currentUsers <= 25) { // RA2: as above
				return(i); // RA2: as above
			}
		}
	}
	int best = -1; // RA2: the lobby with the fewest players
	unsigned int smallest = 10000; // RA2: as above
	for (i = 0; i < count; i++) { // RA2: as above
		g_UserChannels.get(channel, i); // RA2: as above
		if (Session.Type == GAME_INTERNET && Online_Session_State == 2) { // RA2: as above
			if (wcsncmp((wchar_t const *)g_Lobbies[i], territory, wcslen(territory)) != 0) { // RA2: as above
				continue; // RA2: as above
			}
			if (!require_room || (int)channel.currentUsers <= 25) { // RA2: as above
				g_LobbyNumber = atol((char const *)channel.name + strlen("Lob_33_")); // RA2: the number is remembered
				return(i); // RA2: as above
			}
			if (channel.currentUsers < smallest) { // RA2: as above
				best = i; // RA2: as above
				smallest = channel.currentUsers; // RA2: as above
			}
		}
		if (strncmp((char const *)channel.name, "Lob_33_", strlen("Lob_33_")) == 0) { // RA2: as above
			if (!require_room || (int)channel.currentUsers <= 25) { // RA2: as above
				g_LobbyNumber = atol((char const *)channel.name + strlen("Lob_33_")); // RA2: as above
				return(i); // RA2: as above
			}
			if (channel.currentUsers < smallest) { // RA2: as above
				best = i; // RA2: as above
				smallest = channel.currentUsers; // RA2: as above
			}
		}
	}
	if (best != -1) { // RA2: as above
		g_UserChannels.get(channel, best); // RA2: as above
		g_LobbyNumber = atol((char const *)channel.name + strlen("Lob_33_")); // RA2: as above
	}
	return(best); // RA2: as above
}

bool Leave_Current_Channel(unsigned int event, bool block) // RA2: 0x00744610, not in TibSun: leaves the channel the chat is in, waits up to 15 seconds for the server to confirm; true when no channel is left
{
	if (g_ChannelCount) { // RA2: as above
		DebugString("Leave_Channel, channel %s type %d\n", g_CurrentChannel.name, g_CurrentChannel.type); // RA2: as above
		g_pChat->RequestChannelLeave(); // RA2: virtual 0x24
		{
			WideWstring title(StringTable::LoadString("WOL:LeavingChannel", NULL, "D:\\Ra2\\wonline.cpp", 9379)); // RA2: the wait dialog takes a wide text
			Show_Wait_Window(event, title, block); // RA2: as above
		}
		for (int i = 0; g_ChannelCount && i < 150; i++) { // RA2: 150 steps of 100 ms
			Sleep(100); // RA2: as above
			WOL_Wait_Callback(); // RA2: as above
		}
		Close_Wait_Window(0x40); // RA2: expanded here
	}
	return(g_ChannelCount == 0); // RA2: as above
}

int Request_Channel_Join(Channel * channel, Wstring const & password, WideWstring title) // RA2: 0x00744700, not in TibSun: joins a channel (leaves the current one first); title is the text of the wait dialog; the chat's answer, 1 when the old channel could not be left
{
	char const * name = (char const *)channel->name; // RA2: as above
	if (strcmp(name, (char const *)g_CurrentChannel.name) && strcmp(name, (char const *)g_PendingChannel.name)) { // RA2: not the channel the chat is in nor the one it was asked to join
		if (g_ChannelCount) { // RA2: as Leave_Current_Channel
			DebugString("Leave_Channel, channel %s type %d\n", g_CurrentChannel.name, g_CurrentChannel.type); // RA2: as above
			g_pChat->RequestChannelLeave(); // RA2: as above
			{
				WideWstring leaving(StringTable::LoadString("WOL:LeavingChannel", NULL, "D:\\Ra2\\wonline.cpp", 9379)); // RA2: as above
				Show_Wait_Window(0x40, leaving, true); // RA2: as above
			}
			for (int i = 0; g_ChannelCount && i < 150; i++) { // RA2: as above
				Sleep(100); // RA2: as above
				WOL_Wait_Callback(); // RA2: as above
			}
			Close_Wait_Window(0x40); // RA2: expanded here
			if (g_ChannelCount) { // RA2: the channel was not left
				return(1); // RA2: as above
			}
		}
		strcpy((char *)channel->key, password.text()); // RA2: as above
		g_PendingChannel = *channel; // RA2: as above
		{
			WideWstring message(title.text()); // RA2: as above
			Show_Wait_Window(2, message, false); // RA2: as above
		}
		DebugString("Requesting channel join %s\n", name); // RA2: as above
		return g_pChat->RequestChannelJoin(channel); // RA2: virtual 0x20
	}
	return(0); // RA2: as above
}

int Join_Selected_Lobby(bool join) // RA2: 0x00744930, not in TibSun: joins the lobby channel Pick_Lobby chose (the argument is its "needs room" flag); the lobby's index, -1 when none was picked or the join was refused
{
	int choice = Pick_Lobby(join); // RA2: as above
	if (choice != -1) { // RA2: as above
		Channel channel; // RA2: as above
		g_UserChannels.get(channel, choice); // RA2: as above
		int result; // RA2: as above
		{
			Wstring password("zotclot9"); // RA2: the password of the lobby channels
			result = Request_Channel_Join(&channel, password, WideWstring(StringTable::LoadString("TXT_JOINLOB", NULL, "D:\\Ra2\\wonline.cpp", 9428))); // RA2: the wait text is passed by value
		}
		if (!result) { // RA2: as above
			HWND dialog = WS_Find_Dialog(0x113); // RA2: the game list dialog gets the focus
			if (dialog) { // RA2: as above
				SetFocus(GetDlgItem(dialog, 1344)); // RA2: as above
			}
			Refresh_Game_List(); // RA2: as above
			return(choice); // RA2: as above
		}
	}
	Refresh_Game_List(); // RA2: as above
	return(-1); // RA2: as above
}

namespace OwnerDraw { void __fastcall Slide_Dialog(HWND dialog, bool reverse); } // RA2: 0x005E5150, defined in ownrdraw.cpp
extern int g_NewNickResult; // RA2: 0x00B2AC08, the result OnNewNick stores
extern int g_AgeCheckResult; // RA2: 0x00B2AC0C, the result OnAgeCheck stores (0x80040100 when a field was missing)
extern Wstring g_BirthdateEmail; // RA2: 0x00B29CE4, the email the birthdate dialog (0xE7) stores
extern int g_BirthdateMonth; // RA2: 0x00B26894, the month the birthdate dialog stores (the list's index, from 0)
extern int g_BirthdateDay; // RA2: 0x00B26930, the day the birthdate dialog stores
extern int g_BirthdateYear; // RA2: 0x00B2A6D4, the year the birthdate dialog stores
extern int g_BirthdateNewsletter; // RA2: 0x00B29098, the newsletter box of the birthdate dialog
extern Wstring g_AccountName; // RA2: 0x00B2A640, the nickname the account dialog (0xE6, 0xF3, 0xF4) stores
extern Wstring g_AccountPassword; // RA2: 0x00B2A71C, the password the account dialog stores
extern Wstring g_AccountPasswordCheck; // RA2: 0x00B2A724, the repeated password the account dialog stores
extern Wstring g_AccountParentEmail; // RA2: 0x00B2945C, the parent's email the account dialog (0xF3) stores
extern int g_AccountShareInfo; // RA2: 0x00B29AD0, the information sharing box of the account dialog (0xF4)
LRESULT CALLBACK Birthdate_Dialog_Proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam); // RA2: 0x007494D0, the dialog proc of the birthdate dialog (0xE7)
LRESULT CALLBACK Account_Details_Dialog_Proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam); // RA2: 0x007498E0, the dialog proc of the account dialogs (0xE6, 0xF3, 0xF4)
void __fastcall Show_Dialog(HWND dialog, int id); // RA2: 0x005FF7E0, defined in ownrdraw.cpp

void __fastcall Create_Wonline_Account(HWND parent) // RA2: 0x00744A00, not in TibSun: asks the birthdate, checks the age with the server, asks the nickname, password and emails and requests the new nickname
{
	OwnerDraw::Slide_Dialog(parent, false); // RA2: as above
	Darken_Surface(NULL, true); // RA2: as above
	char nickname[32]; // RA2: as above
	char password[32]; // RA2: as above
	char confirmation[32]; // RA2: as above
	char email[128]; // RA2: as above
	char parent_email[128]; // RA2: as above
	int newsletter = 1; // RA2: as above
	int month = 1; // RA2: as above
	int day = 0; // RA2: as above
	int year = 0; // RA2: as above
	int share_info; // RA2: as above
	HWND dialog; // RA2: as above
	for (int index = 0; index < 18; ++index) ResetEvent(g_WaitEventHandles[index]); // RA2: as above
	memset(nickname, 0, sizeof(nickname)); // RA2: as above
	memset(password, 0, sizeof(password)); // RA2: as above
	memset(confirmation, 0, sizeof(confirmation)); // RA2: as above
	memset(email, 0, sizeof(email)); // RA2: as above
	memset(parent_email, 0, sizeof(parent_email)); // RA2: as above
	{
		WideWstring message(StringTable::LoadString("WOL:NewAccountFetch", NULL, "D:\\Ra2\\wonline.cpp", 9501)); // RA2: the wait text of the server list request
		Show_Wait_Window(0x80, message, false); // RA2: as above
	}
	DebugString("Newnick update check\n"); // RA2: as above
	g_pChat->RequestServerList(g_GameSKU, 0x10006, "TibSun", "TibPass99", 35); // RA2: the login of the account server list request
	DWORD event; // RA2: as above
	do { // RA2: as above
		event = WaitForMultipleObjects(18, g_WaitEventHandles, false, 100); // RA2: as above
		while (event == WAIT_TIMEOUT) { // RA2: as above
			WOL_Wait_Callback(); // RA2: as above
			event = WaitForMultipleObjects(18, g_WaitEventHandles, false, 100); // RA2: as above
		}
		ResetEvent(g_WaitEventHandles[event]); // RA2: as above
	} while (event != 0 && event != 1); // RA2: as above
	Close_Wait_Window(0xFFFFFFFF); // RA2: as above
	if (event == 0) { // RA2: the server list request failed
		ShowWindow(parent, SW_SHOW); // RA2: as above
		OwnerDraw::Set_Paint_Disabled(parent, 0); // RA2: as above
		return; // RA2: as above
	}
	if (event == 8) { // RA2: the game was left meanwhile, the event stays set
		SetEvent(g_WaitEventHandles[8]); // RA2: as above
		ShowWindow(parent, SW_SHOW); // RA2: as above
		OwnerDraw::Set_Paint_Disabled(parent, 0); // RA2: as above
		return; // RA2: as above
	}
birthdate_retry: // RA2: the retry is written as a jump, as in the reference body; a loop form has not been tried yet
	Darken_Surface(NULL, true); // RA2: as above
	dialog = WS_Create_Dialog(ProgramInstance, 0xE7, MainWindow, (DLGPROC)Birthdate_Dialog_Proc, 0); // RA2: the birthdate dialog
	Show_Dialog(dialog, 0xE7); // RA2: as above
	SendDlgItemMessage(dialog, 0x62E, CB_SETCURSEL, month - 1, 0); // RA2: the month list
	if (day > 0) { // RA2: the day edit
		Wstring text = Wstring_FromInt(day); // RA2: as above
		char const * number = text.get(); // RA2: as above
		HWND edit = GetDlgItem(dialog, 0x62F); // RA2: as above
		SendMessage(edit, WM_USER + 0xB4, 0, (LPARAM)number); // RA2: as above
	} else {
		HWND edit = GetDlgItem(dialog, 0x62F); // RA2: as above
		SendMessage(edit, WM_USER + 0xB4, 0, (LPARAM)""); // RA2: as above
	}
	if (year > 0) { // RA2: the year edit
		Wstring text = Wstring_FromInt(year); // RA2: as above
		char const * number = text.get(); // RA2: as above
		HWND edit = GetDlgItem(dialog, 0x630); // RA2: as above
		SendMessage(edit, WM_USER + 0xB4, 0, (LPARAM)number); // RA2: as above
	} else {
		HWND edit = GetDlgItem(dialog, 0x630); // RA2: as above
		SendMessage(edit, WM_USER + 0xB4, 0, (LPARAM)""); // RA2: as above
	}
	SendDlgItemMessage(dialog, 0x626, WM_USER + 0xB4, 0, (LPARAM)email); // RA2: the email edit
	SendDlgItemMessage(dialog, 0x631, BM_SETCHECK, newsletter, 0); // RA2: the newsletter box
	ShowWindow(parent, SW_HIDE); // RA2: as above
	ShowWindow(dialog, SW_SHOWNORMAL); // RA2: as above
	if (WS_Wait_Dialog(dialog, Wait_Callback_Thunk, false, true, true) == 1) { // RA2: the birthdate was entered
		email[0] = 0; // RA2: the answers of the birthdate dialog
		strcpy(email, g_BirthdateEmail.get()); // RA2: as above
		month = g_BirthdateMonth + 1; // RA2: as above
		day = g_BirthdateDay; // RA2: as above
		year = g_BirthdateYear; // RA2: as above
		newsletter = g_BirthdateNewsletter; // RA2: as above
		if (month <= 0 || day <= 0 || year <= 0 || strlen(email) < 6) { // RA2: a field is missing
			ODMessageBox(StringTable::LoadString("TXT_REQUIRED_FIELD", NULL, "D:\\Ra2\\wonline.cpp", 9595), 0, Wait_Callback_Thunk, false); // RA2: as above
			goto birthdate_retry; // RA2: as above
		}
		g_UserConsent = 0; // RA2: the age check fills both
		g_UserAge = -1; // RA2: as above
		if (g_pNetUtil->RequestAgeCheck(month, day, year, email) != 0) { // RA2: as above
			ODMessageBox(StringTable::LoadString("TXT_CANT_CONNECT", NULL, "D:\\Ra2\\wonline.cpp", 9603), 0, Wait_Callback_Thunk, false); // RA2: as above
			goto account_retry; // RA2: as above
		}
		{
			WideWstring message(StringTable::LoadString("WOL:RequestingNickname", NULL, "D:\\Ra2\\wonline.cpp", 9606)); // RA2: the wait text of the age check
			Show_Wait_Window(0x100, message, true); // RA2: as above
		}
		if (g_UserAge < 0) { // RA2: the age check failed
			if (g_AgeCheckResult == (int)0x80040100) { // RA2: a field was missing
				ODMessageBox(StringTable::LoadString("TXT_REQUIRED_FIELD", NULL, "D:\\Ra2\\wonline.cpp", 9610), 0, Wait_Callback_Thunk, false); // RA2: as above
			} else {
				ODMessageBox(StringTable::LoadString("TXT_CANT_CONNECT", NULL, "D:\\Ra2\\wonline.cpp", 9612), 0, Wait_Callback_Thunk, false); // RA2: as above
			}
			goto birthdate_retry; // RA2: as above
		}
		if (g_UserAge < 13 && !g_UserConsent) { // RA2: a child needs the consent of the parents
			if (ODMessageBox(StringTable::LoadString("TXT_CONSENT_REQUIRED", NULL, "D:\\Ra2\\wonline.cpp", 9621), MB_YESNO, Wait_Callback_Thunk, false) == IDYES) { // RA2: as above
				char url[256]; // RA2: as above
				sprintf(url, "http://apiregister.westwood.com/consent_form/index_%d.html", g_LanguageCode); // RA2: as above
				ViewHTML(url, 1); // RA2: as above
			}
			goto birthdate_retry; // RA2: as above
		}
account_retry: // RA2: the retry is written as a jump, as in the reference body; a loop form has not been tried yet
		int resource; // RA2: the account dialog of the age group
		if (g_UserAge < 13) resource = 0xE6; // RA2: as above
		else resource = g_UserAge >= 18 ? 0xF4 : 0xF3; // RA2: as above
		dialog = WS_Create_Dialog(ProgramInstance, resource, MainWindow, (DLGPROC)Account_Details_Dialog_Proc, 0); // RA2: as above
		Show_Dialog(dialog, resource); // RA2: as above
		if (nickname && strlen(nickname) > 0) { // RA2: the nickname edit
			HWND edit = GetDlgItem(dialog, 0x628); // RA2: as above
			SendMessage(edit, WM_USER + 0xB4, 0, (LPARAM)nickname); // RA2: as above
		} else {
			HWND edit = GetDlgItem(dialog, 0x628); // RA2: as above
			SendMessage(edit, WM_USER + 0xB4, 0, (LPARAM)""); // RA2: as above
		}
		if (parent_email && strlen(parent_email) > 0) { // RA2: the parent's email edit
			HWND edit = GetDlgItem(dialog, 0x5E5); // RA2: as above
			SendMessage(edit, WM_USER + 0xB4, 0, (LPARAM)parent_email); // RA2: as above
		} else {
			HWND edit = GetDlgItem(dialog, 0x5E5); // RA2: as above
			SendMessage(edit, WM_USER + 0xB4, 0, (LPARAM)""); // RA2: as above
		}
		if (password && strlen(password) > 0) { // RA2: the password edit
			HWND edit = GetDlgItem(dialog, 0x72C); // RA2: as above
			SendMessage(edit, WM_USER + 0xB4, 0, (LPARAM)password); // RA2: as above
		} else {
			HWND edit = GetDlgItem(dialog, 0x72C); // RA2: as above
			SendMessage(edit, WM_USER + 0xB4, 0, (LPARAM)""); // RA2: as above
		}
		if (confirmation && strlen(confirmation) > 0) { // RA2: the repeated password edit
			HWND edit = GetDlgItem(dialog, 0x627); // RA2: as above
			SendMessage(edit, WM_USER + 0xB4, 0, (LPARAM)confirmation); // RA2: as above
		} else {
			HWND edit = GetDlgItem(dialog, 0x627); // RA2: as above
			SendMessage(edit, WM_USER + 0xB4, 0, (LPARAM)""); // RA2: as above
		}
		ShowWindow(dialog, SW_SHOWNORMAL); // RA2: as above
		SendMessage(dialog, WM_USER + 0xA9, 0, 1); // RA2: as above
		if (WS_Wait_Dialog(dialog, Wait_Callback_Thunk, false, true, true) != 1) goto birthdate_retry; // RA2: cancelled, back to the birthdate
		nickname[0] = 0; // RA2: the answers of the account dialog
		strcpy(nickname, g_AccountName.get()); // RA2: as above
		password[0] = 0; // RA2: as above
		strcpy(password, g_AccountPassword.get()); // RA2: as above
		confirmation[0] = 0; // RA2: as above
		strcpy(confirmation, g_AccountPasswordCheck.get()); // RA2: as above
		parent_email[0] = 0; // RA2: as above
		if (resource == 0xF3) strcpy(parent_email, g_AccountParentEmail.get()); // RA2: only the teenagers' dialog asks for it
		share_info = 0; // RA2: as above
		if (resource == 0xF4) share_info = g_AccountShareInfo; // RA2: only the adults' dialog asks for it
		if (strcmp(password, confirmation)) { // RA2: as above
			ODMessageBox(StringTable::LoadString("TXT_PASSWORD_VERIFY", NULL, "D:\\Ra2\\wonline.cpp", 9727), 0, Wait_Callback_Thunk, false); // RA2: as above
			goto account_retry; // RA2: as above
		}
		if (strlen(password) != 8) { // RA2: as above
			ODMessageBox(StringTable::LoadString("TXT_PASSWORD_TOO_SHORT", NULL, "D:\\Ra2\\wonline.cpp", 9732), 0, Wait_Callback_Thunk, false); // RA2: as above
			goto account_retry; // RA2: as above
		}
		if (g_pNetUtil->RequestNewNick(nickname, password, email, parent_email, newsletter, share_info) != 0) { // RA2: as above
			ODMessageBox(StringTable::LoadString("TXT_CANT_CONNECT", NULL, "D:\\Ra2\\wonline.cpp", 9737), 0, Wait_Callback_Thunk, false); // RA2: as above
		} else {
			{
				WideWstring message(StringTable::LoadString("TXT_REQ_NICK", NULL, "D:\\Ra2\\wonline.cpp", 9739)); // RA2: the wait text of the nickname request
				Show_Wait_Window(0x100, message, true); // RA2: as above
			}
			if (g_NewNickResult < 0) goto account_retry; // RA2: the nickname was refused
		}
		ShowWindow(parent, SW_SHOW); // RA2: as above
		OwnerDraw::Set_Paint_Disabled(parent, 0); // RA2: as above
		return; // RA2: as above
	} else {
		ShowWindow(parent, SW_SHOWNORMAL); // RA2: the dialog was cancelled
	}
	ShowWindow(parent, SW_SHOW); // RA2: as above
	OwnerDraw::Set_Paint_Disabled(parent, 0); // RA2: as above
}

int Save_Nickname(char const * name, char const * password, bool mangle, int locale) // RA2: 0x00745320, not in TibSun: stores the persona in the chat API's list of 32 logins (the slot it already has, else an empty one, else the next of a round robin) and its locale; the slot
{
	int slot = 0; // RA2: as above
	char const * nick; // RA2: as above
	char const * stored; // RA2: as above
	for (int i = 1; i <= 32; i++) { // RA2: the slot that already holds the persona
		nick = NULL; // RA2: as above
		stored = NULL; // RA2: as above
		if (g_pChat->GetNick(i, &nick, &stored) == 0 && nick && !strcmp(nick, name)) { // RA2: as above
			slot = i; // RA2: as above
			break; // RA2: as above
		}
	}
	if (slot == 0) { // RA2: else the first empty one
		for (int i = 1; i <= 32; i++) { // RA2: as above
			nick = NULL; // RA2: as above
			stored = NULL; // RA2: as above
			if (g_pChat->GetNick(i, &nick, &stored) != 0 || !nick || strlen(nick) == 0) { // RA2: as above
				slot = i; // RA2: as above
				break; // RA2: as above
			}
		}
	}
	if (slot == 0) { // RA2: else the next slot in turn
		int next = Session.LastNicknameSlot; // RA2: as above
		if (next == -1) { // RA2: as above
			next = 1; // RA2: as above
		}
		slot = next++; // RA2: as above
		Session.LastNicknameSlot = next; // RA2: as above
		if (next > 32) { // RA2: as above
			Session.LastNicknameSlot = 1; // RA2: as above
		}
	}
	if (slot) { // RA2: as above
		g_pChat->SetNick(slot, name, password, mangle); // RA2: virtual 0x94
		g_pChat->SetNickLocale(slot, (Locale)locale); // RA2: virtual 0xC8
	}
	return(slot); // RA2: as above
}

Wstring Server_Name_Without_Prefix(Server const * server) // RA2: 0x00745450, not in TibSun: the name of the server after the first colon (the part before it names the kind of server)
{
	Wstring name((char const *)server->name); // RA2: the server's name is copied into a Wstring
	Wstring token; // RA2: as above
	int position = name.getToken(0, ":", token); // RA2: the first token ends at the first colon
	if (position >= 0) { // RA2: as above
		name.remove(0, position + 1); // RA2: as above
	}
	return(name); // RA2: as above
}

#define OD_ADDSTRING_COMBO (WM_USER + 188) // RA2: 0x4BC, appends the string of lParam to an owner drawn combo box and returns its row (name guessed from its use)
#define OD_ADDSTRING_LIST (WM_USER + 200) // RA2: 0x4C8, the same for an owner drawn list box (name guessed from its use)
int __fastcall Clear_ComboBox(HWND list); // RA2: 0x006007F0, removes every row of an owner drawn combo box, defined in ownrdraw.cpp

bool Populate_Server_Choices(HWND list, bool combo) // RA2: 0x007454C0, not in TibSun: fills a list box (or combo box) with the names of the chat servers and selects the one of the persona's profile; false when the server list is empty
{
	if (combo) { // RA2: as above
		Clear_ComboBox(list); // RA2: as above
	} else {
		Clear_Listbox(list); // RA2: as above
	}
	if (g_Servers.length() == 1) { // RA2: only the entry 0 of the list: no server to choose from
		if (combo) { // RA2: as above
			SendMessage(list, OD_ADDSTRING_COMBO, 0, (LPARAM)"GUI:NoServersAvailable"); // RA2: as above
		} else {
			SendMessage(list, OD_ADDSTRING_LIST, 0, (LPARAM)"GUI:NoServersAvailable"); // RA2: as above
		}
		EnableWindow(list, FALSE); // RA2: as above
		return(false); // RA2: as above
	}
	Wstring preferred; // RA2: the server of the persona's profile
	WOLOptionsClass options; // RA2: as in Connect_To_Server
	options.Load(); // RA2: as above
	rc_ptr<PlayerProfile> profile; // RA2: as above
	if (options.Is_Bit1_Clear()) { // RA2: as above
		profile = PlayerProfile::Load(g_NickName); // RA2: as above
	} else {
		profile = g_PlayerProfile.Get(); // RA2: as above
	}
	if (profile.Get()) { // RA2: as above
		preferred = profile->Name; // RA2: as above
	}
	EnableWindow(list, TRUE); // RA2: as above
	int selected = -1; // RA2: as above
	for (int i = 1; i < g_Servers.length(); i++) { // RA2: as above
		Server * server = NULL; // RA2: as above
		g_Servers.getPointer(&server, i); // RA2: as above
		DebugString("Server found %s\n", (char *)server->name); // RA2: as above
		Wstring display = Server_Name_Without_Prefix(server); // RA2: expanded here
		int row; // RA2: as above
		if (combo) { // RA2: as above
			row = SendMessage(list, OD_ADDSTRING_COMBO, 0, (LPARAM)display.text()); // RA2: as above
			SendMessage(list, CB_SETITEMDATA, row, i); // RA2: as above
		} else {
			row = SendMessage(list, OD_ADDSTRING_LIST, 0, (LPARAM)display.text()); // RA2: as above
			SendMessage(list, LB_SETITEMDATA, row, i); // RA2: as above
		}
		if (preferred.length() > 0) { // RA2: as above
			bool match = ((Wstring const &)preferred).toLower() == ((Wstring const &)display).toLower(); // RA2: the const toLower returns a copy
			if (match) { // RA2: as above
				selected = row; // RA2: as above
				g_CurrentServerIndex = i; // RA2: the server of the profile becomes the one to connect to
				g_TargetServerIndex = i; // RA2: as above
			}
		}
	}
	if (selected == -1) { // RA2: as above
		selected = 0; // RA2: as above
	}
	if (combo) { // RA2: as above
		SendMessage(list, CB_SETCURSEL, selected, 0); // RA2: as above
	} else {
		SendMessage(list, LB_SETCURSEL, selected, 0); // RA2: as above
		SendMessage(list, LB_SETTOPINDEX, selected, 0); // RA2: as above
	}
	return(true); // RA2: as above
}

bool __fastcall Can_Join_Channel(Channel const * channel, bool check, bool observer); // RA2: 0x00755CD0, whether the player may join the game channel (the version, the full game, the observer slot ...); not yet defined
bool Test_Display_Mode_Dialog(int width, int height); // RA2: 0x00543A70, defined in mainopt.cpp
LRESULT CALLBACK Wonline_Password_Dialog_Proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam); // RA2: 0x007492D0, the dialog proc of the password dialog (0xE1)
void Darken_Surface(Surface * surface, bool update); // RA2: 0x005159B0, defined in init.cpp
class WonlineStringDialogControl : public SimpleWonlineDialogControl { // RA2: vtable 0x007AF934, the control of the dialogs that ask for a text
public: // RA2: as above
	WonlineStringDialogControl(void) {} // RA2: expanded where it is used
	virtual ~WonlineStringDialogControl(void) {} // RA2: expanded where it is used
	Wstring Text; // RA2: +0x0C, the text the player typed
};
#include "misc.h" // RA2: VideoModeWidth

bool __fastcall Join_Game_Channel(Channel const * channel, bool observer) // RA2: 0x00745780, not in TibSun: joins a game channel: tests that the player may (Can_Join_Channel), that the screen is not wider than the game's resolution limit (offering to switch to 800x600 or 640x480), asks for the password of a private game; true when the join was requested
{
	if (!Can_Join_Channel(channel, true, observer)) { // RA2: as above
		return(false); // RA2: as above
	}
	unsigned int mode = channel->reserved; // RA2: the flags of the game: the resolution limit
	int maximum = (mode & 0x400) ? 800 : ((mode & 0x200) ? 640 : 1024); // RA2: as above
	if (VideoModeWidth > maximum) { // RA2: the screen is wider than the game allows
		{
			WideWstring warning((mode & 0x400) ? StringTable::LoadString("WOL:ResolutionLimited800", NULL, "D:\\Ra2\\wonline.cpp", 10143) : StringTable::LoadString("WOL:ResolutionLimited640", NULL, "D:\\Ra2\\wonline.cpp", 10144)); // RA2: as above
			if (ODMessageBox(warning.get(), 4, Wait_Callback_Thunk, true) == 6) { // RA2: IDYES: the screen is switched
				int width = (maximum == 800) ? 800 : 640; // RA2: as above
				int height = (width == 800) ? 600 : 480; // RA2: as above
				if (Test_Display_Mode_Dialog(width, height)) { // RA2: as above
					Options.ScreenWidth = width; // RA2: as above
					Options.ScreenHeight = height; // RA2: as above
					Options.Save_Settings(); // RA2: as above
				}
			}
		}
		if (VideoModeWidth > maximum) { // RA2: as above
			return(false); // RA2: as above
		}
	}
	Wstring password; // RA2: as above
	if (channel->flags & 0x100) { // RA2: a private game: the player types the password
		Darken_Surface(NULL, true); // RA2: as above
		WonlineStringDialogControl control; // RA2: as above
		if (OwnerDraw::Controlled_Dialog(0xE1, (DLGPROC)Wonline_Password_Dialog_Proc, &control) == 2) { // RA2: cancelled
			return(false); // RA2: as above
		}
		password = control.Text; // RA2: as above
		password.replace(" ", "_"); // RA2: spaces are not allowed in the key
	}
	return(Request_Channel_Join((Channel *)channel, password, WideWstring(StringTable::LoadString("TXT_JOINLOB", NULL, "D:\\Ra2\\wonline.cpp", 10199))) == 0); // RA2: as above
}

class CreateGameDialogControl : public WonlineStringDialogControl { // RA2: vtable 0x007AF94C, the control of the create game dialog (0xC3); its base constructor 0x00765AA0 is called out of line from Host_Game, the four flags are set where it is used
public: // RA2: as above
	CreateGameDialogControl(bool limit_resolution, bool observer) : IsTournament(false), IsPrivate(false), LimitResolution(limit_resolution), Observer(observer) {} // RA2: the executable stores +0x10 and +0x11 as 0, +0x12 and +0x13 from the arguments
	virtual ~CreateGameDialogControl(void) {} // RA2: expanded where it is used
	bool IsTournament; // RA2: +0x10, the dialog's radio button 0x629 sets it (Host_Game copies it to the channel's tournament member)
	bool IsPrivate; // RA2: +0x11, the radio button 0x66C sets it (the game gets a password and the flag 0x100)
	bool LimitResolution; // RA2: +0x12, the game limits the screen size (flag 0x200 or 0x400)
	bool Observer; // RA2: +0x13, the host is an observer
};
LRESULT CALLBACK Create_Game_Dialog_Proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam); // RA2: 0x00748E10, the dialog proc of the create game dialog (0xC3); not yet defined
BOOL CALLBACK Game_Options_Dialog_Proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam); // RA2: 0x0074A710, the dialog proc of the game options dialog (0xC2); not yet defined
void __fastcall Seat_First_Player(int slot); // RA2: 0x005C8CD0, defined in netshare.cpp
void Clear_Vector(DynamicVectorClass<NodeNameType *> * vector); // RA2: 0x005A7480, defined in mplayer.cpp
MultiplayerGameMode * First_Game_Mode(void); // RA2: 0x005B6EC0, defined in multiplayer.cpp
extern int LobbyObserverSlot; // RA2: 0x007E6D90, the slot of the observer (7 when none), as in netshare.cpp
namespace OwnerDraw { // RA2: as above
void __fastcall Slide_Dialog(HWND dialog, bool reverse); // RA2: 0x005E5150, defined in ownrdraw.cpp
}

static inline void Reset_Session_Players(void) // RA2: the players of the session are cleared (expanded in Host_Game)
{
	MultiplayerGameMode * mode = First_Game_Mode(); // RA2: as above
	Session.MPGameMode = mode; // RA2: as above
	Session.Options.MPModeIndex = mode->MPModeIndex; // RA2: as above
	mode->vt_entry_20(); // RA2: as above
	Set_Scenario_Info_From_Index(0); // RA2: as above
	Clear_Vector(&Session.Players); // RA2: as above
	Clear_Slot_Lists(); // RA2: as above
	Reset_Slots(); // RA2: as above
}

static inline void Show_Game_Options(void) // RA2: opens the game options dialog (0xC2) of a game the player hosts (expanded in Host_Game)
{
	if (!Is_Tournament_Session_Inline()) { // RA2: as above
		HWND top = WS_Top_Window(); // RA2: as above
		if (top) { // RA2: as above
			OwnerDraw::Slide_Dialog(top, false); // RA2: as above
			Darken_Surface(NULL, true); // RA2: as above
		}
		Darken_Surface(NULL, true); // RA2: as above
		HWND dialog = OwnerDraw::Begin_Dialog(0xC2, (DLGPROC)Game_Options_Dialog_Proc, 0); // RA2: as above
		Session.MPGameMode->vt_entry_84(); // RA2: as above
		Session.MPGameMode->vt_entry_20(); // RA2: as above
		Session.MPGameMode->AssignTeamsAtStart(); // RA2: as above
		Session.MPGameMode->UnfixAlliances(); // RA2: as above
		ShowWindow(dialog, SW_SHOWNORMAL); // RA2: as above
		if (top) { // RA2: as above
			ShowWindow(top, SW_HIDE); // RA2: as above
		}
	}
}

/*
**	RA2: this unit's Add of the player list holds the new length in a variable (the executable forms it in the register
**	that held Length(): 0x007459A0 + 1204), as link2.cpp and objtype.cpp have it for their vectors.
*/
template<> inline bool DynamicVectorClass<NodeNameType *>::Add(NodeNameType * const & object) // RA2: as above
{
	if (ActiveCount >= Length()) { // RA2: as above
		if ((IsAllocated || !VectorMax) && GrowthStep > 0) { // RA2: as above
			int new_length = Length(); // RA2: as above
			new_length += GrowthStep; // RA2: as above
			if (!Resize(new_length)) { // RA2: as above
				return(false); // RA2: as above
			}
		} else {
			return(false); // RA2: as above
		}
	}
	(*this)[ActiveCount++] = object; // RA2: as above
	return(true); // RA2: as above
}

bool __fastcall Host_Game(void) // RA2: 0x007459A0, not in TibSun: hosts a game: asks for the game's settings (the create game dialog 0xC3, unless it is an Internet tournament game), makes the host the first player of the lobby, asks the chat service for the game channel and opens the game options dialog (0xC2); true when the channel was created
{
	Wstring password; // RA2: the key of the game channel
	if (VideoModeWidth >= 1024) { // RA2: as above
		Session.WOLLimitResolution = false; // RA2: as above
	}
	CreateGameDialogControl control(Session.WOLLimitResolution, ObserverMode == 1); // RA2: as above
	g_GameStartRequested = false; // RA2: as above
	if (Is_Tournament_Session_Inline()) { // RA2: a tournament game has two players and no dialog
		g_MaxPlayers = 2; // RA2: as above
	} else {
		Darken_Surface(NULL, true); // RA2: as above
		if (OwnerDraw::Controlled_Dialog(0xC3, (DLGPROC)Create_Game_Dialog_Proc, &control) == 2) { // RA2: cancelled
			return(false); // RA2: as above
		}
		password = control.Text; // RA2: as above
		password.replace(" ", "_"); // RA2: spaces are not allowed in the key
	}
	g_pChat->RequestChannelLeave(); // RA2: virtual 0x24
	Channel channel; // RA2: the game channel
	memset(&channel, 0, sizeof(channel)); // RA2: as above
	sprintf((char *)channel.name, "%s's_game", *(char **)&g_NickName); // RA2: the executable reads the string pointer of the Wstring itself (no accessor call)
	channel.type = 0x21; // RA2: a game channel
	channel.minUsers = 1; // RA2: as above
	if (g_MaxPlayers < 2) { // RA2: as above
		g_MaxPlayers = 4; // RA2: as above
	}
	strcpy((char *)channel.key, password.get()); // RA2: as above
	channel.maxUsers = g_MaxPlayers; // RA2: as above
	char info[80]; // RA2: the info text of the channel
	memset(info, 0, sizeof(info)); // RA2: as above
	Encode_Channel_ExInfo(info); // RA2: as above
	info[40] = 0; // RA2: the info member holds 40 characters
	strcpy((char *)channel.exInfo, info); // RA2: as above
	if (Session.Type == GAME_INTERNET && Online_Session_State == 2) { // RA2: an Internet tournament game
		channel.tournament = 1; // RA2: as above
		channel.reserved |= 0x1000; // RA2: as above
	} else if (Is_Tournament_Session_Inline()) { // RA2: as above
		channel.tournament = 1; // RA2: as above
		channel.reserved |= 0x800; // RA2: as above
	} else {
		channel.tournament = control.IsTournament; // RA2: as above
		if (control.IsPrivate) { // RA2: as above
			channel.reserved |= 0x100; // RA2: as above
		}
		Session.WOLLimitResolution = control.LimitResolution; // RA2: as above
		if (control.LimitResolution) { // RA2: as above
			channel.reserved |= (VideoModeWidth == 800) ? 0x400 : 0x200; // RA2: as above
		}
	}
	channel.reserved = channel.reserved + g_LobbyNumber; // RA2: the lobby the host came from
	Leave_Current_Channel(0xC0, false); // RA2: expanded here
	Reset_Session_Players(); // RA2: expanded here
	NodeNameType * host = new NodeNameType; // RA2: the entry of the host
	memset(host, 0, sizeof(NodeNameType)); // RA2: as above
	WideWstring name = Wstring_ToWide(g_NickName); // RA2: as above
	wcscpy(host->Name, name.get()); // RA2: as above
	WOLOptionsClass options; // RA2: as above
	options.Load(); // RA2: as above
	rc_ptr<PlayerProfile> profile; // RA2: as above
	if (options.Is_Bit1_Clear()) { // RA2: as above
		profile = PlayerProfile::Load(g_NickName); // RA2: as above
	} else {
		profile = g_PlayerProfile.Get(); // RA2: as above
	}
	if (profile.Get()) { // RA2: the side and color of the persona
		Session.Set_Side(profile->Country, false); // RA2: as above
		Session.Set_Color(profile->Color, false); // RA2: as above
	}
	host->Set_Country(Session.Chosen_House()); // RA2: as above
	host->Set_Color(Session.Chosen_Color()); // RA2: as above
	host->Slot.Unknown77 = 1; // RA2: as above
	host->Slot.SpectatorFlag = 0; // RA2: as above
	LobbyObserverSlot = 7; // RA2: as above
	ObserverMode = control.Observer; // RA2: as above
	Update_Observer_Buttons(0); // RA2: as above
	if (ObserverMode) { // RA2: the host observes: the first slot, team -1, house -3
		LobbyObserverSlot = 0; // RA2: as above
		host->Slot.SpectatorFlag = -1; // RA2: as above
		host->Set_Country(-3); // RA2: as above
	}
	Session.Players.Add(host); // RA2: as above
	Seat_First_Player(0); // RA2: as above
	wcscpy(Session.GameName, name.get()); // RA2: as above
	HRESULT result = g_pChat->RequestChannelCreate(&channel); // RA2: virtual 0x1C
	if (result == 0x800401F8) { // RA2: the key was refused
		ODMessageBox(StringTable::LoadString("TXT_BAD_CHANKEY", NULL, "D:\\Ra2\\wonline.cpp", 10434), 0, Wait_Callback_Thunk, false); // RA2: as above
		Close_Wait_Window(0x80); // RA2: expanded here
		return(false); // RA2: as above
	}
	if (result != 0) { // RA2: as above
		ODMessageBox(StringTable::LoadString("TXT_CANTCREATE_CHAN", NULL, "D:\\Ra2\\wonline.cpp", 10440), 0, Wait_Callback_Thunk, false); // RA2: as above
		Close_Wait_Window(0x80); // RA2: expanded here
		return(false); // RA2: as above
	}
	WideWstring title(StringTable::LoadString("GUI:CreatingChannel", NULL, "D:\\Ra2\\wonline.cpp", 10448)); // RA2: the wait dialog takes a wide text
	Show_Wait_Window(4, title, false); // RA2: as above
	Close_Wait_Window(0x80); // RA2: expanded here
	Show_Wait_Window(4, title, true); // RA2: as above
	int index = Player_Name_To_Index(g_NickName); // RA2: expanded here
	if (index < 0) { // RA2: the host is not in the list of users
		return(false); // RA2: as above
	}
	User * user = NULL; // RA2: as above
	g_UserList.getPointer(&user, index); // RA2: as above
	host->Address = User_To_IPX_Address(user); // RA2: expanded here
	Show_Game_Options(); // RA2: expanded here
	return(true); // RA2: as above
}

extern char LastFindName[]; // RA2: 0x00B29CD8, the nick the last /find command asked for

int __fastcall Parse_Online_Command(wchar_t const * text) // RA2: 0x00746240, not in TibSun: the lobby chat command that looks a player up (TXT_ONLINE_COMMAND followed by a nick): 1 when the command was handled
{
	int handled = 0; // RA2: as above
	wchar_t command[64]; // RA2: as above
	wcscpy(command, StringTable::LoadString("TXT_ONLINE_COMMAND", 0, "D:\\Ra2\\wonline.cpp", 10505)); // RA2: as above
	if (wcsncmp(text, command, wcslen(command)) == 0) { // RA2: as above
		int count = 0; // RA2: as above
		wchar_t const * cursor = text + wcslen(command); // RA2: as above
		while (wcsncmp(cursor, L" ", 1) == 0) { // RA2: as above
			++cursor; // RA2: as above
		}
		wchar_t name[16]; // RA2: as above
		while (*cursor) { // RA2: as above
			if (wcsncmp(cursor, L" ", 1) == 0 || count >= 10) { // RA2: as above
				break; // RA2: as above
			}
			name[count++] = *cursor++; // RA2: as above
		}
		name[count] = 0; // RA2: as above
		if (wcslen(name) > 0) { // RA2: as above
			User user; // RA2: as above
			strcpy((char *)user.name, Wide_To_Narrow(name)); // RA2: as above
			strcpy(LastFindName, Wide_To_Narrow(name)); // RA2: as above
			g_pChat->RequestFind(&user); // RA2: virtual 0x60
			handled = 1; // RA2: as above
		} else {
			wchar_t message[256]; // RA2: as above
			swprintf(message, StringTable::LoadString("TXT_NO_NAME", 0, "D:\\Ra2\\wonline.cpp", 10525)); // RA2: as above
			HWND window = WS_Top_Window(); // RA2: as above
			if (window) { // RA2: as above
				if (GetDlgItem(window, 0x53F)) { // RA2: as above
					Lobby_Chat_Printf(ColorSystem, message); // RA2: as above
				}
				if (GetDlgItem(window, 0x5B5)) { // RA2: as above
					SMessagePrintf(ColorSystem, message); // RA2: as above
				}
			}
			if (Session.Is_In_Game()) { // RA2: as above
				int color = GadgetClass::Get_Color_Scheme(); // RA2: as above
				Session.Messages.Add_Message(0, 0, message, color, TextPrintType(0x16), (int)(Rule->MessageDelay * TICKS_PER_MINUTE), false); // RA2: as above
			}
			handled = 1; // RA2: as above
		}
	}
	return(handled); // RA2: as above
}

extern int MuteSWLaunches; // RA2: 0x00A3D548, nonzero while the launch announcements are muted (as the other files declare it)

int __fastcall Parse_Page_Command(wchar_t const * text) // RA2: 0x00746480, not in TibSun: the lobby chat command that pages a player (TXT_PAGE_COMMAND, a nick and the text): 1 when handled, 2 when a message was shown instead
{
	int result = 0; // RA2: as above
	wchar_t nickname[16]; // RA2: as above
	User recipient; // RA2: as above
	wchar_t command[128]; // RA2: as above
	wcscpy(command, StringTable::LoadString("TXT_PAGE_COMMAND", 0, "D:\\Ra2\\wonline.cpp", 10559)); // RA2: as above
	if (_wcsnicmp(text, command, wcslen(command)) == 0) { // RA2: as above
		int name_length = 0; // RA2: as above
		unsigned length = wcslen(text) + 1; // RA2: as above
		wchar_t * copy = (wchar_t *)operator new(length * 2); // RA2: as above
		if (!copy) { // RA2: as above
			return(0); // RA2: as above
		}
		copy = wcsncpy(copy, text, length); // RA2: as above
		if (!copy) { // RA2: as above
			return(0); // RA2: as above
		}
		wchar_t * message = copy + wcslen(command); // RA2: as above
		while (*message && wcsncmp(message, L" ", 1) == 0) { // RA2: as above
			++message; // RA2: as above
		}
		while (*message) { // RA2: as above
			if (wcsncmp(message, L" ", 1) == 0 || name_length > 10) { // RA2: as above
				break; // RA2: as above
			}
			nickname[name_length++] = *message++; // RA2: as above
		}
		nickname[name_length] = 0; // RA2: as above
		while (*message && wcsncmp(message, L" ", 1) == 0) { // RA2: as above
			++message; // RA2: as above
		}
		if (name_length > 10) { // RA2: as above
			result = 1; // RA2: as above
		} else if (name_length == 0) { // RA2: as above
			result = 1; // RA2: as above
		} else {
			bool recipient_is_player = false; // RA2: as above
			if (MuteSWLaunches) { // RA2: as above
				for (int i = 0; i < Session.Players.Count(); ++i) { // RA2: as above
					if (_wcsicmp(Session.Players[i]->Name, nickname) == 0) { // RA2: as above
						recipient_is_player = true; // RA2: as above
						break; // RA2: as above
					}
				}
			}
			if (!recipient_is_player) { // RA2: as above
				if (wcslen(message) > 0) { // RA2: as above
					strcpy((char *)recipient.name, Wide_To_Narrow(nickname)); // RA2: as above
					g_pChat->RequestPageWide(&recipient, message); // RA2: virtual 0xF8
				}
				result = 1; // RA2: as above
			} else {
				int color = GadgetClass::Get_Color_Scheme(); // RA2: as above
				Session.Messages.Add_Message(0, 0, StringTable::LoadString("TXT_OBSERVER_CANT_PAGE", 0, "D:\\Ra2\\wonline.cpp", 10619), color, TextPrintType(0x16), (int)(Rule->MessageDelay * TICKS_PER_MINUTE), false); // RA2: as above
				result = 2; // RA2: as above
			}
		}
		operator delete(copy); // RA2: as above
	}
	return(result); // RA2: as above
}

extern char LastPagerName[]; // RA2: 0x00B2AC48, who paged last (as mainloop.cpp declares it)

int __fastcall Reply_To_Last_Pager(wchar_t const * text) // RA2: 0x007466C0, not in TibSun: the lobby chat command that answers the last pager (TXT_REPLY_COMMAND or its short form, then the text): 1 when handled, 2 when a message was shown instead
{
	int result = 0; // RA2: as above
	wchar_t reply_command[128]; // RA2: as above
	User recipient; // RA2: as above
	wchar_t short_command[128]; // RA2: as above
	wchar_t notice[128]; // RA2: as above
	wcscpy(reply_command, StringTable::LoadString("TXT_REPLY_COMMAND", 0, "D:\\Ra2\\wonline.cpp", 10639)); // RA2: as above
	wcscpy(short_command, StringTable::LoadString("TXT_REPLY_COMMAND_SHORT", 0, "D:\\Ra2\\wonline.cpp", 10640)); // RA2: as above
	if (_wcsnicmp(text, reply_command, wcslen(reply_command)) == 0 || wcsncmp(text, short_command, wcslen(short_command)) == 0) { // RA2: as above
		unsigned length = wcslen(text) + 1; // RA2: as above
		wchar_t * copy = (wchar_t *)operator new(length * 2); // RA2: as above
		if (!copy) { // RA2: as above
			return(0); // RA2: as above
		}
		copy = wcsncpy(copy, text, length); // RA2: as above
		if (!copy) { // RA2: as above
			return(0); // RA2: as above
		}
		wchar_t * message; // RA2: as above
		if (wcsncmp(text, reply_command, wcslen(reply_command)) == 0) { // RA2: as above
			message = copy + wcslen(reply_command); // RA2: as above
		} else {
			message = copy + wcslen(short_command); // RA2: as above
		}
		while (*message && wcsncmp(message, L" ", 1) == 0) { // RA2: as above
			++message; // RA2: as above
		}
		bool recipient_is_player = false; // RA2: as above
		if (MuteSWLaunches) { // RA2: as above
			for (int i = 0; i < Session.Players.Count(); ++i) { // RA2: as above
				if (_wcsicmp(Session.Players[i]->Name, Narrow_To_Wide(LastPagerName)) == 0) { // RA2: as above
					recipient_is_player = true; // RA2: as above
					break; // RA2: as above
				}
			}
		}
		if (!recipient_is_player) { // RA2: as above
			if (strlen(LastPagerName) > 0) { // RA2: as above
				if (wcslen(message) > 0) { // RA2: as above
					strcpy((char *)recipient.name, LastPagerName); // RA2: as above
					g_pChat->RequestPageWide(&recipient, message); // RA2: virtual 0xF8
				}
				result = 1; // RA2: as above
			} else {
				wcscpy(notice, StringTable::LoadString("TXT_NO_ONE_WHOM_TO_REPLY", 0, "D:\\Ra2\\wonline.cpp", 10691)); // RA2: as above
				HWND window = WS_Top_Window(); // RA2: as above
				if (window) { // RA2: as above
					if (GetDlgItem(window, 0x53F)) { // RA2: as above
						Lobby_Chat_Printf(ColorSystem, notice); // RA2: as above
					}
					if (GetDlgItem(window, 0x5B5)) { // RA2: as above
						SMessagePrintf(ColorSystem, notice); // RA2: as above
					}
				}
				if (Session.Is_In_Game()) { // RA2: as above
					int color = GadgetClass::Get_Color_Scheme(); // RA2: as above
					Session.Messages.Add_Message(0, 0, notice, color, TextPrintType(0x16), (int)(Rule->MessageDelay * TICKS_PER_MINUTE), false); // RA2: as above
				}
				result = 2; // RA2: as above
			}
		} else {
			int color = GadgetClass::Get_Color_Scheme(); // RA2: as above
			Session.Messages.Add_Message(0, 0, StringTable::LoadString("TXT_OBSERVER_CANT_PAGE", 0, "D:\\Ra2\\wonline.cpp", 10715), color, TextPrintType(0x16), (int)(Rule->MessageDelay * TICKS_PER_MINUTE), false); // RA2: as above
			result = 2; // RA2: as above
		}
		operator delete(copy); // RA2: as above
	}
	return(result); // RA2: as above
}

bool __fastcall Is_AI_Entry(NodeNameType * entry); // RA2: 0x005C9370, defined in netshare.cpp
int __fastcall Get_Scenario_Slot_Limit(int index); // RA2: 0x005C65A0, defined in netshare.cpp
extern NodeNameType * LobbyEntries[]; // RA2: 0x00A72D2C, the fixed entries of the lobby (open, closed, ...), as in netshare.cpp

void Update_Channel_Player_Limit(void) // RA2: 0x00749F20, not in TibSun: tells the chat service how many players the game channel takes (the slots that are open or hold players, plus the host) by a "mode #<channel> +l <number>" command, sends the channel info and the number of players with the computer players
{
	int limit = Get_Scenario_Slot_Limit(Session.Options.ScenarioIndex); // RA2: as above
	if (ObserverMode == 1) { // RA2: the observer takes a slot
		++limit; // RA2: as above
	}
	int fixed_slots = 0; // RA2: the slots held by players
	int open_slots = 0; // RA2: as above
	int player_slots = 0; // RA2: the slots held by computer players
	int i = 1; // RA2: as above
	for (; i < limit; ++i) { // RA2: the slots the scenario has
		NodeNameType * entry = Session.Slots[i]; // RA2: as above
		if (entry != LobbyEntries[0] && entry != LobbyEntries[6]) { // RA2: as above
			if (Is_Real_Player_Entry(entry)) { // RA2: as above
				++fixed_slots; // RA2: as above
			} else if (Is_AI_Entry(entry)) { // RA2: as above
				++player_slots; // RA2: as above
			}
		} else {
			++open_slots; // RA2: as above
		}
	}
	for (; i < 8; ++i) { // RA2: the slots beyond
		NodeNameType * entry = Session.Slots[i]; // RA2: as above
		if (entry == LobbyEntries[6]) { // RA2: as above
			++open_slots; // RA2: as above
		} else if (Is_Real_Player_Entry(entry)) { // RA2: as above
			++fixed_slots; // RA2: as above
		}
	}
	if (g_pChat) { // RA2: as above
		int capacity = open_slots + fixed_slots + 1; // RA2: as above
		Wstring command = Wstring("mode #") + Wstring((char const *)g_CurrentChannel.name) + Wstring(" +l ") + Wstring_FromInt(capacity) + Wstring("\r\n"); // RA2: as above
		g_pChat->RequestRawMessage(command.get()); // RA2: virtual 0x9C
		char info[80]; // RA2: as above
		memset(info, 0, sizeof(info)); // RA2: as above
		Encode_Channel_ExInfo(info); // RA2: as above
		info[40] = 0; // RA2: as above
		g_pChat->SetChannelExInfo(info); // RA2: virtual 0xA8
		g_pChat->Unknown_Virtual_FC(1, capacity + player_slots); // RA2: virtual 0xFC
	}
}

extern wchar_t g_PingTooltip[]; // RA2: 0x00B28DE4, the text Player_Ping_Tooltip returns
int __fastcall Find_Slot_By_Name(wchar_t const * name); // RA2: 0x005C8C30, defined in netshare.cpp

wchar_t * __fastcall Player_Ping_Tooltip(wchar_t const * name) // RA2: 0x0074A4E0, not in TibSun: the tooltip of the ping icon of the player of this slot name: "unknown" when no ping is known, else good (below 300), fair (below 500) or poor with the number
{
	int ping = g_PlayerPings[Find_Slot_By_Name(name)]; // RA2: as above
	if (ping < 0) { // RA2: no ping known
		swprintf(g_PingTooltip, StringTable::LoadString("TXT_LATENCY_ICON_TOOLTIP_NOPING", NULL, "D:\\Ra2\\wonline.cpp", 14063), name, StringTable::LoadString("TXT_UNKNOWN", NULL, "D:\\Ra2\\wonline.cpp", 14063), StringTable::LoadString("TXT_UNKNOWN_PING", NULL, "D:\\Ra2\\wonline.cpp", 14063)); // RA2: as above
	} else if (ping < 300) { // RA2: as above
		swprintf(g_PingTooltip, StringTable::LoadString("TXT_LATENCY_ICON_TOOLTIP_PING", NULL, "D:\\Ra2\\wonline.cpp", 14065), name, StringTable::LoadString("TXT_GOOD", NULL, "D:\\Ra2\\wonline.cpp", 14065), ping); // RA2: as above
	} else if (ping < 500) { // RA2: as above
		swprintf(g_PingTooltip, StringTable::LoadString("TXT_LATENCY_ICON_TOOLTIP_PING", NULL, "D:\\Ra2\\wonline.cpp", 14067), name, StringTable::LoadString("TXT_FAIR", NULL, "D:\\Ra2\\wonline.cpp", 14067), ping); // RA2: as above
	} else {
		swprintf(g_PingTooltip, StringTable::LoadString("TXT_LATENCY_ICON_TOOLTIP_PING", NULL, "D:\\Ra2\\wonline.cpp", 14069), name, StringTable::LoadString("TXT_POOR", NULL, "D:\\Ra2\\wonline.cpp", 14069), ping); // RA2: as above
	}
	return(g_PingTooltip); // RA2: as above
}

int __fastcall Slot_From_Control_Id(int id) // RA2: 0x0074A5D0, not in TibSun: the player slot (0 to 7) of the control ids 0x754 to 0x75B of the game lobby, -1 for any other; no caller (expanded where it is used)
{
	if (id == 0x754) { // RA2: as above
		return(0); // RA2: as above
	}
	if (id == 0x755) { // RA2: as above
		return(1); // RA2: as above
	}
	if (id == 0x756) { // RA2: as above
		return(2); // RA2: as above
	}
	if (id == 0x757) { // RA2: as above
		return(3); // RA2: as above
	}
	if (id == 0x758) { // RA2: as above
		return(4); // RA2: as above
	}
	if (id == 0x759) { // RA2: as above
		return(5); // RA2: as above
	}
	if (id == 0x75A) { // RA2: as above
		return(6); // RA2: as above
	}
	return(id == 0x75B ? 7 : -1); // RA2: as above
}

void __fastcall Enforce_Tournament_Options(HWND dialog) // RA2: 0x0074A640, not in TibSun: in a tournament channel the game options dialog fixes the settings: no computer players or alliances, bases and a short game on, and the controls of them disabled
{
	if (Is_Channel_Tournament(&g_CurrentChannel)) { // RA2: expanded here
		EnableWindow(GetDlgItem(dialog, 0x552), FALSE); // RA2: as above
		EnableWindow(GetDlgItem(dialog, 0x553), FALSE); // RA2: as above
		EnableWindow(GetDlgItem(dialog, 0x597), FALSE); // RA2: as above
		Session.Options.AIPlayers = 0; // RA2: as above
		Session.Options.AlliesAllowed = false; // RA2: as above
		HWND first = GetDlgItem(dialog, 0x54E); // RA2: as above
		SendMessage(first, BM_SETCHECK, 1, 0); // RA2: as above
		EnableWindow(first, FALSE); // RA2: as above
		HWND second = GetDlgItem(dialog, 0x54B); // RA2: as above
		SendMessage(second, BM_SETCHECK, 1, 0); // RA2: as above
		EnableWindow(second, FALSE); // RA2: as above
		Session.Options.ShortGame = true; // RA2: as above
		Session.Options.Bases = true; // RA2: as above
	}
}

void __fastcall Leave_Channel_With_Message(HWND dialog) // RA2: 0x0074C5F0, not in TibSun: the joiner has no slot in the game any more: tells the player (the channel is full, or the observer slot is) and leaves the channel; the dialog is closed when leaving fails
{
	if (dialog) { // RA2: as above
		if (ObserverMode == 0) { // RA2: as above
			ODMessageBox(StringTable::LoadString("TXT_CHANNEL_FULL", NULL, "D:\\Ra2\\wonline.cpp", 15375), 0, Wait_Callback_Thunk, false); // RA2: as above
		} else {
			ODMessageBox(StringTable::LoadString("TXT_OBSERVER_FULL", NULL, "D:\\Ra2\\wonline.cpp", 15377), 0, Wait_Callback_Thunk, false); // RA2: as above
		}
		if (g_pChat->RequestChannelLeave() < 0) { // RA2: virtual 0x24
			WS_Destroy_Dialog(dialog, 2); // RA2: as above
		}
	}
}

LRESULT CALLBACK Wonline_Wait_Dialog_Proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam); // RA2: 0x0074D5E0, the dialog proc of the wait dialog (0xC4)
#define OD_SETLABEL (WM_USER + 178) // RA2: 0x4B2, sets the wide text of the control (name guessed from its use)

void Show_Wait_Window(unsigned int event, WideWstring const & text, bool block) // RA2: 0x0074D4D0, not in TibSun: opens the wait dialog (0xC4) with the text, or when it is open adds the events it waits for and changes its text; block waits until it closes
{
	HWND dialog = WS_Find_Dialog(0xC4); // RA2: as above
	if (dialog) { // RA2: the dialog is open
		g_ActiveWaitFlags |= event; // RA2: as above
		if (text.length() > 0) { // RA2: as above
			HWND label = GetDlgItem(dialog, 0x5B2); // RA2: as above
			SendMessage(label, OD_SETLABEL, 0, (LPARAM)text.text()); // RA2: as above
		}
	} else if (event) { // RA2: as above
		HWND focus = WS_Top_Window(); // RA2: the control that has the focus goes back to it
		focus = GetDlgItem(focus, 0x540); // RA2: as above
		dialog = WS_Create_Dialog(ProgramInstance, 0xC4, MainWindow, (DLGPROC)Wonline_Wait_Dialog_Proc, 0); // RA2: as above
		Show_Dialog(dialog, 0xC4); // RA2: as above
		HWND label = GetDlgItem(dialog, 0x5B2); // RA2: as above
		SendMessage(label, OD_SETLABEL, 0, (LPARAM)text.text()); // RA2: as above
		g_ActiveWaitFlags = event; // RA2: as above
		if (focus) { // RA2: as above
			SetFocus(focus); // RA2: as above
		}
	}
	if (block && g_ActiveWaitFlags) { // RA2: as above
		WS_Wait_Dialog(dialog, Wait_Callback_Thunk, false, true, false); // RA2: as above
	}
}

void __fastcall Dialog_On_WM_PAINT(HWND dialog); // RA2: 0x005FEE70, before a dialog paints, defined in ownrdraw.cpp
void __fastcall Dialog_On_WM_DRAWITEM(long item); // RA2: 0x005FE380
namespace OwnerDraw { void __fastcall Set_Dialog_Waiting(HWND control); } // RA2: 0x005E6390, defined in ownrdraw.cpp
extern int g_WaitDialogDelay; // RA2: 0x007FD384, 7000 milliseconds: how long the wait dialog stays hidden

LRESULT CALLBACK Wonline_Wait_Dialog_Proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) // RA2: 0x0074D5E0, not in TibSun: the dialog proc of the wait dialog (0xC4): the abort button sets the first wait event, the other opens the network status page; the dialog shows itself after a delay
{
	switch (message) { // RA2: as above
	case WM_DESTROY: // RA2: as above
		KillTimer(window, 0x45); // RA2: as above
		break; // RA2: as above
	case WM_DRAWITEM: // RA2: as above
		Dialog_On_WM_DRAWITEM(lparam); // RA2: as above
		return(1); // RA2: as above
	case WM_PAINT: // RA2: as above
		Dialog_On_WM_PAINT(window); // RA2: as above
		Slide_Pending_Dialog(window); // RA2: as above
		ValidateRect(window, NULL); // RA2: as above
		break; // RA2: as above
	case WM_ERASEBKGND: // RA2: as above
		return(1); // RA2: as above
	case WM_INITDIALOG: // RA2: as above
		return(0); // RA2: as above
	case WM_COMMAND: // RA2: as above
		switch ((unsigned short)wparam) { // RA2: as above
		case 0x58D: // the abort button // RA2: as above
			SetEvent(g_WaitEventHandles[0]); // RA2: as above
			WS_Destroy_Dialog(window, 1); // RA2: as above
			break; // RA2: as above
		case 0x58E: // the status button // RA2: as above
			ViewHTML("http://networkstatus.westwood.com", 0); // RA2: as above
			break; // RA2: as above
		}
		break; // RA2: as above
	case WM_TIMER: // RA2: as above
		if (wparam == 0x45) { // RA2: the delay is over: the dialog shows
			ShowWindow(window, SW_SHOW); // RA2: as above
			KillTimer(window, 0x45); // RA2: as above
		}
		break; // RA2: as above
	case 0x497: // RA2: the dialog was made
		SendMessage(window, 0x4A9, 0, 1); // RA2: as above
		SetTimer(window, 0x45, g_WaitDialogDelay, NULL); // RA2: as above
		OwnerDraw::Set_Dialog_Waiting(window); // RA2: as above
		break; // RA2: as above
	}
	return(0); // RA2: as above
}

void Set_Wait_Window_Text(LPARAM text) // RA2: 0x0074D750, not in TibSun: changes the wide text of the wait dialog when it is open; no caller (expanded where it is used)
{
	HWND dialog = WS_Find_Dialog(0xC4); // RA2: as above
	if (dialog != NULL) { // RA2: as above
		SendDlgItemMessage(dialog, 0x5B2, OD_SETLABEL, 0, text); // RA2: as above
	}
}

void Close_Wait_Window(unsigned int event) // RA2: 0x0074D780, not in TibSun: takes the events out of what the wait dialog (0xC4) waits for, closes it when none is left, and gives the focus back to the chat line of the top dialog
{
	HWND win = WS_Find_Dialog(196); // RA2: as above
	if (win) { // RA2: as above
		g_ActiveWaitFlags &= ~event; // RA2: as above
		if (g_ActiveWaitFlags == 0) { // RA2: as above
			WS_Destroy_Dialog(win, 0); // RA2: as above
		}
	}
	HWND top = WS_Top_Window(); // RA2: as above
	HWND control = GetDlgItem(top, 1344); // RA2: as above
	if (control) { // RA2: as above
		SetFocus(control); // RA2: as above
	}
}

extern int PlayerPings[8]; // RA2: 0x00B2A734, as in mpcoop.cpp
NodeNameType * __fastcall Find_Lobby_Name(wchar_t const * name); // RA2: 0x005C8B20, defined in netshare.cpp
#include "wsproto.h" // RA2: PacketTransport

void __fastcall User_List_To_Session_Players(User * user) // RA2: 0x0074D7D0, not in TibSun (the executable's debug text names it): copies what the chat knows about the users into the session: the squad of each player, the address of the channel owner (the host), the pings, and the broadcast addresses of the players; the argument is not used
{
	Session.NumPlayers = 1; // RA2: the local player is slot 0
	User entry; // RA2: a copy of each user of the channel
	for (int i = 0; i < g_UserList.length(); i++) { // RA2: as above
		g_UserList.get(entry, i); // RA2: as above
		NodeNameType * player = Find_Lobby_Name(Narrow_To_Wide((char const *)entry.name)); // RA2: as above
		if (player && Is_Real_Player_Entry(player)) { // RA2: as above
			player->Player.SquadID = entry.squadID; // RA2: as above
		}
		if (stricmp((char const *)entry.name, g_NickName.get())) { // RA2: the others
			if (entry.flags & 1) { // RA2: the owner of the channel hosts the game
				NodeNameType * owner = Find_Lobby_Name(Narrow_To_Wide((char const *)entry.name)); // RA2: as above
				NetNumType network; // RA2: as above
				NetNodeType node; // RA2: as above
				owner->Address.Get_Address(network, node); // RA2: as above
				Session.HostAddress.Set_Address(network, node, 0); // RA2: as above
			}
			int ping = 0; // RA2: as above
			g_pNetUtil->GetAvgPing(entry.ipaddr, &ping); // RA2: virtual 0x20
			if (ping < 0) { // RA2: as above
				ping = 0; // RA2: as above
			}
			PlayerPings[Session.NumPlayers++] = ping; // RA2: as above
		}
	}
	PacketTransport->Clear_Broadcast_Addresses(); // RA2: as above
	for (int n = 1; n < Session.Players.Count(); n++) { // RA2: as above
		DebugString("Adding broadcast address %s\n", Session.Players[n]->Address.As_String()); // RA2: as above
		PacketTransport->Set_Broadcast_Address((char const *)&Session.Players[n]->Address, 0); // RA2: as above
	}
	DebugString("User_List_To_Session_Players: Session.HostAddress = %s\n", Session.HostAddress.As_String()); // RA2: as above
}

extern User g_OwnerUser; // RA2: 0x00B2A658, the copy of the channel owner's user record Find_Owner_User returns
extern int g_PlayerCheckBoxIDs[8]; // RA2: 0x007FD334, the ids of the check boxes of the players in the readiness dialog (0x12C)
extern bool g_PlayerChecked[8]; // RA2: 0x00B28DCC, which of them were ticked
bool User_Is_Channel_Owner(Wstring const & name); // RA2: 0x0074D9E0, defined below
User * Find_Owner_User(void); // RA2: 0x0074DA60, defined below

bool __fastcall Local_User_Is_Channel_Owner(void) // RA2: 0x0074D960, not in TibSun: the local user is the owner of the channel (WOL)
{
	return(User_Is_Channel_Owner(g_NickName)); // RA2: expanded here
}

bool User_Is_Channel_Owner(Wstring const & name) // RA2: 0x0074D9E0, not in TibSun: the channel owner (the user with flag 1) has this name; no caller (expanded where it is used)
{
	User * owner = Find_Owner_User(); // RA2: expanded here
	if (owner == NULL) { // RA2: as above
		return(false); // RA2: as above
	}
	bool same = stricmp((char *)owner->name, name.text()) == 0; // RA2: as above
	return(same); // RA2: a bool variable gives the executable's sete
}

User * Find_Owner_User(void) // RA2: 0x0074DA60, not in TibSun: the user of the channel with the owner flag (1), a copy kept in a global; NULL when there is none
{
	for (int i = 0; i < g_UserList.length(); i++) { // RA2: as above
		g_UserList.get(g_OwnerUser, i); // RA2: as above
		if (g_OwnerUser.flags & 1) { // RA2: as above
			return(&g_OwnerUser); // RA2: as above
		}
	}
	return(NULL); // RA2: as above
}

void __fastcall Mark_Player_Checked(int index) // RA2: 0x0074DAB0, not in TibSun: ticks the check box of a player in the readiness dialog (0x12C) and has the dialog redrawn
{
	if (index >= 0 && index < Session.Players.Count()) { // RA2: as above
		HWND dialog = WS_Find_Dialog(0x12C); // RA2: as above
		if (dialog) { // RA2: as above
			HWND box = GetDlgItem(dialog, g_PlayerCheckBoxIDs[index]); // RA2: as above
			g_PlayerChecked[index] = true; // RA2: as above
			SendMessage(box, BM_SETCHECK, 1, 0); // RA2: 0xF1
			PostMessage(dialog, WM_PAINT, 0, 0); // RA2: 0xF
		}
	}
}

LRESULT CALLBACK Player_Readiness_Dialog_Proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam); // RA2: 0x0074DB40, the dialog proc of the readiness dialog (0x12C)

void Open_Readiness_Dialog(void) // RA2: 0x0074DB10, not in TibSun: opens the dialog (0x12C) that shows which players are ready; no caller (expanded where it is used)
{
	HWND dialog = WS_Create_Dialog(ProgramInstance, 300, MainWindow, (DLGPROC)Player_Readiness_Dialog_Proc, 0); // RA2: as above
	if (dialog != NULL) { // RA2: as above
		Show_Dialog(dialog, 300); // RA2: as above
	}
}

extern bool g_PlayerReady[8]; // RA2: 0x00B28DC4, the check boxes the program has ticked (read by the readiness dialog proc)

LRESULT CALLBACK Player_Readiness_Dialog_Proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) // RA2: 0x0074DB40, not in TibSun: the dialog proc of the readiness dialog (0x12C): a check box for each player of the game, set by Mark_Player_Checked; the player cannot change them
{
	switch (message) { // RA2: as above
	case WM_PAINT: // RA2: as above
		Dialog_On_WM_PAINT(window); // RA2: as above
		ValidateRect(window, NULL); // RA2: as above
		return(0); // RA2: as above
	case WM_ERASEBKGND: // RA2: as above
		return(1); // RA2: as above
	case WM_DRAWITEM: // RA2: as above
		Dialog_On_WM_DRAWITEM(lparam); // RA2: as above
		return(1); // RA2: as above
	case WM_COMMAND: // RA2: as above
		wparam &= 0xFFFF; // RA2: as above
		switch (wparam) { // RA2: as above
		case 0x761: case 0x762: case 0x764: case 0x765: // RA2: as above
		case 0x766: case 0x767: case 0x768: case 0x769: // RA2: the check boxes of the players
			if (g_PlayerReady[wparam - 0x761]) { // RA2: ticked by the program: it stays ticked
				if (SendDlgItemMessage(window, wparam, BM_GETCHECK, 0, 0) == 0) { // RA2: as above
					SendDlgItemMessage(window, wparam, BM_SETCHECK, 1, 0); // RA2: as above
				}
			} else {
				if (SendDlgItemMessage(window, wparam, BM_GETCHECK, 0, 0) == 1) { // RA2: as above
					SendDlgItemMessage(window, wparam, BM_SETCHECK, 0, 0); // RA2: as above
					return(0); // RA2: as above
				}
			}
		}
		break; // RA2: as above
	case WM_CTLCOLORMSGBOX: case WM_CTLCOLOREDIT: case WM_CTLCOLORLISTBOX: case WM_CTLCOLORBTN: // RA2: as above
	case WM_CTLCOLORDLG: case WM_CTLCOLORSCROLLBAR: case WM_CTLCOLORSTATIC: // RA2: as above
		return((long)GetStockObject(BLACK_BRUSH)); // RA2: the controls are painted on black
	case 0x497: // RA2: the dialog was made
		for (int i = 0; i < 8; ++i) { // RA2: as above
			HWND control = GetDlgItem(window, g_PlayerCheckBoxIDs[i]); // RA2: as above
			memset(g_PlayerChecked, 0, 8); // RA2: as above
			if (i > Session.Players.Count() - 1) { // RA2: no player: the check box goes
				DestroyWindow(control); // RA2: as above
			}
		}
		for (int player = 0; player < Session.Players.Count(); ++player) { // RA2: as above
			HWND control = GetDlgItem(window, g_PlayerCheckBoxIDs[player]); // RA2: as above
			SendMessage(control, 0x4B2, 0, (LPARAM)Session.Players[player]->Name); // RA2: the name of the player
			SendMessage(control, BM_SETCHECK, 0, 0); // RA2: as above
		}
		SendMessage(window, 0x4A9, 0, 1); // RA2: as above
		OwnerDraw::Set_Dialog_Waiting(window); // RA2: as above
		ShowWindow(window, SW_SHOW); // RA2: as above
		break; // RA2: as above
	}
	return(0); // RA2: as above
}

extern int g_ConnectionStatus[8]; // RA2: 0x00B26A2C, how the connection of each player of the game ended (0 reported, 1 local, 2 pserver port, 3 host aborted)
extern int g_ConnectionStatusRank[4]; // RA2: 0x007FD324, 1, 2, 0, 1: the statuses a connection state may replace

void __fastcall Set_Connection_Status(int index, int status) // RA2: 0x0074DD20, not in TibSun: records how the connection of a player ended, unless the status it has is already worse; no caller (expanded where it is used)
{
	if (index < Session.Players.Count() && g_ConnectionStatus[index] < g_ConnectionStatusRank[index]) { // RA2: as above
		g_ConnectionStatus[index] = status; // RA2: as above
	}
}

void Report_Connection_Errors(void) // RA2: 0x0074DD50, not in TibSun: prints into the lobby chat what went wrong with the connections of the players: the status each player's connection ended with, the channel owner being the host; a general text when none is known
{
	wchar_t message[256]; // RA2: as above
	User * owner = Find_Owner_User(); // RA2: expanded here
	if (owner) { // RA2: as above
		int owner_index = -1; // RA2: as above
		int i; // RA2: as above
		for (i = 0; i < Session.Players.Count(); i++) { // RA2: as above
			if (!stricmp(Wide_To_Narrow(Session.Players[i]->Name), (char const *)owner->name)) { // RA2: as above
				owner_index = i; // RA2: as above
				break; // RA2: as above
			}
		}
		bool reported = false; // RA2: as above
		for (i = 0; i < Session.Players.Count(); i++) { // RA2: as above
			switch (g_ConnectionStatus[i]) { // RA2: as above
			case 0: // RA2: as above
				reported = true; // RA2: as above
				DebugString("Player %s was unable to establish a connection\n", Wide_To_Narrow(Session.Players[i]->Name)); // RA2: as above
				swprintf(message, StringTable::LoadString("TXT_TALK_REPORTED_PROBLEM", NULL, "D:\\Ra2\\wonline.cpp", 16894), Session.Players[i]->Name); // RA2: as above
				Lobby_Chat_Printf(ColorSystem, message); // RA2: as above
				break; // RA2: as above
			case 1: // RA2: as above
				reported = true; // RA2: as above
				DebugString("Unable to connect to player %s\n", Wide_To_Narrow(Session.Players[i]->Name)); // RA2: as above
				swprintf(message, StringTable::LoadString("TXT_TALK_LOCAL_PROBLEM", NULL, "D:\\Ra2\\wonline.cpp", 16904), Session.Players[i]->Name); // RA2: as above
				Lobby_Chat_Printf(ColorSystem, message); // RA2: as above
				break; // RA2: as above
			case 2: // RA2: as above
				reported = true; // RA2: as above
				if (i == owner_index) { // RA2: the host could not be reached
					DebugString("Unable to talk to the pserver port\n"); // RA2: as above
					Lobby_Chat_Printf(ColorSystem, StringTable::LoadString("TXT_TALK_FORWARDER_PROBLEM_GUES", NULL, "D:\\Ra2\\wonline.cpp", 16915)); // RA2: as above
				} else {
					DebugString("Player %s was unable to connect to the pserver port\n", Wide_To_Narrow(Session.Players[i]->Name)); // RA2: as above
					swprintf(message, StringTable::LoadString("TXT_TALK_FORWARDER_PROBLEM_HOST", NULL, "D:\\Ra2\\wonline.cpp", 16918), Session.Players[i]->Name); // RA2: as above
					Lobby_Chat_Printf(ColorSystem, message); // RA2: as above
				}
				break; // RA2: as above
			case 3: // RA2: as above
				reported = true; // RA2: as above
				DebugString("The game host reported a connection problem between players\n"); // RA2: as above
				Lobby_Chat_Printf(ColorSystem, StringTable::LoadString("TXT_TALK_HOST_ABORTED", NULL, "D:\\Ra2\\wonline.cpp", 16929)); // RA2: as above
				break; // RA2: as above
			}
		}
		if (!reported) { // RA2: as above
			Lobby_Chat_Printf(ColorSystem, StringTable::LoadString("TXT_FAILED_TO_TALK", NULL, "D:\\Ra2\\wonline.cpp", 16942)); // RA2: as above
		}
	}
}

unsigned short __fastcall Get_Port_Blitz_Port_Number(int send_port, int mangled_port, int unused, int port_spread) // RA2: 0x0074DFD0, not in TibSun (the executable's debug text names it so): the port to send to for a peer whose router allocates ports by this spread (1 consecutive, 2 every second, a larger one in pairs, a negative one downwards), never below 1024; no caller (expanded where it is used)
{
	int port = send_port + mangled_port; // RA2: as above
	if (port_spread != 1) { // RA2: as above
		if (port_spread == 2) { // RA2: as above
			port = mangled_port + send_port * 2; // RA2: as above
		} else if (port_spread > 1) { // RA2: as above
			port = send_port % 2 + mangled_port + (send_port / 2) * port_spread; // RA2: as above
		} else if (port_spread < 0) { // RA2: as above
			if (port_spread == -100) { // RA2: as above
				port = mangled_port + send_port * 100; // RA2: as above
			} else {
				port = send_port % 2 - (send_port / 2) * port_spread + mangled_port; // RA2: as above
			}
		}
	}
	DebugString("Get_Port_Blitz_Port_Number - send port = %d, mangled_port = %d, port_spread = %d, port = %d\n", send_port, mangled_port, port_spread, port); // RA2: as above
	if ((unsigned short)port < 1024) { // RA2: as above
		port = 1024; // RA2: as above
	}
	return((unsigned short)port); // RA2: as above
}

#include "firewall.h" // RA2: FirewallHelper.Rank_A
extern FirewallHelperClass FirewallHelper; // RA2: 0x00A71B18, as in init.cpp

int __fastcall Build_Connection_Pairs(int (* pairs)[2]) // RA2: 0x0074E070, not in TibSun: orders the players of the game by how awkward their connections are (FirewallHelper.Rank_A of their state) and writes every pair of them into the table; the number of pairs
{
	int order[8]; // RA2: the slots of the players
	memset(order, 0, sizeof(order)); // RA2: as above
	int i = 0; // RA2: as above
	int * out = order; // RA2: as above
	for (; i < 8; i++) { // RA2: the slots that hold real players
		if (Is_Real_Player_Entry(Session.Slots[i])) { // RA2: as above
			*out++ = i;
		}
	}
	for (int a = 0; a < Session.Players.Count(); a++) { // RA2: sorted, the most awkward first
		for (int b = 0; b < Session.Players.Count() - 1; b++) { // RA2: as above
			int current = Session.WolPlayerState[order[b]]; // RA2: as above
			int next = Session.WolPlayerState[order[b + 1]]; // RA2: as above
			if (FirewallHelper.Rank_A(current) < FirewallHelper.Rank_A(next)) { // RA2: as above
				int temp = order[b]; // RA2: as above
				order[b] = order[b + 1]; // RA2: as above
				order[b + 1] = temp; // RA2: as above
			}
		}
	}
	DebugString("Connection order = %d, %d, %d, %d, %d, %d\n", order[0], order[1], order[2], order[3], order[4], order[5]); // RA2: as above
	int n = 0; // RA2: as above
	DebugString("Connection pairs....\n"); // RA2: as above
	for (int x = 0; x < Session.Players.Count(); x++) { // RA2: as above
		for (int y = x + 1; y < Session.Players.Count(); y++) { // RA2: as above
			pairs[n][0] = order[x]; // RA2: as above
			pairs[n][1] = order[y]; // RA2: as above
			DebugString("\t%d & %d\n", order[x], order[y]); // RA2: as above
			n++; // RA2: as above
		}
	}
	return(n); // RA2: as above
}

void __fastcall Register_Connection_State(char const * sender, char const * receiver, int state) // RA2: 0x0074E1F0, not in TibSun: a player reported the state of its connection to another (both by name): stored for the pair of slots in either direction
{
	int sender_slot = -1; // RA2: as above
	int receiver_slot = -1; // RA2: as above
	DebugString("Registering connection state from %s to %s. conn_state = %d\n", sender, receiver, state); // RA2: as above
	int i = 0; // RA2: as above
	for (NodeNameType ** slot = Session.Slots; (long)slot < (long)(Session.Slots + 8); ++slot, ++i) { // RA2: the slots of the game
		if (stricmp(Wide_To_Narrow((*slot)->Name), sender) == 0) { // RA2: as above
			sender_slot = i; // RA2: as above
		}
		if (stricmp(Wide_To_Narrow((*slot)->Name), receiver) == 0) { // RA2: as above
			receiver_slot = i; // RA2: as above
		}
	}
	for (i = 0; i < Session.ConnectionResultCount; ++i) { // RA2: the pair is looked up
		if (Session.PeerProbeData[i][0] == sender_slot && Session.PeerProbeData[i][1] == receiver_slot) { // RA2: as above
			Session.ConnectionResults[i][0] = state; // RA2: as above
			return; // RA2: as above
		}
		if (Session.PeerProbeData[i][0] == receiver_slot && Session.PeerProbeData[i][1] == sender_slot) { // RA2: as above
			Session.ConnectionResults[i][1] = state; // RA2: as above
			return; // RA2: as above
		}
	}
}

void __fastcall Store_Player_Port(char const * name, unsigned short port) // RA2: 0x0074E2D0, not in TibSun: a player says which port to send to: kept for its slot
{
	DebugString("Player %s is telling us we should send to port %d\n", name, port); // RA2: as above
	for (int i = 0; i < 8; ++i) { // RA2: as above
		if (!stricmp(Wide_To_Narrow(Session.Slots[i]->Name), name)) { // RA2: as above
			Session.PeerPorts[i] = port; // RA2: as above
			return; // RA2: as above
		}
	}
}

void __fastcall Send_User_Port(char const * name, unsigned short port) // RA2: 0x0074E330, not in TibSun: tells the user of this name by private game options string ("Z4" and the port) which port to send to
{
	User * user = NULL; // RA2: as above
	char text[256]; // RA2: as above
	for (int i = 0; i < g_UserList.length(); ++i) { // RA2: as above
		g_UserList.getPointer(&user, i); // RA2: as above
		if (stricmp(name, (char const *)user->name) == 0) { // RA2: as above
			text[0] = 'Z'; // RA2: as above
			text[1] = '4'; // RA2: as above
			sprintf(text + 2, "%d", port); // RA2: as above
			DebugString("Sending string %s to user %s\n", text, name); // RA2: as above
			g_pChat->RequestPrivateGameOptions(user, text); // RA2: virtual 0x38
		}
	}
}

void __fastcall Send_Public_Options(char kind, char const * text) // RA2: 0x0074E3F0, not in TibSun: sends a public game options string "Z1", the kind and the text to the channel
{
	char options[256]; // RA2: as above
	options[0] = 'Z'; // RA2: as above
	options[1] = '1'; // RA2: as above
	options[2] = kind + '0'; // RA2: as above
	if (text) { // RA2: as above
		strcpy(options + 3, text); // RA2: as above
	} else {
		options[3] = 0; // RA2: as above
	}
	DebugString("Sending public options string %s\n", options); // RA2: as above
	g_pChat->RequestPublicGameOptions(options); // RA2: virtual 0x3C
}

void Resend_Pending_Requests(void) // RA2: 0x0074E470, not in TibSun: sends a keepalive (NET_PING) to each peer whose countdown ran out and restarts it
{
	for (int index = 0; index < Session.KeepAliveCount; ++index) { // RA2: as above
		if (Session.KeepAliveTimer[index] == 0) { // RA2: as above
			GlobalPacketType packet; // RA2: as above
			memset(&packet, 0, sizeof(packet)); // RA2: as above
			packet.Command = NET_PING; // RA2: as above
			strcpy(packet.Name, g_NickName.get()); // RA2: as above
			char const * name = Wide_To_Narrow(Session.Slots[Session.KeepAliveSlot[index]]->Name); // RA2: as above
			DebugString("Sending keepalive NET_PING packet to %s\n", name); // RA2: as above
			Ipx.Send_Global_Message_To_Name(&packet, 0x2F, 0, Session.Slots[Session.KeepAliveSlot[index]]->Name); // RA2: as above
			Session.KeepAliveTimer[index] = 1200; // RA2: 20 seconds
		}
	}
}

void __fastcall Queue_Peer_Timeout(int peer) // RA2: 0x0074E590, not in TibSun: queues a keepalive for this slot
{
	Session.KeepAliveSlot[Session.KeepAliveCount] = peer; // RA2: as above
	Session.KeepAliveTimer[Session.KeepAliveCount] = 1200; // RA2: as above
	++Session.KeepAliveCount; // RA2: as above
}

bool __fastcall Start_Talking_To(int player, int pair_slot, int retries); // RA2: 0x0074E5E0, the executable's debug text "Start_Talking_To player %d, retries = %d" names it: makes the connection to one peer by port probes, tries again up to the number of retries; true when it is made
struct SessionPortAllocator : public SessionClass { // RA2: the file-local subclass of session.cpp that holds Allocate_Player_Ports (0x00673090); no data
	void Allocate_Player_Ports(void); // RA2: as above
};

bool Connect_Player_Pairs(void) // RA2: 0x0074EF80, not in TibSun: makes the connections of the pairs of players Build_Connection_Pairs arranged: talks to the peer of each pair the local player is in (Start_Talking_To), waits for the others to connect among themselves; false when one of them fails
{
	CDTimerClass<SystemTimerClass> timer; // RA2: as above
	Session.KeepAliveCount = 0; // RA2: as above
	memset(&Session.PeerAttemptBlock[3], 0, 0x20); // RA2: the eight values at +0x2EF4 are cleared
	int pair_slot = -1; // RA2: as above
	g_SuspendChatPump = 0; // RA2: the chat is pumped again
	if (Session.ConnectionResultCount == 0) { // RA2: no pair to make
		return(false); // RA2: as above
	}
	((SessionPortAllocator &)Session).Allocate_Player_Ports(); // RA2: as above
	if (Session.Players.Count() > 0) { // RA2: the first check box of the readiness dialog (the local player) is ticked
		HWND window = WS_Find_Dialog(300); // RA2: as above
		if (window) { // RA2: as above
			HWND control = GetDlgItem(window, g_PlayerCheckBoxIDs[0]); // RA2: as above
			g_PlayerChecked[0] = true; // RA2: as above
			SendMessage(control, BM_SETCHECK, 1, 0); // RA2: as above
			PostMessage(window, WM_PAINT, 0, 0); // RA2: as above
		}
	}
	for (int pair = 0; pair < Session.ConnectionResultCount; ++pair) { // RA2: as above
		int first = Session.PeerProbeData[pair][0]; // RA2: the slots of the pair
		int second = Session.PeerProbeData[pair][1]; // RA2: as above
		int peer = -1; // RA2: as above
		NodeNameType * first_player = Session.Slots[first]; // RA2: as above
		NodeNameType * second_player = Session.Slots[second]; // RA2: as above
		char const * first_name = Wide_To_Narrow(first_player->Name); // RA2: as above
		char const * second_name = Wide_To_Narrow(second_player->Name); // RA2: as above
		DebugString("Connecting pair %d, %s and %s\n", pair, first_name, second_name); // RA2: as above
		char const * name = Wide_To_Narrow(Session.Slots[first]->Name); // RA2: as above
		if (stricmp(name, g_NickName.get()) == 0) { // RA2: the local player is the first of the pair
			peer = second; // RA2: as above
			pair_slot = pair * 2 + 1; // RA2: as above
			Session.ConnectionResults[pair][0] = 11; // RA2: as above
		}
		name = Wide_To_Narrow(Session.Slots[second]->Name); // RA2: as above
		if (stricmp(name, g_NickName.get()) == 0) { // RA2: or the second
			peer = first; // RA2: as above
			pair_slot = pair * 2; // RA2: as above
			Session.ConnectionResults[pair][1] = 11; // RA2: as above
		}
		if (peer != -1) { // RA2: the local player is in the pair
			int step; // RA2: the harder of the two connections decides how many retries
			if (FirewallHelper.Rank_B(Session.WolPlayerState[first]) > FirewallHelper.Rank_B(Session.WolPlayerState[second])) { // RA2: as above
				step = FirewallHelper.Rank_B(Session.WolPlayerState[first]); // RA2: as above
			} else {
				step = FirewallHelper.Rank_B(Session.WolPlayerState[second]); // RA2: as above
			}
			if (!Start_Talking_To(peer, pair_slot, step)) { // RA2: as above
				return(false); // RA2: as above
			}
		} else { // RA2: the others connect among themselves: wait for both results
			DebugString("Waiting for other guys to connect\n"); // RA2: as above
			timer = CDTimerClass<SystemTimerClass>(960); // RA2: 16 seconds
			int progress = 0; // RA2: as above
			do { // RA2: as above
				Sleep(5); // RA2: as above
				WOL_Wait_Callback(); // RA2: as above
				Resend_Pending_Requests(); // RA2: as above
				int first_status = Session.ConnectionResults[pair][0]; // RA2: as above
				int second_status = Session.ConnectionResults[pair][1]; // RA2: as above
				if ((first_status == 11 || first_status == 0) && (second_status == 11 || second_status == 0)) { // RA2: as above
					DebugString("Got both results - woohoo!\n"); // RA2: as above
					break; // RA2: as above
				}
				if (first_status >= 1 && first_status < 11 && first_status - 1 > progress) { // RA2: the first one tries again
					DebugString("Connecting player 1 is trying again - try_now = %d\n", first_status - 1); // RA2: as above
					progress = first_status - 1; // RA2: as above
					timer = CDTimerClass<SystemTimerClass>(840); // RA2: 14 seconds more
				}
				if (second_status >= 1 && second_status < 11 && second_status - 1 > progress) { // RA2: the second one
					DebugString("Connecting player 2 is trying again - try_now = %d\n", second_status - 1); // RA2: as above
					progress = second_status - 1; // RA2: as above
					timer = CDTimerClass<SystemTimerClass>(840); // RA2: as above
				}
			} while (timer.Value() != 0); // RA2: as above
		}
		if (Session.ConnectionResults[pair][0] != 11) { // RA2: as above
			return(false); // RA2: as above
		}
		if (Session.ConnectionResults[pair][1] != 11) { // RA2: as above
			return(false); // RA2: as above
		}
		DebugString("Connection good, next pair\n"); // RA2: as above
	}
	return(true); // RA2: as above
}

extern bool g_ReceivingHostPreview; // RA2: 0x00B2AC46, set while Receive_Host_Map_Preview takes the map preview from the host
namespace OwnerDraw { void __fastcall WideToCharString(char * dest, wchar_t const * src, int count); } // RA2: 0x006FA0F0, as in conquer.cpp

bool Receive_Host_Map_Preview(void) // RA2: 0x00750D10, not in TibSun: a guest of a game channel takes the map preview from the host when the host's NET_PREVIEW_MODE packet is waiting in the global channel; true when the preview was received
{
	IPXAddressClass address; // RA2: as above
	if (g_CurrentChannel.type && g_UserList.length() >= 2) { // RA2: in a channel with at least one other user
		User * owner = Find_Owner_User(); // RA2: expanded here
		if (owner && !stricmp((char const *)owner->name, g_NickName.text())) { // RA2: the host does not receive its own preview
			return(false); // RA2: as above
		}
		if (WolOptionsLocked == 0 && Ipx.Global_Num_Receive()) { // RA2: a packet waits
			Call_Back(); // RA2: as above
			GlobalPacketType packet; // RA2: as above
			int length = sizeof(packet); // RA2: as above
			unsigned short product; // RA2: as above
			if (Ipx.Peek_Global_Message(0, &packet, &length, &address, &product, NULL) && packet.Command == NET_PREVIEW_MODE) { // RA2: as above
				HWND dialog = WS_Find_Dialog(0xC9); // RA2: the game lobby dialog shows its preview control
				if (dialog) { // RA2: as above
					ShowWindow(GetDlgItem(dialog, 0x468), SW_SHOWNORMAL); // RA2: as above
				}
				User user; // RA2: as above
				g_UserList.get(user, 0); // RA2: as above
				user.next = NULL; // RA2: as above
				User_List_To_Session_Players(&user); // RA2: the session addresses first
				if (Ipx.Get_Global_Message(&packet, &length, &address, &product) && packet.Command == NET_PREVIEW_MODE && address.Is_Same_Node(Session.HostAddress) && product == 0xAA03) { // RA2: from the host
					owner = Find_Owner_User(); // RA2: expanded here
					if (owner && !stricmp(packet.Name, (char const *)owner->name)) { // RA2: as above
						DebugString("Received NET_PREVIEW_MODE packet from host\n"); // RA2: as above
						Ipx.Set_Timing(50, (unsigned long)-1, 5000, true); // RA2: slow resends while the preview comes
						g_ReceivingHostPreview = true; // RA2: as above
						Receive_Random_Map_Preview(); // RA2: as above
						g_ReceivingHostPreview = false; // RA2: as above
						Ipx.Set_Timing(30, (unsigned long)-1, 600, true); // RA2: the usual timing again
						return(true); // RA2: as above
					}
				}
			}
		}
	}
	return(false); // RA2: as above
}

void Add_Test_Player_Ol_Cruddy(void) // RA2: 0x007513E0, not in TibSun: a test player "Ol Cruddy" at 10.2.81.82 is added to the session (house 0, color 3), as a stand-in for a real opponent; no caller
{
	NodeNameType * player = new NodeNameType; // RA2: as above
	Char_To_Wide(player->Name, "Ol Cruddy", -1); // RA2: as above
	player->Player.SquadID = -1; // RA2: as above
	player->Player.House = 0; // RA2: as above
	player->Player.Color = 3; // RA2: as above
	NetNumType network; // RA2: as above
	NetNodeType node; // RA2: as above
	memset(network, 0, sizeof(network)); // RA2: as above
	node[5] = 0; // RA2: the address 10.2.81.82
	node[4] = 0; // RA2: as above
	node[3] = 82; // RA2: as above
	node[2] = 81; // RA2: as above
	node[1] = 2; // RA2: as above
	node[0] = 10; // RA2: as above
	player->Address.Set_Address(network, node, 0); // RA2: as above
	Session.Players.Add(player); // RA2: as above
	PlayerPings[Session.NumPlayers] = 100; // RA2: as above
	++Session.NumPlayers; // RA2: as above
}

void Add_Test_Player_Potato(void) // RA2: 0x007514D0, not in TibSun: a test player "Potato" at 10.2.20.97 is added to the session (house 0, color 0); no caller
{
	NodeNameType * player = new NodeNameType; // RA2: as above
	Char_To_Wide(player->Name, "Potato", -1); // RA2: as above
	player->Player.SquadID = -1; // RA2: as above
	player->Player.House = 0; // RA2: as above
	player->Player.Color = 0; // RA2: as above
	NetNumType network; // RA2: as above
	NetNodeType node; // RA2: as above
	memset(network, 0, sizeof(network)); // RA2: as above
	node[5] = 0; // RA2: the address 10.2.20.97
	node[4] = 0; // RA2: as above
	node[3] = 97; // RA2: as above
	node[2] = 20; // RA2: as above
	node[1] = 2; // RA2: as above
	node[0] = 10; // RA2: as above
	player->Address.Set_Address(network, node, 0); // RA2: as above
	Session.Players.Add(player); // RA2: as above
	PlayerPings[Session.NumPlayers] = 100; // RA2: as above
	++Session.NumPlayers; // RA2: as above
}

void Add_Test_Player_Potato_Too(void) // RA2: 0x007515B0, not in TibSun: a test player "PotatoToo" at 10.2.80.250 is added to the session as the host (house 1, color 2); no caller
{
	NodeNameType * player = new NodeNameType; // RA2: as above
	Char_To_Wide(player->Name, "PotatoToo", -1); // RA2: as above
	player->Player.SquadID = -1; // RA2: as above
	player->Player.House = 1; // RA2: as above
	player->Player.Color = 2; // RA2: as above
	NetNumType network; // RA2: as above
	NetNodeType node; // RA2: as above
	memset(network, 0, sizeof(network)); // RA2: as above
	node[5] = 0; // RA2: the address 10.2.80.250
	node[4] = 0; // RA2: as above
	node[3] = 250; // RA2: as above
	node[2] = 80; // RA2: as above
	node[1] = 2; // RA2: as above
	node[0] = 10; // RA2: as above
	player->Address.Set_Address(network, node, 0); // RA2: as above
	Session.Players.Add(player); // RA2: as above
	PlayerPings[Session.NumPlayers] = 100; // RA2: as above
	++Session.NumPlayers; // RA2: as above
	Session.HostAddress = player->Address; // RA2: the host
}

void Reset_Wait_Events(void) // RA2: 0x00752930, not in TibSun: resets every wait event
{
	for (int i = 0; i < EV_COUNT; i++) { // RA2: as above
		ResetEvent(g_WaitEventHandles[i]); // RA2: as above
	}
}


#pragma auto_inline(off) // RA2: as above
template<> char Dictionary<Wstring, int>::add(Wstring const & key, int & value) // RA2: as above
{
	int offset; // RA2: as above
	DNode<Wstring, int> * node, * item, * temp; // RA2: as above
	float percent; // RA2: as above
	item = new DNode<Wstring, int>; // RA2: as above
	item->key = key; // RA2: as above
	item->value = value; // RA2: as above
	item->hashNext = NULL; // RA2: as above
	offset = keyHash(key); // RA2: as above
	node = table[offset]; // RA2: as above
	if (node == NULL) { table[offset] = item; } // RA2: as above
	else { temp = table[offset]; table[offset] = item; item->hashNext = temp; } // RA2: as above
	entries++; // RA2: as above
	percent = (float)entries; // RA2: as above
	percent /= (float)getSize(); // RA2: as above
	if (percent >= EXPAND_THRESHOLD) { expand(); } // RA2: as above
	return(TRUE); // RA2: as above
}
template<> char Dictionary<Wstring, int>::remove(Wstring const & key, int & value) // RA2: as above
{
	int offset; // RA2: as above
	DNode<Wstring, int> * node, * last, * temp; // RA2: as above
	float percent; // RA2: as above
	if (entries == 0) return(FALSE); // RA2: as above
	percent = (float)(entries - 1); // RA2: as above
	percent /= (float)getSize(); // RA2: as above
	offset = keyHash(key); // RA2: as above
	node = table[offset]; // RA2: as above
	last = node; // RA2: as above
	if (node == NULL) return(FALSE); // RA2: as above
	if ((node->key) == key) { // RA2: as above
		value = node->value; // RA2: as above
		temp = table[offset]->hashNext; // RA2: as above
		delete(table[offset]); // RA2: as above
		table[offset] = temp; // RA2: as above
		entries--; // RA2: as above
		if (percent <= SHRINK_THRESHOLD) shrink(); // RA2: as above
		return(TRUE); // RA2: as above
	}
	node = node->hashNext; // RA2: as above
	char retval = FALSE; // RA2: as above
	while (node != NULL) { // RA2: as above
		if (node->key == key) { // RA2: as above
			value = node->value; // RA2: as above
			last->hashNext = node->hashNext; // RA2: as above
			entries--; // RA2: as above
			delete(node); // RA2: as above
			retval = TRUE; // RA2: as above
			break; // RA2: as above
		}
		last = node; // RA2: as above
		node = node->hashNext; // RA2: as above
	}
	if (percent <= SHRINK_THRESHOLD) shrink(); // RA2: as above
	return(retval); // RA2: as above
}
#pragma auto_inline(on) // RA2: as above

SimpleWonlineDialogControl::~SimpleWonlineDialogControl(void) {} // RA2: 0x0073C020; defined in this file, so Login_Wonline (0x00742FD0) expands it as the executable does
