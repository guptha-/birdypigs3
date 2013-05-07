/*==============================================================================
 *
 *       Filename:  birdmain.cpp
 *
 *    Description:  The bird is responsible for spawning pigs, sending the bird
 *                  landing coordinates and the bird land msg to the coordinator
 *
 * =============================================================================
 */

#include "../inc/birdinc.h"

vector<unsigned short int> pigPorts;
vector<unsigned short int> pigPosns;
static unsigned short int coord[2];
static mutex pigLock;

/* ===  FUNCTION  ==============================================================
 *         Name:  calculatePigPosns
 *  Description:  Randomly calculates positions of each pig
 * =============================================================================
 */
void calculatePigPosns ()
{
  pigLock.lock();
  pigPosns.clear();
  for (int i = 0, n = pigPorts.size(); i < n; i++) {
    pigPosns.push_back(rand() % MAX_POSN + 1);
  }
  pigLock.unlock();
  return;
}		/* -----  end of function calculatePigPosns  ----- */

/* ===  FUNCTION  ==============================================================
 *         Name:  determineCoordinators
 *  Description:  Calculates the coordinators. It helps to have a balanced set 
 *                of even and odd port numbers in order to balance the load
 * =============================================================================
 */
void determineCoordinators ()
{
  pigLock.lock();
  coord[0] = 0;
  coord[1] = 0;
  unsigned short int minEvenPosn= MAX_POSN + 1;
  unsigned short int minOddPosn = MAX_POSN + 1;
  for (int i = 0, n = pigPorts.size(); i < n; i++) {
    if (pigPorts[i] % 2 == 0) {
      if (pigPosns[i] < minEvenPosn) {
        coord[0] = pigPorts[i];
        minEvenPosn = pigPosns[i];
      }
    } else {
      if (pigPosns[i] < minOddPosn) {
        coord[1] = pigPorts[i];
        minOddPosn = pigPosns[i];
      }
    }
  }

  if ((coord[0] == 0) || (coord[1] == 0)) {
    cout<<"There is no coordinator candidate for one of the types"<<endl;
    exit(0);
  }

  pigLock.unlock();
  return;
}		/* -----  end of function determineCoordinators  ----- */

/* ===  FUNCTION  ==============================================================
 *         Name:  birdStartNewLaunch
 *  Description:  This function starts off a new bird launch
 * =============================================================================
 */
void birdStartNewLaunch()
{
  int birdPosn = rand() % MAX_POSN + 1;
  cout<<"The bird posn is "<<birdPosn<<endl;
  birdSendBirdPosnMsg(coord[0], birdPosn);
  birdSendBirdPosnMsg(coord[1], birdPosn);
  return;
}		/* -----  end of function birdStartNewLaunch  ----- */

/* ===  FUNCTION  ==============================================================
 *         Name:  birdStartNewGame
 *  Description:  This function starts off a new game
 * =============================================================================
 */
void birdStartNewGame()
{
  sleep(1);
  cout<<"Bird: Starting new game"<<endl;
  calculatePigPosns();
  determineCoordinators();

  pigLock.lock();
  birdSendStartGameMsg (coord[0], coord[1]);
  birdSendStartGameMsg (coord[1], coord[0]);
  // Wait for the pigs to process the start game message
  usleep (10000);
  birdStartNewLaunch();
  pigLock.unlock();
  return;
}		/* -----  end of function birdStartNewGame  ----- */

/* ===  FUNCTION  ==============================================================
 *         Name:  listenerFlow
 *  Description:  The bird listens for incoming messages here
 * =============================================================================
 */
static void listenerFlow ()
{
 UDPSocket listenSocket (BIRD_LISTEN_PORT);

  while (true) {
    // Block for msg receipt
    int inMsgSize;
    char *inMsg;
    inMsg = new char[MAX_MSG_SIZE]();
    try {
      inMsgSize = listenSocket.recv(inMsg, MAX_MSG_SIZE);
    } catch (SocketException &e) {
      cout<<"Bird: "<<e.what()<<endl;
    }
    inMsg[inMsgSize] = '\0';

    thread handlerThread (birdMsgHandler, inMsgSize, inMsg);
    handlerThread.detach();
  }
}   /* -----  end of function listenerFlow  ----- */


/* ===  FUNCTION  ==============================================================
 *         Name:  getPigPorts
 * =============================================================================
 */
static int getPigPorts ()
{
  ifstream portFile;
  string str;
  portFile.open("portConfig");
  if (!portFile.good()) {
    cout<<"No port config file found.\n";
    return EXIT_FAILURE;
  }
  cout<<"Port numbers \t";
  while (true) {
    getline(portFile, str);
    if (portFile.eof()) {
      break;
    }
    int portNum = atoi(str.c_str());
    if (portNum == BIRD_LISTEN_PORT) {
      cout<<portNum<<" is the port the coordinator listens at! Remove from"<<
        "port list!"<<endl;
      return EXIT_FAILURE;
    }
    pigPorts.push_back(portNum);
    cout<<portNum<<"\t";
    fflush(stdout);
  }
  cout<<endl;

  return EXIT_SUCCESS;
}   /* -----  end of function getPigPorts  ----- */

/* ===  FUNCTION  ==============================================================
 *         Name:  spawnDb
 * =============================================================================
 */
void spawnDb ()
{
  int child = fork();
  if (child < 0) {
    cout<<"Problem spawning db!"<<endl;
    return;
  } else if (child == 0) {
    // Child address space
    execl ("./bin/db", "db", NULL);
    cout<<"Problem spawning child"<<endl;
    cout<<errno;
    exit(0);
  }
  return;
}		/* -----  end of function spawnDb  ----- */

/* ===  FUNCTION  ==============================================================
 *         Name:  spawnPigs
 * =============================================================================
 */ 
int spawnPigs ()
{
  int pigPortsCount = pigPorts.size();
  system("killall -q -9 pig");
  
  for (auto &curPort : pigPorts) {
    int child = fork();
    if (child < 0) {
      cout<<"Problem spawning pigs!"<<endl;
      return EXIT_FAILURE;
    } else if (child == 0) {
      // Child address space
      char **array = (char **) malloc (sizeof(char *) * (pigPortsCount + 2));
      char **actualArray = array;
      (*array) = new char [4]();
      sprintf((*array), "pig");
      array++;
      (*array) = new char [6]();
      sprintf((*array), "%d", curPort);
      array++;
      for (auto &otherPort : pigPorts) {
        if (otherPort == curPort) {
          continue;
        }

        (*array) = new char [6]();
        sprintf((*array), "%d", otherPort);
        array++;
      }

      execv ("./bin/pig", actualArray);
      cout<<"Problem spawning child"<<endl;
      cout<<errno;
      exit(0);
    }
  }
  return EXIT_SUCCESS;
}   /* -----  end of function spawnPigs  ----- */

/* ===  FUNCTION  ==============================================================
 *         Name:  main
 * =============================================================================
 */
int main (int argc, char **argv) 
{
  srand(time(NULL));
  
  // Getting the ports of the pigs from the file
  if (EXIT_FAILURE == getPigPorts ()) {
    return EXIT_FAILURE;
  }

  // Listen on incoming port for messages
  thread handlerThread (listenerFlow);

  // Spawning the pigs
  if (EXIT_FAILURE == spawnPigs ()) {
    return EXIT_FAILURE;
  }
  spawnDb();

  birdStartNewGame();
  handlerThread.join();
  return EXIT_SUCCESS;
}				/* ----------  end of function main  ---------- */
