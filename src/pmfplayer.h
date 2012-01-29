/*
 *	PMF Player Module
 *	Copyright (c) 2006 by Sorin P. C. <magik@hypermagik.com>
 */
#ifndef __PMFPLAYER_H__
#define __PMFPLAYER_H__

#include <pspkernel.h>
#include "pspmpeg.h"
#include <psptypes.h>

#include "decoder.h"
#include "reader.h"
#include "audio.h"
#include "video.h"

class CPMFPlayer
{

public:

	CPMFPlayer(bool autoVideo);
	~CPMFPlayer(void);

	char*				GetLastError();
	SceInt32			Initialize(SceInt32 nPackets = 0x3C0);
	SceInt32			Load(char* pFileName);
	SceInt32			Play();
	SceVoid				Shutdown();
	
	SceVoid				Suspend();
	SceVoid				Resume();
	
	SceBool				IsFinished();

	SceVoid				ThreadCleanUp();
	SceVoid				Pause();
	SceBool				IsPaused();
	SceVoid				Stop();
	SceVoid				Slower();
	SceVoid				Faster();
	SceVoid*			GetTexture();	
	SceVoid				UnlockTexture();
	SceInt32			GetWidth();
	SceInt32			GetHeight();
	SceInt32			GetTexWidth();
	SceInt32			GetTexHeight();

private:

	SceUID				m_ThreadID;

	SceBool				m_AutoVideo;

	SceInt32 			ParseHeader();

	char				m_LastError[256];
	
	char				m_FileName[256];
	SceInt32			m_FileOffset;

	SceUID				m_FileHandle;
	SceInt32			m_MpegStreamOffset;
	SceInt32			m_MpegStreamSize;
	SceInt32			m_MpegVideoWidth;
	SceInt32			m_MpegVideoHeight;

	SceFloat32			m_fWidthScale;
	SceFloat32			m_fHeightScale;
	
	SceMpeg				m_Mpeg;
	SceInt32			m_MpegMemSize;
	ScePVoid			m_MpegMemData;

	SceInt32			m_RingbufferPackets;
	SceInt32			m_RingbufferSize;
	ScePVoid			m_RingbufferData;
	SceMpegRingbuffer	m_Ringbuffer;

	SceMpegStream*		m_MpegStreamAVC;
	ScePVoid			m_pEsBufferAVC;
	SceMpegAu			m_MpegAuAVC;

	SceMpegStream*		m_MpegStreamAtrac;
	ScePVoid			m_pEsBufferAtrac;
	SceMpegAu			m_MpegAuAtrac;

	SceInt32			m_MpegAtracEsSize;
	SceInt32			m_MpegAtracOutSize;

	SceInt32			m_iLastTimeStamp;

	DecoderThreadData	Decoder;
	SceInt32			InitDecoder();
	SceInt32			ShutdownDecoder();

	ReaderThreadData	Reader;
	SceInt32			InitReader();
	SceInt32			ShutdownReader();

	VideoThreadData		Video;
	SceInt32			InitVideo();
	SceInt32			ShutdownVideo();

	AudioThreadData		Audio;
	SceInt32			InitAudio();
	SceInt32			ShutdownAudio();
};
typedef struct{
	SceUID Reader_ThreadID,
	       Audio_ThreadID, 
	       Video_ThreadID, 
	       Decoder_ThreadID;
	ReaderThreadData* TDR;
	DecoderThreadData* TDD;
	CPMFPlayer* player;
}PlayerThreadArgs;	
	
#define SWAPINT(x) (((x)<<24) | (((uint)(x)) >> 24) | (((x) & 0x0000FF00) << 8) | (((x) & 0x00FF0000) >> 8))

#endif
