#include "cs104_connection.h"
#include "hal_thread.h"
#include "hal_time.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define BUF_SIZE 2048

static bool running = true;

void sigint_handler(int signalId) { running = false; }

/* Callback handler to log sent or received messages (optional) */
static void rawMessageHandler(void *parameter, uint8_t *msg, int msgSize,
                              bool sent) {
  if (sent)
    printf("raw SEND: ");
  else
    printf("raw RCVD: ");

  int i;
  for (i = 0; i < msgSize; i++) {
    printf("%02x ", msg[i]);
  }
  printf("\n");
}

/* Connection event handler */
static void connectionHandler(void *parameter, CS104_Connection connection,
                              CS104_ConnectionEvent event) {
  switch (event) {
  case CS104_CONNECTION_OPENED:
    printf("Connection established\n");
    break;
  case CS104_CONNECTION_CLOSED:
    printf("Connection closed\n");
    break;
  case CS104_CONNECTION_STARTDT_CON_RECEIVED:
    printf("Received STARTDT_CON\n");
    break;
  case CS104_CONNECTION_STOPDT_CON_RECEIVED:
    printf("Received STOPDT_CON\n");
    break;
  }
}


void sendFloatData(int typeID, int address, float value, int q, long long ts) {
   printf("{\"ASDU\":\"%s\", \"type\":\"%i\", \"address\":%i, \"value\":%f, \"chstatus\":%i, \"ts\":%llu}\n",
           TypeID_toString(typeID), typeID, address, value, q, ts);
}

void sendIntData(int typeID, int address, int value, int q, long long ts) {
   printf("{\"ASDU\":\"%s\", \"type\":\"%i\", \"address\":%i, \"value\":%d, \"chstatus\":%i, \"ts\":%llu}\n",
           TypeID_toString(typeID), typeID, address, value, q, ts);
}

void sendBoolData(int typeID, int address, bool bval, int q, long long ts) {
  int value = 0;
  if (bval) value = 1;
   printf("{\"ASDU\":\"%s\", \"type\":\"%i\", \"address\":%i, \"value\":%d, \"chstatus\":%i, \"ts\":%llu}\n",
           TypeID_toString(typeID), typeID, address, value, q, ts);
}


/*
 * CS101_ASDUReceivedHandler implementation
 *
 * For CS104 the address parameter has to be ignored
 */
