/*==============================================================================
 *
 *       Filename:  birdmsg.cpp
 *
 *    Description:  This sends/receives message for the bird.
 *
 * =============================================================================
 */

#include "../inc/birdinc.h"

static atomic<int> score(0);
static atomic<int> launchCount(0);
static atomic<int> gameCount(0);
static atomic<bool> receivedEndLaunch(false);

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
    cout<<"Bird: Cannot send msg"<<endl;
  }
  return;
}   /* -----  end of function sendMsg  ----- */

/* ===  FUNCTION  ==============================================================
 *         Name:  birdSendStartGameMsg
 * =============================================================================
 */
void birdSendStartGameMsg (int destPort, int otherCoord)
{
  /*
  |--- 1 ---|--- 2 ---|--- 3 ---|--- 4 ---|
  <----- Msg Type ----X- Other coord port-> 
  <------- Count -----X--- Port Pig 1 ---->
  <---- Posn Pig 1 ---X ...
  */
  char msg[MAX_MSG_SIZE];
  char *outMsg = msg;
  memset(outMsg, 0, MAX_MSG_SIZE);
  char *permOutMsg = outMsg;
  int outMsgSize = 0;

  addTwoBytes(outMsg, outMsgSize, START_GAME_MSG);
  addTwoBytes(outMsg, outMsgSize, otherCoord);
  int count = 0;
  char *countPtr = outMsg;
  outMsg = outMsg + 2;
  for (int i = 0, n = pigPorts.size(); i < n; i++) {
    if (destPort % 2 == pigPorts[i] % 2) {
      // Adding only the ports this coordinator is responsible for
      count++;
      addTwoBytes(outMsg, outMsgSize, pigPorts[i]);
      addTwoBytes(outMsg, outMsgSize, pigPosns[i]);
    }
  }
  addTwoBytes(countPtr, outMsgSize, count);

  sendMsg(permOutMsg, outMsgSize, destPort);

  return;
}		/* -----  end of function birdSendStartGameMsg  ----- */

/* ===  FUNCTION  ==============================================================
 *         Name:  birdSendBirdPosnMsg
 * =============================================================================
 */
void birdSendBirdPosnMsg (unsigned short int destPort, int birdPosn)
{
  /*
  |--- 1 ---|--- 2 ---|--- 3 ---|--- 4 ---|
  <----- Msg Type ----X--- Bird Posn ---->
  */
  char msg[MAX_MSG_SIZE];
  char *outMsg = msg;
  memset(outMsg, 0, MAX_MSG_SIZE);
  char *permOutMsg = outMsg;
  int outMsgSize = 0;

  addTwoBytes(outMsg, outMsgSize, BIRD_POSN_MSG);
  addTwoBytes(outMsg, outMsgSize, birdPosn);

  sendMsg(permOutMsg, outMsgSize, destPort);

  return;
}		/* -----  end of function birdSendBirdPosnMsg  ----- */

/* ===  FUNCTION  ==============================================================
 *         Name:  handleEndLaunchMsg
 *  Description:  This is the response from the coordinator(s) saying that the
 *                launch is completed. The bird can start another launch, or 
 *                start a new game
 * =============================================================================
 */
void handleEndLaunchMsg (char *inMsg, int inMsgSize)
{
  /*
  |--- 1 ---|--- 2 ---|--- 3 ---|--- 4 ---|
  <----- Msg Type ----X------ Score ------>
  <--- Sole Coord ---->
  */
  int newScore = getTwoBytes(inMsg, inMsgSize);
  score += newScore;
  int soleCoord = getTwoBytes(inMsg, inMsgSize);
  if (soleCoord == 1 || receivedEndLaunch == true) {
    // This launch is over
    receivedEndLaunch = false;
    launchCount++;
    if (launchCount == 10) {
      // This game is over
      launchCount = 0;
      gameCount++;
      cout<<"!!!The score is "<<score<<" after "<<gameCount<<" games"<<endl;
      cout<<endl;
      sleep(2);
      birdStartNewGame();
      return;
    }
    // Start off the next launch
    cout<<"!!Current score "<<score<<endl<<endl;
    sleep(2);
    birdStartNewLaunch();
  } else {
    receivedEndLaunch = true;
  }
  return;
}		/* -----  end of function handleEndLaunchMsg  ----- */

/* ===  FUNCTION  ==============================================================
 *         Name:  birdMsgHandler
 *  Description:  This function accepts all the messages, finds out their type,
 *                and calls their respective handlers.
 * =============================================================================
 */
void birdMsgHandler (int inMsgSize, char *inMsg)
{
  if (inMsgSize < 2) {
    cout<<"Corrupted message at bird "<<endl;
    return;
  }

  short unsigned int msgType = getTwoBytes(inMsg, inMsgSize);;

  switch (msgType) {
    case END_LAUNCH_MSG: {
      handleEndLaunchMsg(inMsg, inMsgSize);
      break;
    }
    default: {
      cout<<"Invalid msg received at bird "<<msgType<<endl;
      break;
    }
  }
  return;
}
