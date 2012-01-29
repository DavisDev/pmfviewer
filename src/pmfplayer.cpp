/*
 *	PMF Player Module
 *	Copyright (c) 2006 by Sorin P. C. <magik@hypermagik.com>
 *	Copyright (c) 2011 Musa Mitchell <mowglisanu@gmail.com>
 */
#include <pspkernel.h>
#include <pspsdk.h>
#include <pspmpeg.h>
#include <pspaudio.h>

#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <iostream>
using namespace std;

#include "pmfplayer.h"

CPMFPlayer::CPMFPlayer(bool autoVideo)
{
	m_RingbufferData	= NULL;
	m_MpegMemData		= NULL;

	m_FileHandle		= -1;

	m_MpegStreamAVC		= NULL;
	m_MpegStreamAtrac	= NULL;

	m_pEsBufferAVC		= NULL;
	m_pEsBufferAtrac	= NULL;
	m_AutoVideo 		= autoVideo;
}

CPMFPlayer::~CPMFPlayer(void)
{
}

char *CPMFPlayer::GetLastError()
{
	return m_LastError;
}

SceInt32 RingbufferCallback(ScePVoid pData, SceInt32 iNumPackets, ScePVoid pParam)
{
	int retVal, iPackets;
	SceUID hFile = *(SceUID*)pParam;

	retVal = sceIoRead(hFile, pData, iNumPackets * 2048);
	if(retVal < 0)
		return -1;

	iPackets = retVal / 2048;

	return iPackets;
}

SceInt32 CPMFPlayer::Initialize(SceInt32 nPackets)
{
	int retVal = -1;
	m_ThreadID = -1;

	m_RingbufferPackets = nPackets;

	retVal = sceMpegInit();
	if(retVal != 0)
	{
		sprintf(m_LastError, "sceMpegInit() failed: 0x%08X", retVal);
		goto error;
	}

	retVal = sceMpegRingbufferQueryMemSize(m_RingbufferPackets);
	if(retVal < 0)
	{
		sprintf(m_LastError, "sceMpegRingbufferQueryMemSize(%d) failed: 0x%08X", (int)nPackets, retVal);
		goto finish;
	}

	m_RingbufferSize = retVal;

	retVal = sceMpegQueryMemSize(0);
	if(retVal < 0)
	{
		sprintf(m_LastError, "sceMpegQueryMemSize() failed: 0x%08X", retVal);
		goto finish;
	}

	m_MpegMemSize = retVal;

	m_RingbufferData = malloc(m_RingbufferSize);
	if(m_RingbufferData == NULL)
	{
		sprintf(m_LastError, "malloc() failed!");
		goto finish;
	}

	m_MpegMemData = malloc(m_MpegMemSize);
	if(m_MpegMemData == NULL)
	{
		sprintf(m_LastError, "malloc() failed!");
		goto freeringbuffer;
	}

	retVal = sceMpegRingbufferConstruct(&m_Ringbuffer, m_RingbufferPackets, m_RingbufferData, m_RingbufferSize, &RingbufferCallback, &m_FileHandle);
	if(retVal != 0)
	{
		sprintf(m_LastError, "sceMpegRingbufferConstruct() failed: 0x%08X", retVal);
		goto freempeg;
	}

	retVal = sceMpegCreate(&m_Mpeg, m_MpegMemData, m_MpegMemSize, &m_Ringbuffer, BUFFER_WIDTH, 0, 0);
	if(retVal != 0)
	{
		sprintf(m_LastError, "sceMpegCreate() failed: 0x%08X", retVal);
		goto destroyringbuffer;
	}

	SceMpegAvcMode m_MpegAvcMode;
	m_MpegAvcMode.iUnk0 = -1;
	m_MpegAvcMode.iPixelFormat = SCE_MPEG_AVC_FORMAT_8888;

	sceMpegAvcDecodeMode(&m_Mpeg, &m_MpegAvcMode);

	return 0;

destroyringbuffer:
	sceMpegRingbufferDestruct(&m_Ringbuffer);

freempeg:
	free(m_MpegMemData);

freeringbuffer:
	free(m_RingbufferData);

finish:
	sceMpegFinish();

error:
	return -1;
}

