#ifndef inc_DB_INC
#define inc_DB_INC

#include <iostream>
#include <thread>
#include <vector>
#include <stdexcept>
#include <mutex>
#include <unordered_map>
#include <netinet/in.h>

using namespace std;
#include "comconst.h"
#include "PracticalSocket.h"

void dbMsgHandler(int inMsgSize, char *inMsg);
typedef struct {
  unsigned short int posn;
  bool live;
} ValueStruct;

typedef unordered_map<unsigned short int, ValueStruct> PortToValueType;

void dbSetElement (unsigned short int port, const ValueStruct &value);
bool dbGetElement (unsigned short int port, ValueStruct &value);
#endif

