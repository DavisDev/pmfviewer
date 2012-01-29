#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspdebug.h>
#include <pspdebugkb.h>
#include <pspctrl.h>
#include <pspiofilemgr.h>
#include <pspumd.h>
#include <pspgu.h>
#include <pspsdk.h>
#include <psppower.h>

#include <cstdio>
#include <cstdarg> 
#include <malloc.h>
#include <cstring>
#include <cmath>

#include <psputility_avmodules.h>
#include <systemctrl_se.h>
#include <intraFont.h>
#include "pmfplayer.h"
#include "main.h"

#include <iostream>
#include <algorithm>
#include <vector>
using namespace std;

PSP_MODULE_INFO("PmfViewer", 0, 0, 1);
PSP_HEAP_SIZE_KB(-2048);

static unsigned int __attribute__((aligned(64)))  dlist[128 * 1024 * 4];
/*\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
int running;
int PowerCallback(int unknown, int pwrflags,void *common){
	if ((pwrflags & PSP_POWER_CB_SUSPENDING)||(pwrflags & PSP_POWER_CB_POWER_SWITCH)){
		CPMFPlayer *player = **(CPMFPlayer***)common;
		if (player){
			player->Suspend();
		}
	}
	else if (pwrflags & PSP_POWER_CB_RESUME_COMPLETE) {
		CPMFPlayer *player = **(CPMFPlayer***)common;
		if (player){
			player->Resume();
		}
	}
	return 0;
}
/* Exit callback */
int exit_callback(int arg1, int arg2, void *common){
    running = 0;
    return 0;
}

/* Callback thread */
int CallbackThread(SceSize args, void *argp){
    SceUID cbid = sceKernelCreateCallback("Exit Callback", exit_callback, NULL);
    sceKernelRegisterExitCallback(cbid);
    cbid = sceKernelCreateCallback("Power Callback", PowerCallback, argp);
    scePowerRegisterCallback(0, cbid); 
    sceKernelSleepThreadCB();
    return 0;
}

/* Sets up the callback thread and returns its thread id */
SceUID SetupCallbacks(CPMFPlayer **player){
    SceUID thid;

    thid = sceKernelCreateThread("update_thread", CallbackThread, 0x11, 0xFA0, 0, 0);
    if (thid >= 0){
       sceKernelStartThread(thid, 4, (void*)&player);
    }
    running = 1;
    return thid;
}

///////////////////////////////////////////////////////////////////////////////////
struct _Vertex{
	short u,v;
	float x,y,z;
};
struct _Vertex2{
	int colour;
	float x,y,z;
};
#define maxchar 50
#define maxline 17
/*->->->->->->->->->->->->->->->->->->->->->->->->->->->->->->->->->->->->->->->->*/
struct NodeData{
		char name[256];
		int type;
};
vector<NodeData> *list = new vector<NodeData>(), *parent = new vector<NodeData>();
int selection, startline;
/*->->->->->->->->->->->->->->->->->->->->->->->->->->->->->->->->->->->->->->->->*/
char *path, *self, Title[maxchar+1];
int archive = 0, xso = 0;
#define RGB(r, g, b) ((r)|((g)<<8)|((b)<<16))