SceInt32 CPMFPlayer::Load(char* pFileName)
{
	int retVal;
	strcpy(m_FileName, pFileName);
	m_FileHandle = sceIoOpen(m_FileName, PSP_O_RDONLY, 0777);
	if(m_FileHandle < 0)
	{
		sprintf(m_LastError, "sceIoOpen() failed! 0x%x", m_FileHandle);
		return -1;
	}

	if(ParseHeader() < 0)
		return -1;

	m_MpegStreamAVC = sceMpegRegistStream(&m_Mpeg, 0, 0);
	if(m_MpegStreamAVC == NULL)
	{
		sprintf(m_LastError, "sceMpegRegistStream() failed!");
		return -1;
	}

	m_MpegStreamAtrac = sceMpegRegistStream(&m_Mpeg, 1, 0);
	if(m_MpegStreamAtrac == NULL)
	{
		sprintf(m_LastError, "sceMpegRegistStream() failed!");
		return -1;
	}

	m_pEsBufferAVC = sceMpegMallocAvcEsBuf(&m_Mpeg);
	if(m_pEsBufferAVC == NULL)
	{
		sprintf(m_LastError, "sceMpegMallocAvcEsBuf() failed!");
		return -1;
	}

	retVal = sceMpegInitAu(&m_Mpeg, m_pEsBufferAVC, &m_MpegAuAVC);
	if(retVal != 0)
	{
		sprintf(m_LastError, "sceMpegInitAu() failed: 0x%08X", retVal);
		return -1;
	}

	retVal = sceMpegQueryAtracEsSize(&m_Mpeg, &m_MpegAtracEsSize, &m_MpegAtracOutSize);
	if(retVal != 0)
	{
		sprintf(m_LastError, "sceMpegQueryAtracEsSize() failed: 0x%08X", retVal);
		return -1;
	}

	m_pEsBufferAtrac = memalign(64, m_MpegAtracEsSize);
	if(m_pEsBufferAtrac == NULL)
	{
		sprintf(m_LastError, "malloc() failed!");
		return -1;
	}

	retVal = sceMpegInitAu(&m_Mpeg, m_pEsBufferAtrac, &m_MpegAuAtrac);
	if(retVal != 0)
	{
		sprintf(m_LastError, "sceMpegInitAu() failed: 0x%08X", retVal);
		return -1;
	}

	return 0;
}

SceInt32 CPMFPlayer::ParseHeader()
{
	int retVal;
	char * pHeader = new char[2048];

	sceIoLseek(m_FileHandle, 0, SEEK_SET);

	retVal = sceIoRead(m_FileHandle, pHeader, 2048);
	if(retVal < 2048)
	{
		sprintf(m_LastError, "sceIoRead() failed!");
		goto error;
	}

	retVal = sceMpegQueryStreamOffset(&m_Mpeg, pHeader, &m_MpegStreamOffset);
	if(retVal != 0)
	{
		sprintf(m_LastError, "sceMpegQueryStreamOffset() failed: 0x%08X", retVal);
		goto error;
	}

	retVal = sceMpegQueryStreamSize(pHeader, &m_MpegStreamSize);
	if(retVal != 0)
	{
		sprintf(m_LastError, "sceMpegQueryStreamSize() failed: 0x%08X", retVal);
		goto error;
	}

	m_iLastTimeStamp = *(int*)(pHeader + 80 + 12);
	m_iLastTimeStamp = SWAPINT(m_iLastTimeStamp);
	
	m_MpegVideoWidth = *(pHeader + 0x8E) << 4;
	m_MpegVideoHeight = *(pHeader + 0x8F) << 4;

	delete[] pHeader;

	sceIoLseek(m_FileHandle, m_MpegStreamOffset, SEEK_SET);

	return 0;

error:
	delete[] pHeader;
	return -1;
}

