// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2026 ra2-decomp contributors
// Copyright 2025 Electronic Arts Inc.; 2026 OpenTS contributors
// Derived from Electronic Arts source code; EA's GPL Section 7 terms apply (COPYING.md).

#include "always.h"
#include "savever.h"
#include "saveload.h"
#include "dbgprint.h"
#include "session.h"
#include <comdef.h>
#include "fastmath.h"

SavegameInformation::SavegameInformation(void)
{
	InternalVersion = 0;
	Version = 0;
	Campaign = -1;
	ScenarioNumber = 0;
	GameType = 0;
	ScenarioDescription[0] = 0;
	PlayerHouse[0] = 0;
	UnknownString[0] = 0;
	PlayerName[0] = 0;
	ExecutableName[0] = 0;
	StartTime.dwLowDateTime = 0;
	StartTime.dwHighDateTime = 0;
	PlayTime.dwLowDateTime = 0;
	PlayTime.dwHighDateTime = 0;
	LastSaveTime.dwLowDateTime = 0;
	LastSaveTime.dwHighDateTime = 0;
}

void SavegameInformation::Set_Version(int num)
{
	Version = num;
}

int SavegameInformation::Get_Version(void)
{
	return Version;
}

void SaveVersionInfo::Set_Internal_Version(int num)
{
	InternalVersion = num;
}

int SaveVersionInfo::Get_Internal_Version(void)
{
	return InternalVersion;
}

void SaveVersionInfo::Set_Scenario_Description(const wchar_t * desc)
{
	ScenarioDescription[ARRAY_SIZE(ScenarioDescription) - 1] = 0;
	wcsncpy(ScenarioDescription, desc, ARRAY_SIZE(ScenarioDescription) - 1);
}

const wchar_t * SaveVersionInfo::Get_Scenario_Description(void)
{
	return ScenarioDescription;
}

void SaveVersionInfo::Set_Player_House(const wchar_t * name)
{
	PlayerHouse[ARRAY_SIZE(PlayerHouse) - 1] = 0;
	wcsncpy(PlayerHouse, name, ARRAY_SIZE(PlayerHouse) - 1);
}

const wchar_t * SaveVersionInfo::Get_Player_House(void)
{
	return PlayerHouse;
}

void SaveVersionInfo::Set_Campaign_Number(int num)
{
	CampaignNumber = num;
}

int SaveVersionInfo::Get_Campaign_Number(void)
{
	return CampaignNumber;
}

void SaveVersionInfo::Set_Scenario_Number(int num)
{
	ScenarioNumber = num;
}

int SaveVersionInfo::Get_Scenario_Number(void)
{
	return ScenarioNumber;
}

void SaveVersionInfo::Set_Unknown_String(const char * name)
{
	UnknownString[ARRAY_SIZE(UnknownString) - 1] = 0;
	strncpy(UnknownString, name, ARRAY_SIZE(UnknownString) - 1);
}

const char * SaveVersionInfo::Get_Unknown_String(void)
{
	return UnknownString;
}

void SaveVersionInfo::Set_Player_Name(const char * name)
{
	PlayerName[ARRAY_SIZE(PlayerName) - 1] = 0;
	strncpy(PlayerName, name, ARRAY_SIZE(PlayerName) - 1);
}

const char * SaveVersionInfo::Get_Player_Name(void)
{
	return PlayerName;
}

void SaveVersionInfo::Set_Executable_Name(const char * name)
{
	ExecutableName[ARRAY_SIZE(ExecutableName) - 1] = 0;
	strncpy(ExecutableName, name, ARRAY_SIZE(ExecutableName) - 1);
}

const char * SaveVersionInfo::Get_Executable_Name(void)
{
	return ExecutableName;
}

void SaveVersionInfo::Set_Start_Time(FILETIME &time)
{
	StartTime = time;
}

FILETIME SaveVersionInfo::Get_Start_Time(void)
{
	return StartTime;
}

