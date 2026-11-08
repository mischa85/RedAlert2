// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2026 ra2-decomp contributors
// Copyright 2025 Electronic Arts Inc.
// Derived from Electronic Arts source code; EA's GPL Section 7 terms apply (COPYING.md).
#include "always.h"
#include "audbag.h"
#include "ccfile.h"
#include <windows.h>
#include <string.h>
#include <stdlib.h>

AudioIDXEntry * __fastcall AudioIDXData::GetEntry(int index)
{
	return &Samples[index];
}

AudioIDXData * __fastcall AudioIDXData::Create(char const * filename, char const * path)
{
	AudioIDXData * idx;
	char name[260];
	char * ext;
	struct {
		unsigned Magic;
		unsigned Version;
		int Count;
	} header;

	idx = new AudioIDXData;
	memset(idx, 0, sizeof(AudioIDXData));
	strcpy(name, filename);

	if ((ext = strrchr(name, '.'))) {
		*ext = 0;
	}
	strcat(name, ".idx");

	CCFileClass idxFile(name);

	if (!idxFile.Is_Available(0)) {
		goto fail;
	}

	if ((ext = strrchr(name, '.'))) {
		*ext = 0;
	}
	strcat(name, ".bag");

	idx->BagFile = new CCFileClass(name);

	if (!idx->BagFile->Is_Available(0)) {
		goto fail;
	}

	idxFile.Open(FileClass::READ);
	idx->BagFile->Open(FileClass::READ);

	if (idxFile.Read(&header, 12) != 12) {
		goto fail;
	}

	idx->SampleCount = header.Count;

	if (idx->SampleCount > 0) {
		idx->Samples = new AudioIDXEntry[idx->SampleCount];
		memset(idx->Samples, 0, idx->SampleCount * sizeof(AudioIDXEntry));

		if (header.Version == 1) {
			int i;
			for (i = 0; i < idx->SampleCount; i++) {
				if (idxFile.Read(&idx->Samples[i], 0x20) != 0x20) {
					goto fail;
				}
				idx->Samples[i].ChunkSize = 0;
			}
		} else if (idxFile.Read(idx->Samples, idx->SampleCount * sizeof(AudioIDXEntry)) != idx->SampleCount * (int)sizeof(AudioIDXEntry)) {
			goto fail;
		}
	}

	qsort(idx->Samples, idx->SampleCount, sizeof(AudioIDXEntry), (int (__cdecl *)(void const *, void const *))stricmp);

	idx->PathFound = 0;

	if (path) {
		WIN32_FIND_DATA find;
		HANDLE handle;

		strcpy(idx->Path, path);
		if (idx->Path[strlen(idx->Path)] == '\\') {
			idx->Path[strlen(idx->Path)] = 0;
		}

		handle = FindFirstFile(idx->Path, &find);

		if (handle != INVALID_HANDLE_VALUE) {
			do {
				if (find.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
					idx->PathFound = 1;
					break;
				}
			} while (FindNextFile(handle, &find));
			FindClose(handle);
		}

		if (idx->Path[strlen(idx->Path)] != '\\') {
			strcat(idx->Path, "\\");
		}
	}

	return idx;

fail:
	if (idx) {
		idx->Destroy();
	}
	return 0;
}

void AudioIDXData::Destroy(void)
{
	if (BagFile) {
		delete BagFile;
	}
	if (ExternalFile) {
		delete ExternalFile;
	}
	if (Samples) {
		delete Samples;
	}
	delete this;
}

int __fastcall AudioIDXData::FindSampleIndex(char const * name)
{
	AudioIDXEntry * found = (AudioIDXEntry *)bsearch(name, Samples, SampleCount, sizeof(AudioIDXEntry), (int (__cdecl *)(void const *, void const *))stricmp);

	if (found) {
		return (unsigned)((char *)found - (char *)Samples) / sizeof(AudioIDXEntry);
	}
	return -1;
}

char const * __fastcall AudioIDXData::GetSampleName(int index)
{
	if (index < 0 || index >= SampleCount) {
		return "Invalid";
	}
	return Samples[index].Name;
}

int __fastcall AudioIDXData::GetSampleSize(int index)
{
	if (index < 0 || index >= SampleCount) {
		return 0;
	}
	return Samples[index].Size;
}

AudioFormat * __fastcall AudioIDXData::GetSampleInformation(int index, AudioFormat * format)
{
	AudioIDXEntry * entry = &Samples[index];

	format->Flags = mAUDIO_FORMAT_PCM;
	format->Rate = entry->SampleRate;
	format->Channels = (entry->Flags & 1) ? 2 : 1;
	format->BlockSize = entry->ChunkSize;

	if (entry->Flags & 8) {
		format->Compression = AUDIO_COMPRESS_IMA_ADPCM;
		format->SampleWidth = 2;
	} else {
		format->Compression = AUDIO_COMPRESS_NONE;
		format->SampleWidth = (entry->Flags & 4) ? 2 : 1;
	}
	return format;
}

void __fastcall AudioIDXData::SetSampleInformation(int index, AudioFormat * format)
{
	AudioIDXEntry * entry = &Samples[index];

	entry->SampleRate = format->Rate;
	entry->Flags = 0;

	if (format->Channels == 2) {
		entry->Flags = 1;
	}
	if (format->SampleWidth == 2) {
		entry->Flags |= 4;
	}
}

int __fastcall AudioIDXData::OpenSample(int index)
{
	AudioIDXEntry * entry = &Samples[index];
	int ok = 1;
	AudioFormat format;
	int size;

	ClearCurrentSample();

	CurrentSampleFile = (RawFileClass *)BagFile;
	CurrentSampleSize = entry->Size;

	if (CurrentSampleFile->Seek(entry->Offset, 0) != entry->Offset) {
		ok = 0;
	}
	if (entry->Size == 0) {
		ok = 0;
	}

	if (PathFound) {
		char file[260];

		strcpy(file, Path);
		strcat(file, entry->Name);
		strcat(file, ".wav");

		RawFileClass * external = new RawFileClass(file);
		ExternalFile = external;

		if (external->Is_Available(0) && external->Open(FileClass::READ)) {
			AudioFormatInit(&format);

			if (AudioFormatReadWaveFile(external, &format, &size)) {
				entry->Flags = 2;
				if (format.SampleWidth == 2) {
					entry->Flags |= 4;
				}
				if (format.Channels == 2) {
					entry->Flags |= 1;
				}
				entry->SampleRate = format.Rate;
				CurrentSampleSize = size;
				CurrentSampleFile = ExternalFile;
				return 1;
			}
		}
		if (ExternalFile) {
			delete ExternalFile;
			ExternalFile = 0;
		}
	}
	return ok;
}

int __fastcall AudioIDXData::ReadSample(void * buffer, int bytes)
{
	int done;

	if (!CurrentSampleFile) {
		return 0;
	}
	if (bytes > CurrentSampleSize) {
		bytes = CurrentSampleSize;
	}
	done = bytes > 0 ? CurrentSampleFile->Read(buffer, bytes) : 0;
	if (done < 0) {
		done = 0;
	}
	CurrentSampleSize -= done;
	return done;
}

void AudioIDXData::ClearCurrentSample(void)
{
	CurrentSampleFile = 0;
	if (ExternalFile) {
		delete ExternalFile;
		ExternalFile = 0;
	}
}

// RawFileClass::File_Name

// CCFileClass::Open
