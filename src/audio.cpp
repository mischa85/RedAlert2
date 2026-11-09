// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2026 ra2-decomp contributors
// Copyright 2025 Electronic Arts Inc.; 2026 OpenTS contributors
// Derived from Electronic Arts source code; EA's GPL Section 7 terms apply (COPYING.md).
#include "always.h"
#include "mixfile.h"
#include "ccfile.h"
#include "auddev.h"
#include "audbag.h"
#include "audsrc.h"
#include "audtimer.h"
#include "audlevel.h"
#include "audplay.h"
#include "theme.h"
#include "_pk.h"
#include "_mixfile.h"

void Call_Back(void);

int AudioEventSetUp(AudioDevice * device, AudioIDXData * idx);
void AudioEventCloseDown(void);
void AudioServiceAllEvents(void);
void AudioKillAllEvents(void);
void AudioPauseAllEvents(void);
void AudioResumeAllEvents(void);
void Sound_Service(void);
void Sound_Pause_All(void);
void Sound_Resume_All(void);
void Speech_Init(void);
void Speech_Shutdown(void);
void Speech_Service(void);
void Aux_Stream_Init(void);
void Aux_Stream_Shutdown(void);
void Aux_Stream_Pause(void);
void Aux_Stream_Resume(void);
void Theme_Suspend(void);
void Theme_Resume(void);
void Audio_Worker_Service(void);
extern AudioSystemMaster * AudioSystemDSD;
void AudioLoseFocus(void);
void AudioRegainFocus(void);

static AudioAttribs AudioMasterData;
static AudioAttribs AudioCompData;
static AudioAttribs AudioGroupData1;
static AudioAttribs AudioSoundData;
static AudioAttribs AudioScoreData;
static AudioAttribs AudioVoiceData;
static AudioAttribs AudioGroupData2;
static AudioAttribs AudioFadeData;

AudioIDXData * AudioIDX;
AudioDevice * AudioMainDevice;
int AudioEnabled;
int AudioFocusLocks;
MixFileClass * AudioMix;
MFCD * ScoresMix;
AudioAttribs * AudioMasterAttribs;
AudioAttribs * AudioVoiceAttribs;
AudioAttribs * AudioScoreAttribs;
AudioAttribs * AudioSoundAttribs;
AudioAttribs * AudioCompAttribs;
AudioAttribs * AudioGroupAttribs2;
AudioAttribs * AudioFadeAttribs;
AudioAttribs * AudioGroupAttribs1;
static int AudioServiceBusy;
static TimeStamp AudioServiceLast;

int __fastcall Audio_Init(int enable)
{
	InitAudioTimer();
	AudioAttribsInit(&AudioMasterData);
	AudioMasterAttribs = &AudioMasterData;
	AudioAttribsInit(&AudioScoreData);
	AudioScoreAttribs = &AudioScoreData;
	AudioAttribsInit(&AudioVoiceData);
	AudioVoiceAttribs = &AudioVoiceData;
	AudioAttribsInit(&AudioSoundData);
	AudioSoundAttribs = &AudioSoundData;
	AudioAttribsInit(&AudioCompData);
	AudioCompAttribs = &AudioCompData;
	AudioAttribsInit(&AudioFadeData);
	AudioFadeAttribs = &AudioFadeData;
	AudioAttribsInit(&AudioGroupData2);
	AudioGroupAttribs2 = &AudioGroupData2;
	AudioAttribsInit(&AudioGroupData1);
	AudioGroupAttribs1 = &AudioGroupData1;

	if (enable) {
		AudioSetUp();
		AudioLoadSystem(AudioSystemDSD);

		AudioFormat format;
		AudioFormatInit(&format);
		format.Rate = 0x5622;
		format.Compression = AUDIO_COMPRESS_NONE;
		format.Channels = 2;
		format.SampleWidth = 2;
		format.Flags = mAUDIO_FORMAT_PCM;
		AudioFormatUpdate(&format);

		AudioMainDevice = AudioOpenDevice(-1, &format);
		if (!AudioMainDevice || AudioDeviceCreateChannels(AudioMainDevice, 0x10) != 0x10) {
			enable = 0;
		} else {
			AudioMasterAttribs = &AudioMainDevice->Attribs;
			AudioMix = new MixFileClass("AUDIO.MIX", &FastKey); // RA2: no AUDIOMD.MIX
			AudioIDX = AudioIDXData::Create("audio", 0);
			if (!AudioIDX) {
				enable = 0;
			}
		}
	}

	AudioStreamerInit();
	if (!AudioEventSetUp(AudioMainDevice, AudioIDX)) {
		enable = 0;
	}
	Speech_Init();
	Aux_Stream_Init();
	AudioEnabled = enable;
	return 1;
}