void SaveVersionInfo::Set_Play_Time(FILETIME &time)
{
	PlayTime = time;
}

FILETIME SaveVersionInfo::Get_Play_Time(void)
{
	return PlayTime;
}

void SaveVersionInfo::Set_Last_Time(FILETIME &time)
{
	LastSaveTime = time;
}

// FUNCTION: GAMEMD 0x006812a0
FILETIME SaveVersionInfo::Get_Last_Time(void)
{
	return LastSaveTime;
}

void SaveVersionInfo::Set_Game_Type(int id)
{
	GameType = id;
}

int SaveVersionInfo::Get_Game_Type(void)
{
	return GameType;
}

HRESULT SaveVersionInfo::Save(IStorage *storage)
{
	if (storage == NULL) {
		return(E_POINTER);
	}

	IPropertySetStoragePtr storageset;
	HRESULT res;

	res = storage->QueryInterface(IID_IPropertySetStorage, (void **)&storageset);
	if (SUCCEEDED(res)) {

		res = Save_String_Set(storageset, PIDSI_SCEN_DESCRIP, ScenarioDescription);
		if (FAILED(res)) {
			return(res);
		}

		res = Save_String_Set(storageset, PIDSI_PLAYER_HOUSE, PlayerHouse);
		if (FAILED(res)) {
			return(res);
		}

		res = Save_Int_Set(storageset, PIDSI_G_VERSION, Version);
		if (FAILED(res)) {
			return(res);
		}

		res = Save_Int_Set(storageset, PIDSI_INTERNAL_VER, InternalVersion);
		if (FAILED(res)) {
			return(res);
		}

		res = Save_Time_Set(storageset, PIDSI_G_START_TIME, &StartTime);
		if (FAILED(res)) {
			return(res);
		}

		res = Save_Time_Set(storageset, PIDSI_LAST_SAVE_TIME, &LastSaveTime);
		if (FAILED(res)) {
			return(res);
		}

		res = Save_Time_Set(storageset, PIDSI_G_PLAY_TIME, &PlayTime);
		if (FAILED(res)) {
			return(res);
		}

		res = Save_String_Set(storageset, PIDSI_EXEC_NAME, ExecutableName);
		if (FAILED(res)) {
			return(res);
		}

		res = Save_String_Set(storageset, PIDSI_PLAYER_NAME1, PlayerName);
		if (FAILED(res)) {
			return(res);
		}

		res = Save_String_Set(storageset, PIDSI_PLAYER_NAME2, PlayerName);
		if (FAILED(res)) {
			return(res);
		}

		res = Save_Int_Set(storageset, PIDSI_SCENARIO_NUM, ScenarioNumber);
		if (FAILED(res)) {
			return(res);
		}

		res = Save_Int_Set(storageset, PIDSI_CAMPAIGN_NUM, CampaignNumber);
		if (FAILED(res)) {
			return(res);
		}

		res = Save_Int_Set(storageset, PIDSI_GAME_TYPE, GameType);
		if (FAILED(res)) {
			return(res);
		}

	}

	res = Save_String(storage, PIDSI_SCEN_DESCRIP, ScenarioDescription);
	if (FAILED(res)) {
		return(res);
	}

	res = Save_String(storage, PIDSI_PLAYER_HOUSE, PlayerHouse);
	if (FAILED(res)) {
		return(res);
	}

	res = Save_Int(storage, PIDSI_G_VERSION, Version);
	if (FAILED(res)) {
		return(res);
	}

	res = Save_Int(storage, PIDSI_INTERNAL_VER, InternalVersion);
	if (FAILED(res)) {
		return(res);
	}

	res = Save_Time(storage, PIDSI_G_START_TIME, &StartTime);
	if (FAILED(res)) {
		return(res);
	}

	res = Save_Time(storage, PIDSI_LAST_SAVE_TIME, &LastSaveTime);
	if (FAILED(res)) {
		return(res);
	}

	res = Save_Time(storage, PIDSI_G_PLAY_TIME, &PlayTime);
	if (FAILED(res)) {
		return(res);
	}

	res = Save_String(storage, PIDSI_EXEC_NAME, ExecutableName);
	if (FAILED(res)) {
		return(res);
	}

	res = Save_String(storage, PIDSI_PLAYER_NAME1, PlayerName);
	if (FAILED(res)) {
		return(res);
	}

	res = Save_String(storage, PIDSI_PLAYER_NAME2, PlayerName);
	if (FAILED(res)) {
		return(res);
	}

	res = Save_Int(storage, PIDSI_SCENARIO_NUM, ScenarioNumber);
	if (FAILED(res)) {
		return(res);
	}

	res = Save_Int(storage, PIDSI_CAMPAIGN_NUM, CampaignNumber);
	if (FAILED(res)) {
		return(res);
	}

	res = Save_Int(storage, PIDSI_GAME_TYPE, GameType);
	if (FAILED(res)) {
		return(res);
	}

	return(S_OK);
}

