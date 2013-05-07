/*==============================================================================
 *
 *       Filename:  pigmsg.cpp
 *
 *    Description:  The message related functions for pig
 *
 * =============================================================================
 */
#include "../inc/piginc.h"

static atomic<int> birdPosn(0);
static atomic<int> sentCount(0);
static atomic<int> score(0);
static atomic<unsigned short int> otherCoordPort(0);
static atomic<bool> receivedOtherCoordMsg(false);
static atomic<bool> receivedBirdPosnMsg(false);
static atomic<bool> otherCoordAlive(false);
static atomic<bool> otherCoordPrevAlive;
static atomic<bool> ownCoordPrevAlive;
static atomic<bool> ownCoordAlive(false);
static atomic<bool> isHit(false);
static mutex coordLock;

/* ===  FUNCTION  ==============================================================
 *         Name:  getTwoBytes
 *  Description:  Gets two bytes from the message, and returns equiv. value
 * =============================================================================
 */
unsigned short int getTwoBytes (char *&inMsg, int &inMsgSize)
{
  unsigned short int reply;
  memcpy (&reply, inMsg, 2);
  reply = ntohs(reply);
  inMsg += 2;
  inMsgSize -= 2;

  return reply;
}		/* -----  end of function getTwoBytes  ----- */

/* ===  FUNCTION  ==============================================================
 *         Name:  addTwoBytes
 *  Description:  Adds two bytes to the message from the given value
 * =============================================================================
 */
void addTwoBytes (char *&inMsg, int &inMsgSize, int value)
{
  value = htons(value);
  memcpy (inMsg, &value, 2);
  inMsg += 2;
  inMsgSize += 2;

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
    cout<<ownNode.port<<": Cannot send msg"<<endl;
  }
  return;
}   /* -----  end of function sendMsg  ----- */

/* ===  FUNCTION  ==============================================================
 *         Name:  isAffected
 *  Description:  Checks if we are affected by the bird and indicates the 
 *                position we should move to if true
 * =============================================================================
 */
bool isAffected (int &posn) {
  if (posn == birdPosn) {
    // We are affected
    if (posn == MAX_POSN) {
      posn--;
    } else {
      posn++;
    }
    return true;
  }
  return false;
}		/* -----  end of function isAffected  ----- */

/* ===  FUNCTION  ==============================================================
 *         Name:  handleFromCoordBirdApproachMsg
 *  Description:  This receives the bird approaching msg from the coordinator
 * =============================================================================
 */
void handleFromCoordBirdApproachMsg (int inMsgSize, char *inMsg)
{
  /*
  |--- 1 ---|--- 2 ---|--- 3 ---|--- 4 ---|
  <----- Msg Type ----X----- Bird Posn --->
  */
  unsigned short int bPosn = getTwoBytes(inMsg, inMsgSize);
  birdPosn = bPosn;

  isHit = false;
  int posn = ownNode.posn;
  if (isAffected(posn)) {
    cout<<ownNode.port<<": We are targeted at "<<ownNode.posn<<endl;
    ownNode.posn = posn;
    if (rand() % PIG_NOTIFY_CHANCE == 1) {
      // We will not have received this msg in time
      cout<<ownNode.port<<": We are hit"<<endl;
      isHit = true;
    }
  }
  return;
}		/* -----  end of function handleFromCoordBirdApproachMsg  ----- */

/* ===  FUNCTION  ==============================================================
 *         Name:  sendToPigBirdApproachMsg
 *  Description:  This indicates to the affected pigs that the bird is 
 *                approaching
 * =============================================================================
 */
void sendToPigBirdApproachMsg (int destPort, int posn)
{
  /*
  |--- 1 ---|--- 2 ---|--- 3 ---|--- 4 ---|
  <----- Msg Type ----X----- Bird Posn --->
  */
  char msg[MAX_MSG_SIZE];
  char *outMsg = msg;
  memset(outMsg, 0, MAX_MSG_SIZE);
  char *permOutMsg = outMsg;
  int outMsgSize = 0;

  addTwoBytes(outMsg, outMsgSize, BIRD_APPROACH_MSG);
  addTwoBytes(outMsg, outMsgSize, posn);

  sendMsg(permOutMsg, outMsgSize, destPort);
  return;
}		/* -----  end of function sendToPigBirdApproachMsg  ----- */