bool compare(NodeData i, NodeData j){
	if (strcmp(i.name, "../") == 0){
		return true;
	}
	if (strcmp(j.name, "../") == 0){
		return false;
	}
	if (i.type == j.type){
		return strcmp(i.name, j.name) < 0;
	}
	if (FIO_S_ISDIR(i.type)){
		return true;
	}
	return false;
}
bool CreateList(char *dir){//alocate dir on the heap unless predefined
	int entry, i = 0, umd = 0;
	SceUID dfd;
	if (!dir){
		list->clear();
		dfd = sceIoDopen("ms0:/");
		if (dfd >= 0){
			sceIoDclose(dfd);
			i++;
			NodeData nd = {"ms0:/", FIO_S_IFDIR};
			list->push_back(nd);
		}		
		dfd = sceIoDopen("ef0:/");
		if (dfd >= 0){
			sceIoDclose(dfd);
			i++;
			NodeData nd = {"ef0:/", FIO_S_IFDIR};
			list->push_back(nd);
		}

		if (sceUmdCheckMedium()){
			i++;
			NodeData nd = {"disc0:/", FIO_S_IFDIR};
			list->push_back(nd);
		}
		
		dfd = sceIoDopen("flash0:/");
		if (dfd >= 0){
			sceIoDclose(dfd);
			i++;
			NodeData nd = {"flash0:/", FIO_S_IFDIR};
			list->push_back(nd);
		}
		dfd = sceIoDopen("flash1:/");
		if (dfd >= 0){
			sceIoDclose(dfd);
			i++;
			NodeData nd = {"flash1:/", FIO_S_IFDIR};
			list->push_back(nd);
		}
		dfd = sceIoDopen("flash2:/");
		if (dfd >= 0){
			sceIoDclose(dfd);
			i++;
			NodeData nd = {"flash2:/", FIO_S_IFDIR};
			list->push_back(nd);
		}
		dfd = sceIoDopen("flash3:/");
		if (dfd >= 0){
			sceIoDclose(dfd);
			i++;
			NodeData nd = {"flash3:/", FIO_S_IFDIR};
			list->push_back(nd);
		}
		
		dfd = sceIoDopen("host0:/");
		if (dfd >= 0){
			sceIoDclose(dfd);
			i++;
			NodeData nd = {"host0:/", FIO_S_IFDIR};
			list->push_back(nd);
		}
		dfd = sceIoDopen("host1:/");
		if (dfd >= 0){
			sceIoDclose(dfd);
			i++;
			NodeData nd = {"host1:/", FIO_S_IFDIR};
			list->push_back(nd);
		}
		dfd = sceIoDopen("host2:/");
		if (dfd >= 0){
			sceIoDclose(dfd);
			i++;
			NodeData nd = {"host2:/", FIO_S_IFDIR};
			list->push_back(nd);
		}
		dfd = sceIoDopen("host3:/");
		if (dfd >= 0){
			sceIoDclose(dfd);
			i++;
			NodeData nd = {"host3:/", FIO_S_IFDIR};
			list->push_back(nd);
		}
		dfd = sceIoDopen("host4:/");
		if (dfd >= 0){
			sceIoDclose(dfd);
			i++;
			NodeData nd = {"host4:/", FIO_S_IFDIR};
			list->push_back(nd);
		}
		dfd = sceIoDopen("host5:/");
		if (dfd >= 0){
			sceIoDclose(dfd);
			i++;
			NodeData nd = {"host5:/", FIO_S_IFDIR};
			list->push_back(nd);
		}
		dfd = sceIoDopen("host6:/");
		if (dfd >= 0){
			sceIoDclose(dfd);
			i++;
			NodeData nd = {"host6:/", FIO_S_IFDIR};
			list->push_back(nd);
		}
		dfd = sceIoDopen("host7:/");
		if (dfd >= 0){
			sceIoDclose(dfd);
			i++;
			NodeData nd = {"host7:/", FIO_S_IFDIR};
			list->push_back(nd);
		}
		dfd = sceIoDopen("eh0:/");
		if (dfd >= 0){
			sceIoDclose(dfd);
			i++;
			NodeData nd = {"ef0:/", FIO_S_IFDIR};
			list->push_back(nd);
		}
		selection = 0;
		startline = 0;
		return true; 
	}
	char *Dir;
	Dir = strrchr(dir, '\\');
	if (!Dir) 
		Dir = dir;
	else
		Dir++;
	if (strcmp(Dir, "disc0:/") == 0){
		i = sceUmdCheckMedium();
		if (i == 0){
			list->erase(list->begin()+selection);
			free(dir);
			return false;
		}
		sceUmdActivate(1, "disc0:");
		sceUmdWaitDriveStat(UMD_WAITFORINIT);			
		umd = 1;
	}
	else if (strcmp(Dir, "../") == 0){
		if (parent->size() == 1){
			free(path);
			path = NULL;
			return CreateList(NULL); 
		}
		char *slash = strrchr(path, '/'); //get last "/"
		*slash = '\0';					 // remove it
		*(strrchr(path, '/')+1) = '\0';	//  get next one, to get parent folder
		if (archive){
			if (strrchr(path, '\\') == NULL){
				sctrlSEUmountUmd_p();
				sctrlSESetDiscOut_p(0);
				xso = 0;
				archive = 0;
			}
		}
		dir = (char*)malloc(strlen(path)+1);
		strcpy(dir, path);
		Dir = strrchr(dir, '\\');
		if (Dir)
			Dir++;
		else
			Dir = dir;
	}

	if ((dfd = sceIoDopen(Dir)) < 0){
		if (dfd < 0){
			cout << "Unable to open " << Dir << " " << hex << dfd << endl;
			free(dir);
			return false;//0x11ff dir 0x116d dir 0x216d file 0x116d dir
		}
	}
	SceIoDirent *fde = new SceIoDirent;	
	memset(fde, 0, sizeof(SceIoDirent));
	startline = selection = 0;
	entry = sceIoDread(dfd, fde);
	list->clear();
	NodeData nd = {"../", FIO_S_IFDIR};
	list->push_back(nd);
	while (entry > 0){
		if (strcmp(fde->d_name, ".") != 0 && strcmp(fde->d_name, "..") != 0){
			if (FIO_S_ISDIR(fde->d_stat.st_mode)){
				sprintf(nd.name, "%s%c", fde->d_name, '/');
			}
			else{
				sprintf(nd.name, fde->d_name);				
			}
			nd.type = fde->d_stat.st_mode;
			list->push_back(nd);
		}
		entry = sceIoDread(dfd, fde);
	}
	delete fde;
	sceIoDclose(dfd);
	sort(list->begin(), list->end(), compare);
	if (entry < 0){
		cout << "Error reading directory entries " << hex << entry << endl;
	}
	free(path);
	path = dir;
	return true;
}	

