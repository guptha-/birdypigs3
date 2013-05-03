#ifndef inc_PIG_INC
#define inc_PIG_INC

#include <iostream>
#include <thread>
#include <atomic>
#include <vector>
#include <cstring>
#include <mutex>
#include <netinet/in.h>
#include <unistd.h>

#include "pigds.h"
#include "pigprot.h"
#include "comconst.h"
#include "PracticalSocket.h"
using namespace std;

extern OwnStruct ownNode;
extern vector<OtherStruct> otherVector;
extern mutex otherVectorLock;
#endif

