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


#pragma once

// RA2: only the part of the WOL API that netshare.cpp uses, in a flat header; the rest of TibSun's wolapi.h is not included

#include <windows.h>
#include <ole2.h>


typedef 
enum Locale
    {	LOC_UNKNOWN	= 0,
	LOC_OTHER	= LOC_UNKNOWN + 1,
	LOC_USA	= LOC_OTHER + 1,
	LOC_CANADA	= LOC_USA + 1,
	LOC_UK	= LOC_CANADA + 1,
	LOC_GERMANY	= LOC_UK + 1,
	LOC_FRANCE	= LOC_GERMANY + 1,
	LOC_SPAIN	= LOC_FRANCE + 1,
	LOC_NETHERLANDS	= LOC_SPAIN + 1,
	LOC_BELGIUM	= LOC_NETHERLANDS + 1,
	LOC_AUSTRIA	= LOC_BELGIUM + 1,
	LOC_SWITZERLAND	= LOC_AUSTRIA + 1,
	LOC_ITALY	= LOC_SWITZERLAND + 1,
	LOC_DENMARK	= LOC_ITALY + 1,
	LOC_SWEDEN	= LOC_DENMARK + 1,
	LOC_NORWAY	= LOC_SWEDEN + 1,
	LOC_FINLAND	= LOC_NORWAY + 1,
	LOC_ISRAEL	= LOC_FINLAND + 1,
	LOC_SOUTH_AFRICA	= LOC_ISRAEL + 1,
	LOC_JAPAN	= LOC_SOUTH_AFRICA + 1,
	LOC_SOUTH_KOREA	= LOC_JAPAN + 1,
	LOC_CHINA	= LOC_SOUTH_KOREA + 1,
	LOC_SINGAPORE	= LOC_CHINA + 1,
	LOC_TAIWAN	= LOC_SINGAPORE + 1,
	LOC_MALAYSIA	= LOC_TAIWAN + 1,
	LOC_AUSTRALIA	= LOC_MALAYSIA + 1,
	LOC_NEW_ZEALAND	= LOC_AUSTRALIA + 1,
	LOC_BRAZIL	= LOC_NEW_ZEALAND + 1,
	LOC_THAILAND	= LOC_BRAZIL + 1,
	LOC_ARGENTINA	= LOC_THAILAND + 1,
	LOC_PHILIPPINES	= LOC_ARGENTINA + 1,
	LOC_GREECE	= LOC_PHILIPPINES + 1,
	LOC_IRELAND	= LOC_GREECE + 1,
	LOC_POLAND	= LOC_IRELAND + 1,
	LOC_PORTUGAL	= LOC_POLAND + 1,
	LOC_MEXICO	= LOC_PORTUGAL + 1,
	LOC_RUSSIA	= LOC_MEXICO + 1,
	LOC_TURKEY	= LOC_RUSSIA + 1
    }	Locale;

struct  Ladder
    {
    unsigned int sku;
    unsigned int team_no;
    unsigned int wins;
    unsigned int losses;
    unsigned int points;
    unsigned int kills;
    unsigned int rank;
    unsigned int rung;
    unsigned int disconnects;
    unsigned int team_rung;
    unsigned int provisional;
    unsigned int last_game_date;
    unsigned int win_streak;
    unsigned int reserved1;
    unsigned int reserved2;
    struct Ladder *next;
    unsigned char login_name[ 40 ];
    Locale locale;
    };
typedef int GroupID;

struct  Server
    {
    int gametype;
    int chattype;
    int timezone;
    float longitude;
    float lattitude;
    struct Server *next;
    unsigned char name[ 71 ];
    unsigned char connlabel[ 5 ];
    unsigned char conndata[ 128 ];
    unsigned char login[ 10 ];
    unsigned char password[ 10 ];
    };
struct  Channel
    {
    int type;
    unsigned int minUsers;
    unsigned int maxUsers;
    unsigned int currentUsers;
    unsigned int official;
    unsigned int tournament;
    unsigned int ingame;
    unsigned int flags;
    unsigned long reserved;
    unsigned long ipaddr;
    int latency;
    int hidden;
    struct Channel *next;
    unsigned char name[ 17 ];
    unsigned char topic[ 81 ];
    unsigned char location[ 65 ];
    unsigned char key[ 9 ];
    unsigned char exInfo[ 41 ];
    };
struct  User
    {
    unsigned int flags;
    GroupID group;
    unsigned long reserved;
    unsigned long reserved2;
    unsigned long reserved3;
    unsigned long squadID;
    unsigned long ipaddr;
    unsigned long squad_icon;
    struct User *next;
    unsigned char name[ 10 ];
    unsigned char squadname[ 41 ];
    unsigned char squadabbrev[ 10 ];
    Locale locale;
    int team;
    };