int Back(void){
	if (path == NULL) return 0;
	char dir[4] = "../";
	if (!CreateList(dir)) return 0;
	int j = 0;
	for (vector<NodeData>::iterator i = list->begin(); i != list->end(); i++){
		if (strcmp(i->name, (*parent)[parent->size()-1].name) == 0){
			selection = j;
			break;
		} 
		j++;
	}
	if (selection >= maxline){
		startline = selection;
	}
	parent->pop_back();
	if (path)
		sprintf(Title, "%.50s", path);
	else
		strcpy(Title, "myPSP");
	return 0;
}

char *getextension(char *file){
	char *ext = strrchr(file, '.');
	if (!ext) return NULL;
	int l = strlen(ext);
	char *cext = (char*)malloc(l);
	strcpy(cext, ext+1);
	return cext; 
}

SceUID cbth;					
int processDir(char *dir){
	NodeData nd;
	strcpy(nd.name, (*list)[selection].name);//need to come before list is destroyed
	if (!CreateList(dir)){
		return 0;
	}
	sprintf(Title, "%.50s", path);
	parent->push_back(nd);
	return 1;	
}
int Open(CPMFPlayer **player){
	char * dir;

	if (strcmp((*list)[selection].name, "../") == 0){
		Back();
		return 0;
	}
	else if (path == NULL){
		dir = (char*)malloc(strlen((*list)[selection].name) + 1);
		strcpy(dir, (*list)[selection].name);
	}
	else{
		dir = (char*)malloc(strlen(path) + strlen((*list)[selection].name) + 1);
//		strcat(strcpy(dir, path), (*list)[selection].name);							
		sprintf(dir, "%s%s", path, (*list)[selection].name);							
	}
	if (FIO_S_ISDIR((*list)[selection].type)){
		if (!processDir(dir)) return 0;
	}
	else{
		char *ext = getextension((*list)[selection].name);
		if (ext){
			if ((stricmp(ext, "iso") == 0)||(stricmp(ext, "cso") == 0)){
				SEConfig config;
				sctrlSEGetConfigEx_p(&config, sizeof(config));
				int ret;
				if (config.umdmode){
					ret = sctrlSEMountUmdFromFile_p(dir, 1, 1);
				}
				else{
					ret = sctrlSEMountUmdFromFile_p(dir, 0, config.useisofsonumdinserted);
				}
				if (ret != 0){
					return -1;
				}
				else{
					archive = 1;
					xso = 1;
				}
				free(dir);
				free(ext);
				dir = (char*)malloc(strlen(path) + strlen((*list)[selection].name) + 8 + 1);
				sprintf(dir, "%s%s%s", path, (*list)[selection].name, "\\disc0:/");
//				strcat(strcat(strcpy(dir, path), (*list)[selection].name), "\\disc0:/");							
				return processDir(dir);
			}
			free(ext);
		}
		char *Dir = strrchr(dir, '\\');
		if (Dir) Dir++;
		else Dir = dir;
		CPMFPlayer *MpegDecoder = new CPMFPlayer(false);
		if(MpegDecoder->Initialize() < 0)
		{
			cout << MpegDecoder->GetLastError() << endl;
			MpegDecoder->Shutdown();
			delete MpegDecoder;
			return -1;
		}
		if(MpegDecoder->Load(Dir) < 0)
		{
			cout << MpegDecoder->GetLastError() << endl;
			MpegDecoder->Shutdown();
			delete MpegDecoder;
			return -1;
		}
		*player = MpegDecoder;
		free(dir);
	}	
	return 1;
}
#define fontHeight 15
void DrawPause(){
	int bufferwidth, pixelformat, pixelSize;
	void *buf;
	sceDisplayGetFrameBuf(&buf, &bufferwidth, &pixelformat, PSP_DISPLAY_SETBUF_IMMEDIATE);
	switch (pixelformat){
		case PSP_DISPLAY_PIXEL_FORMAT_565 :
		case PSP_DISPLAY_PIXEL_FORMAT_5551: 	
			pixelSize = 2;
		break;
		case PSP_DISPLAY_PIXEL_FORMAT_4444: 	
		case PSP_DISPLAY_PIXEL_FORMAT_8888:;
		default:
			pixelSize = 4;
		break;
	}
	for (int i = 0; i < 20; i++){
		for (int j = 0; j < 10; j++){
			void *loc = (char*)buf+(pixelSize*(bufferwidth*(i+242)+(j+10)));
			if (pixelSize == 2){
				short *loc2 = (short*)loc;
				loc2[00] = 0xfff;
				loc2[20] = 0xfff;
			}
			else{
				int *loc4 = (int*)loc;
				loc4[00] = 0xffff;
				loc4[20] = 0xffff;
			}

		}
	}
}
/*void DrawSeek(){
	int bufferwidth, pixelformat;
	void *buf;
	sceDisplayGetFrameBuf(&buf, &bufferwidth, &pixelformat, PSP_DISPLAY_SETBUF_IMMEDIATE);
	
	sceGuStart(GU_DIRECT, dlist);
	sceGuEnable(GU_TEXTURE_2D);
	struct _Vertex* Vertices = (struct _Vertex*)sceGuGetMemory(2 * sizeof(struct _Vertex));

	Vertices[0].u = 0.0f;			Vertices[0].v = 0.0f;
	Vertices[0].x = 0.0f;			Vertices[0].y = 0.0f;			Vertices[0].z = 0.0f;
	Vertices[1].u = 480.0f;			Vertices[1].v = 272.0f;
	Vertices[1].x = 480.0f;			Vertices[1].y = 272.0f;			Vertices[1].z = 0.0f;

	sceGuTexImage(0, bufferwidth, 512, bufferwidth, buf);

	sceGuDrawArray(GU_SPRITES, GU_TEXTURE_32BITF|GU_VERTEX_32BITF|GU_TRANSFORM_2D, 2, 0, Vertices);
	
	struct _Vertex2* Vertices2 = (struct _Vertex2*)sceGuGetMemory(2 * sizeof(struct _Vertex2));

	Vertices2[0].colour = 0xffffff;
	Vertices2[0].x = 20;				Vertices2[0].y = 242;			Vertices2[0].z = 0;
	Vertices2[1].colour = 0xffffff;
	Vertices2[1].x = 10;				Vertices2[1].y = 242;			Vertices2[1].z = 0;
	Vertices2[2].colour = 0xffffff;
	Vertices2[2].x = 10;				Vertices2[2].y = 262;			Vertices2[2].z = 0;
	Vertices2[3].colour = 0xffffff;
	Vertices2[3].x = 20;				Vertices2[3].y = 262;			Vertices2[3].z = 0;

	sceGuDrawArray(GU_TRIANGLE_STRIP, GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_2D, 4, 0, Vertices2);

	Vertices2 = (struct _Vertex2*)sceGuGetMemory(2 * sizeof(struct _Vertex2));

	Vertices2[0].colour = 0xffffff;
	Vertices2[0].x = 40;				Vertices2[0].y = 242;			Vertices2[0].z = 0;
	Vertices2[1].colour = 0xffffff;
	Vertices2[1].x = 30;				Vertices2[1].y = 242;			Vertices2[1].z = 0;
	Vertices2[2].colour = 0xffffff;
	Vertices2[2].x = 30;				Vertices2[2].y = 262;			Vertices2[2].z = 0;
	Vertices2[3].colour = 0xffffff;
	Vertices2[3].x = 40;				Vertices2[3].y = 262;			Vertices2[3].z = 0;
sceGuDisable(GU_TEXTURE_2D);
	sceGuDrawArray(GU_TRIANGLE_STRIP, GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_2D, 4, 0, Vertices2);
sceGuEnable(GU_TEXTURE_2D);

	sceGuFinish();

	sceGuSync(0, 0);

	sceDisplayWaitVblankStart();

	sceGuSwapBuffers();
	
}*/
void DrawInfo(intraFont *font){
	sceGuStart(GU_DIRECT, dlist);
	sceGuClearColor(0);
	sceGuClear(GU_COLOR_BUFFER_BIT);

	intraFontSetStyle(font, 1.75f, 0xffff0000, 0, 0);
	intraFontPrint(font, 10, 40, "Pmf Viewer 0.1 by mowglisanu"); 
	intraFontSetStyle(font, 1.25f, 0xff880088, 0, 0);
	intraFontPrint(font, 40, 65, "Thanks to:"); 
	intraFontSetStyle(font, 1.0f, 0xff888888, 0, 0);
	intraFontPrint(font, 60, 85, "magik for his pmfPlayer"); 
	intraFontPrint(font, 60, 105, "jpcsp developers for info on PsmfPlayer"); 
	intraFontPrint(font, 60, 125, "BenHur for intraFont"); 
	intraFontPrint(font, 60, 145, "Heimdall for minpspw"); 
	intraFontPrint(font, 60, 165, "TyRaNiD  for psplink"); 
	intraFontPrint(font, 60, 185, "And of course the psp homebrew community"); 
	intraFontSetStyle(font, 0.75f, 0xff666666, 0, 0);
	intraFontPrint(font, 200, 260, "Send comment or bugs to <mowglisanu@gmail.com>"); 
       // End drawing
	sceGuFinish();
	sceGuSync(0,0);
	
	// Swap buffers (waiting for vsync)
	sceDisplayWaitVblankStart();
	sceGuSwapBuffers();
}
void Draw(intraFont *font){
	sceGuStart(GU_DIRECT, dlist);
	sceGuClearColor(0);
	sceGuClear(GU_COLOR_BUFFER_BIT);

	intraFontSetStyle(font, 1.0f, 0xff0000ff, 0, 0);
	intraFontPrint(font, 50, 17, Title); 
	intraFontSetStyle(font, 1.0f, 0xff00ffff, 0, 0);
	int y = 30, x = 5, end = list->size() - 1;
	if (list->size() > maxline){
		end = startline + maxline - 1; 
	}
	for (int i = startline; i <= end; i++){ 
		if (i == selection){
			intraFontSetStyle(font, 1.0f, 0xffcc4400, 0, 0);
			intraFontPrint(font, x, y, (*list)[i].name);
			intraFontSetStyle(font, 1.0f, FIO_S_ISDIR((*list)[i].type)?0xff00ffff:0xffcc4400, 0, 0);
		}
		else if (FIO_S_ISDIR((*list)[i].type)){
			intraFontPrint(font, x, y, (*list)[i].name); 
		}
		else{
			intraFontSetStyle(font, 1.0f, 0xffffffff, 0, 0);
			intraFontPrint(font, x, y, (*list)[i].name); 
		}
		y += fontHeight;
	}
       // End drawing
	sceGuFinish();
	sceGuSync(0,0);
	
	// Swap buffers (waiting for vsync)
	sceDisplayWaitVblankStart();
	sceGuSwapBuffers();
}
void setupGu(){
    sceGuInit();
	sceGuStart(GU_DIRECT, dlist);

	sceGuDrawBuffer(GU_PSM_8888, (void*)0, 512);
	sceGuDispBuffer(480, 272, (void*)0x88000, 512);
	sceGuDepthBuffer((void*)0x110000, 512);
 
	sceGuOffset(2048 - (480/2), 2048 - (272/2));
	sceGuViewport(2048, 2048, 480, 272);
	sceGuDepthRange(65535, 0);
	sceGuScissor(0, 0, 480, 272);
	sceGuEnable(GU_SCISSOR_TEST);
	sceGuDepthFunc(GU_GEQUAL);
	sceGuEnable(GU_DEPTH_TEST);
	sceGuFrontFace(GU_CW);
	sceGuShadeModel(GU_SMOOTH);
	sceGuEnable(GU_CULL_FACE);
	sceGuEnable(GU_CLIP_PLANES);
	sceGuEnable(GU_BLEND);
	sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
	sceGuFinish();
	sceGuSync(0,0);
	sceDisplayWaitVblankStart();
	sceGuDisplay(GU_TRUE);
	
}
int main(int argc, char *argv[]){

	int ret = sceUtilityLoadAvModule(PSP_AV_MODULE_AVCODEC);
	if (ret < 0 && ret != 0x80110f02){
		sceKernelExitGame();
	}
	ret = sceUtilityLoadAvModule(PSP_AV_MODULE_ATRAC3PLUS);
	if (ret < 0 && ret != 0x80110f02){
		sceUtilityUnloadAvModule(PSP_AV_MODULE_AVCODEC);
		sceKernelExitGame();
	}
	ret = sceUtilityLoadAvModule(PSP_AV_MODULE_MPEGBASE);
	if (ret < 0 && ret != 0x80110f02){
		sceUtilityUnloadAvModule(PSP_AV_MODULE_ATRAC3PLUS);
		sceUtilityUnloadAvModule(PSP_AV_MODULE_AVCODEC);
		sceKernelExitGame();
	}

	CPMFPlayer *player = NULL;
	//cout << "address " << &player << endl;
	SetupCallbacks(&player);
	intraFontInit();
	intraFont *font = intraFontLoad("flash0:/font/ltn8.pgf", 0);
	intraFontSetStyle(font, 1.0f, 0xFFFFFFFF, 0x00000000, 0);
	
	CreateList(NULL);
	sprintf(Title, "myPsp");
	setupGu();
	SceCtrlData pad, oldpad;	
	sceCtrlSetSamplingCycle(0);	
	sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);
	sceCtrlReadBufferPositive(&oldpad, 1); 
	int press = 0;
	bool viewInfo = false;
	while (running){
		sceCtrlReadBufferPositive(&pad, 1); 
		int skip_controls = 0;
		if (pad.Buttons == oldpad.Buttons){
			if (pad.Buttons < PSP_CTRL_LEFT){
				if (press < 15 || (press%5)){
					skip_controls = 1;
				}
			}
			else
				skip_controls = 1;
			press++;
		}
		else
			press = 0;
		
		if (!skip_controls){
			if (viewInfo && pad.Buttons){
				viewInfo = false;
			}
			else if (pad.Buttons & PSP_CTRL_RTRIGGER){
				if (player){
					player->Faster();
				}
			}
			else if (pad.Buttons & PSP_CTRL_LTRIGGER){
				if (player){
					player->Slower();
				}
			}
			else if (pad.Buttons & PSP_CTRL_SELECT){
				if (!player){
					viewInfo = true;
				}
			}
			else if (pad.Buttons & PSP_CTRL_LEFT){
			}
			else if (pad.Buttons & PSP_CTRL_RIGHT){
			}
			else if (pad.Buttons & PSP_CTRL_UP){
				if (!player){
					if (selection != 0){
						if (selection == startline){
							startline--; 
						}
						selection--;
					}
					else{
						selection = list->size() - 1;
						if (list->size() > maxline){
							startline = list->size() - (maxline);
						}
					}
				}
			} 
			else if (pad.Buttons & PSP_CTRL_DOWN){
				if (!player){
					if (selection != (list->size()-1)){
						if (selection == startline+(maxline-1)){
							startline++;
						}
						selection++;
					}
					else{
						startline = selection = 0;
					}
				}
			} 
			else if (pad.Buttons & PSP_CTRL_SQUARE){
				if (player){
					player->Pause();
				}
			} 
			else if (pad.Buttons & PSP_CTRL_TRIANGLE){
				if (!player){
					Back();
				}
			}
			else if (pad.Buttons & PSP_CTRL_CIRCLE){
				if (player){
					player->Stop();
					player->Shutdown();
					delete player;
					player = NULL;
					setupGu();
				}
			}
			if (pad.Buttons & (PSP_CTRL_CROSS|PSP_CTRL_START)){
				if (player){
					player->Play();
				}
				else{
					Open(&player);
					if (player){
						if (player->Play() < 0){
							cout << player->GetLastError() << endl;
						}
					}
				}
				//cout << player << endl;
			}
		}
		oldpad = pad;
		if (viewInfo){
			DrawInfo(font);
		}
		else if (player){
			if (player->IsFinished()){
				player->Shutdown();
				delete player;
				player = NULL;
				setupGu();
			}
			else if (player->IsPaused()){
				DrawPause();
				//DrawSeek();
			}
		}
		else{
			Draw(font);
		}
	}
	
	if (player){
		player->Stop();
		player->Shutdown();
		delete player;
	}
	
	sceUtilityUnloadAvModule(PSP_AV_MODULE_MPEGBASE);
	sceUtilityUnloadAvModule(PSP_AV_MODULE_ATRAC3PLUS);
	sceUtilityUnloadAvModule(PSP_AV_MODULE_AVCODEC);
	sceKernelExitGame();
	return 0;
}
