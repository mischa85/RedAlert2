// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2026 ra2-decomp contributors
// Copyright 2025 Electronic Arts Inc.
// Derived from Electronic Arts source code; EA's GPL Section 7 terms apply (COPYING.md).
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string.h>
#include "always.h"
#include "wwfile.h"
#include "rawfile.h"
#include "ccfile.h"
#include "audplay.h"
#include "audlist.h"
#include "audlock.h"
#include "audlevel.h"
#include "audattr.h"
#include "audsvc.h"
#include "audsrc.h"
#include "audchan.h"
#include "audstrm.h"
#include "auddev.h"
#include "conquer.h"
#include "dbgprint.h"

#define AS_PLAYING 0x01
#define AS_PAUSED 0x02
#define AS_FILL 0x04

#define STM_FAIL 0
#define STM_EOF 2





struct AudioStreamer
{
	ListNode nd;
	int flags;
	AudioDevice * device;
	AudioChannel * channel;
	TimeStamp buffering;
	TimeStamp end_time_stamp;
	TimeStamp start_time_stamp;
	STM_STREAM * stream;
	STM_ACCESS * in;
	STM_ACCESS * out;
	AudioSample sample;
	AudioFormat format;
	FileClass * file;
	Lock pause;
	int unused84;
	Lock lock;
	int unused8C;
	int pending_bytes;
	int total_bytes;
	int data_start;
	int bytes_left;
	int frame_size;
	char stream_name[80];
};

struct StreamThread
{
	volatile int quit;
	volatile int count;
	volatile int done;
	int unusedC;
	volatile TimeStamp interval;
	volatile int running;
	int unused1C;
	AudioServiceInfo info;
};



extern bool IsReadingAudioFile; // RA2: set while the streamer reads from a file

static DWORD WINAPI streamerThread(void * data);
static int __fastcall streamNextFrame(AudioChannel * channel);
static int __fastcall streamSampleDone(AudioChannel * channel);
static int __fastcall streamStop(AudioChannel * channel);

static StreamThread thread;
static HANDLE threadHandle;
static DWORD threadId;
static CRITICAL_SECTION streamLock;
static ListHead streams;
static int initialized;
static int threadCreated;

#define max(a, b) (((a) > (b)) ? (a) : (b))

static __forceinline int recalcBuffering(AudioStreamer * as, TimeStamp buffering, AudioFormat * format)
{
	unsigned size;
	STM_STREAM * stm;
	STM_SBUFFER * buffer;

	stm = as->stream;
	size = AudioFormatBytes(format, buffering);
	as->frame_size = AudioFormatBytes(format, as->channel->frameTime * 2);
	if (STM_StreamTotalBytes(stm) != size) {
		STM_StreamDestroyBuffers(stm);
		buffer = STM_SBufferCreate(size, 0);
		if (buffer) {
			STM_StreamAddSBuffer(stm, buffer);
		}
	}
	return STM_StreamTotalBytes(stm) == size;
}

static int streamFill(AudioStreamer * as, TimeStamp amount)
{
	int bytes;
	int transfered;
	int result;

	EnterCriticalSection(&streamLock);
	if (amount) {
		bytes = AudioFormatBytes(&as->format, amount);
		if (bytes > as->bytes_left) {
			bytes = as->bytes_left;
		}
	} else {
		bytes = as->bytes_left;
	}
	IsReadingAudioFile = true;
	result = STM_AccessFileTransfer(as->in, as->file, bytes, &transfered);
	IsReadingAudioFile = false;
	as->bytes_left -= transfered;
	if (as->bytes_left <= 0 || result == STM_EOF || result == STM_FAIL) {
		as->flags &= ~AS_FILL;
	}
	LeaveCriticalSection(&streamLock);
	return transfered;
}

static void streamStopChannel(AudioStreamer * as)
{
	if (NotLocked(&as->lock) && as->channel) {
		EnterCriticalSection(&streamLock);
		AudioChannelStop(as->channel);
		LeaveCriticalSection(&streamLock);
	}
}