static bool asduReceivedHandler(void *parameter, int address, CS101_ASDU asdu) {
  int i;
  int typeID = CS101_ASDU_getTypeID(asdu);
  int numberOfElements = CS101_ASDU_getNumberOfElements(asdu);
  printf("%s, elements: %i\n", TypeID_toString(typeID), numberOfElements);

  switch (typeID) {
  case M_SP_NA_1: /* 1 */
    for (i = 0; i < numberOfElements; i++) {
      SinglePointInformation io = 
        (SinglePointInformation) CS101_ASDU_getElement(asdu, i);

      sendBoolData(typeID, 
        InformationObject_getObjectAddress((InformationObject)io), 
        SinglePointInformation_getValue((SinglePointInformation)io),
        SinglePointInformation_getQuality((SinglePointInformation)io),
        0);
      SinglePointInformation_destroy(io);
    }
    break;

  case M_SP_TA_1: /* 2 */
   for (i = 0; i < numberOfElements; i++) {
      SinglePointWithCP24Time2a io = 
        (SinglePointWithCP24Time2a) CS101_ASDU_getElement(asdu, i);

      sendBoolData(typeID, 
        InformationObject_getObjectAddress((InformationObject)io), 
        SinglePointInformation_getValue((SinglePointInformation)io),
        SinglePointInformation_getQuality((SinglePointInformation)io),
        0);  // SinglePointWithCP24Time2a_getTimestamp( io)); НЕПОНЯТНО КАК ВЗЯТЬ ts
      SinglePointWithCP24Time2a_destroy(io);
    }
    break;

   case M_DP_NA_1: /* 3 */
   for (i = 0; i < numberOfElements; i++) {
      DoublePointInformation io = 
        (DoublePointInformation) CS101_ASDU_getElement(asdu, i);

      sendBoolData(typeID, 
        InformationObject_getObjectAddress((InformationObject)io), 
        DoublePointInformation_getValue(io),
        DoublePointInformation_getQuality(io),
        0);
      DoublePointInformation_destroy(io);
    }
    break;

  case M_ME_NB_1: /* 11 */
    for (i = 0; i < numberOfElements; i++) {
      MeasuredValueScaled io =
        (MeasuredValueScaled)CS101_ASDU_getElement(asdu, i);

      sendIntData(typeID, 
        InformationObject_getObjectAddress((InformationObject)io),
        MeasuredValueScaled_getValue((MeasuredValueScaled)io),
        MeasuredValueScaled_getQuality((MeasuredValueScaled)io),
        0);
      MeasuredValueScaled_destroy(io);
    }
    break;

    
    case M_IT_NA_1: /* 15 */
    for (i = 0; i < numberOfElements; i++) {
      IntegratedTotals io =
        (IntegratedTotals)CS101_ASDU_getElement(asdu, i);

      sendIntData(typeID, 
        InformationObject_getObjectAddress((InformationObject)io),
        BinaryCounterReading_getValue(IntegratedTotals_getBCR( io )),
        0, // ?quality
        0);
      IntegratedTotals_destroy(io);
    }
    break;


  case M_ME_TE_1: /* 35 */
    for (i = 0; i < numberOfElements; i++) {
      MeasuredValueScaledWithCP56Time2a io =
        (MeasuredValueScaledWithCP56Time2a)CS101_ASDU_getElement(asdu, i);

      sendIntData(typeID, 
        InformationObject_getObjectAddress((InformationObject)io),
        MeasuredValueScaled_getValue((MeasuredValueScaled)io),
        MeasuredValueScaled_getQuality((MeasuredValueScaled)io),
        CP56Time2a_toMsTimestamp( MeasuredValueScaledWithCP56Time2a_getTimestamp(io)) );
      MeasuredValueScaledWithCP56Time2a_destroy(io);
    }
    break;
    
//  case C_TS_TA_1:
//    printf("C_TS_TA_1  test command with timestamp\n");
//    break;
  default:
    printf("Received ASDU type: %s (%i)\n", TypeID_toString(typeID), typeID);
  }
  return true;
}


/**
 * Обработать команду с сервера NODE
 *   Формат: CMD:<type> <adr> <value> 
 */
void processStdinCommand(CS104_Connection con) {
  int nread;
  char buf[BUF_SIZE + 1];
  int typeID = 0;
  int val = 0;
  int adr = 0;

  bzero(buf, BUF_SIZE);
  nread = read(STDIN_FILENO, buf, BUF_SIZE);
  if (nread <= 0) return;

  printf("Get command %s (%d chars)\n", buf, nread);
  if (strncmp("CMD:",buf,4)!=0) {
       printf("Expected CMD:type adr value, received %s (%d chars)\n", buf, nread);
       return;
  }
  sscanf(&buf[4], "%d %d %d",  &typeID, &adr, &val);
  printf("Type=%d adr=%d val=%d\n", typeID, adr, val);

  InformationObject sc;
  bool bval = false;

  switch (typeID) {
  case 45: /* C_SC_NA */
    if (val == 1) bval = true;
    sc = (InformationObject) SingleCommand_create(NULL, adr, bval, false, 0);
    break;

  case 48: /* C_SE_NA */
    sc = (InformationObject) SetpointCommandNormalized_create(NULL, adr, val, false, 0);
    break;

  case 49: /* C_SE_NB */
    sc = (InformationObject) SetpointCommandScaled_create(NULL, adr, val, false, 0);
    break;
 
  default:
    printf("Unknown CMD type: %s (%i) Address: %i\n", TypeID_toString(typeID), typeID, adr);
    return;
  }
  printf("Send control command %s(%d) adr=%d value=%d\n", TypeID_toString(typeID), typeID, adr, val);
  CS104_Connection_sendProcessCommandEx(con, CS101_COT_ACTIVATION, 1, sc);
  InformationObject_destroy(sc);
}