/* ===  FUNCTION  ==============================================================
 *         Name:  sendStatusReqMsg
 *  Description:  It sends a request to the database to fill in the missing gaps
 * =============================================================================
 */
void sendStatusReqMsg (int port)
{
  /*
  |--- 1 ---|--- 2 ---|--- 3 ---|--- 4 ---|
  <----- Msg Type ----X--- Coord Port  --->
  */
  char msg[MAX_MSG_SIZE];
  char *outMsg = msg;
  memset(outMsg, 0, MAX_MSG_SIZE);
  char *permOutMsg = outMsg;
  int outMsgSize = 0;

  addTwoBytes(outMsg, outMsgSize, STATUS_REQ_MSG);
  addTwoBytes(outMsg, outMsgSize, ownNode.port);

  sendMsg(permOutMsg, outMsgSize, port);
  return;
}		/* -----  end of function sendStatusReqMsg  ----- */

/* ===  FUNCTION  ==============================================================
 *         Name:  sendEndLaunchToBird
 * =============================================================================
 */
void sendEndLaunchToBird ()
{
  /*
  |--- 1 ---|--- 2 ---|--- 3 ---|--- 4 ---|
  <----- Msg Type ----X------ Score ------>
  <--- Sole Coord ---->
  */

  char msg[MAX_MSG_SIZE];
  char *outMsg = msg;
  memset(outMsg, 0, MAX_MSG_SIZE);
  char *permOutMsg = outMsg;
  int outMsgSize = 0;

  addTwoBytes(outMsg, outMsgSize, END_LAUNCH_MSG);
  addTwoBytes(outMsg, outMsgSize, score);
  int soleCoord;
  if (otherCoordAlive == false) {
    soleCoord = 1;
  } else {
    soleCoord = 0;
  }
  addTwoBytes(outMsg, outMsgSize, soleCoord);

  sendMsg(permOutMsg, outMsgSize, BIRD_LISTEN_PORT);

  return;
}		/* -----  end of function sendEndLaunchToBird  ----- */

/* ===  FUNCTION  ==============================================================
 *         Name:  handlePigs
 *  Description:  Once the status of the other coordinator is known, the bird
 *                posn message is sent out to the other pigs
 * =============================================================================
 */
void handlePigs ()
{
  otherVectorLock.lock();
  for (int i = 0, n = otherVector.size(); i < n; i++) {
    if (((otherVector[i].port % 2 == ownNode.port % 2) ||
         (otherCoordAlive == false)) &&
        (otherVector[i].live == true)) /* No point informing dead ones */{
      sendToPigBirdApproachMsg(otherVector[i].port, birdPosn);
    }
  }
  otherVectorLock.unlock();
  if (ownNode.live == true) {
    sendToPigBirdApproachMsg(ownNode.port, birdPosn);
  }
  usleep(10000);
  for (int i = 0, n = otherVector.size(); i < n; i++) {
    if (((otherVector[i].port % 2 == ownNode.port % 2) ||
         (otherCoordAlive == false)) &&
        (otherVector[i].live == true)) /* No point informing dead ones */{
      sentCount++;
      sendStatusReqMsg(otherVector[i].port);
    }
  }
  otherVectorLock.unlock();
  if (ownNode.live == true) {
    sendStatusReqMsg(ownNode.port);
    sentCount++;
  }
  if (sentCount == 0) {
    // All pigs are already dead
    cout<<"Sent end launch to bird"<<endl;
    sendEndLaunchToBird();
  }
  return;
}		/* -----  end of function handlePigs  ----- */

/* ===  FUNCTION  ==============================================================
 *         Name:  sendDbRequestMsg
 *  Description:  It sends a request to the database to fill in the missing gaps
 * =============================================================================
 */
