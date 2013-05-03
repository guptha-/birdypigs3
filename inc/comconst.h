#ifndef inc_COM_CONST
#define inc_COM_CONST

#define BIRD_LISTEN_PORT 11200
#define DB_LISTEN_PORT 11201
#define COM_IP_ADDR "127.0.0.1"
#define COORD_ALIVE_CHANCE 10
#define MAX_POSN 5

// Messages
#define MSG_TYPE_SIZE 2
#define PORT_SIZE 2
#define POSN_SIZE 2
#define MAX_MSG_SIZE 1000

enum MessageTypes {
  START_GAME_MSG = 1,
  DB_RESP_MSG,
  DB_REQ_MSG,
  DB_UPD_MSG,
  BIRD_POSN_MSG,
  PIG_COORD_MSG
};

#endif
