/*
 *	PMF Player Module
 *	Copyright (c) 2006 by Sorin P. C. <magik@hypermagik.com>
 */
#include "pmfplayer.h"
#include <psputilsforkernel.h>
#include <pspdisplay.h>
#include <pspge.h>
#include <pspgu.h>

#include <cstdio>
#include <cstring>

static unsigned int __attribute__((aligned(64)))  DisplayList[128 * 1024 * 4];

__attribute__ ((noinline))SceInt32  nextPow2(int i){
	asm volatile (".set noreorder\n"
				  "clz $3, $4\n"
				  "bitrev $5, $4\n"
				  "clz $2, $5\n"
				  "addu $6, $2, $3\n"
				  "sltiu $6, $6, 31\n"
				  "beqz $6, L1\n"
				  "move $2, $4\n"
				  "li $6, 1\n"
				  "li $7, 32\n"
				  "sub $7, $7, $3\n"
				  "sllv $2, $6, $7\n"
				  "L1:\n"
				  ".set reorder\n");
}
SceInt32 AVSyncStatus(DecoderThreadData* D)
{
	if(D->Audio->m_iFullBuffers == 0 || D->Video->m_iFullBuffers == 0)
		return 1;

	int iAudioTS = D->Audio->m_iBufferTimeStamp[D->Audio->m_iPlayBuffer];
	int iVideoTS = D->Video->m_iBufferTimeStamp[D->Video->m_iPlayBuffer];

	// if video ahead of audio, do nothing
	if(iVideoTS - iAudioTS > 2 * D->m_iVideoFrameDuration)
		return 0;

	// if audio ahead of video, skip frame
	if(iAudioTS - iVideoTS > 2 * D->m_iVideoFrameDuration)
		return 2;

	return 1;
}
void copySlow(int *slow){
	memset (slow, 0, 64*32*4);
	for (int i = 0; i < 20; i++){
		for (int j = 0; j < 10; j++){
			slow[i*64+j] = -1;			
		}
	}
	for (int i = 0; i < 10; i++){
		for (int j = 0; j < i; j++){
			slow[i*64+(j*2)+15] = -1;			
			slow[i*64+(j*2+1)+15] = -1;			
		}
	}
	for (int i = 0; i < 10; i++){
		for (int j = 9-i; j >= 0; j--){
			slow[(i+10)*64+(j*2)+15] = -1;			
			slow[(i+10)*64+(j*2+1)+15] = -1;			
		}
	}
}
int RenderFrame(int locx, int locy, int width, int height, int texWidth, int texHeight, float s, void* Buffer, int speed)
{

	sceGuStart(GU_DIRECT, DisplayList);
	sceGuClearColor(0);
	sceGuClear(GU_COLOR_BUFFER_BIT);

	struct Vertex* Vertices = (struct Vertex*)sceGuGetMemory(2 * sizeof(struct Vertex));

	Vertices[0].u = 0;				Vertices[0].v = 0;
	Vertices[0].x = locx;			Vertices[0].y = locy;				Vertices[0].z = 0;
	Vertices[1].u = width;			Vertices[1].v = height;
	Vertices[1].x = (width*s)+locx;	Vertices[1].y = (height*s)+locy;	Vertices[1].z = 0;

	sceGuTexImage(0, texWidth, texHeight, texWidth, Buffer);

	sceGuDrawArray(GU_SPRITES, GU_TEXTURE_16BIT|GU_VERTEX_32BITF|GU_TRANSFORM_2D, 2, 0, Vertices);
	
	if (speed == 0){
		sceGuAlphaFunc(GU_GREATER, 0, 0xff);
		sceGuEnable(GU_ALPHA_TEST);
		sceGuTexFunc(GU_TFX_REPLACE, GU_TCC_RGBA);
		sceGuEnable(GU_BLEND);
		sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);

		int *slow = (int*)sceGuGetMemory(64*32*4);
		copySlow(slow);
		Vertices = (struct Vertex*)sceGuGetMemory(2 * sizeof(struct Vertex));

		Vertices[0].u = 0;				Vertices[0].v = 0;
		Vertices[0].x = 10;				Vertices[0].y = 242;				Vertices[0].z = 0;
		Vertices[1].u = 35;				Vertices[1].v = 20;
		Vertices[1].x = 45;				Vertices[1].y = 262;					Vertices[1].z = 0;

		sceGuTexImage(0, 64, 32, 64, slow);
		
		sceGuDrawArray(GU_SPRITES, GU_TEXTURE_16BIT|GU_VERTEX_32BITF|GU_TRANSFORM_2D, 2, 0, Vertices);
		
		sceGuDisable(GU_ALPHA_TEST);
		sceGuDisable(GU_BLEND);
	}

	sceGuFinish();

	sceGuSync(0, 0);

	sceDisplayWaitVblankStart();

	sceGuSwapBuffers();

	return 0;
}