CS104_Connection createConnection(int argc, char **argv) {
  const char *ip = "localhost";
  uint16_t port = IEC_60870_5_104_DEFAULT_PORT;
  const char *localIp = NULL;
  int localPort = -1;
  int originatorAddress = 1;

  if (argc > 1)
    ip = argv[1];

  if (argc > 2)
    port = atoi(argv[2]);

  if (argc > 3)
    originatorAddress = atoi(argv[3]);

  /*
  if (argc > 3)
    localIp = argv[3];

  if (argc > 4)
    port = atoi(argv[4]);
  */

  printf("Connecting to: %s:%i\n", ip, port);
  CS104_Connection con = CS104_Connection_create(ip, port);
  CS101_AppLayerParameters alParams =
      CS104_Connection_getAppLayerParameters(con);
  alParams->originatorAddress = 3;


// static struct sCS101_AppLayerParameters defaultAppLayerParameters = {
//    /* .sizeOfTypeId =  */ 1,
//    /* .sizeOfVSQ = */ 1,
//    /* .sizeOfCOT = */ 2,
//    /* .originatorAddress = */ 0,
//    /* .sizeOfCA = */ 2,
//    /* .sizeOfIOA = */ 3,
//    /* .maxSizeOfASDU = */ 249   
// };



//  CS104_APCIParameters apciParams = CS104_Connection_getAPCIParameters(con);
//  static struct sCS104_APCIParameters defaultConnectionParameters = {
//	/* .k = */ 12,
//	/* .w = */ 8,
//	/* .t0 = */ 10,
//	/* .t1 = */ 15,
//	/* .t2 = */ 10,
//	/* .t3 = */ 20
// };

  CS104_Connection_setConnectionHandler(con, connectionHandler, NULL);
  CS104_Connection_setASDUReceivedHandler(con, asduReceivedHandler, NULL);

  /* optional bind to local IP address/interface */
  if (localIp)
    CS104_Connection_setLocalAddress(con, localIp, localPort);

  /* uncomment to log messages */
  // CS104_Connection_setRawMessageHandler(con, rawMessageHandler, NULL);
  return con;
}

int main(int argc, char **argv) {
  fd_set fds;
  struct timeval tv;
  int max_fd, res, nread;
  char buf[BUF_SIZE + 1];

  signal(SIGINT, sigint_handler);
  signal(SIGTERM, sigint_handler);
  // Если не установить, данные очень долго накапливаются
  setvbuf(stdin, NULL, _IONBF, 0);
  setvbuf(stdout, NULL, _IONBF, 0);

  CS104_Connection con = createConnection(argc, argv);
  if (CS104_Connection_connect(con)) {
    printf("Connected!\n");
    CS104_Connection_sendStartDT(con);
    Thread_sleep(2000);

    CS104_Connection_sendInterrogationCommand(con, CS101_COT_ACTIVATION, 1, IEC60870_QOI_STATION);
    Thread_sleep(5000);

    // struct sCP56Time2a testTimestamp;
    // CP56Time2a_createFromMsTimestamp(&testTimestamp, Hal_getTimeInMs());
    // CS104_Connection_sendTestCommandWithTimestamp(con, 1, 0x4938, &testTimestamp);

    while (running) {
      FD_ZERO(&fds); // Команды через stdin
      FD_SET(STDIN_FILENO, &fds);
      max_fd = STDIN_FILENO + 1;
      tv.tv_sec = 1;
      tv.tv_usec = 0; //в микросекундах = 1000 мсек
      res = select(max_fd, &fds, NULL, NULL, &tv);
      if (res >= 0) {
        if (FD_ISSET(STDIN_FILENO, &fds))
          processStdinCommand(con);
      } else
        perror("select()");
    }
  } else {
    printf("Connect failed!\n");
  }

  CS104_Connection_destroy(con);
  printf("exit\n");
}