HRESULT SaveVersionInfo::Load(IStorage *storage)
{
	char buf[256];
	wchar_t wbuf[256];

	if (storage == NULL) {
		return(E_POINTER);
	}

	IPropertySetStoragePtr storageset;
	HRESULT res;

	if (SUCCEEDED(storage->QueryInterface(IID_IPropertySetStorage, (void **)&storageset))
			&& SUCCEEDED(Load_String_Set(storageset, PIDSI_SCEN_DESCRIP, wbuf))) {

		wcscpy(ScenarioDescription, wbuf);

		res = Load_String_Set(storageset, PIDSI_PLAYER_HOUSE, wbuf);
		if (res < 0) {
			return(res);
		}
		wcscpy(PlayerHouse, wbuf);

		res = Load_Int_Set(storageset, PIDSI_G_VERSION, &Version);
		if (res < 0) {
			return(res);
		}

		res = Load_Int_Set(storageset, PIDSI_INTERNAL_VER, &InternalVersion);
		if (res < 0) {
			return(res);
		}

		res = Load_Time_Set(storageset, PIDSI_G_START_TIME, &StartTime);
		if (res < 0) {
			return(res);
		}

		res = Load_Time_Set(storageset, PIDSI_LAST_SAVE_TIME, &LastSaveTime);
		if (res < 0) {
			return(res);
		}

		res = Load_Time_Set(storageset, PIDSI_G_PLAY_TIME, &PlayTime);
		if (res < 0) {
			return(res);
		}

		res = Load_String_Set(storageset, PIDSI_EXEC_NAME, buf);
		if (res < 0) {
			return(res);
		}
		strcpy(ExecutableName, buf);

		res = Load_String_Set(storageset, PIDSI_PLAYER_NAME1, buf);
		if (res < 0) {
			return(res);
		}
		strcpy(PlayerName, buf);

		res = Load_Int_Set(storageset, PIDSI_SCENARIO_NUM, &ScenarioNumber);
		if (res < 0) {
			return(res);
		}

		res = Load_Int_Set(storageset, PIDSI_CAMPAIGN_NUM, &CampaignNumber);
		if (res < 0) {
			return(res);
		}

		res = Load_Int_Set(storageset, PIDSI_GAME_TYPE, &GameType);
		if (res < 0) {
			return(res);
		}

	} else {

		res = Load_String(storage, PIDSI_SCEN_DESCRIP, wbuf);
		if (res < 0) {
			return(res);
		}
		wcscpy(ScenarioDescription, wbuf);

		res = Load_String(storage, PIDSI_PLAYER_HOUSE, wbuf);
		if (res < 0) {
			return(res);
		}
		wcscpy(PlayerHouse, wbuf);

		res = Load_Int(storage, PIDSI_G_VERSION, &Version);
		if (res < 0) {
			return(res);
		}

		res = Load_Int(storage, PIDSI_INTERNAL_VER, &InternalVersion);
		if (res < 0) {
			return(res);
		}

		res = Load_Time(storage, PIDSI_G_START_TIME, &StartTime);
		if (res < 0) {
			return(res);
		}

		res = Load_Time(storage, PIDSI_LAST_SAVE_TIME, &LastSaveTime);
		if (res < 0) {
			return(res);
		}

		res = Load_Time(storage, PIDSI_G_PLAY_TIME, &PlayTime);
		if (res < 0) {
			return(res);
		}

		res = Load_String(storage, PIDSI_EXEC_NAME, buf);
		if (res < 0) {
			return(res);
		}
		strcpy(ExecutableName, buf);

		res = Load_String(storage, PIDSI_PLAYER_NAME1, buf);
		if (res < 0) {
			return(res);
		}
		strcpy(PlayerName, buf);

		res = Load_Int(storage, PIDSI_SCENARIO_NUM, &ScenarioNumber);
		if (res < 0) {
			return(res);
		}

		res = Load_Int(storage, PIDSI_CAMPAIGN_NUM, &CampaignNumber);
		if (res < 0) {
			return(res);
		}

		res = Load_Int(storage, PIDSI_GAME_TYPE, &GameType);
		if (res < 0) {
			return(res);
		}
	}

	return(S_OK);
}