int T_Video(SceSize _args, void *_argp)
{
	int iSyncStatus = 1;

	DecoderThreadData* D = *((DecoderThreadData**)_argp);

	sceKernelWaitSema(D->Video->m_SemaphoreStart, 1, 0);

	for(;;)
	{
		if(D->Video->m_iAbort != 0)
			break;
		if(D->Video->m_iFullBuffers > 0)
		{
			iSyncStatus = AVSyncStatus(D);

			if(iSyncStatus > 0)
			{
				if(iSyncStatus == 1){
					if (D->Video->m_bAutoVideo){
					}
					else
						RenderFrame(D->Video->m_iLocX, D->Video->m_iLocY, D->Video->m_iWidth, D->Video->m_iHeight, D->Video->m_iTexWidth, D->Video->m_iTexHeight, D->Video->m_fWidthScale, D->Video->m_pVideoBuffer[D->Video->m_iPlayBuffer], D->Video->m_speed);
				}

				sceKernelWaitSema(D->Video->m_SemaphoreLock, 1, 0);

				D->Video->m_iFullBuffers--;

				sceKernelSignalSema(D->Video->m_SemaphoreLock, 1);
			}
		}
		else
		{
			sceDisplayWaitVblankStart();
		}

		sceKernelSignalSema(D->Video->m_SemaphoreWait, 1);

		sceDisplayWaitVblankStart();
	}

	while(D->Video->m_iFullBuffers > 0)
	{
		if (D->Video->m_bAutoVideo){
		}
		else
			RenderFrame(D->Video->m_iLocX, D->Video->m_iLocY, D->Video->m_iWidth, D->Video->m_iHeight, D->Video->m_iTexWidth, D->Video->m_iTexHeight, D->Video->m_fWidthScale, D->Video->m_pVideoBuffer[D->Video->m_iPlayBuffer], D->Video->m_speed);

		sceKernelWaitSema(D->Video->m_SemaphoreLock, 1, 0);

		D->Video->m_iFullBuffers--;

		sceKernelSignalSema(D->Video->m_SemaphoreLock, 1);

		sceDisplayWaitVblankStart();
	}

	sceGuTerm();

	sceKernelExitThread(0);

	return 0;
}

SceInt32 CPMFPlayer::InitVideo()
{
	Video.m_ThreadID = sceKernelCreateThread("video_thread", T_Video, 0x3F, 0x10000, PSP_THREAD_ATTR_USER, NULL);
	if(Video.m_ThreadID < 0)
	{
		sprintf(m_LastError, "sceKernelCreateThread() failed: 0x%08X", (int)Video.m_ThreadID);
		return -1;
	}

	Video.m_SemaphoreStart = sceKernelCreateSema("video_start_sema", 0, 0, 1, NULL);
	if(Video.m_SemaphoreStart < 0)
	{
		sprintf(m_LastError, "sceKernelCreateSema() failed: 0x%08X", (int)Video.m_SemaphoreStart);
		goto exit0;
	}

	Video.m_SemaphoreWait = sceKernelCreateSema("video_wait_sema", 0, 1, 1, NULL);
	if(Video.m_SemaphoreWait < 0)
	{
		sprintf(m_LastError, "sceKernelCreateSema() failed: 0x%08X", (int)Video.m_SemaphoreWait);
		goto exit1;
	}

	Video.m_SemaphoreLock = sceKernelCreateSema("video_lock_sema", 0, 1, 1, NULL);
	if(Video.m_SemaphoreLock < 0)
	{
		sprintf(m_LastError, "sceKernelCreateSema() failed: 0x%08X", (int)Video.m_SemaphoreLock);
		goto exit2;
	}

	Video.m_iBufferTimeStamp[0]	= 0;
	Video.m_iNumBuffers			= 1;
	Video.m_iFullBuffers		= 0;
	Video.m_iPlayBuffer			= 0;
	Video.m_iAbort				= 0;
	Video.m_LastError			= m_LastError;

	Video.m_iWidth				= m_MpegVideoWidth;
	Video.m_iHeight				= m_MpegVideoHeight;
	Video.m_iTexWidth			= nextPow2(m_MpegVideoWidth);
	Video.m_iTexHeight			= nextPow2(m_MpegVideoHeight);
	Video.m_iLocX = 0;
	Video.m_iLocY = 0;
	Video.m_fWidthScale = 1;
	Video.m_speed = 1;
	
	Video.m_bAutoVideo = m_AutoVideo;
	
	Video.m_pVideoBuffer[0]		= ((char *)sceGeEdramGetAddr()) + DRAW_BUFFER_SIZE + DISP_BUFFER_SIZE;

	sceGuInit();

	sceGuStart(GU_DIRECT, DisplayList);

	sceGuDrawBuffer(GU_PSM_8888, (void*)0, BUFFER_WIDTH);
	sceGuDispBuffer(SCREEN_W, SCREEN_H, (void*)(DRAW_BUFFER_SIZE), BUFFER_WIDTH);

	sceGuDisable  (GU_SCISSOR_TEST);
	
	sceGuTexFilter(GU_NEAREST,GU_LINEAR);

	sceGuTexMode  (GU_PSM_8888, 0, 0, GU_FALSE);  

	sceGuEnable   (GU_TEXTURE_2D);
	
	sceGuColor    (0xFFFFFFFF);

	sceGuClear    (GU_COLOR_BUFFER_BIT);

	sceGuFinish   ();
	
	sceGuSync     (0, 0);

	sceDisplayWaitVblankStart();

	sceGuDisplay  (GU_TRUE);

	return 0;

exit2:
	sceKernelDeleteSema(Video.m_SemaphoreWait);
exit1:
	sceKernelDeleteSema(Video.m_SemaphoreStart);
exit0:
	sceKernelDeleteThread(Video.m_ThreadID);

	return -1;
}

SceInt32 CPMFPlayer::ShutdownVideo()
{
	sceKernelDeleteThread(Video.m_ThreadID);

	sceKernelDeleteSema(Video.m_SemaphoreStart);
	sceKernelDeleteSema(Video.m_SemaphoreWait);
	sceKernelDeleteSema(Video.m_SemaphoreLock);

	return 0;
}