int AudioStreamerInit(void)
{
	char buffer[1024];

	ListInit(&streams);
	InitializeCriticalSection(&streamLock);
	thread.quit = 0;
	thread.done = 0;
	thread.running = 0;
	thread.interval = 1000;
	thread.count = 0;
	AudioServiceInfoInit(&thread.info);
	AudioServiceSetInterval(&thread.info, thread.interval);
	AudioServiceSetMustServiceInterval(&thread.info, thread.interval * 4);
	AudioServiceSetResetInterval(&thread.info, thread.interval * 4);
	threadHandle = CreateThread(NULL, 0x1000, (LPTHREAD_START_ROUTINE)streamerThread, &thread, 0, &threadId);
	if (!threadHandle) {
		return 0;
	}
	if (!SetThreadPriority(threadHandle, THREAD_PRIORITY_HIGHEST)) {
		FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), 0, buffer, sizeof(buffer), NULL);
		DebugString("Unable to change the priority of the Streamer Thread - %s\n", buffer);
	}
	threadCreated = 1;
	initialized = 1;
	return 1;
}

static DWORD WINAPI streamerThread(void * data)
{
	StreamThread * t = (StreamThread *)data;
	AudioStreamer * as;

	t->running = 1;
	t->done = 0;
	while (!t->quit) {
		AudioServicePerform(&t->info, AudioGetTime());
		EnterCriticalSection(&streamLock);
		for (as = (AudioStreamer *)ListFirstItem(&streams); as; as = (AudioStreamer *)ListNextItem(&as->nd)) {
			if (as->flags & AS_FILL) {
				streamFill(as, 0);
			}
		}
		LeaveCriticalSection(&streamLock);
		Sleep((DWORD)t->interval);
		t->count++;
	}
	t->done = 1;
	return 0;
}

void AudioStreamerDeinit(void)
{
	AudioStreamer * as;

	if (!initialized) {
		return;
	}
	if (threadCreated) {
		thread.quit = 1;
		while (!thread.done) {
			Sleep(0);
		}
		WaitForSingleObject(threadHandle, 5000);
		CloseHandle(threadHandle);
		threadCreated = 0;
	}
	while ((as = (AudioStreamer *)ListFirstItem(&streams))) {
		streamStopChannel(as);
		AudioStreamerDestroy(as);
	}
	DeleteCriticalSection(&streamLock);
	initialized = 0;
}

AudioStreamer * __fastcall AudioStreamerCreate(AudioDevice * dev, TimeStamp buffering)
{
	AudioStreamer * as;

	if (!dev) {
		return NULL;
	}
	if (buffering <= 0 || buffering < SECONDS(7)) {
		buffering = SECONDS(7);
	}
	as = (AudioStreamer *)operator new(sizeof(AudioStreamer));
	memset(as, 0, sizeof(AudioStreamer));
	ListNodeInit(&as->nd);
	as->device = dev;
	as->channel = AudioDeviceReserveChannel(dev, 0x1000);
	as->channel->GroupAttribs = NULL;
	as->channel->SfxAttribs = NULL;
	as->channel->CompAttribs = NULL;
	as->channel->FadeAttribs = NULL;
	as->buffering = buffering;
	AudioSampleInit(&as->sample);
	AudioFormatInit(&as->format);
	LockInit(&as->pause);
	LockInit(&as->lock);
	strcpy(as->stream_name, "Audio Stream");
	if (!as->channel) {
		goto error;
	}
	as->channel->Data = as;
	as->channel->CB_NextFrame = streamNextFrame;
	as->channel->CB_SampleDone = streamSampleDone;
	as->channel->CB_Stop = streamStop;
	as->stream = STM_StreamCreate();
	if (!as->stream) {
		goto error;
	}
	EnterCriticalSection(&streamLock);
	ListNodeAppend(&streams, &as->nd);
	LeaveCriticalSection(&streamLock);
	return as;
error:
	AudioStreamerDestroy(as);
	return NULL;
}