void sendDbRequestMsg ()
{
  /*
  |--- 1 ---|--- 2 ---|--- 3 ---|--- 4 ---|
  <----- Msg Type ----X---- Dest Port  --->
  <----- Count -------X----- Port ------ .... 
  */
  char msg[MAX_MSG_SIZE];
  char *outMsg = msg;
  memset(outMsg, 0, MAX_MSG_SIZE);
  char *permOutMsg = outMsg;
  int outMsgSize = 0;

  addTwoBytes(outMsg, outMsgSize, DB_REQ_MSG);
  addTwoBytes(outMsg, outMsgSize, ownNode.port);

  char *countPtr = outMsg;
  outMsg += 2;
  int count = 0;

  otherVectorLock.lock();
  for (int i = 0, n = otherVector.size(); i < n; i++) {
    if (otherVector[i].port % 2 != ownNode.port % 2) {
      // We were not originally responsible for them. The other coord also is
      // included in this
      addTwoBytes(outMsg, outMsgSize, otherVector[i].port);
      count++;
    }
  }
  otherVectorLock.unlock();
  addTwoBytes(countPtr, outMsgSize, count);

  sendMsg(permOutMsg, outMsgSize, DB_LISTEN_PORT);
  return;
}		/* -----  end of function sendDbRequestMsg  ----- */

/* ===  FUNCTION  ==============================================================
 *         Name:  sendPigCoordMsg
 *  Description:  Sends our status to the other coordinator
 * =============================================================================
 */
void sendPigCoordMsg (bool areWeAlive)
{
  /*
  |--- 1 ---|--- 2 ---|--- 3 ---|--- 4 ---|
  <----- Msg Type ----X------ Status ----->
  */
  char msg[MAX_MSG_SIZE];
  char *outMsg = msg;
  memset(outMsg, 0, MAX_MSG_SIZE);
  char *permOutMsg = outMsg;
  int outMsgSize = 0;

  addTwoBytes(outMsg, outMsgSize, PIG_COORD_MSG);
  int status = (areWeAlive == true) ? 1 : 0;
  addTwoBytes(outMsg, outMsgSize, status);

  sendMsg(permOutMsg, outMsgSize, otherCoordPort);

  return;
}		/* -----  end of function sendPigCoordMsg  ----- */

/* ===  FUNCTION  ==============================================================
 *         Name:  sendStatusRespMsg
 * =============================================================================
 */
void sendStatusRespMsg (int coordPort)
{
  /*
  |--- 1 ---|--- 2 ---|--- 3 ---|--- 4 ---|
  <----- Msg Type ----X------- Port ------>
  <------- Posn ------X------ Status ----->
  */

  char msg[MAX_MSG_SIZE];
  char *outMsg = msg;
  memset(outMsg, 0, MAX_MSG_SIZE);
  char *permOutMsg = outMsg;
  int outMsgSize = 0;

  addTwoBytes(outMsg, outMsgSize, STATUS_RESP_MSG);
  int port = ownNode.port;
  addTwoBytes(outMsg, outMsgSize, port);
  int posn = ownNode.posn;
  addTwoBytes(outMsg, outMsgSize, posn);
  int status = (isHit == true) ? 0 : 1;
  addTwoBytes(outMsg, outMsgSize, status);

  sendMsg(permOutMsg, outMsgSize, coordPort);

  return;
}		/* -----  end of function sendStatusRespMsg  ----- */

/* ===  FUNCTION  ==============================================================
 *         Name:  handleStatusRespMsg
 * =============================================================================
 */
