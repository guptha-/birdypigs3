/*==============================================================================
 *
 *       Filename:  dbmsg.cpp
 *
 *    Description:  This sends/receives message for the db.
 *
 * =============================================================================
 */

#include "../inc/dbinc.h"

/* ===  FUNCTION  ==============================================================
 *         Name:  getTwoBytes
 *  Description:  Gets two bytes from the message, and returns equiv. int
 * =============================================================================
 */
unsigned short int getTwoBytes (char *&msg, int &msgSize)
{
  unsigned short int reply;
  memcpy (&reply, msg, 2);
  reply = ntohs(reply);
  msg += 2;
  msgSize -= 2;

  return reply;
}		/* -----  end of function getTwoBytes  ----- */

/* ===  FUNCTION  ==============================================================
 *         Name:  addTwoBytes
 *  Description:  Adds two bytes to the message from the given value
 * =============================================================================
 */
void addTwoBytes (char *&msg, int &msgSize, int value)
{
  value = htons(value);
  memcpy (msg, &value, 2);
  msg += 2;
  msgSize += 2;

}		/* -----  end of function addTwoBytes  ----- */

/* ===  FUNCTION  ==============================================================
 *         Name:  sendMsg
 *  Description:  This function sends the given msg to the give port
 * =============================================================================
 */
void sendMsg(char *outMsg, int outMsgSize, unsigned short int destPort)
{
  try {
    static UDPSocket sendSocket;
    sendSocket.sendTo(outMsg, outMsgSize, COM_IP_ADDR, destPort);
  } catch (SocketException &e) {
    cout<<"db: Cannot send msg"<<endl;
  }
  return;
}   /* -----  end of function sendMsg  ----- */

/* ===  FUNCTION  ==============================================================
 *         Name:  sendDbResponseMsg
 * =============================================================================
 */
void sendDbResponseMsg (int destPort, const vector<short unsigned int> &port, 
                        const vector<short unsigned int> &posn,
                        const vector<bool> &status)
{
  /*
  |--- 1 ---|--- 2 ---|--- 3 ---|--- 4 ---|
  <----- Msg Type ----X------ Count ------>
  <------- Port ------X------- Posn ------>
  <------ Status ----- ...
  */
  cout<<"Sending db resp msg"<<endl;
  char msg[MAX_MSG_SIZE];
  char *outMsg = msg;
  memset(outMsg, 0, MAX_MSG_SIZE);
  char *permOutMsg = outMsg;
  int outMsgSize = 0;

  addTwoBytes(outMsg, outMsgSize, DB_RESP_MSG);
  short unsigned int field;
  for (int i = 0, n = port.size(); i < n; i++) {
    field = port[i];
    addTwoBytes(outMsg, outMsgSize, field);
    field = posn[i];
    addTwoBytes(outMsg, outMsgSize, field);
    field = (status[i] == false) ? 0 : 1;
    addTwoBytes(outMsg, outMsgSize, field);
  }

  sendMsg(permOutMsg, outMsgSize, destPort);

  return;
}		/* -----  end of function sendDbResponseMsg  ----- */

/* ===  FUNCTION  ==============================================================
 *         Name:  handleDbUpdMsg
 *  Description:  The update message contains the port and its corresponding 
 *                posn and status. This is updated in the database
 * =============================================================================
 */
void handleDbUpdMsg (int inMsgSize, char *inMsg)
{
  /*
  |--- 1 ---|--- 2 ---|--- 3 ---|--- 4 ---|
  <----- Msg Type ----X------- Port ------>
  <------- Posn ------X----- Status ------> 
  */

  ValueStruct val;
  unsigned short int port = getTwoBytes(inMsg, inMsgSize);
  val.posn = getTwoBytes(inMsg, inMsgSize);
  unsigned short int status = getTwoBytes(inMsg, inMsgSize);
  if (status == 0) {
    cout<<"Killed one at the db: "<<port<<endl;
  }
  val.live = (status == 1) ? true : false;

  dbSetElement (port, val);
  return;
}		/* -----  end of function handleDbUpdMsg  ----- */

/* ===  FUNCTION  ==============================================================
 *         Name:  handleDbRequestMsg
 *  Description:  The request message contains a list of port numbers, for which
 *                the posn and status have to be supplied.
 * =============================================================================
 */
void handleDbRequestMsg (int inMsgSize, char *inMsg)
{
  /*
  |--- 1 ---|--- 2 ---|--- 3 ---|--- 4 ---|
  <----- Msg Type ----X---- Dest Port  --->
  <----- Count -------X----- Port ------ .... 
  */

  unsigned short int destPort = getTwoBytes(inMsg, inMsgSize);
  unsigned short int count = getTwoBytes(inMsg, inMsgSize);
  vector<unsigned short int> ports;
  while (count--) {
    ports.push_back(getTwoBytes(inMsg, inMsgSize));
  }

  ValueStruct value;
  vector<short unsigned int> posn;
  vector<bool> status;
  for (int i = 0, n = ports.size(); i < n; i++) {
    if (dbGetElement (ports[i], value) == false) {
      cout<<"Cannot find "<<ports[i]<<" at the db"<<endl;
    }
    posn.push_back(value.posn);
    status.push_back(value.live);
    if(value.live == false) {
      cout<<"Retrievng a dead one from the db: "<<ports[i];
    }
  }

  sendDbResponseMsg (destPort, ports, posn, status);
  return;
}		/* -----  end of function handleDbRequestMsg  ----- */

/* ===  FUNCTION  ==============================================================
 *         Name:  dbMsgHandler
 *  Description:  This function accepts all the messages, finds out their type,
 *                and calls their respective handlers.
 * =============================================================================
 */
void dbMsgHandler (int inMsgSize, char *inMsg)
{
  if (inMsgSize < 2) {
    cout<<"Corrupted message at db "<<endl;
    return;
  }

  short unsigned int msgType = getTwoBytes(inMsg, inMsgSize);;

  switch (msgType) {
    case DB_REQ_MSG: {
      handleDbRequestMsg(inMsgSize, inMsg);
      break;
    }
    case DB_UPD_MSG: {
      handleDbUpdMsg(inMsgSize, inMsg);
      break;
    }
    default: {
      cout<<"Invalid msg received at db "<<msgType<<endl;
      break;
    }
  }
  return;
}