static int __fastcall streamNextFrame(AudioChannel * channel)
{
	AudioStreamer * as = (AudioStreamer *)channel->Data;

	STM_AccessAdvance(as->out, as->pending_bytes);
	STM_AccessGetBlock(as->out);
	if (as->frame_size > as->out->Block.Bytes) {
		as->pending_bytes = as->out->Block.Bytes;
	} else {
		as->pending_bytes = as->frame_size;
	}
	channel->bytesInFrame = as->pending_bytes;
	channel->bytesRemaining = channel->bytesInFrame;
	channel->frameData = (char *)as->out->Block.Data;
	return 0;
}

static int __fastcall streamSampleDone(AudioChannel * channel)
{
	AudioStreamer * as = (AudioStreamer *)channel->Data;

	if (as->flags & (AS_PLAYING | AS_PAUSED)) {
		as->end_time_stamp = AudioGetTime();
	}
	as->flags &= ~(AS_PLAYING | AS_PAUSED | AS_FILL);
	LockInit(&as->pause);
	return 0;
}

static int __fastcall streamStop(AudioChannel * channel)
{
	AudioStreamer * as = (AudioStreamer *)channel->Data;

	if (as->flags & (AS_PLAYING | AS_PAUSED)) {
		as->end_time_stamp = AudioGetTime();
	}
	as->flags &= ~(AS_PLAYING | AS_PAUSED | AS_FILL);
	LockInit(&as->pause);
	return 0;
}

void __fastcall AudioStreamerDestroy(AudioStreamer * as)
{
	if (as->nd.prev != &as->nd) {
		EnterCriticalSection(&streamLock);
		ListNodeRemove(&as->nd);
		LeaveCriticalSection(&streamLock);
	}
	streamStopChannel(as);
	if (as->in) {
		STM_AccessRelease(as->in);
		as->in = NULL;
	}
	if (as->out) {
		STM_AccessRelease(as->out);
		as->out = NULL;
	}
	if (as->stream) {
		STM_StreamDestroy(as->stream);
	}
	if (as->channel) {
		AudioChannelRelease(as->channel);
	}
	if (as->file) {
		delete as->file;
	}
	operator delete(as);
}

void __fastcall AudioStreamerSetAttribs(AudioStreamer * as, AudioAttribs * attribs)
{
	as->channel->GroupAttribs = attribs;
}

void __fastcall AudioStreamerSetFadeAttribs(AudioStreamer * as, AudioAttribs * attribs)
{
	as->channel->FadeAttribs = attribs;
}

