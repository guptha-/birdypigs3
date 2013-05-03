#ifndef inc_PIG_DS
#define inc_PIG_DS

#include "piginc.h"

using namespace std;
struct OwnStruct{
  atomic<unsigned int> port;
  atomic<unsigned int> posn;
  atomic<bool> live;
};

struct OtherStruct{
  unsigned int port;
  unsigned int posn;
  bool live;
};
#endif

