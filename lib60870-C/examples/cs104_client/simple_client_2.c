#include "cs104_connection.h"
#include "hal_thread.h"
#include "hal_time.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#ifdef __WIN32__
#include <winsock2.h>
#include <windows.h>
#endif

#define BUF_SIZE 2048

static bool running = true;

void sigint_handler(int signalId) { running = false; }

/* Callback handler to log sent or received messages (optional) */
static void rawMessageHandler(void *parameter, uint8_t *msg, int msgSize,
                              bool sent)
{
  if (sent)
    printf("raw SEND: ");
  else
    printf("raw RCVD: ");

  int i;
  for (i = 0; i < msgSize; i++)
  {
    printf("%02x ", msg[i]);
  }
  printf("\n");
}

/* Connection event handler */
static void connectionHandler(void *parameter, CS104_Connection connection,
                              CS104_ConnectionEvent event)
{
  switch (event)
  {
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

void sendFloatData(int typeID, int address, float value, int q, long long ts)
{
  printf("{\"ASDU\":\"%s\", \"type\":\"%i\", \"address\":%i, \"value\":%f, \"chstatus\":%i, \"ts\":%llu}\n",
         TypeID_toString(typeID), typeID, address, value, q, ts);
}

void sendIntData(int typeID, int address, int value, int q, long long ts)
{
  printf("{\"ASDU\":\"%s\", \"type\":\"%i\", \"address\":%i, \"value\":%d, \"chstatus\":%i, \"ts\":%llu}\n",
         TypeID_toString(typeID), typeID, address, value, q, ts);
}

void sendInt32Data(int typeID, int address, int32_t value, int q, long long ts)
{
  printf("{\"ASDU\":\"%s\", \"type\":\"%i\", \"address\":%i, \"value\":%d, \"chstatus\":%i, \"ts\":%llu}\n",
         TypeID_toString(typeID), typeID, address, value, q, ts);
}

void sendDwordData(int typeID, int address, uint32_t value, int q, long long ts)
{
  printf("{\"ASDU\":\"%s\", \"type\":\"%i\", \"address\":%i, \"value\":%d, \"chstatus\":%i, \"ts\":%llu}\n",
         TypeID_toString(typeID), typeID, address, value, q, ts);
}

void sendBoolData(int typeID, int address, bool bval, int q, long long ts)
{
  int value = 0;
  if (bval)
    value = 1;
  printf("{\"ASDU\":\"%s\", \"type\":\"%i\", \"address\":%i, \"value\":%d, \"chstatus\":%i, \"ts\":%llu}\n",
         TypeID_toString(typeID), typeID, address, value, q, ts);
}

/*
 * CS101_ASDUReceivedHandler implementation
 *
 * For CS104 the address parameter has to be ignored
 */
static bool asduReceivedHandler(void *parameter, int address, CS101_ASDU asdu)
{
  int i;
  int typeID = CS101_ASDU_getTypeID(asdu);
  int numberOfElements = CS101_ASDU_getNumberOfElements(asdu);
  printf("%s, elements: %i\n", TypeID_toString(typeID), numberOfElements);

  switch (typeID)
  {
  case M_SP_NA_1: /* 1 */
    for (i = 0; i < numberOfElements; i++)
    {
      SinglePointInformation io =
          (SinglePointInformation)CS101_ASDU_getElement(asdu, i);

      sendBoolData(typeID,
                   InformationObject_getObjectAddress((InformationObject)io),
                   SinglePointInformation_getValue((SinglePointInformation)io),
                   SinglePointInformation_getQuality((SinglePointInformation)io),
                   0);
      SinglePointInformation_destroy(io);
    }
    break;

  case M_DP_NA_1: /* 3 */
    for (i = 0; i < numberOfElements; i++)
    {
      DoublePointInformation io =
          (DoublePointInformation)CS101_ASDU_getElement(asdu, i);

      sendIntData(typeID,
                  InformationObject_getObjectAddress((InformationObject)io),
                  DoublePointInformation_getValue(io),
                  DoublePointInformation_getQuality(io),
                  0);
      DoublePointInformation_destroy(io);
    }
    break;

  case M_ST_NA_1: /* 5 */
    for (i = 0; i < numberOfElements; i++)
    {
      StepPositionInformation io =
          (StepPositionInformation)CS101_ASDU_getElement(asdu, i);

      sendIntData(typeID,
                  InformationObject_getObjectAddress((InformationObject)io),
                  StepPositionInformation_getValue(io),
                  StepPositionInformation_getQuality(io),
                  0);
      StepPositionInformation_destroy(io);
    }
    break;

  case M_BO_NA_1: /* 7 */
    for (i = 0; i < numberOfElements; i++)
    {
      BitString32 io =
          (BitString32)CS101_ASDU_getElement(asdu, i);

      sendDwordData(typeID,
                    InformationObject_getObjectAddress((InformationObject)io),
                    BitString32_getValue(io),
                    BitString32_getQuality(io),
                    0);
      BitString32_destroy(io);
    }
    break;

  case M_ME_NA_1: /* 9 */
    for (i = 0; i < numberOfElements; i++)
    {
      MeasuredValueNormalized io =
          (MeasuredValueNormalized)CS101_ASDU_getElement(asdu, i);

      sendFloatData(typeID,
                    InformationObject_getObjectAddress((InformationObject)io),
                    MeasuredValueNormalized_getValue(io),
                    MeasuredValueNormalized_getQuality(io),
                    0);
      MeasuredValueNormalized_destroy(io);
    }
    break;

  case M_ME_NB_1: /* 11 */
    for (i = 0; i < numberOfElements; i++)
    {
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

  case M_ME_NC_1: /* 13 */
    for (i = 0; i < numberOfElements; i++)
    {
      MeasuredValueShort io =
          (MeasuredValueShort)CS101_ASDU_getElement(asdu, i);

      sendFloatData(typeID,
                    InformationObject_getObjectAddress((InformationObject)io),
                    MeasuredValueShort_getValue(io),
                    MeasuredValueShort_getQuality(io),
                    0);
      MeasuredValueShort_destroy(io);
    }
    break;
  case M_IT_NA_1: /* 15 */
    for (i = 0; i < numberOfElements; i++)
    {
      IntegratedTotals io =
          (IntegratedTotals)CS101_ASDU_getElement(asdu, i);
      int isInvalid;

      if (BinaryCounterReading_isInvalid(IntegratedTotals_getBCR(io))) {
        isInvalid = 1;
      } else {
        isInvalid = 0;
      }
       
      sendInt32Data(typeID,
                  InformationObject_getObjectAddress((InformationObject)io),
                  BinaryCounterReading_getValue(IntegratedTotals_getBCR(io)),
                  isInvalid, // isInvalid
                  0);
      IntegratedTotals_destroy(io);
    }
    break;

  case M_SP_TB_1: /* 30 */
    for (i = 0; i < numberOfElements; i++)
    {
      SinglePointWithCP56Time2a io =
          (SinglePointWithCP56Time2a)CS101_ASDU_getElement(asdu, i);

      sendBoolData(typeID,
                   InformationObject_getObjectAddress((InformationObject)io),
                   SinglePointInformation_getValue((SinglePointInformation)io),
                   SinglePointInformation_getQuality((SinglePointInformation)io),
                   CP56Time2a_toMsTimestamp(SinglePointWithCP56Time2a_getTimestamp(io)));
      SinglePointWithCP56Time2a_destroy(io);
    }
    break;

  case M_DP_TB_1: /* 31 */
    for (i = 0; i < numberOfElements; i++)
    {
      DoublePointWithCP56Time2a io =
          (DoublePointWithCP56Time2a)CS101_ASDU_getElement(asdu, i);

      sendIntData(typeID,
                  InformationObject_getObjectAddress((InformationObject)io),
                  DoublePointInformation_getValue((DoublePointInformation)io),
                  DoublePointInformation_getQuality((DoublePointInformation)io),
                  CP56Time2a_toMsTimestamp(DoublePointWithCP56Time2a_getTimestamp(io)));
      DoublePointWithCP56Time2a_destroy(io);
    }
    break;

  case M_ST_TB_1: /* 32 */
    for (i = 0; i < numberOfElements; i++)
    {
      StepPositionWithCP56Time2a io =
          (StepPositionWithCP56Time2a)CS101_ASDU_getElement(asdu, i);

      sendIntData(typeID,
                  InformationObject_getObjectAddress((InformationObject)io),
                  StepPositionInformation_getValue((StepPositionInformation)io),
                  StepPositionInformation_getQuality((StepPositionInformation)io),
                  CP56Time2a_toMsTimestamp(StepPositionWithCP56Time2a_getTimestamp(io)));
      StepPositionWithCP56Time2a_destroy(io);
    }
    break;

  case M_BO_TB_1: /* 33 */
    for (i = 0; i < numberOfElements; i++)
    {
      Bitstring32WithCP56Time2a io =
          (Bitstring32WithCP56Time2a)CS101_ASDU_getElement(asdu, i);

      sendDwordData(typeID,
                    InformationObject_getObjectAddress((InformationObject)io),
                    BitString32_getValue((BitString32)io),
                    BitString32_getQuality((BitString32)io),
                    CP56Time2a_toMsTimestamp(Bitstring32WithCP56Time2a_getTimestamp(io)));
      Bitstring32WithCP56Time2a_destroy(io);
    }
    break;

  case M_ME_TD_1: /* 34 */
    for (i = 0; i < numberOfElements; i++)
    {
      MeasuredValueNormalizedWithCP56Time2a io =
          (MeasuredValueNormalizedWithCP56Time2a)CS101_ASDU_getElement(asdu, i);

      sendFloatData(typeID,
                    InformationObject_getObjectAddress((InformationObject)io),
                    MeasuredValueNormalized_getValue((MeasuredValueNormalized)io),
                    MeasuredValueNormalized_getQuality((MeasuredValueNormalized)io),
                    CP56Time2a_toMsTimestamp(MeasuredValueNormalizedWithCP56Time2a_getTimestamp(io)));
      MeasuredValueNormalizedWithCP56Time2a_destroy(io);
    }
    break;

  case M_ME_TE_1: /* 35 */
    for (i = 0; i < numberOfElements; i++)
    {
      MeasuredValueScaledWithCP56Time2a io =
          (MeasuredValueScaledWithCP56Time2a)CS101_ASDU_getElement(asdu, i);

      sendIntData(typeID,
                  InformationObject_getObjectAddress((InformationObject)io),
                  MeasuredValueScaled_getValue((MeasuredValueScaled)io),
                  MeasuredValueScaled_getQuality((MeasuredValueScaled)io),
                  CP56Time2a_toMsTimestamp(MeasuredValueScaledWithCP56Time2a_getTimestamp(io)));
      MeasuredValueScaledWithCP56Time2a_destroy(io);
    }
    break;

  case M_ME_TF_1: /* 36 */
    for (i = 0; i < numberOfElements; i++)
    {
      MeasuredValueShortWithCP56Time2a io =
          (MeasuredValueShortWithCP56Time2a)CS101_ASDU_getElement(asdu, i);

      sendFloatData(typeID,
                    InformationObject_getObjectAddress((InformationObject)io),
                    MeasuredValueShort_getValue((MeasuredValueShort)io),
                    MeasuredValueShort_getQuality((MeasuredValueShort)io),
                    CP56Time2a_toMsTimestamp(MeasuredValueShortWithCP56Time2a_getTimestamp(io)));
      MeasuredValueShortWithCP56Time2a_destroy(io);
    }
    break;

  case M_IT_TB_1: /* 37 */
    for (i = 0; i < numberOfElements; i++)
    {
      IntegratedTotalsWithCP56Time2a io =
          (IntegratedTotalsWithCP56Time2a)CS101_ASDU_getElement(asdu, i);
      int isInvalid;
      
      if (BinaryCounterReading_isInvalid((BinaryCounterReading)(io))) {
        isInvalid = 1;
      } else {
        isInvalid = 0;
      }
      sendInt32Data(typeID,
                  InformationObject_getObjectAddress((InformationObject)io),
                  BinaryCounterReading_getValue((BinaryCounterReading)(io)),
                  isInvalid, // isInvalid
                  CP56Time2a_toMsTimestamp(IntegratedTotalsWithCP56Time2a_getTimestamp(io)));
      IntegratedTotalsWithCP56Time2a_destroy(io);
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
void processStdinCommand(CS104_Connection con)
{
  int nread;
  char buf[BUF_SIZE + 1];
  int typeID = 0;
  int val = 0;
  int selCmd = 0;
  int ql = 0;
  long long ts;
  float valF = 0;
  int adr = 0;

  //bzero(buf, BUF_SIZE);
  memset(buf, 0, sizeof *buf);
  nread = read(STDIN_FILENO, buf, BUF_SIZE);
  if (nread <= 0)
    return;

  printf("Get command %s (%d chars)\n", buf, nread);
  if (strncmp("CMD:", buf, 4) != 0)
  {
    printf("Expected CMD:type adr value, received %s (%d chars)\n", buf, nread);
    return;
  }
  //sscanf(&buf[4], "%d %d %d", &typeID, &adr, &val);
  sscanf(&buf[4], "%d", &typeID);
  printf("Type=%d", typeID);
  //printf("Type=%d adr=%d val=%d\n", typeID, adr, val);

  InformationObject sc;
  bool bval = false;
  bool bselCmd = false;
  
  switch (typeID)
  {
  case 45: /* C_SC_NA */
    sscanf(&buf[4], "%d %d %d %d %d", &typeID, &adr, &val, &selCmd, &ql);
    if (val == 1) bval = true;
    if (selCmd == 1) bselCmd = true;
    sc = (InformationObject)SingleCommand_create(NULL, adr, bval, bselCmd, ql);
    break;
  case 46: /* C_DC_NA */
    sscanf(&buf[4], "%d %d %d %d %d", &typeID, &adr, &val, &selCmd, &ql);
    if (selCmd == 1) bselCmd = true;
    sc = (InformationObject)DoubleCommand_create(NULL, adr, val, bselCmd, ql);
    break;
  case 47: /* C_RC_NA */
    sscanf(&buf[4], "%d %d %d %d %d", &typeID, &adr, &val, &selCmd, &ql);
    if (selCmd == 1) bselCmd = true;
    sc = (InformationObject)StepCommand_create(NULL, adr, val, bselCmd, ql);
    break;
  case 48: /* C_SE_NA */
    sscanf(&buf[4], "%d %d %f %d %d", &typeID, &adr, &valF, &selCmd, &ql);
    if (selCmd == 1) bselCmd = true;
    sc = (InformationObject)SetpointCommandNormalized_create(NULL, adr, valF, bselCmd, ql);
    break;

  case 49: /* C_SE_NB */
    sscanf(&buf[4], "%d %d %d %d %d", &typeID, &adr, &val, &selCmd, &ql);
    if (selCmd == 1) bselCmd = true;
    sc = (InformationObject)SetpointCommandScaled_create(NULL, adr, val, bselCmd, ql);
    break;

  case 50: /* C_SE_NC */
    sscanf(&buf[4], "%d %d %f %d %d", &typeID, &adr, &valF, &selCmd, &ql);
    if (selCmd == 1) bselCmd = true;
    sc = (InformationObject)SetpointCommandShort_create(NULL, adr, valF, bselCmd, ql);
    break; 

  case 51: /* C_BO_NA */
    sscanf(&buf[4], "%d %d %d", &typeID, &adr, &val);
    sc = (InformationObject)Bitstring32Command_create(NULL, adr, val);
    break;

  case 58: /* C_SC_TA */
    sscanf(&buf[4], "%d %d %d %d %d %llu", &typeID, &adr, &val, &selCmd, &ql, &ts);
    if (val == 1) bval = true;
    if (selCmd == 1) bselCmd = true;
    sc = (InformationObject)SingleCommandWithCP56Time2a_create(NULL, adr, bval, bselCmd, ql, CP56Time2a_createFromMsTimestamp(NULL, ts));
    break;

  case 59: /* C_DC_TA */
    sscanf(&buf[4], "%d %d %d %d %d %llu", &typeID, &adr, &val, &selCmd, &ql, &ts);
    if (val == 1) bval = true;
    if (selCmd == 1) bselCmd = true;
    sc = (InformationObject)DoubleCommandWithCP56Time2a_create(NULL, adr, bval, bselCmd, ql, CP56Time2a_createFromMsTimestamp(NULL, ts));
    break;

  case 60: /* C_RC_TA */
    sscanf(&buf[4], "%d %d %d %d %d %llu", &typeID, &adr, &val, &selCmd, &ql, &ts);
    if (selCmd == 1) bselCmd = true;
    sc = (InformationObject)StepCommandWithCP56Time2a_create(NULL, adr, val, bselCmd, ql, CP56Time2a_createFromMsTimestamp(NULL, ts));
    break;

  case 61: /* C_SE_TA */
    sscanf(&buf[4], "%d %d %f %d %d %llu", &typeID, &adr, &valF, &selCmd, &ql, &ts);
    if (selCmd == 1) bselCmd = true;
    sc = (InformationObject)SetpointCommandNormalizedWithCP56Time2a_create(NULL, adr, valF, bselCmd, ql, CP56Time2a_createFromMsTimestamp(NULL, ts));
    break;

  case 62: /* C_SE_TB */
    sscanf(&buf[4], "%d %d %d %d %d %llu", &typeID, &adr, &val, &selCmd, &ql, &ts);
    if (selCmd == 1) bselCmd = true;
    sc = (InformationObject)SetpointCommandScaledWithCP56Time2a_create(NULL, adr, val, bselCmd, ql, CP56Time2a_createFromMsTimestamp(NULL, ts));
    break;

  case 63: /* C_SE_TC */
    sscanf(&buf[4], "%d %d %f %d %d %llu", &typeID, &adr, &valF, &selCmd, &ql, &ts);
    if (selCmd == 1) bselCmd = true;
    sc = (InformationObject)SetpointCommandShortWithCP56Time2a_create(NULL, adr, valF, bselCmd, ql, CP56Time2a_createFromMsTimestamp(NULL, ts));
    break; 

  case 64: /* C_BO_TA */
    sscanf(&buf[4], "%d %d %d %llu", &typeID, &adr, &val, &ts);
    sc = (InformationObject)Bitstring32CommandWithCP56Time2a_create(NULL, adr, val, CP56Time2a_createFromMsTimestamp(NULL, ts));
    break;

  default:
    printf("Unknown CMD type: %s (%i) Address: %i\n", TypeID_toString(typeID), typeID, adr);
    return;
  }
  printf("Send control command %s(%d) adr=%d value=%d\n", TypeID_toString(typeID), typeID, adr, val);
  CS104_Connection_sendProcessCommandEx(con, CS101_COT_ACTIVATION, 1, sc);
  InformationObject_destroy(sc);
}

CS104_Connection createConnection(int argc, char *argv[])
{
  const char *ip = "localhost";
  uint16_t port = IEC_60870_5_104_DEFAULT_PORT;
  const char *localIp = NULL;
  int localPort = -1;
  int originatorAddress = 1;
  int k = 12;
  int w = 8;
  int t0 = 30;
  int t1 = 15;
  int t2 = 10;
  int t3 = 20;

  if (argc > 1)
    ip = argv[1];

  if (argc > 2)
    port = atoi(argv[2]);

  if (argc > 3)
    originatorAddress = atoi(argv[3]);

  if (argc > 4) 
    k = atoi(argv[4]);

  if (argc > 5) 
    w = atoi(argv[5]);

  if (argc > 6) 
    t0 = atoi(argv[6]);

  if (argc > 7) 
    t1 = atoi(argv[7]);

  if (argc > 8) 
    t2 = atoi(argv[8]);

  if (argc > 9) 
    t3 = atoi(argv[9]);

  printf("Connecting to: %s:%i\n", ip, port);
  CS104_Connection con = CS104_Connection_create(ip, port);
  CS101_AppLayerParameters alParams =
      CS104_Connection_getAppLayerParameters(con);
  alParams->originatorAddress = originatorAddress;

  // static struct sCS101_AppLayerParameters defaultAppLayerParameters = {
  //    /* .sizeOfTypeId =  */ 1,
  //    /* .sizeOfVSQ = */ 1,
  //    /* .sizeOfCOT = */ 2,
  //    /* .originatorAddress = */ 0,
  //    /* .sizeOfCA = */ 2,
  //    /* .sizeOfIOA = */ 3,
  //    /* .maxSizeOfASDU = */ 249
  // };

  CS104_APCIParameters apciParams = CS104_Connection_getAPCIParameters(con);
    apciParams->k = k;
    apciParams->w = w;
    apciParams->t0 = t0;
    apciParams->t1 = t1;
    apciParams->t2 = t2;
    apciParams->t3 = t3;
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

int main(int argc, char **argv)
{
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
  if (CS104_Connection_connect(con))
  {
    printf("Connected!\n");
    CS104_Connection_sendStartDT(con);
    Thread_sleep(2000);

    CS104_Connection_sendInterrogationCommand(con, CS101_COT_ACTIVATION, 1, IEC60870_QOI_STATION);
    Thread_sleep(5000);

    // struct sCP56Time2a testTimestamp;
    // CP56Time2a_createFromMsTimestamp(&testTimestamp, Hal_getTimeInMs());
    // CS104_Connection_sendTestCommandWithTimestamp(con, 1, 0x4938, &testTimestamp);

    while (running)
    {
      FD_ZERO(&fds); // Команды через stdin
      FD_SET(STDIN_FILENO, &fds);
      max_fd = STDIN_FILENO + 1;
      tv.tv_sec = 1;
      tv.tv_usec = 0; // в микросекундах = 1000 мсек
      res = select(max_fd, &fds, NULL, NULL, &tv);
      if (res >= 0)
      {
        if (FD_ISSET(STDIN_FILENO, &fds))
          processStdinCommand(con);
      }
      //else
        //perror("select()");
    }
  }
  else
  {
    printf("Connect failed!\n");
  }

  CS104_Connection_destroy(con);
  printf("exit\n");
}
