#include <systemctrl_se.h>
int sctrlSEUmountUmd_p(){
	return sctrlSEUmountUmd();
}
void sctrlSESetDiscOut_p(int out){
	sctrlSESetDiscOut(out);
}
int sctrlSEGetConfigEx_p(SEConfig* config, int size){
	return sctrlSEGetConfigEx(config, size);
}
int sctrlSEMountUmdFromFile_p(char *file, int noumd, int isofs){
	return sctrlSEMountUmdFromFile(file, noumd, isofs);
}