HRESULT SaveVersionInfo::Load_String(IStorage *storage, int id, char *string)
{
	*string = '\0';

	HRESULT res;
	IStreamPtr stm;

	res = storage->OpenStream(Stream_Name_From_ID(id), NULL, STGM_SHARE_EXCLUSIVE, 0, &stm);
	if (FAILED(res)) {
		return(res);
	}

	int i = 0;
	while (true) {
		WCHAR buf[128];
		res = stm->Read(&buf[i], sizeof(buf[i]), NULL);
		if (FAILED(res)) {
			return(res);
		}
		if (buf[i] == '\0') {
			WideCharToMultiByte(CP_ACP, 0, buf, -1, string, ARRAY_SIZE(buf) - 1, 0, 0);
			return(res);
		}
		i++;
	}

	return(S_OK);
}

HRESULT SaveVersionInfo::Load_String(IStorage *storage, int id, wchar_t *string)
{
	*string = '\0';

	HRESULT res;
	IStreamPtr stm;

	res = storage->OpenStream(Stream_Name_From_ID(id), NULL, STGM_SHARE_EXCLUSIVE, 0, &stm);
	if (FAILED(res)) {
		return(res);
	}

	while (true) {
		res = stm->Read(string, sizeof(*string), NULL);
		if (FAILED(res)) {
			return(res);
		}
		if (*string == '\0') {
			return(res);
		}
		string++;
	}
}

HRESULT SaveVersionInfo::Load_String_Set(IPropertySetStorage *storageset, int id, char *string)
{
	*string = '\0';

	HRESULT res;
	IPropertyStoragePtr storage;

	res = storageset->Open(FMTID_SummaryInformation, STGM_SHARE_EXCLUSIVE|STGM_READWRITE, &storage);
	if (FAILED(res)) {
		return(res);
	}

	PROPSPEC propsec;
	propsec.ulKind = PRSPEC_PROPID;
	propsec.propid = id;
	PROPVARIANT propvar;

	res = storage->ReadMultiple(1, &propsec, &propvar);
	if (FAILED(res)) {
		return(res);
	}

	if (propvar.vt == VT_LPWSTR) {
		WideCharToMultiByte(CP_ACP, 0, propvar.pwszVal, -1, string, 128, 0, 0);
	}

	return(res);
}