struct  Group
    {
    GroupID ident;
    int type;
    unsigned int members;
    struct Group *next;
    unsigned char name[ 65 ];
    };
struct  Squad
    {
    unsigned long id;
    int sku;
    int members;
    int color1;
    int color2;
    int color3;
    int icon1;
    int icon2;
    int icon3;
    struct Squad *next;
    int rank;
    int team;
    int status;
    unsigned char email[ 81 ];
    unsigned char icq[ 17 ];
    unsigned char motto[ 81 ];
    unsigned char url[ 129 ];
    unsigned char name[ 41 ];
    unsigned char abbreviation[ 41 ];
    };
struct  Update
    {
    unsigned long SKU;
    unsigned long version;
    int required;
    struct Update *next;
    unsigned char server[ 65 ];
    unsigned char patchpath[ 256 ];
    unsigned char patchfile[ 33 ];
    unsigned char login[ 33 ];
    unsigned char password[ 65 ];
    unsigned char localpath[ 256 ];
    };
typedef struct Server Server;

typedef struct Channel Channel;

typedef struct User User;

typedef struct Group Group;

typedef struct Update Update;

typedef struct Ladder Ladder;

typedef struct Squad Squad;

    struct __declspec(uuid("4DD3BAF4-7579-11D1-B1C6-006097176556")) IChat : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE PumpMessages( void) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RequestServerList( 
            /* [in] */ unsigned long SKU,
            /* [in] */ unsigned long current_version,
            /* [in] */ LPCSTR loginname,
            /* [in] */ LPCSTR password,
            /* [in] */ int timeout) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RequestConnection( 
            /* [in] */ Server *server,
            /* [in] */ int timeout,
            int domangle) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RequestChannelList( 
            /* [in] */ int channelType,
            /* [in] */ int autoping) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RequestChannelCreate( 
            /* [in] */ Channel *channel) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RequestChannelJoin( 
            /* [in] */ Channel *channel) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RequestChannelLeave( void) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RequestUserList( void) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RequestPublicMessage( 
            /* [in] */ LPCSTR message) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RequestPrivateMessage( 
            /* [in] */ User *users,
            /* [in] */ LPCSTR message) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RequestLogout( void) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RequestPrivateGameOptions( 
            /* [in] */ User *users,
            /* [in] */ LPCSTR options) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RequestPublicGameOptions( 
            /* [in] */ LPCSTR options) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RequestPublicAction( 
            /* [in] */ LPCSTR action) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RequestPrivateAction( 
            /* [in] */ User *users,
            /* [in] */ LPCSTR action) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RequestGameStart( 
            /* [in] */ User *users) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RequestChannelTopic( 
            /* [in] */ LPCSTR topic) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetVersion( 
            /* [in] */ unsigned long *version) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RequestUserKick( 
            /* [in] */ User *user) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RequestUserIP( 
            /* [in] */ User *user) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetGametypeInfo( 
            unsigned int gtype,
            int icon_size,
            unsigned char *__RPC_FAR *bitmap,
            int *bmp_bytes,
            LPCSTR *name,
            LPCSTR *URL) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RequestFind( 
            User *user) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RequestPage( 
            User *user,
            LPCSTR message) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetFindPage( 
            int findOn,
            int pageOn) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetSquelch( 
            User *user,
            int squelch) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetSquelch( 
            User *user) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetChannelFilter( 
            int channelType) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RequestGameEnd( void) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetLangFilter( 
            int onoff) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RequestChannelBan( 
            LPCSTR name,
            int ban) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetGametypeList( 
            LPCSTR *list) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetHelpURL( 
            LPCSTR *url) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetProductSKU( 
            unsigned long SKU) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetNick( 
            int num,
            LPCSTR *nick,
            LPCSTR *pass) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetNick( 
            int num,
            LPCSTR nick,
            LPCSTR pass,
            int domangle) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetLobbyCount( 
            int *count) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RequestRawMessage( 
            LPCSTR ircmsg) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetAttributeValue( 
            LPCSTR attrib,
            LPCSTR *value) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetAttributeValue( 
            LPCSTR attrib,
            LPCSTR value) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetChannelExInfo( 
            LPCSTR info) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE StopAutoping( void) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RequestSquadInfo( 
            unsigned long id) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RequestSetTeam( 
            int team) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RequestSetLocale( 
            Locale locale) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RequestUserLocale( 
            User *users) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RequestUserTeam( 
            User *users) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetNickLocale( 
            int nicknum,
            Locale *locale) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetNickLocale( 
            int nicknum,
            Locale locale) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetLocaleString( 
            LPCSTR *loc_string,
            Locale locale) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetLocaleCount( 
            int *num) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetClientVersion( 
            unsigned long version) = 0;
        
        // RA2: IChat has ten more slots after SetClientVersion (vtable offsets 0xD8-0xFC)
        virtual HRESULT STDMETHODCALLTYPE Unknown_Virtual_D8(void) = 0;
        virtual HRESULT STDMETHODCALLTYPE Unknown_Virtual_DC(void) = 0;
        virtual HRESULT STDMETHODCALLTYPE Unknown_Virtual_E0(User *user) = 0;
        virtual HRESULT STDMETHODCALLTYPE Unknown_Virtual_E4(User *user) = 0;
        virtual HRESULT STDMETHODCALLTYPE RequestPublicMessageWide(unsigned short const *message) = 0; // RA2: slot 58, called by 0x0073CA50 (Send_Chat_Message)
        virtual HRESULT STDMETHODCALLTYPE RequestPrivateMessageWide(User *users, unsigned short const *message) = 0; // RA2: as above
        virtual HRESULT STDMETHODCALLTYPE RequestPublicActionWide(unsigned short const *action) = 0; // RA2: slot 60, called by 0x0073CB80 (Send_Chat_Action)
        virtual HRESULT STDMETHODCALLTYPE RequestPrivateActionWide(User *users, unsigned short const *action) = 0; // RA2: as above
        virtual HRESULT STDMETHODCALLTYPE RequestPageWide(User *user, unsigned short const *message) = 0; // RA2: slot 62 (0xF8), the wide counterpart of RequestPage: Parse_Page_Command 0x00746480 and Reply_To_Last_Pager 0x007466C0 call it with the recipient and the text
        virtual HRESULT STDMETHODCALLTYPE Unknown_Virtual_FC(int update, int count) = 0;
        
    };