void handleStatusRespMsg (int inMsgSize, char *inMsg)
{
  /*
  |--- 1 ---|--- 2 ---|--- 3 ---|--- 4 ---|
  <----- Msg Type ----X------- Port ------>
  <------- Posn ------X------ Status ----->
  */

  char *outMsg = inMsg - 2;
  char *permOutMsg = outMsg;
  int outMsgSize = inMsgSize;
  // Updating database here
  addTwoBytes(outMsg, outMsgSize, DB_UPD_MSG);
  sendMsg(permOutMsg, outMsgSize, DB_LISTEN_PORT);

  unsigned short int port = getTwoBytes(inMsg, inMsgSize);
  int posn = getTwoBytes(inMsg, inMsgSize);
  int status = getTwoBytes(inMsg, inMsgSize);
  bool live = (status == 1) ? true : false;
  if (live == false) {
    score++;
  }
  if (port == ownNode.port) {
    ownNode.posn = posn;
    ownNode.live = live;
  } else {
    otherVectorLock.lock();
    for (int i = 0, n = otherVector.size(); i < n; i++) {
      if (otherVector[i].port == port) {
        otherVector[i].posn = posn;
        otherVector[i].live = live;
      }
    }
    otherVectorLock.unlock();
  }

  sentCount--;
  if (sentCount == 0) {
    // All responses have been received
    cout<<"Sent end launch to bird"<<endl;
    sendEndLaunchToBird();
    score = 0;
  }
  return;
}		/* -----  end of function handleStatusRespMsg  ----- */

/* ===  FUNCTION  ==============================================================
 *         Name:  handleStatusReqMsg
 *  Description:  The start of the game is indicated here. We know here whether
 *                we are a coordinator, and also all the pig positions
 * =============================================================================
 */
void handleStatusReqMsg (int inMsgSize, char *inMsg)
{
  /*
  |--- 1 ---|--- 2 ---|--- 3 ---|--- 4 ---|
  <----- Msg Type ----X--- Coord Port  --->
  */

  int coordPort = getTwoBytes(inMsg, inMsgSize);
  sendStatusRespMsg(coordPort);
}		/* -----  end of function handleStatusReqMsg  ----- */

/* ===  FUNCTION  ==============================================================
 *         Name:  sendDbUpdMsg
 *  Description:  Sends an update to the database
 * =============================================================================
 */
void sendDbUpdMsg (int port, int posn, int status)
{
  /*
  |--- 1 ---|--- 2 ---|--- 3 ---|--- 4 ---|
  <----- Msg Type ----X------- Port ------>
  <------- Posn ------X----- Status ------> 
  */
  char msg[MAX_MSG_SIZE];
  char *outMsg = msg;
  memset(outMsg, 0, MAX_MSG_SIZE);
  char *permOutMsg = outMsg;
  int outMsgSize = 0;

  addTwoBytes(outMsg, outMsgSize, DB_UPD_MSG);
  addTwoBytes(outMsg, outMsgSize, port);
  addTwoBytes(outMsg, outMsgSize, posn);
  addTwoBytes(outMsg, outMsgSize, status);

  sendMsg(permOutMsg, outMsgSize, DB_LISTEN_PORT);

  return;
}		/* -----  end of function sendDbUpdMsg  ----- */

/* ===  FUNCTION  ==============================================================
 *         Name:  handleStartGameMsg
 *  Description:  The start of the game is indicated here. We know here whether
 *                we are a coordinator, and also all the pig positions
 * =============================================================================
 */
void handleStartGameMsg (int inMsgSize, char *inMsg)
{
  /*
  |--- 1 ---|--- 2 ---|--- 3 ---|--- 4 ---|
  <----- Msg Type ----X- Other coord port-> 
  <------- Count -----X--- Port Pig 1 ---->
  <---- Posn Pig 1 ---X ...
  */

  // Since we received this message, we are a coordinator

  cout<<"Received start game msg"<<endl;
  otherCoordPort = getTwoBytes(inMsg, inMsgSize);
  unsigned int count = getTwoBytes(inMsg, inMsgSize);

  otherVectorLock.lock();
  while (count--) {
    unsigned int port = getTwoBytes(inMsg, inMsgSize);
    unsigned int posn = getTwoBytes(inMsg, inMsgSize);
    if (port == ownNode.port) {
      ownNode.posn = posn;
      ownNode.live = true;
    } else {
      for (int i = 0, n = otherVector.size(); i < n; i++) {
        if (otherVector[i].port == port) {
          otherVector[i].posn = posn;
          otherVector[i].live = true;
        }
      }
    }
    sendDbUpdMsg(port, posn, 1);
  }
  otherVectorLock.unlock();
  ownCoordPrevAlive = true;
  otherCoordPrevAlive = true;
}		/* -----  end of function handleStartGameMsg  ----- */