int __fastcall AudioStreamerPlayWAV(AudioStreamer * as, const char * filename, int flag)
{
	TimeStamp time;
	TimeStamp buf;

	if (Locked(&as->lock)) {
		return 0;
	}
	streamStopChannel(as);
	if (as->file) {
		delete as->file;
		as->file = NULL;
	}
	if (!flag) {
		as->file = new RawFileClass(filename);
		if (as->file && !as->file->Is_Available(0)) {
			delete as->file;
			as->file = NULL;
		}
	}
	if (!as->file) {
		as->file = new CCFileClass(filename);
	}
	if (!as->file) {
		goto error;
	}
	if (!as->file->Is_Available(0) || !as->file->Open(FileClass::READ)) {
		delete as->file;
		as->file = NULL;
		goto error;
	}
	if (!AudioFormatReadWaveFile(as->file, &as->format, &as->total_bytes)) {
		goto error;
	}
	as->bytes_left = as->total_bytes;
	as->data_start = as->file->Seek(0, SEEK_CUR);
	if (as->in) {
		STM_AccessRelease(as->in);
		as->in = NULL;
	}
	if (as->out) {
		STM_AccessRelease(as->out);
		as->out = NULL;
	}
	if (!recalcBuffering(as, as->buffering, &as->format)) {
		goto cleanup;
	}
	STM_StreamReset(as->stream);
	as->in = STM_StreamAcquireAccess(as->stream, 0);
	as->out = STM_StreamAcquireAccess(as->stream, 1);
	if (!as->in || !as->out) {
		goto cleanup;
	}
	buf = as->channel->bufferTime * 2;
	time = max(buf, thread.interval * 4);
	time = max(time, (TimeStamp)SECONDS(4));
	if (!streamFill(as, time)) {
		goto cleanup;
	}
	STM_AccessGetBlock(as->out);
	as->sample.Data = (char *)as->out->Block.Data;
	as->sample.Bytes = AudioFormatBytes(&as->format, as->channel->frameTime);
	if (as->sample.Bytes > as->out->Block.Bytes) {
		as->sample.Bytes = as->out->Block.Bytes;
	}
	as->sample.Format = &as->format;
	as->pending_bytes = as->sample.Bytes;
	AudioLevelSet(&as->channel->ChannelAttribs.VolumeLevel, AUDIO_VOLUME_MAX);
	AudioChannelSetSample(as->channel, &as->sample);
	as->flags |= AS_PLAYING;
	if (AudioChannelStart(as->channel)) {
		as->flags &= ~AS_PLAYING;
		STM_AccessRelease(as->in);
		as->in = NULL;
		STM_AccessRelease(as->out);
		as->out = NULL;
		goto cleanup;
	}
	as->start_time_stamp = AudioGetTime();
	as->end_time_stamp = as->start_time_stamp;
	as->flags |= AS_FILL;
	return 1;
cleanup:
error:
	if (as->in) {
		STM_AccessRelease(as->in);
		as->in = NULL;
	}
	if (as->out) {
		STM_AccessRelease(as->out);
		as->out = NULL;
	}
	return 0;
}

void __fastcall AudioStreamerStop(AudioStreamer * as)
{
	streamStopChannel(as);
}

void __fastcall AudioStreamerLock(AudioStreamer * as)
{
	LockAcquire(&as->lock);
}

void __fastcall AudioStreamerUnlock(AudioStreamer * as)
{
	LockRelease(&as->lock);
}

void __fastcall AudioStreamerPause(AudioStreamer * as)
{
	if (NotLocked(&as->pause) && as->channel) {
		EnterCriticalSection(&streamLock);
		AudioChannelPause(as->channel);
		if (as->flags & AS_PLAYING) {
			as->flags = (as->flags & ~AS_PLAYING) | AS_PAUSED;
		}
		LeaveCriticalSection(&streamLock);
	}
	LockAcquire(&as->pause);
}

void __fastcall AudioStreamerResume(AudioStreamer * as)
{
	if (Locked(&as->pause)) {
		LockRelease(&as->pause);
		if (NotLocked(&as->pause) && as->channel) {
			EnterCriticalSection(&streamLock);
			if (as->flags & AS_PAUSED) {
				as->flags = (as->flags & ~AS_PAUSED) | AS_PLAYING;
			}
			AudioChannelResume(as->channel);
			LeaveCriticalSection(&streamLock);
		}
	}
}

int __fastcall AudioStreamerIsPlaying(AudioStreamer * as)
{
	return as->flags & AS_PLAYING;
}

void __fastcall AudioStreamerSetName(AudioStreamer * as, const char * name)
{
	strncpy(as->stream_name, name, sizeof(as->stream_name));
	as->stream_name[sizeof(as->stream_name) - 1] = 0;
}

const char * __fastcall AudioStreamerName(AudioStreamer * as)
{
	return as->stream_name;
}

void __fastcall AudioStreamerFadeIn(AudioStreamer * as)
{
	AudioLevelAdjust(&as->channel->ChannelAttribs.VolumeLevel, AUDIO_VOLUME_MAX);
}