// RA2: wonline.cpp interfaces (IChatEvent, IDownload, IDownloadEvent, INetUtil, INetUtilEvent), TibSun's wolapi.h in vtable order; VC6 has no MIDL_INTERFACE
struct __declspec(uuid("4DD3BAF6-7579-11D1-B1C6-006097176556")) IChatEvent : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE OnServerList( 
            /* [in] */ HRESULT res,
            /* [in] */ Server *servers) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE OnUpdateList( 
            /* [in] */ HRESULT res,
            /* [in] */ Update *updates) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE OnServerError( 
            /* [in] */ HRESULT res,
            /* [in] */ LPCSTR ircmsg) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE OnConnection( 
            /* [in] */ HRESULT res,
            /* [in] */ LPCSTR motd) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE OnMessageOfTheDay( 
            /* [in] */ HRESULT res,
            /* [in] */ LPCSTR motd) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE OnChannelList( 
            /* [in] */ HRESULT res,
            /* [in] */ Channel *channels) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE OnChannelCreate( 
            /* [in] */ HRESULT res,
            /* [in] */ Channel *channel) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE OnChannelJoin( 
            /* [in] */ HRESULT res,
            /* [in] */ Channel *channel,
            /* [in] */ User *user) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE OnChannelLeave( 
            /* [in] */ HRESULT res,
            /* [in] */ Channel *channel,
            /* [in] */ User *user) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE OnChannelTopic( 
            /* [in] */ HRESULT res,
            /* [in] */ Channel *channel,
            /* [in] */ LPCSTR topic) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE OnPrivateAction( 
            /* [in] */ HRESULT res,
            /* [in] */ User *user,
            /* [in] */ LPCSTR action) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE OnPublicAction( 
            /* [in] */ HRESULT res,
            /* [in] */ Channel *channel,
            User *user,
            /* [in] */ LPCSTR action) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE OnUserList( 
            /* [in] */ HRESULT res,
            /* [in] */ Channel *channel,
            /* [in] */ User *users) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE OnPublicMessage( 
            /* [in] */ HRESULT res,
            /* [in] */ Channel *channel,
            /* [in] */ User *user,
            /* [in] */ LPCSTR message) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE OnPrivateMessage( 
            /* [in] */ HRESULT res,
            /* [in] */ User *user,
            /* [in] */ LPCSTR message) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE OnSystemMessage( 
            /* [in] */ HRESULT res,
            /* [in] */ LPCSTR message) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE OnNetStatus( 
            /* [in] */ HRESULT res) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE OnLogout( 
            /* [in] */ HRESULT status,
            /* [in] */ User *user) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE OnPrivateGameOptions( 
            /* [in] */ HRESULT res,
            /* [in] */ User *user,
            /* [in] */ LPCSTR options) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE OnPublicGameOptions( 
            /* [in] */ HRESULT res,
            /* [in] */ Channel *channel,
            /* [in] */ User *user,
            /* [in] */ LPCSTR options) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE OnGameStart( 
            /* [in] */ HRESULT res,
            /* [in] */ Channel *channel,
            /* [in] */ User *users,
            /* [in] */ int gameid) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE OnUserKick( 
            /* [in] */ HRESULT res,
            /* [in] */ Channel *channel,
            /* [in] */ User *kicked,
            /* [in] */ User *kicker) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE OnUserIP( 
            /* [in] */ HRESULT res,
            /* [in] */ User *user) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE OnFind( 
            HRESULT res,
            Channel *chan) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE OnPageSend( 
            HRESULT res) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE OnPaged( 
            HRESULT res,
            User *user,
            LPCSTR message) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE OnServerBannedYou( 
            HRESULT res,
            time_t bannedTill) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE OnUserFlags( 
            HRESULT res,
            LPCSTR name,
            unsigned int flags,
            unsigned int mask) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE OnChannelBan( 
            HRESULT res,
            LPCSTR name,
            int banned) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE OnSquadInfo( 
            HRESULT res,
            unsigned long id,
            Squad *squad) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE OnUserLocale( 
            HRESULT res,
            User *users) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE OnUserTeam( 
            HRESULT res,
            User *users) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE OnSetLocale( 
            HRESULT res,
            Locale newlocale) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE OnSetTeam( 
            HRESULT res,
            int newteam) = 0;
        
    };