void Audio_Shutdown(void)
{
	if (AudioMainDevice) {
		AudioDeviceStopAllChannels(AudioMainDevice, 1);
	}
	AudioKillAllEvents();
	AudioStreamerStopAllStreams();
	Speech_Shutdown();
	Aux_Stream_Shutdown();
	AudioEventCloseDown();
	AudioStreamerDeinit();
	if (AudioIDX) {
		AudioIDX->Destroy();
		AudioIDX = NULL;
	}
	if (AudioMix) {
		delete AudioMix;
		AudioMix = NULL;
	}
	if (ScoresMix) {
		delete ScoresMix;
		ScoresMix = NULL;
	}
	if (AudioMainDevice) {
		AudioDeviceDestroy(AudioMainDevice);
		AudioMainDevice = NULL;
	}
	AudioCloseDown();
}

int Audio_Open_Device(void)
{
	AudioFormat format;

	AudioLoadSystem(AudioSystemDSD);
	AudioFormatInit(&format);
	format.Rate = 0x5622;
	format.Compression = AUDIO_COMPRESS_NONE;
	format.Channels = 2;
	format.SampleWidth = 2;
	format.Flags = mAUDIO_FORMAT_PCM;
	AudioFormatUpdate(&format);

	AudioMainDevice = AudioOpenDevice(-1, &format);
	if (AudioMainDevice && AudioDeviceCreateChannels(AudioMainDevice, 0x10) == 0x10) {
		return 1;
	}
	return 0;
}

void Audio_Close_Device(void)
{
	if (AudioMainDevice) {
		AudioDeviceDestroy(AudioMainDevice);
		AudioMainDevice = NULL;
	}
}

void Audio_Stop_Events(void)
{
	if (AudioMainDevice) {
		AudioDeviceStopAllChannels(AudioMainDevice, 1);
	}
	AudioKillAllEvents();
}

void Audio_Pause_Channels(void)
{
	if (AudioMainDevice) {
		AudioDevicePauseAllChannels(AudioMainDevice, 1);
	}
	AudioPauseAllEvents();
	Sound_Pause_All();
}

void Audio_Resume_Channels(void)
{
	Sound_Resume_All();
	AudioResumeAllEvents();
	if (AudioMainDevice) {
		AudioDeviceResumeAllChannels(AudioMainDevice, 1);
	}
}

void Audio_Stop_All(void)
{
	if (AudioMainDevice) {
		AudioDeviceStopAllChannels(AudioMainDevice, 1);
	}
	AudioKillAllEvents();
	AudioStreamerStopAllStreams();
}

void Audio_Pause_All(void)
{
	if (AudioMainDevice) {
		AudioDevicePauseAllChannels(AudioMainDevice, 1);
	}
	AudioPauseAllEvents();
	Sound_Pause_All();
	Theme_Suspend();
	Aux_Stream_Pause();
}

void Audio_Resume_All(void)
{
	Aux_Stream_Resume();
	Theme_Resume();
	Sound_Resume_All();
	AudioResumeAllEvents();
	if (AudioMainDevice) {
		AudioDeviceResumeAllChannels(AudioMainDevice, 1);
	}
}

void Audio_Service(void)
{
	if (!AudioServiceBusy && AudioMainDevice) {
		AudioServiceBusy = 1;
		TimeStamp now = AudioGetTime();
		if (now - AudioServiceLast > 33) {
			AudioServiceAllEvents();
			Sound_Service();
			Speech_Service();
			Theme.AI();
			Audio_Worker_Service();
			AudioServiceLast = now;
		}
		AudioServiceBusy = 0;
	}
}

int AudioDeviceAvailable(void)
{
	return AudioMainDevice != NULL;
}

AudioDevice * __fastcall AudioGetDevice(void)
{
	return AudioMainDevice;
}

void Audio_Focus_Lose(void)
{
	if (!AudioFocusLocks) {
		AudioLoseFocus();
	}
	AudioFocusLocks++;
}

void Audio_Focus_Regain(void)
{
	if (AudioFocusLocks) {
		AudioFocusLocks--;
		if (!AudioFocusLocks) {
			AudioRegainFocus();
		}
	}
}

void __fastcall AudioLevelFull(AudioLevel * level)
{
	AudioLevelAdjust(level, AUDIO_LEVEL_MAX);
}

void __fastcall AudioLevelMute(AudioLevel * level)
{
	AudioLevelAdjust(level, 0);
}

void __fastcall AudioLevelFadeTo(AudioLevel * level, int newLevel)
{
	AudioLevelAdjust(level, newLevel);
}

int __fastcall AudioLevelIsFading(AudioLevel * level)
{
	return level->level != level->newLevel;
}

void __fastcall AudioLevelWaitFade(AudioLevel * level)
{
	TimeStamp end = AudioGetTime() + level->duration;

	while (level->level != level->newLevel && AudioGetTime() < end) {
		Call_Back();
	}
}