/* ===  FUNCTION  ==============================================================
 *         Name:  handleBirdPosnMsg
 *  Description:  We are a coordinator and we have received a bird posn msg for
 *                this bird launch
 * =============================================================================
 */
void handleBirdPosnMsg (int inMsgSize, char *inMsg)
{
  /*
  |--- 1 ---|--- 2 ---|--- 3 ---|--- 4 ---|
  <----- Msg Type ----X---- Bird Posn ---->
  */
  coordLock.lock();
  unsigned short int posn = getTwoBytes(inMsg, inMsgSize);
  birdPosn = posn;

  if (ownCoordPrevAlive == false) {
    // We are already dead. Ignore
    coordLock.unlock();
    return;
  }

  receivedBirdPosnMsg = true;
  ownCoordAlive =  (rand() % COORD_ALIVE_CHANCE == 0) ? false : true;
  if (ownCoordAlive == false) {
    cout<<ownNode.port<<": we are dead"<<endl;
  }

  if (otherCoordPrevAlive == false || receivedOtherCoordMsg == true) {
    // The other coord has already made a decision
    if (otherCoordPrevAlive == false) {
      otherCoordAlive = false;
    }
    if ((otherCoordAlive == false) && (ownCoordAlive == false)) {
      // If we have also decided to die, we need to tie break
      if (otherCoordPrevAlive == false || ownNode.port > otherCoordPort) {
        // If the other coord had previously been dead, we can't die
        cout<<ownNode.port<<": Both are dead, we will stay alive"<<endl;
        ownCoordAlive = true;
      } else {
        otherCoordAlive = true;
      }
    }
    if (otherCoordPrevAlive == true && otherCoordAlive == false) {
      // We need to get the other coord's subordinate ports' info
      sendDbRequestMsg();
    }
    if (otherCoordPrevAlive == true) {
      sendPigCoordMsg(ownCoordAlive);
    }
    bool temp = otherCoordAlive;
    otherCoordPrevAlive = temp;
    temp = ownCoordAlive;
    ownCoordPrevAlive = temp;
    receivedOtherCoordMsg = false;
    receivedBirdPosnMsg = false;
    coordLock.unlock();
    if (ownCoordAlive == true) {
      handlePigs();
    }
  } else {
    if (otherCoordPrevAlive == true) {
      sendPigCoordMsg(ownCoordAlive);
    }
    coordLock.unlock();
  }
  return;
}		/* -----  end of function handleBirdPosnMsg  ----- */

/* ===  FUNCTION  ==============================================================
 *         Name:  handleDbRespMsg
 *  Description:  Receives other coordinator's status
 * =============================================================================
 */
void handleDbRespMsg (int inMsgSize, char *inMsg)
{
  /*
  |--- 1 ---|--- 2 ---|--- 3 ---|--- 4 ---|
  <----- Msg Type ----X------ Count ------>
  <------- Port ------X------- Posn ------>
  <------ Status ----- ...
  */

  cout<<ownNode.port<<": Got DB resp msg"<<endl;
  unsigned short int count = getTwoBytes(inMsg, inMsgSize);
  while (count--) {
    unsigned short int port = getTwoBytes(inMsg, inMsgSize);
    unsigned short int posn = getTwoBytes(inMsg, inMsgSize);
    unsigned short int status = getTwoBytes(inMsg, inMsgSize);
    bool live = (status == 1) ? true : false;

    otherVectorLock.lock();
    for (int i = 0, n = otherVector.size(); i < n; i++) {
      if (port == otherVector[i].port) {
        if (live == false) {
          cout<<"Received dead pig from the db: "<<port<<endl;
        }
        otherVector[i].posn = posn;
        otherVector[i].live = live;
        break;
      }
    }
    otherVectorLock.unlock();
  }

  coordLock.lock();
  bool temp = otherCoordAlive;
  otherCoordPrevAlive = temp;
  temp = ownCoordAlive;
  ownCoordPrevAlive = temp;
  receivedOtherCoordMsg = false;
  receivedBirdPosnMsg = false;
  coordLock.unlock();
  if (ownCoordAlive == true) {
    handlePigs();
  }
  return;
}		/* -----  end of function handleDbRespMsg  ----- */