HRESULT SaveVersionInfo::Load_String_Set(IPropertySetStorage *storageset, int id, wchar_t *string)
{
	*string = '\0';

	HRESULT res;
	IPropertyStoragePtr storage;

	res = storageset->Open(FMTID_SummaryInformation, STGM_SHARE_EXCLUSIVE|STGM_READWRITE, &storage);
	if (FAILED(res)) {
		return(res);
	}

	PROPSPEC propsec;
	propsec.ulKind = PRSPEC_PROPID;
	propsec.propid = id;
	PROPVARIANT propvar;

	res = storage->ReadMultiple(1, &propsec, &propvar);
	if (FAILED(res)) {
		return(res);
	}

	if (propvar.vt == VT_LPWSTR) {
		wcsncpy(string, propvar.pwszVal, 128);
	}

	return(res);
}

HRESULT SaveVersionInfo::Load_Int(IStorage *storage, int id, int *integer)
{
	*integer = 0;

	HRESULT res;
	IStreamPtr stm;

	res = storage->OpenStream(Stream_Name_From_ID(id), NULL, STGM_SHARE_EXCLUSIVE, 0, &stm);
	if (FAILED(res)) {
		return(res);
	}

	res = stm->Read(integer, sizeof(*integer), NULL);
	if (FAILED(res)) {
		return(res);
	}

	return(res);
}

HRESULT SaveVersionInfo::Load_Int_Set(IPropertySetStorage *storageset, int id, int *integer)
{
	*integer = 0;

	HRESULT res;
	IPropertyStoragePtr storage;

	res = storageset->Open(FMTID_SummaryInformation, STGM_SHARE_EXCLUSIVE|STGM_READWRITE, &storage);
	if (FAILED(res)) {
		return(res);
	}

	PROPSPEC propsec;
	propsec.ulKind = PRSPEC_PROPID;
	propsec.propid = id;
	PROPVARIANT propvar;

	res = storage->ReadMultiple(1, &propsec, &propvar);
	if (FAILED(res)) {
		return(res);
	}

	if (propvar.vt == VT_I4) {
		*integer = propvar.lVal;
	}

	return(res);
}

HRESULT SaveVersionInfo::Save_String(IStorage *storage, int id, char *string)
{
	WCHAR buf[128];

	MultiByteToWideChar(CP_ACP, 0, string, -1, buf, ARRAY_SIZE(buf));

	IStreamPtr stm(NULL);

	HRESULT res = storage->CreateStream(Stream_Name_From_ID(id), STGM_SHARE_EXCLUSIVE|STGM_READWRITE, 0, 0, &stm);
	if (FAILED(res)) {
		return(res);
	}

	res = stm->Write(buf, sizeof(WCHAR) * wcslen(buf) + 2, NULL);
	if (FAILED(res)) {
		return(res);
	}
	res = stm->Commit(0);
	if (FAILED(res)) {
		return(res);
	}

	return(res);
}

HRESULT SaveVersionInfo::Save_String(IStorage *storage, int id, const wchar_t *string)
{
	IStreamPtr stm(NULL);

	HRESULT res = storage->CreateStream(Stream_Name_From_ID(id), STGM_SHARE_EXCLUSIVE|STGM_READWRITE, 0, 0, &stm);
	if (FAILED(res)) {
		return(res);
	}

	res = stm->Write(string, sizeof(WCHAR) * wcslen(string) + 2, NULL);
	if (FAILED(res)) {
		return(res);
	}
	res = stm->Commit(0);
	if (FAILED(res)) {
		return(res);
	}

	return(res);
}

HRESULT SaveVersionInfo::Save_String_Set(IPropertySetStorage *storageset, int id, const char *string)
{
	WCHAR buf[128];

	MultiByteToWideChar(CP_ACP, 0, string, -1, buf, ARRAY_SIZE(buf));

	HRESULT res;
	IPropertyStoragePtr storage;

	res = storageset->Open(FMTID_SummaryInformation, STGM_SHARE_EXCLUSIVE|STGM_READWRITE, &storage);
	if (FAILED(res)) {
		res = storageset->Create(FMTID_SummaryInformation, NULL, PROPSETFLAG_DEFAULT, STGM_SHARE_EXCLUSIVE|STGM_READWRITE|STGM_CREATE, &storage);
		if (FAILED(res)) {
			return(res);
		}
	}

	PROPSPEC propsec;
	propsec.ulKind = PRSPEC_PROPID;
	propsec.propid = id;
	PROPVARIANT propvar;

	propvar.vt = VT_LPWSTR;
	propvar.pwszVal = buf;

	res = storage->WriteMultiple(1, &propsec, &propvar, PID_FIRST_USABLE);
	if (FAILED(res)) {
		return(res);
	}

	return(res);
}

