#include <systemctrl_se.h>
extern "C"{
int sctrlSEUmountUmd_p();
void sctrlSESetDiscOut_p(int);
int sctrlSEGetConfigEx_p(SEConfig*, int);
int sctrlSEMountUmdFromFile_p(char*, int, int);
 }