SceVoid CPMFPlayer::Shutdown()
{
	if(m_pEsBufferAtrac != NULL)
		free(m_pEsBufferAtrac);

	if(m_pEsBufferAVC != NULL)
		sceMpegFreeAvcEsBuf(&m_Mpeg, m_pEsBufferAVC);

	if(m_MpegStreamAVC != NULL)
		sceMpegUnRegistStream(&m_Mpeg, m_MpegStreamAVC);

	if(m_MpegStreamAtrac != NULL)
		sceMpegUnRegistStream(&m_Mpeg, m_MpegStreamAtrac);

	if(m_FileHandle > -1)
		sceIoClose(m_FileHandle);

	sceMpegDelete(&m_Mpeg);

	sceMpegRingbufferDestruct(&m_Ringbuffer);

	sceMpegFinish();

	if(m_RingbufferData != NULL)
		free(m_RingbufferData);

	if(m_MpegMemData != NULL)
		free(m_MpegMemData);
	
}
SceBool CPMFPlayer::IsFinished(){
	return m_ThreadID == -1;
}
SceBool CPMFPlayer::IsPaused(){
	return Reader.m_Pause;
}
SceVoid CPMFPlayer::ThreadCleanUp(){
	ShutdownDecoder();
	ShutdownAudio();
	ShutdownVideo();
	ShutdownReader();
	m_ThreadID = -1;
}
int T_Player(SceSize _args, void *_argp){
	PlayerThreadArgs* args = (PlayerThreadArgs*)_argp;

	sceKernelStartThread(args->Reader_ThreadID,  sizeof(void*), &args->TDR);
	sceKernelStartThread(args->Audio_ThreadID,   sizeof(void*), &args->TDD);
	sceKernelStartThread(args->Video_ThreadID,   sizeof(void*), &args->TDD);
	sceKernelStartThread(args->Decoder_ThreadID, sizeof(void*), &args->TDD);

	sceKernelWaitThreadEnd(args->Reader_ThreadID,  0);
	sceKernelWaitThreadEnd(args->Audio_ThreadID,   0);
	sceKernelWaitThreadEnd(args->Video_ThreadID,   0);
	sceKernelWaitThreadEnd(args->Decoder_ThreadID, 0);

	args->player->ThreadCleanUp();
	sceKernelExitDeleteThread(0);
	return 0;
}
SceInt32 CPMFPlayer::Play()
{
	Reader.m_Pause = 0;
	if (m_ThreadID < 0){
		int retVal, fail = 0;
		//ReaderThreadData* TDR = Reader;
		//DecoderThreadData* TDD = Decoder;
	
		retVal = InitReader();
		if(retVal < 0)
		{
			fail++;
			goto exit_reader;
		}
		
		retVal = InitVideo();
		if(retVal < 0)
		{
			fail++;
			goto exit_video;
		}
	
		retVal = InitAudio();
		if(retVal < 0)
		{
			fail++;
			goto exit_audio;
		}
	
		retVal = InitDecoder();
		if(retVal < 0)
		{
			fail++;
			goto exit_decoder;
		}

		m_ThreadID = sceKernelCreateThread("playerer_thread", T_Player, 0x10, 0xFA0, PSP_THREAD_ATTR_USER, NULL);
		if (m_ThreadID < 0){
			sprintf(m_LastError, "sceKernelCreateThread() failed: 0x%08X", (int)m_ThreadID);
		}
		else{
			PlayerThreadArgs args = {this->Reader.m_ThreadID, this->Audio.m_ThreadID, this->Video.m_ThreadID, this->Decoder.m_ThreadID, &Reader, &Decoder, this};
			sceKernelStartThread(m_ThreadID,  sizeof(args), &args);
			return 0;
		}
		
		ShutdownDecoder();
exit_decoder:
		ShutdownAudio();
exit_audio:
		ShutdownVideo();
exit_video:
		ShutdownReader();
exit_reader:
		return -1;
	}
	return 0;
}
SceVoid CPMFPlayer::Stop(){
	Reader.m_Status = ReaderThreadData::READER_ABORT;
	if (m_ThreadID > 0){
		sceKernelWaitThreadEnd(m_ThreadID, 0);
	}
}
SceVoid CPMFPlayer::Suspend(){
	if (!Reader.m_Suspended){
		Reader.m_Pause  = 1;
		Reader.m_Suspended = true;
		m_FileOffset = sceIoLseek(m_FileHandle, 0, SEEK_CUR);
	}
}
SceVoid CPMFPlayer::Resume(){
	if (Reader.m_Suspended){
		Reader.m_Pause  = 1;
		m_FileHandle = sceIoOpen(m_FileName, PSP_O_RDONLY, 0777);
		if (m_FileHandle < 0){
			sprintf(m_LastError, "%s 0x%x", "sceIoOpen() failed! ", m_FileHandle);
			Reader.m_Status = ReaderThreadData::READER_ABORT;
		}
		else{
			int seek = sceIoLseek(m_FileHandle, m_FileOffset, SEEK_SET);
		}
		Reader.m_Suspended = false;
	}
}


