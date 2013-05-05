/*==============================================================================
 *
 *       Filename:  dbmain.cpp
 *
 *    Description:  This file starts execution for the db process
 *
 * =============================================================================
 */

#include "../inc/dbinc.h"

static PortToValueType hashMap;
static mutex DBLock;

/* ===  FUNCTION  ==============================================================
 *         Name:  dbGetElement
 *  Description:  Gets the required element
 * =============================================================================
 */
bool dbGetElement (unsigned short int port, ValueStruct &value)
{
  DBLock.lock();
  try {
    value = hashMap.at(port);
    cout<<"Getting one from the db "<<value.live<<" "<<port<<endl;
    DBLock.unlock();
    return true;
  } catch (const out_of_range& excp) {
    DBLock.unlock();
    return false;
  }
}		/* -----  end of function dbGetElement  ----- */


/* ===  FUNCTION  ==============================================================
 *         Name:  dbSetElement
 *  Description:  Sets the given row with the given value
 * =============================================================================
 */
void dbSetElement (unsigned short int port, const ValueStruct &value)
{
  DBLock.lock();
  hashMap[port] = value;
  cout<<"At db "<<hashMap[port].live<<endl;
  DBLock.unlock();
  return;
}		/* -----  end of function dbSetElement  ----- */
/* ===  FUNCTION  ==============================================================
 *         Name:  listenerFlow
 *  Description:  The bird listens for incoming messages here
 * =============================================================================
 */
static void listenerFlow ()
{
 UDPSocket listenSocket (DB_LISTEN_PORT);

  while (true) {
    // Block for msg receipt
    int inMsgSize;
    char *inMsg;
    inMsg = new char[MAX_MSG_SIZE]();
    try {
      inMsgSize = listenSocket.recv(inMsg, MAX_MSG_SIZE);
    } catch (SocketException &e) {
      cout<<"DB: "<<e.what()<<endl;
    }
    inMsg[inMsgSize] = '\0';

    thread handlerThread (dbMsgHandler, inMsgSize, inMsg);
    handlerThread.detach();
  }
}   /* -----  end of function listenerFlow  ----- */

/* ===  FUNCTION  ==============================================================
 *         Name:  main
 * =============================================================================
 */
int main (int argc, char *argv[]) {
  
  // Listen on incoming port for messages
  listenerFlow();
  return EXIT_SUCCESS;
}				/* ----------  end of function main  ---------- */