/* ===  FUNCTION  ==============================================================
 *         Name:  handlePigCoordMsg
 *  Description:  Receives other coordinator's status
 * =============================================================================
 */
void handlePigCoordMsg (int inMsgSize, char *inMsg)
{
  /*
  |--- 1 ---|--- 2 ---|--- 3 ---|--- 4 ---|
  <----- Msg Type ----X------ Status ----->
  */
  cout<<ownNode.port<<": Received pig coord msg"<<endl;
  coordLock.lock();
  unsigned short int status = getTwoBytes(inMsg, inMsgSize);
  receivedOtherCoordMsg = true;
  if (status == 1) {
    otherCoordAlive = true;
  } else {
    otherCoordAlive = false;
  }
  if (receivedBirdPosnMsg == true) {
    // We have already made our decision
    if ((otherCoordAlive == false) && (ownCoordAlive == false)) {
      // If we have also decided to die, we need to tie break
      cout<<"Both are dead coordmsg"<<endl;
      if (ownNode.port > otherCoordPort) {
        ownCoordAlive = true;
      } else {
        otherCoordAlive = true;
      }
    }
    if (otherCoordPrevAlive == true && otherCoordAlive == false) {
      // We need to get the other coord's subordinate ports' info
      sendDbRequestMsg();
      coordLock.unlock();
      return;
    }
    bool temp = otherCoordAlive;
    otherCoordPrevAlive = temp;
    temp = ownCoordAlive;
    ownCoordPrevAlive = temp;
    receivedOtherCoordMsg = false;
    receivedBirdPosnMsg = false;
    coordLock.unlock();
    if (ownCoordAlive == true) {
      handlePigs();
    }
  } else {
    coordLock.unlock();
  }
  return;
}		/* -----  end of function handlePigCoordMsg  ----- */

/* ===  FUNCTION  ==============================================================
 *         Name:  pigMsgHandler
 *  Description:  This function accepts all the messages, finds out their type,
 *                and calls their respective handlers.
 * =============================================================================
 */
void pigMsgHandler (int inMsgSize, char *inMsg)
{
  if (inMsgSize < 2) {
    cout<<"Corrupted message at pig "<<ownNode.port<<endl;
    return;
  }

  short unsigned int msgType = getTwoBytes(inMsg, inMsgSize);;

  switch (msgType) {
    case START_GAME_MSG: {
      handleStartGameMsg (inMsgSize, inMsg);
      break;
    }
    case BIRD_POSN_MSG: {
      handleBirdPosnMsg (inMsgSize, inMsg);
      break;
    }
    case PIG_COORD_MSG: {
      handlePigCoordMsg (inMsgSize, inMsg);
      break;
    }
    case STATUS_REQ_MSG: {
      handleStatusReqMsg (inMsgSize, inMsg);
      break;
    }
    case STATUS_RESP_MSG: {
      handleStatusRespMsg (inMsgSize, inMsg);
      break;
    }
    case BIRD_APPROACH_MSG: {
      handleFromCoordBirdApproachMsg (inMsgSize, inMsg);
      break;
    }
    case DB_RESP_MSG: {
      handleDbRespMsg (inMsgSize, inMsg);
      break;
    }
    default:
    {
      cout<<"Invalid msg "<<msgType<<" received at pig "<<ownNode.port<<endl;
      break;
    }
  }
  return;
}		/* -----  end of function pigMsgHandler  ----- */
