#ifndef inc_BIRD_PROT
#define inc_BIRD_PROT

#include "birdinc.h"
using namespace std;

void birdMsgHandler(int inMsgSize, char *inMsg);
void birdSendStartGameMsg (int destPort, int coord);
void birdSendBirdPosnMsg (unsigned short int destPort, int birdPosn);
#endif

