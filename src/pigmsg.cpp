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
static atomic<unsigned short int> otherCoordPort(0);
static atomic<bool> receivedOtherCoordMsg(false);
static atomic<bool> receivedBirdPosnMsg(false);
static atomic<bool> otherCoordAlive(false);
static atomic<bool> otherCoordPrevAlive;
static atomic<bool> ownCoordPrevAlive;
static atomic<bool> ownCoordAlive(false);
static mutex coordLock;;

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
 *         Name:  handlePigs
 *  Description:  Once the status of the other coordinator is known, the bird
 *                posn message is sent out to the other pigs
 * =============================================================================
 */
void handlePigs ()
{
  // TODO
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
// TODO
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
    return;
  }

  receivedBirdPosnMsg = true;
  ownCoordAlive =  (rand() % COORD_ALIVE_CHANCE == 0) ? false : true;

  if (receivedOtherCoordMsg == true) {
    // The other coord has already made a decision
    if ((otherCoordAlive == false) && (ownCoordAlive == false)) {
      // If we have also decided to die, we need to tie break
      if (ownNode.port > otherCoordPort) {
        ownCoordAlive = true;
      } else {
        otherCoordAlive = true;
      }
    }
    if (otherCoordPrevAlive == true && otherCoordAlive == false) {
      // We need to get the other coord's subordinate ports' info
      sendDbRequestMsg();
    }
    bool temp = otherCoordAlive;
    otherCoordPrevAlive = temp;
    temp = ownCoordAlive;
    ownCoordPrevAlive = temp;
    receivedOtherCoordMsg = false;
    receivedBirdPosnMsg = false;
    sendPigCoordMsg(ownCoordAlive);
    coordLock.unlock();
    if (ownCoordAlive == true) {
      handlePigs();
    }
  } else {
    sendPigCoordMsg(ownCoordAlive);
    coordLock.unlock();
  }
  return;
}		/* -----  end of function handleBirdPosnMsg  ----- */

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
      if (ownNode.port > otherCoordPort) {
        ownCoordAlive = true;
      } else {
        otherCoordAlive = true;
      }
    }
    if (otherCoordPrevAlive == true && otherCoordAlive == false) {
      // We need to get the other coord's subordinate ports' info
      sendDbRequestMsg();
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
    default:
    {
      cout<<"Invalid msg received at pig "<<ownNode.port<<endl;
      break;
    }
  }
  return;
}		/* -----  end of function pigMsgHandler  ----- */