void __fastcall AudioStreamerFadeOut(AudioStreamer * as)
{
	AudioLevelAdjust(&as->channel->ChannelAttribs.VolumeLevel, 0);
}

int __fastcall AudioStreamerIsFading(AudioStreamer * as)
{
	return (as->flags & AS_PLAYING) && as->channel->ChannelAttribs.VolumeLevel.level != as->channel->ChannelAttribs.VolumeLevel.newLevel;
}

void __fastcall AudioStreamerWaitForFade(AudioStreamer * as)
{
	TimeStamp time_out = AudioGetTime() + as->channel->ChannelAttribs.VolumeLevel.duration;

	while (AudioStreamerIsFading(as) && AudioGetTime() < time_out) {
		Call_Back();
	}
}

TimeStamp __fastcall AudioStreamerEndTimeStamp(AudioStreamer * as)
{
	return as->end_time_stamp;
}

void AudioStreamerStarve(void)
{
	AudioStreamer * as;

	if (!initialized) {
		return;
	}
	as = (AudioStreamer *)ListFirstItem(&streams);
	EnterCriticalSection(&streamLock);
	while (as) {
		as->flags &= ~AS_FILL;
		as = (AudioStreamer *)ListNextItem(&as->nd);
	}
	LeaveCriticalSection(&streamLock);
}

void AudioStreamerStopAllStreams(void)
{
	AudioStreamer * as;

	if (!initialized) {
		return;
	}
	as = (AudioStreamer *)ListFirstItem(&streams);
	while (as) {
		streamStopChannel(as);
		as = (AudioStreamer *)ListNextItem(&as->nd);
	}
}

void AudioStreamerLockAllStreams(void)
{
	AudioStreamer * as;

	if (!initialized) {
		return;
	}
	as = (AudioStreamer *)ListFirstItem(&streams);
	while (as) {
		LockAcquire(&as->lock);
		as = (AudioStreamer *)ListNextItem(&as->nd);
	}
}

void AudioStreamerUnlockAllStreams(void)
{
	AudioStreamer * as;

	if (!initialized) {
		return;
	}
	as = (AudioStreamer *)ListFirstItem(&streams);
	while (as) {
		LockRelease(&as->lock);
		as = (AudioStreamer *)ListNextItem(&as->nd);
	}
}

void AudioStreamerPauseAllStreams(void)
{
	AudioStreamer * as;

	if (!initialized) {
		return;
	}
	as = (AudioStreamer *)ListFirstItem(&streams);
	while (as) {
		AudioStreamerPause(as);
		as = (AudioStreamer *)ListNextItem(&as->nd);
	}
}

void AudioStreamerResumeAllStreams(void)
{
	AudioStreamer * as;

	if (!initialized) {
		return;
	}
	as = (AudioStreamer *)ListFirstItem(&streams);
	while (as) {
		AudioStreamerResume(as);
		as = (AudioStreamer *)ListNextItem(&as->nd);
	}
}

void AudioStreamerFadeOutAllStreams(void)
{
	AudioStreamer * as;

	if (!initialized) {
		return;
	}
	as = (AudioStreamer *)ListFirstItem(&streams);
	while (as) {
		AudioStreamerFadeOut(as);
		as = (AudioStreamer *)ListNextItem(&as->nd);
	}
}

void AudioStreamerFadeInAllStreams(void)
{
	AudioStreamer * as;

	if (!initialized) {
		return;
	}
	as = (AudioStreamer *)ListFirstItem(&streams);
	while (as) {
		AudioStreamerFadeIn(as);
		as = (AudioStreamer *)ListNextItem(&as->nd);
	}
}

int AudioStreamerAllFaded(void)
{
	AudioStreamer * as;

	if (!initialized) {
		return 1;
	}
	as = (AudioStreamer *)ListFirstItem(&streams);
	while (as) {
		if (AudioStreamerIsFading(as)) {
			return 0;
		}
		as = (AudioStreamer *)ListNextItem(&as->nd);
	}
	return 1;
}