HRESULT SaveVersionInfo::Save_String_Set(IPropertySetStorage *storageset, int id, const wchar_t *string)
{
	HRESULT res;
	IPropertyStoragePtr storage;

	res = storageset->Open(FMTID_SummaryInformation, STGM_SHARE_EXCLUSIVE|STGM_READWRITE, &storage);
	if (FAILED(res)) {
		res = storageset->Create(FMTID_SummaryInformation, NULL, PROPSETFLAG_DEFAULT, STGM_SHARE_EXCLUSIVE|STGM_READWRITE|STGM_CREATE, &storage);
		if (FAILED(res)) {
			return(res);
		}
	}

	PROPSPEC propsec;
	propsec.ulKind = PRSPEC_PROPID;
	propsec.propid = id;
	PROPVARIANT propvar;

	propvar.vt = VT_LPWSTR;
	propvar.pwszVal = (wchar_t *)string;

	res = storage->WriteMultiple(1, &propsec, &propvar, PID_FIRST_USABLE);
	if (FAILED(res)) {
		return(res);
	}

	return(res);
}

HRESULT SaveVersionInfo::Save_Int(IStorage *storage, int id, int integer)
{
	IStreamPtr stm(NULL);

	HRESULT res = storage->CreateStream(Stream_Name_From_ID(id), STGM_SHARE_EXCLUSIVE|STGM_READWRITE, 0, 0, &stm);
	if (FAILED(res)) {
		return(res);
	}

	res = stm->Write(&integer, sizeof(integer), NULL);
	if (FAILED(res)) {
		return(res);
	}
	res = stm->Commit(STGM_READ);
	if (FAILED(res)) {
		return(res);
	}

	return(res);
}

HRESULT SaveVersionInfo::Save_Int_Set(IPropertySetStorage *storageset, int id, int integer)
{
	HRESULT res;
	IPropertyStoragePtr storage;

	res = storageset->Open(FMTID_SummaryInformation, STGM_SHARE_EXCLUSIVE|STGM_READWRITE, &storage);
	if (FAILED(res)) {
		res = storageset->Create(FMTID_SummaryInformation, NULL, PROPSETFLAG_DEFAULT, STGM_SHARE_EXCLUSIVE|STGM_READWRITE|STGM_CREATE, &storage);
		if (FAILED(res)) {
			return(res);
		}
	}

	PROPSPEC propsec;
	propsec.ulKind = PRSPEC_PROPID;
	propsec.propid = id;
	PROPVARIANT propvar;

	propvar.vt = VT_I4;
	propvar.lVal = integer;

	res = storage->WriteMultiple(1, &propsec, &propvar, PID_FIRST_USABLE);
	if (FAILED(res)) {
		return(res);
	}

	return(res);
}

HRESULT SaveVersionInfo::Load_Time(IStorage *storage, int id, FILETIME *time)
{
	time->dwLowDateTime = 0;
	time->dwHighDateTime = 0;

	HRESULT res;
	IStreamPtr stm;

	res = storage->OpenStream(Stream_Name_From_ID(id), NULL, STGM_SHARE_EXCLUSIVE, 0, &stm);
	if (FAILED(res)) {
		return(res);
	}

	res = stm->Read(time, sizeof(*time), NULL);
	if (FAILED(res)) {
		return(res);
	}

	return(res);
}