struct __declspec(uuid("0BF5FCEB-9F03-11D1-9DC7-006097C54321")) IDownload : public IUnknown
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE DownloadFile( 
            LPCSTR server,
            LPCSTR login,
            LPCSTR password,
            LPCSTR file,
            LPCSTR localfile,
            LPCSTR regkey) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Abort( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE PumpMessages( void) = 0;
        
    };

struct __declspec(uuid("6869E99D-9FB4-11D1-9DC8-006097C54321")) IDownloadEvent : public IUnknown
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE OnEnd( void) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE OnError( 
            int error) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE OnProgressUpdate( 
            int bytesread,
            int totalsize,
            int timetaken,
            int timeleft) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE OnQueryResume( void) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE OnStatusUpdate( 
            int status) = 0;
        
    };

struct __declspec(uuid("B832B0AA-A7D3-11D1-97C3-00609706FA0C")) INetUtil : public IUnknown
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE RequestGameresSend( 
            LPCSTR host,
            int port,
            unsigned char *data,
            int length) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE RequestLadderSearch( 
            LPCSTR host,
            int port,
            LPCSTR key,
            unsigned long SKU,
            int team,
            int cond,
            int sort,
            int number,
            int leading) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE RequestLadderList( 
            LPCSTR host,
            int port,
            LPCSTR keys,
            unsigned long SKU,
            int team,
            int cond,
            int sort) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE RequestPing( 
            LPCSTR host,
            int timeout,
            int *handle) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE PumpMessages( void) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetAvgPing( 
            unsigned long ip,
            int *avg) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RequestNewNick( 
            LPCSTR nick,
            LPCSTR pass,
            LPCSTR email,
            LPCSTR parentEmail,
            int newsletter,
            int shareinfo) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RequestAgeCheck( 
            int month,
            int day,
            int year,
            LPCSTR email) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RequestWDTState( 
            LPCSTR host,
            int port,
            unsigned char request) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RequestLocaleLadderList( 
            LPCSTR host,
            int port,
            LPCSTR keys,
            unsigned long SKU,
            int team,
            int cond,
            int sort,
            Locale locale) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RequestLocaleLadderSearch( 
            LPCSTR host,
            int port,
            LPCSTR key,
            unsigned long sku,
            int team,
            int cond,
            int sort,
            int number,
            int leading,
            Locale locale) = 0;
        
    };

struct __declspec(uuid("B832B0AC-A7D3-11D1-97C3-00609706FA0C")) INetUtilEvent : public IUnknown
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE OnPing( 
            HRESULT res,
            int time,
            unsigned long ip,
            int handle) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE OnLadderList( 
            HRESULT res,
            /* [in] */ Ladder *list,
            int totalCount,
            long timeStamp,
            int keyRung) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE OnGameresSent( 
            HRESULT res) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE OnNewNick( 
            HRESULT res,
            LPCSTR message,
            LPCSTR nick,
            LPCSTR pass) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE OnAgeCheck( 
            HRESULT res,
            int years,
            int consent) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE OnWDTState( 
            HRESULT res,
            unsigned char *state,
            int length) = 0;
        
    };