SceVoid CPMFPlayer::Pause(){
	Reader.m_Pause = 1;
}
SceVoid CPMFPlayer::Faster(){
	SceBool bk = Reader.m_Pause;
	Reader.m_Pause = 1;
	switch (Video.m_speed){
		case 0://slower
			while (sceAudioOutput2GetRestSample())
				sceKernelDelayThread(5);
			sceAudioSRCChRelease();
			sceAudioSRCChReserve(m_MpegAtracOutSize/4, 44100, 2);
			Video.m_speed = 1;
			Audio.m_speed = 1;
		break;
		case 1://normal
			while (sceAudioOutput2GetRestSample())
				sceKernelDelayThread(8);
			sceAudioSRCChRelease();
			sceAudioSRCChReserve(m_MpegAtracOutSize/8, 44100, 2);
			Video.m_speed = 2;
			Audio.m_speed = 2;
			Audio.m_AudioSamples = m_MpegAtracOutSize/8;
		break;
		case 2://faster
		break;
	}
	Reader.m_Pause = bk;
}
SceVoid CPMFPlayer::Slower(){
	SceBool bk = Reader.m_Pause;
	Reader.m_Pause = 1;
	switch (Video.m_speed){
		case 0://slower
		break;
		case 1://normal
			while (sceAudioOutput2GetRestSample())
				sceKernelDelayThread(8);
			sceAudioSRCChRelease();
			sceAudioSRCChReserve(m_MpegAtracOutSize/4, 22050, 2);
			Video.m_speed = 0;
			Audio.m_speed = 0;
		break;
		case 2://faster
			while (sceAudioOutput2GetRestSample())
				sceKernelDelayThread(5);
			sceAudioSRCChRelease();
			sceAudioSRCChReserve(m_MpegAtracOutSize/4, 44100, 2);
			Video.m_speed = 1;
			Audio.m_speed = 1;
		break;
	}
	Reader.m_Pause = bk;
}
SceVoid* CPMFPlayer::GetTexture(){//may need syncing and stuff
	if (Video.m_iFullBuffers)
		return Video.m_pVideoBuffer[Video.m_iPlayBuffer];
	return NULL;
}
SceVoid CPMFPlayer::UnlockTexture(){
	if (Video.m_iFullBuffers)
		Video.m_iFullBuffers--;
}
SceInt32 CPMFPlayer::GetWidth(){
	return Video.m_iWidth;
}
SceInt32 CPMFPlayer::GetHeight(){
	return Video.m_iHeight;
}
SceInt32 CPMFPlayer::GetTexWidth(){
	return Video.m_iTexWidth;
}
SceInt32 CPMFPlayer::GetTexHeight(){
	return Video.m_iTexHeight;
}