HRESULT SaveVersionInfo::Load_Time_Set(IPropertySetStorage *storageset, int id, FILETIME *time)
{
	time->dwLowDateTime = 0;
	time->dwHighDateTime = 0;

	HRESULT res;
	IPropertyStoragePtr storage;

	res = storageset->Open(FMTID_SummaryInformation, STGM_SHARE_EXCLUSIVE|STGM_READWRITE, &storage);
	if (FAILED(res)) {
		return(res);
	}

	PROPSPEC propsec;
	propsec.ulKind = PRSPEC_PROPID;
	propsec.propid = id;
	PROPVARIANT propvar;

	res = storage->ReadMultiple(1, &propsec, &propvar);
	if (FAILED(res)) {
		return(res);
	}

	if (propvar.vt == VT_FILETIME) {
		*time = propvar.filetime;
	}

	return(res);
}

HRESULT SaveVersionInfo::Save_Time(IStorage *storage, int id, FILETIME *time)
{
	IStreamPtr stm(NULL);

	HRESULT res = storage->CreateStream(Stream_Name_From_ID(id), STGM_SHARE_EXCLUSIVE|STGM_READWRITE, 0, 0, &stm);
	if (FAILED(res)) {
		return(res);
	}

	res = stm->Write(time, sizeof(*time), NULL);
	if (FAILED(res)) {
		return(res);
	}
	res = stm->Commit(STGM_READ);
	if (FAILED(res)) {
		return(res);
	}

	return(res);
}

HRESULT SaveVersionInfo::Save_Time_Set(IPropertySetStorage *storageset, int id, FILETIME *time)
{
	HRESULT res;
	IPropertyStoragePtr storage;

	res = storageset->Open(FMTID_SummaryInformation, STGM_SHARE_EXCLUSIVE|STGM_READWRITE, &storage);
	if (FAILED(res)) {
		res = storageset->Create(FMTID_SummaryInformation, NULL, PROPSETFLAG_DEFAULT, STGM_SHARE_EXCLUSIVE|STGM_READWRITE|STGM_CREATE, &storage);
		if (FAILED(res)) {
			return(res);
		}
	}

	PROPSPEC propsec;
	propsec.ulKind = PRSPEC_PROPID;
	propsec.propid = id;
	PROPVARIANT propvar;

	propvar.vt = VT_FILETIME;
	propvar.filetime = *time;

	res = storage->WriteMultiple(1, &propsec, &propvar, PID_FIRST_USABLE);
	if (FAILED(res)) {
		return(res);
	}

	return(res);
}

const WCHAR *Stream_Name_From_ID(int id)
{
	static struct pidsiStruct {
		int ID;
		WCHAR const *Name;
	} _ids[] = {
		{PIDSI_SCEN_DESCRIP,	L"Scenario Description"},
		{PIDSI_PLAYER_HOUSE,	L"Player House"},
		{PIDSI_G_VERSION,	L"Version"},
		{PIDSI_INTERNAL_VER,	L"Internal Version"},
		{PIDSI_G_START_TIME,	L"Start Time"},
		{PIDSI_LAST_SAVE_TIME,	L"Last Save Time"},
		{PIDSI_G_PLAY_TIME,		L"Play Time"},
		{PIDSI_EXEC_NAME,		L"Executable Name"},
		{PIDSI_PLAYER_NAME1,	L"Player Name"},
		{PIDSI_PLAYER_NAME2,	L"Player Name2"},
		{PIDSI_SCENARIO_NUM,	L"Scenario Number"},
		{PIDSI_CAMPAIGN_NUM,	L"Campaign"},
		{PIDSI_GAME_TYPE,		L"GameType"},
	};

	for (int i = 0; i < ARRAY_SIZE(_ids); i++) {
		if (_ids[i].ID == id) {
			return(_ids[i].Name);
		}
	}

	return(NULL);
}

template class _com_ptr_t<_com_IIID<IStream, &__uuidof(IStream)> >;
