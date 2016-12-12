

/**********************************************************************
 * INCLUDES
 */
#include <termios.h>
#include <fcntl.h>

#include "types.h"
#include "config.h"
#include "socCmd.h"
#include "appCmd.h"

/**********************************************************************
 * LOCAL CONSTANTS
 */

#define APPCMDHEADER(len) \
0xFE,                                                                             \
len,   /*RPC payload Len                                      */     \
0x29, /*MT_RPC_CMD_AREQ + MT_RPC_SYS_APP        */     \
0x00, /*MT_APP_MSG                                                   */     \
0x0B, /*Application Endpoint                                  */     \
0x02, /*short Addr 0x0002                                     */     \
0x00, /*short Addr 0x0002                                     */     \
0x0B, /*Dst EP                                                             */     \
0xFF, /*Cluster ID 0xFFFF invalid, used for key */     \
0xFF, /*Cluster ID 0xFFFF invalid, used for key */     \






#define ZLL_MT_APP_RPC_CMD_TOUCHLINK                    0x01
#define ZLL_MT_APP_RPC_CMD_RESET_TO_FN                  0x02
#define ZLL_MT_APP_RPC_CMD_CH_CHANNEL                   0x03
#define ZLL_MT_APP_RPC_CMD_JOIN_HA                      0x04
#define ZLL_MT_APP_RPC_CMD_PERMIT_JOIN                  0x05
#define ZLL_MT_APP_RPC_CMD_SEND_RESET_TO_FN             0x06

#define MT_APP_RSP                                      0x80
#define MT_APP_ZLL_TL_IND                               0x81
#define MT_APP_ZLL_NEW_DEV_IND                          0x82

#define MT_DEBUG_MSG                                    0x80

#define COMMAND_LIGHTING_MOVE_TO_HUE                    0x00
#define COMMAND_LIGHTING_MOVE_TO_SATURATION             0x03
#define COMMAND_LEVEL_MOVE_TO_LEVEL                     0x00
#define COMMAND_LEVEL_MOVE_TO_LEVEL_WITH_ONOFF          0x04

/*** Foundation Command IDs ***/
#define ZCL_CMD_READ                                    0x00
#define ZCL_CMD_READ_RSP                                0x01
#define ZCL_CMD_WRITE                                   0x02
#define ZCL_CMD_WRITE_UNDIVIDED                         0x03
#define ZCL_CMD_WRITE_RSP                               0x04

// General Clusters
#define ZCL_CLUSTER_ID_GEN_IDENTIFY                          0x0003
#define ZCL_CLUSTER_ID_GEN_GROUPS                            0x0004
#define ZCL_CLUSTER_ID_GEN_SCENES                            0x0005
#define ZCL_CLUSTER_ID_GEN_ON_OFF                            0x0006
#define ZCL_CLUSTER_ID_GEN_LEVEL_CONTROL                     0x0008
// Lighting Clusters
#define ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL                0x0300

/*******************************/
/*** Scenes Cluster Commands ***/
/*******************************/
#define COMMAND_SCENE_STORE                               0x04
#define COMMAND_SCENE_RECALL                              0x05

/*******************************/
/*** Groups Cluster Commands ***/
/*******************************/
#define COMMAND_GROUP_ADD                                 0x00

#define ATTRID_ON_OFF                                     0x0000
#define ATTRID_LEVEL_CURRENT_LEVEL                        0x0000
#define ATTRID_LIGHTING_COLOR_CONTROL_CURRENT_HUE         0x0000
#define ATTRID_LIGHTING_COLOR_CONTROL_CURRENT_SATURATION  0x0001

/* The 3 MSB's of the 1st command field byte are for command type. */
#define MT_RPC_CMD_TYPE_MASK  0xE0

/* The 5 LSB's of the 1st command field byte are for the subsystem. */
#define MT_RPC_SUBSYSTEM_MASK 0x1F

#define MT_RPC_SOF         0xFE

typedef enum {
  MT_RPC_CMD_POLL = 0x00,
  MT_RPC_CMD_SREQ = 0x20,
  MT_RPC_CMD_AREQ = 0x40,
  MT_RPC_CMD_SRSP = 0x60,
  MT_RPC_CMD_RES4 = 0x80,
  MT_RPC_CMD_RES5 = 0xA0,
  MT_RPC_CMD_RES6 = 0xC0,
  MT_RPC_CMD_RES7 = 0xE0
} mtRpcCmdType_t;

typedef enum {
  MT_RPC_SYS_RES0,   /* Reserved. */
  MT_RPC_SYS_SYS,
  MT_RPC_SYS_MAC,
  MT_RPC_SYS_NWK,
  MT_RPC_SYS_AF,
  MT_RPC_SYS_ZDO,
  MT_RPC_SYS_SAPI,   /* Simple API. */
  MT_RPC_SYS_UTIL,
  MT_RPC_SYS_DBG,
  MT_RPC_SYS_APP,
  MT_RPC_SYS_OTA,
  MT_RPC_SYS_ZNP,
  MT_RPC_SYS_SPARE_12,
  MT_RPC_SYS_UBL = 13,  // 13 to be compatible with existing RemoTI.
  MT_RPC_SYS_MAX        // Maximum value, must be last (so 14-32 available, not yet assigned).
} mtRpcSysType_t;


/**********************************************************************
 * LOCAL TYPES
 */

/* None */


/**********************************************************************
 * LOCAL VARIABLES
 */
int serialPortFd = 0;
u8 transSeqNumber = 0;


/**********************************************************************
 * LOCAL FUNCTIONS
 */


 /*********************************************************************
 * @fn      calcFcs
 *
 * @brief   populates the Frame Check Sequence of the RPC payload.
 *
 * @param   msg - pointer to the RPC message
 *
 * @return  none
 */
void calcFcs(u8 *msg, int size)
{
	u8 result = 0;
	int idx = 1; //skip SOF
	int len = (size - 1);  // skip FCS

	while ((len--) != 0) {
		result ^= msg[idx++];
	}

	msg[(size-1)] = result;
}


/*********************************************************************
 * @fn      socOpen
 *
 * @brief   opens the serial port to the ZigBee chip.
 *
 * @param   devicePath - path to the UART device
 *
 * @return  status
 */
int socOpen(char *devicePath)
{
    int i;
    struct termios tio;

    /* open the device to be non-blocking (read will return immediatly) */
    serialPortFd = open(devicePath, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (serialPortFd <0) {
        perror(devicePath);
        printf("%s open failed\n",devicePath);
        return(-1);
    }

    /* c-iflags
     B115200 : set board rate to 115200
     CRTSCTS : HW flow control (disabled below)
     CS8     : 8n1 (8bit,no parity,1 stopbit)
     CLOCAL  : local connection, no modem contol
     CREAD   : enable receiving characters*/
    tio.c_cflag = B115200 | /*CRTSCTS |*/ CS8 | CLOCAL | CREAD;
    /* c-iflags
     ICRNL   : maps 0xD (CR) to 0x10 (LR), we do not want this.
     IGNPAR  : ignore bits with parity erros, I guess it is
     better to ignStateore an erronious bit then interprit it incorrectly. */
    tio.c_iflag = IGNPAR  & ~ICRNL;
    tio.c_oflag = 0;
    tio.c_lflag = 0;

    tcflush(serialPortFd, TCIFLUSH);
    tcsetattr(serialPortFd,TCSANOW,&tio);

    return serialPortFd;
}


/*********************************************************************
 * @fn      socClose
 *
 * @brief   close the serial port.
 *
 * @param   none
 *
 * @return  none
 */
void socClose( void )
{
    tcflush(serialPortFd, TCOFLUSH);
    close(serialPortFd);
    return;
}

 /*********************************************************************
 * @fn      zll_ctrlRspHandler
 *
 * @brief   process the contrl pipe command
 *
 * @param   none
 *
 * @return  none
 */
void zll_ctrlRspHandler(ctrl_cmd_t* pCmd)
{
    u16 nwkAddr, devID;
    u8 extAddr[8];
    u8 devType;
    u8 i, j;
    if (pCmd->cmdID == ZLL_CTRL_CMD_DEV_ANN_IND) {
        nwkAddr = *((u16*)&pCmd->payload[0]);
        memcpy(extAddr, &pCmd->payload[2], 8);
        printf("\nNew light join:\n    Network Addr : 0x%04x\n", nwkAddr);
        printf("    Long Addr    : ");
        for (i = 0; i < 8; i++) {
            printf("0x%02x ", extAddr[i]);
        }
        printf("\n\n");

        /* Report the data to apps */
        devID = *((u16*)&pCmd->payload[12]);
        if (devID == 0x0100 || devID == 0x0101 || devID == 0x0102 || devID == 0x0210) {
            devType = DEV_TYPE_LIGHT;
        } else if (devID = 0x0000) {
            devType = DEV_TYPE_ONOFF_SWITCH;
        } else {
            devType = DEV_TYPE_UNKNOWN;
        }
        nodes_add(nwkAddr, extAddr, pCmd->payload[10], devID, pCmd->payload[11]);
        app_sendDeviceReportCmd(devType, nwkAddr, extAddr);
    }
    else if (pCmd->cmdID == ZLL_CTRL_CMD_GET_NODES) {
        u8 *ptr = (u8*)&pCmd->payload[0];
        u8 nodeNum = *ptr++;
        printf("Node List (%d Node(s)) is:\n", nodeNum);
        for (i=0; i<nodeNum; i++) {
            printf("Node %d: \n", i + 1);
            nwkAddr = *((u16*)ptr);
            ptr += 2;
            printf("\tNetwork Addr : 0x%04x\n", nwkAddr);
        }
        printf("\n\n");
    }
}

 /*********************************************************************
 * @fn      zll_dataRspHandler
 *
 * @brief   process the data pipe command, ZCL
 *
 * @param   none
 *
 * @return  none
 */
void zll_dataRspHandler(data_cmd_t *pData)
{
    //if (transSeqNumber-1 != pData->zclTransSeqNo) {
    //    printf("not last command response\n");
    //}
    u8 status;
    u16 groupID;

    switch (pData->clusterID) {
    case ZCL_CLUSTER_ID_GEN_ON_OFF:
        if ( pData->payload[0] == 4 ) {
            printf("resetflash ");
            break;
        }
        #if 0
        printf("setstate ");
        if (pData->payload[0] == 1) {
            printf("on ");
        } else if (pData->payload[0] == 0) {
            printf("off ");
        } else if (pData->payload[0] == 2) {
            printf("toggle ");
        }
        #endif
         printf("received ZCL command ");
        if (pData->cmdID == 1) {
            printf("on \n");
        } else if (pData->cmdID == 0) {
            printf("off \n");
        } else if (pData->cmdID == 2) {
            printf("toggle \n");
        }
        break;

    case ZCL_CLUSTER_ID_GEN_LEVEL_CONTROL:
        printf("setlevel ");
        if (pData->payload[0] == 4) {
            printf("moveToLevWithOnoff ");
        } else if (pData->payload[0] == 0) {
            printf("moveToLev ");
        } else if (pData->payload[0] == 2) {
            printf("step ");
        }
        break;

    case ZCL_CLUSTER_ID_GEN_GROUPS:
        printf("group command ");
        if (pData->cmdID == 0x0B) {
            /* Default Response */
            break;
        }
        status = pData->payload[0];
        memcpy((u8*)&groupID, &pData->payload[1], 2);
        if (pData->cmdID == 0) {
            printf("add group response: 0x%x, groupID: 0x%x\n", status, groupID);
        }

        app_sendGroupRspCmd(pData->dstNwkAddr, groupID, pData->cmdID, status);

    default:

        break;
    }

    //if (pData->payload[1] == 0) {
    //    printf("success!\n\n");
    //} else {
    //    printf("fail! error code: 0x%x\n\n", pData->payload[1]);
    //}
}

 /*********************************************************************
 * @fn      processSocCmd
 *
 * @brief   read and process the RPC from the ZLL controller
 *
 * @param   none
 *
 * @return  none
 */
static u8 retryAttempts = 0;
void processSocCmd(void)
{
    u8 rpcLen, bytesRead, *rpcBuff, rpcBuffIdx;
    u8 rspLen;
    u8 rspBuf[50];
    u8 sof;

    int i;
    gw_app_cmd_t* pCmd;

    read(serialPortFd, &sof, 1);
    if (sof != 0xFE) {
        printf("zllSocProcessRpc: soc failed\n");
        return;
    }

    read(serialPortFd, &rspLen, 1);
    rspBuf[0] = rspLen;

    rpcBuffIdx = 1;
    while(rspLen > 0) {
        bytesRead = read(serialPortFd, &(rspBuf[rpcBuffIdx]), rspLen);

        //check for error
        if (bytesRead > rspLen) {
          //there was an error
          //printf("zllSocProcessRpc: read of %d bytes failed - %s\n", rpcLen, strerror(errno) );

            if (retryAttempts++ < 5 ) {
                //sleep for 10ms
	            usleep(10000);
                //try again
                bytesRead = 0;
            } else {
                //something went wrong.
                printf("zllSocProcessRpc: failed\n");
                return;
            }
        }

        rspLen -= bytesRead;
        rpcBuffIdx += bytesRead;
    }

    printf("received ZC command:\n");
    for (i = 0; i < rpcBuffIdx; i++) {
        printf("0x%x ", rspBuf[i]);
    }
    printf("\n");

    pCmd = (gw_app_cmd_t*)&rspBuf;
    switch (pCmd->cmd1) {
    case 0x80:
        zll_dataRspHandler(&pCmd->data.dataCmd);
        break;

    case 0x81:
        zll_ctrlRspHandler(&pCmd->data.ctrlCmd);
        break;

    default:
        break;
    }

}


/*********************************************************************
 * @fn      zllSocTouchLink
 *
 * @brief   Send the touchLink command to the CC253x.
 *
 * @param   none
 *
 * @return  none
 */
void zllSocTouchLink(void)
{
	u8 tlCmd[] = {
		APPCMDHEADER(13)
		0x06, //Data Len
		0x02, //Address Mode
		0x00, //2dummy bytes
		0x00,
		ZLL_MT_APP_RPC_CMD_TOUCHLINK,
		0x00,     //
		0x00,     //
		0x00       //FCS - fill in later
    };

    calcFcs(tlCmd, sizeof(tlCmd));
    write(serialPortFd,tlCmd, sizeof(tlCmd));
    tcflush(serialPortFd, TCOFLUSH);
}

/*********************************************************************
 * @fn      zllSocGetNodes
 *
 * @brief   Send the get nodes command to the CC253x.
 *
 * @param   none
 *
 * @return  none
 */
void zllSocGetNodes(void)
{
    u8 cmd[30];
  	int i;
  	cmd[0] = 0xFE;
  	gw_app_cmd_t *pCmd = (gw_app_cmd_t*)(&cmd[1]);
  	pCmd->len = 15;
  	pCmd->cmd0 = 0x49;
  	pCmd->cmd1 = 0x00;
  	pCmd->data.ctrlCmd.endpoint = 0xB;
    pCmd->data.ctrlCmd.clusterID = 0xffff;
    pCmd->data.ctrlCmd.dataLen = 3;
    pCmd->data.ctrlCmd.cmdID = ZLL_CTRL_CMD_GET_NODES;

    //for(i=0; i<pCmd->len+1; i++) {
    //    printf("0x%x ", cmd[i]);
    //}
    //printf("\n");

    write(serialPortFd,cmd,sizeof(cmd));
    tcflush(serialPortFd, TCOFLUSH);

}



/*********************************************************************
 * @fn      zllSocDemoBind
 *
 * @brief   Send the bind address to Coordinator
 *
 * @param   none
 *
 * @return  none
 */
void zllSocDemoBind(u8 addrMode, u16 addr)
{
    u8 cmd[30];
  	int i;
  	cmd[0] = 0xFE;
  	gw_app_cmd_t *pCmd = (gw_app_cmd_t*)(&cmd[1]);
  	pCmd->len = 15;
  	pCmd->cmd0 = 0x49;
  	pCmd->cmd1 = 0x00;
  	pCmd->data.ctrlCmd.endpoint = 0xB;
    pCmd->data.ctrlCmd.clusterID = 0xffff;
    pCmd->data.ctrlCmd.dataLen = 3;
    pCmd->data.ctrlCmd.cmdID = ZLL_CTRL_CMD_DEMO_BIND;


    pCmd->data.ctrlCmd.payload[0] = (addr & 0xff);
    pCmd->data.ctrlCmd.payload[1] = (addr & 0xff00) >> 8;
    pCmd->data.ctrlCmd.payload[2] = addrMode;

    for(i=0; i<20; i++) {
        printf("0x%x ", cmd[i]);
    }
    printf("\n");

    write(serialPortFd,cmd,sizeof(cmd));
    tcflush(serialPortFd, TCOFLUSH);

}
/*********************************************************************
 * @fn      zllSocResetToFn
 *
 * @brief   Send the reset to factory new command to the CC253x.
 *
 * @param   none
 *
 * @return  none
 */
void zllSocResetToFn(void)
{
	u8 tlCmd[] = {
		APPCMDHEADER(13)
		0x06, //Data Len
		0x02, //Address Mode
		0x00, //2dummy bytes
		0x00,
		ZLL_MT_APP_RPC_CMD_RESET_TO_FN,
		0x00,     //
		0x00,     //
		0x00       //FCS - fill in later
    };

    calcFcs(tlCmd, sizeof(tlCmd));
    write(serialPortFd,tlCmd, sizeof(tlCmd));
    tcflush(serialPortFd, TCOFLUSH);
}

/*********************************************************************
 * @fn      zllSocSendResetToFn
 *
 * @brief   Send the reset to factory new command to a ZLL device.
 *
 * @param   none
 *
 * @return  none
 */
void zllSocSendResetToFn(void)
{
	u8 tlCmd[] = {
		APPCMDHEADER(13)
		0x06, //Data Len
		0x02, //Address Mode
		0x00, //2dummy bytes
		0x00,
		ZLL_MT_APP_RPC_CMD_SEND_RESET_TO_FN,
		0x00,     //
		0x00,     //
		0x00       //FCS - fill in later
    };

    calcFcs(tlCmd, sizeof(tlCmd));
    write(serialPortFd,tlCmd, sizeof(tlCmd));
    tcflush(serialPortFd, TCOFLUSH);
}

/*********************************************************************
 * @fn      zllSocSetState
 *
 * @brief   Send the on/off command to a ZLL light.
 *
 * @param   state - 0: Off, 1: On.
 * @param   dstAddr - Nwk Addr or Group ID of the Light(s) to be controled.
 * @param   endpoint - endpoint of the Light.
 * @param   addrMode - Unicast or Group cast.
 *
 * @return  none
 */
void zllSocSetState(u8 state, u16 dstAddr, u8 endpoint, u8 addrMode)
{
  	u8 cmd[30];
  	int i;
  	cmd[0] = 0xFE;
  	gw_app_cmd_t *pCmd = (gw_app_cmd_t*)(&(cmd[1]));
  	pCmd->len = 13;
  	pCmd->cmd0 = 0x49;
  	pCmd->cmd1 = 0x00;
  	pCmd->data.dataCmd.endpoint = 0xB;
  	pCmd->data.dataCmd.dstNwkAddr = dstAddr;
  	pCmd->data.dataCmd.dstEndpoint = endpoint;
  	pCmd->data.dataCmd.clusterID = ZCL_CLUSTER_ID_GEN_ON_OFF;
  	pCmd->data.dataCmd.dataLen = 3;
  	pCmd->data.dataCmd.addrMode = addrMode;
  	pCmd->data.dataCmd.zclFrameCtrl = 0x01;
  	pCmd->data.dataCmd.zclTransSeqNo = transSeqNumber++;
  	pCmd->data.dataCmd.cmdID = (state ? 1:0);

    for(i=0; i<pCmd->len+1; i++) {
        printf("0x%x ", cmd[i]);
    }
    printf("\n");

    write(serialPortFd,cmd,sizeof(cmd));
    tcflush(serialPortFd, TCOFLUSH);
}

/*********************************************************************
 * @fn      zllSocSetLevel
 *
 * @brief   Send the level command to a ZLL light.
 *
 * @param   level - 0-128 = 0-100%
 * @param   dstAddr - Nwk Addr or Group ID of the Light(s) to be controled.
 * @param   endpoint - endpoint of the Light.
 * @param   addrMode - Unicast or Group cast.
 *
 * @return  none
 */
void zllSocSetLevel(u8 level, u16 time, u16 dstAddr, u8 endpoint, u8 addrMode)
{
  	u8 cmd[30];
  	int i;
  	cmd[0] = 0xFE;
  	gw_app_cmd_t *pCmd = (gw_app_cmd_t*)(&(cmd[1]));
  	pCmd->len = 16;
  	pCmd->cmd0 = 0x49;
  	pCmd->cmd1 = 0x00;
  	pCmd->data.dataCmd.endpoint = 0xB;
  	pCmd->data.dataCmd.dstNwkAddr = dstAddr;
  	pCmd->data.dataCmd.dstEndpoint = endpoint;
  	pCmd->data.dataCmd.clusterID = ZCL_CLUSTER_ID_GEN_LEVEL_CONTROL;
  	pCmd->data.dataCmd.dataLen = 6;
  	pCmd->data.dataCmd.addrMode = addrMode;
  	pCmd->data.dataCmd.zclFrameCtrl = 0x01;
  	pCmd->data.dataCmd.zclTransSeqNo = transSeqNumber++;
  	pCmd->data.dataCmd.cmdID = COMMAND_LEVEL_MOVE_TO_LEVEL_WITH_ONOFF;

  	pCmd->data.dataCmd.payload[0] = level;
  	pCmd->data.dataCmd.payload[1] = (time & 0xff);
  	pCmd->data.dataCmd.payload[2] = (time & 0xff00) >> 8;

    for(i=0; i<pCmd->len+1; i++) {
        printf("0x%x ", cmd[i]);
    }
    printf("\n");


    write(serialPortFd,cmd,sizeof(cmd));
    tcflush(serialPortFd, TCOFLUSH);
}


/*********************************************************************
 * @fn      zllSocIdentify
 *
 * @brief   Send the Identify command to a ZLL light.
 *
 * @param   dstAddr - Nwk Addr or Group ID of the Light(s) to be controled.
 * @param   endpoint - endpoint of the Light.
 * @param   addrMode - Unicast or Group cast.
 *
 * @return  none
 */
void zllSocIdentify(u16 time, u16 dstAddr, u8 endpoint, u8 addrMode)
{
  	u8 cmd[30];
  	int i;
  	cmd[0] = 0xFE;
  	gw_app_cmd_t *pCmd = (gw_app_cmd_t*)(&(cmd[1]));
  	pCmd->len = 16;
  	pCmd->cmd0 = 0x49;
  	pCmd->cmd1 = 0x00;
  	pCmd->data.dataCmd.endpoint = 0xB;
  	pCmd->data.dataCmd.dstNwkAddr = dstAddr;
  	pCmd->data.dataCmd.dstEndpoint = endpoint;
  	pCmd->data.dataCmd.clusterID = ZCL_CLUSTER_ID_GEN_IDENTIFY;
  	pCmd->data.dataCmd.dataLen = 6;
  	pCmd->data.dataCmd.addrMode = addrMode;
  	pCmd->data.dataCmd.zclFrameCtrl = 0x01;
  	pCmd->data.dataCmd.zclTransSeqNo = transSeqNumber++;
  	pCmd->data.dataCmd.cmdID = 0; // identify

  	pCmd->data.dataCmd.payload[0] = (time & 0xff);
  	pCmd->data.dataCmd.payload[1] = (time & 0xff00) >> 8;

    for(i=0; i<pCmd->len+1; i++) {
        printf("0x%x ", cmd[i]);
    }
    printf("\n");


    write(serialPortFd,cmd,sizeof(cmd));
    tcflush(serialPortFd, TCOFLUSH);
}

/*********************************************************************
 * @fn      zllSocSetHue
 *
 * @brief   Send the hue command to a ZLL light.
 *
 * @param   hue - 0-128 represent the 360Deg hue color wheel : 0=red, 42=blue, 85=green
 * @param   dstAddr - Nwk Addr or Group ID of the Light(s) to be controled.
 * @param   endpoint - endpoint of the Light.
 * @param   addrMode - Unicast or Group cast.
 *
 * @return  none
 */
void zllSocSetHue(u8 hue, u16 time, u16 dstAddr, u8 endpoint, u8 addrMode)
{
	u8 cmd[30];
  	int i;
  	cmd[0] = 0xFE;
  	gw_app_cmd_t *pCmd = (gw_app_cmd_t*)(&(cmd[1]));
  	pCmd->len = 16;
  	pCmd->cmd0 = 0x49;
  	pCmd->cmd1 = 0x00;
  	pCmd->data.dataCmd.endpoint = 0xB;
  	pCmd->data.dataCmd.dstNwkAddr = dstAddr;
  	pCmd->data.dataCmd.dstEndpoint = endpoint;
  	pCmd->data.dataCmd.clusterID = ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL;
  	pCmd->data.dataCmd.dataLen = 8;
  	pCmd->data.dataCmd.addrMode = addrMode;
  	pCmd->data.dataCmd.zclFrameCtrl = 0x01;
  	pCmd->data.dataCmd.zclTransSeqNo = transSeqNumber++;
  	pCmd->data.dataCmd.cmdID = COMMAND_LIGHTING_MOVE_TO_HUE;

  	pCmd->data.dataCmd.payload[0] = hue;
  	pCmd->data.dataCmd.payload[1] = (time & 0xff);
  	pCmd->data.dataCmd.payload[2] = (time & 0xff00) >> 8;

    for(i=0; i<pCmd->len+1; i++) {
        printf("0x%x ", cmd[i]);
    }
    printf("\n");

    write(serialPortFd,cmd,sizeof(cmd));
    tcflush(serialPortFd, TCOFLUSH);
}

/*********************************************************************
 * @fn      zllSocSetSat
 *
 * @brief   Send the satuartion command to a ZLL light.
 *
 * @param   sat - 0-128 : 0=white, 128: fully saturated color
 * @param   dstAddr - Nwk Addr or Group ID of the Light(s) to be controled.
 * @param   endpoint - endpoint of the Light.
 * @param   addrMode - Unicast or Group cast.
 *
 * @return  none
 */
void zllSocSetSat(u8 sat, u16 time, u16 dstAddr, u8  endpoint, u8 addrMode)
{
  u8 cmd[] = {
		0xFE,
		14,   //RPC payload Len
		0x29, //MT_RPC_CMD_AREQ + MT_RPC_SYS_APP
		0x00, //MT_APP_MSG
		0x0B, //Application Endpoint
		(dstAddr & 0x00ff),
		(dstAddr & 0xff00) >> 8,
		endpoint, //Dst EP
  	(ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL & 0x00ff),
  	(ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL & 0xff00) >> 8,
		0x07, //Data Len
		addrMode,
		0x01, //0x01 ZCL frame control field.  (send to the light cluster only)
    transSeqNumber++,
		COMMAND_LIGHTING_MOVE_TO_SATURATION,
		(sat & 0xff),
		(time & 0xff),
		(time & 0xff00) >> 8,
		0x00       //FCS - fill in later
	};

	calcFcs(cmd, sizeof(cmd));
  write(serialPortFd,cmd,sizeof(cmd));
  tcflush(serialPortFd, TCOFLUSH);
}

/*********************************************************************
 * @fn      zllSocSetHueSat
 *
 * @brief   Send the hue and satuartion command to a ZLL light.
 *
 * @param   hue - 0-128 represent the 360Deg hue color wheel : 0=red, 42=blue, 85=green
 * @param   sat - 0-128 : 0=white, 128: fully saturated color
 * @param   dstAddr - Nwk Addr or Group ID of the Light(s) to be controled.
 * @param   endpoint - endpoint of the Light.
 * @param   addrMode - Unicast or Group cast.
 *
 * @return  none
 */
void zllSocSetHueSat(u8 hue, u8 sat, u16 time, u16 dstAddr, u8 endpoint, u8 addrMode)
{
	u8 cmd[] = {
		0xFE,
		15, //RPC payload Len
		0x29, //MT_RPC_CMD_AREQ + MT_RPC_SYS_APP
		0x00, //MT_APP_MSG
		0x0B, //Application Endpoint
		(dstAddr & 0x00ff),
		(dstAddr & 0xff00) >> 8,
		endpoint, //Dst EP
  	(ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL & 0x00ff),
  	(ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL & 0xff00) >> 8,
		0x08, //Data Len
    addrMode,
		0x01, //ZCL Header Frame Control
		transSeqNumber++,
		0x06, //ZCL Header Frame Command (COMMAND_LEVEL_MOVE_TO_HUE_AND_SAT)
		hue, //HUE - fill it in later
		sat, //SAT - fill it in later
		(time & 0xff),
		(time & 0xff00) >> 8,
		0x00 //fcs
  };

  calcFcs(cmd, sizeof(cmd));
  write(serialPortFd,cmd,sizeof(cmd));
  tcflush(serialPortFd, TCOFLUSH);
}

/*********************************************************************
 * @fn      zllSocAddGroup
 *
 * @brief   Add Group.
 *
 * @param   groupId - Group ID of the Scene.
 * @param   dstAddr - Nwk Addr or Group ID of the Light(s) to be controled.
 * @param   endpoint - endpoint of the Light.
 * @param   addrMode - Unicast or Group cast.
 *
 * @return  none
 */
void zllSocAddGroup(u16 groupId, u16 dstAddr, u8 endpoint, u8 addrMode)
{
#if 0
	u8 cmd[] = {
		0xFE,
		14,   /*RPC payload Len */
		0x29, /*MT_RPC_CMD_AREQ + MT_RPC_SYS_APP */
		0x00, /*MT_APP_MSG  */
		0x0B, /*Application Endpoint */
		(dstAddr & 0x00ff),
		(dstAddr & 0xff00) >> 8,
		endpoint, /*Dst EP */
		(ZCL_CLUSTER_ID_GEN_GROUPS & 0x00ff),
		(ZCL_CLUSTER_ID_GEN_GROUPS & 0xff00) >> 8,
		0x07, //Data Len
		addrMode,
		0x01, //0x01 ZCL frame control field.  (send to the light cluster only)
		transSeqNumber++,
		COMMAND_GROUP_ADD,
 		(groupId & 0x00ff),
		(groupId & 0xff00) >> 8,
		0, //Null group name - Group Name not pushed to the devices
		0x00       //FCS - fill in later
	};

	printf("zllSocAddGroup: dstAddr 0x%x\n", dstAddr);

	calcFcs(cmd, sizeof(cmd));
#endif

    u8 cmd[30];
  	int i;
  	cmd[0] = 0xFE;
  	gw_app_cmd_t *pCmd = (gw_app_cmd_t*)(&(cmd[1]));
  	pCmd->len = 16;
  	pCmd->cmd0 = 0x49;
  	pCmd->cmd1 = 0x00;
  	pCmd->data.dataCmd.endpoint = 0xB;
  	pCmd->data.dataCmd.dstNwkAddr = dstAddr;
  	pCmd->data.dataCmd.dstEndpoint = endpoint;
  	pCmd->data.dataCmd.clusterID = ZCL_CLUSTER_ID_GEN_GROUPS;
  	pCmd->data.dataCmd.dataLen = 7;
  	pCmd->data.dataCmd.addrMode = addrMode;
  	pCmd->data.dataCmd.zclFrameCtrl = 0x01;
  	pCmd->data.dataCmd.zclTransSeqNo = transSeqNumber++;
  	pCmd->data.dataCmd.cmdID = COMMAND_GROUP_ADD;

  	pCmd->data.dataCmd.payload[0] = (groupId & 0xff);
  	pCmd->data.dataCmd.payload[1] = (groupId & 0xff00) >> 8;
  	pCmd->data.dataCmd.payload[2] = 0; //Null group name - Group Name not pushed to the devices
  	pCmd->data.dataCmd.payload[3] = 0; // FCS

    //for(i=0; i<pCmd->len+1; i++) {
    //    printf("0x%x ", cmd[i]);
    //}
    //printf("\n");


    write(serialPortFd,cmd,sizeof(cmd));
    tcflush(serialPortFd, TCOFLUSH);

  write(serialPortFd,cmd,sizeof(cmd));
  tcflush(serialPortFd, TCOFLUSH);
}

/*********************************************************************
 * @fn      zllSocStoreScene
 *
 * @brief   Store Scene.
 *
 * @param   groupId - Group ID of the Scene.
 * @param   sceneId - Scene ID of the Scene.
 * @param   dstAddr - Nwk Addr or Group ID of the Light(s) to be controled.
 * @param   endpoint - endpoint of the Light.
 * @param   addrMode - Unicast or Group cast.
 *
 * @return  none
 */
void zllSocStoreScene(u16 groupId, u8 sceneId, u16 dstAddr, u8 endpoint, u8 addrMode)
{
	u8 cmd[] = {
		0xFE,
		14,   /*RPC payload Len */
		0x29, /*MT_RPC_CMD_AREQ + MT_RPC_SYS_APP */
		0x00, /*MT_APP_MSG  */
		0x0B, /*Application Endpoint */
		(dstAddr & 0x00ff),
		(dstAddr & 0xff00) >> 8,
		endpoint, /*Dst EP */
		(ZCL_CLUSTER_ID_GEN_SCENES & 0x00ff),
		(ZCL_CLUSTER_ID_GEN_SCENES & 0xff00) >> 8,
		0x07, //Data Len
		addrMode,
		0x01, //0x01 ZCL frame control field.  (send to the light cluster only)
		transSeqNumber++,
		COMMAND_SCENE_STORE,
 		(groupId & 0x00ff),
		(groupId & 0xff00) >> 8,
		sceneId++,
		0x00       //FCS - fill in later
	};

	calcFcs(cmd, sizeof(cmd));

  write(serialPortFd,cmd,sizeof(cmd));
  tcflush(serialPortFd, TCOFLUSH);
}

/*********************************************************************
 * @fn      zllSocFlashReset
 *
 * @brief   Send the flash reset command to a ZLL light.
 *
 * @param   state - 0: Off, 1: On.
 * @param   dstAddr - Nwk Addr or Group ID of the Light(s) to be controled.
 * @param   endpoint - endpoint of the Light.
 * @param   addrMode - Unicast or Group cast.
 *
 * @return  none
 */
void zllSocFlashReset(u16 dstAddr, u8 endpoint, u8 addrMode)
{
  	u8 cmd[30];
  	int i;
  	gw_app_cmd_t *pCmd = (gw_app_cmd_t*)&cmd;
  	pCmd->len = 13;
  	pCmd->cmd0 = 0x49;
  	pCmd->cmd1 = 0x00;
  	pCmd->data.dataCmd.endpoint = 0xB;
  	pCmd->data.dataCmd.dstNwkAddr = dstAddr;
  	pCmd->data.dataCmd.dstEndpoint = endpoint;
  	pCmd->data.dataCmd.clusterID = ZCL_CLUSTER_ID_GEN_ON_OFF;
  	pCmd->data.dataCmd.dataLen = 3;
  	pCmd->data.dataCmd.addrMode = addrMode;
  	pCmd->data.dataCmd.zclFrameCtrl = 0x01;
  	pCmd->data.dataCmd.zclTransSeqNo = transSeqNumber++;
  	pCmd->data.dataCmd.cmdID = 0x04;

    //for(i=0; i<pCmd->len+1; i++) {
    //    printf("0x%x ", cmd[i]);
    //}
    //printf("\n");

    write(serialPortFd,cmd,sizeof(cmd));
    tcflush(serialPortFd, TCOFLUSH);
}


/*********************************************************************
 * @fn      zllSocRecallScene
 *
 * @brief   Recall Scene.
 *
 * @param   groupId - Group ID of the Scene.
 * @param   sceneId - Scene ID of the Scene.
 * @param   dstAddr - Nwk Addr or Group ID of the Light(s) to be controled.
 * @param   endpoint - endpoint of the Light.
 * @param   addrMode - Unicast or Group cast.

 * @return  none
 */
void zllSocRecallScene(u16 groupId, u8 sceneId, u16 dstAddr, u8 endpoint, u8 addrMode)
{
	u8 cmd[] = {
		0xFE,
		14,   /*RPC payload Len */
		0x29, /*MT_RPC_CMD_AREQ + MT_RPC_SYS_APP */
		0x00, /*MT_APP_MSG  */
		0x0B, /*Application Endpoint */
		(dstAddr & 0x00ff),
		(dstAddr & 0xff00) >> 8,
		endpoint, /*Dst EP */
		(ZCL_CLUSTER_ID_GEN_SCENES & 0x00ff),
		(ZCL_CLUSTER_ID_GEN_SCENES & 0xff00) >> 8,
		0x07, //Data Len
		addrMode,
		0x01, //0x01 ZCL frame control field.  (send to the light cluster only)
		transSeqNumber++,
		COMMAND_SCENE_RECALL,
 		(groupId & 0x00ff),
		(groupId & 0xff00) >> 8,
		sceneId++,
		0x00       //FCS - fill in later
	};

	calcFcs(cmd, sizeof(cmd));

  write(serialPortFd,cmd,sizeof(cmd));
  tcflush(serialPortFd, TCOFLUSH);
}

/*********************************************************************
 * @fn      zllSocGetState
 *
 * @brief   Send the get state command to a ZLL light.
 *
 * @param   dstAddr - Nwk Addr or Group ID of the Light(s) to be sent the command.
 * @param   endpoint - endpoint of the Light.
 * @param   addrMode - Unicast or Group cast.
 *
 * @return  none
 */
void zllSocGetState(u16 dstAddr, u8 endpoint, u8 addrMode)
{
  	u8 cmd[] = {
  		0xFE,
  		13,   /*RPC payload Len */
  		0x29, /*MT_RPC_CMD_AREQ + MT_RPC_SYS_APP */
  		0x00, /*MT_APP_MSG  */
  		0x0B, /*Application Endpoint */
  		(dstAddr & 0x00ff),
  		(dstAddr & 0xff00) >> 8,
  		endpoint, /*Dst EP */
  		(ZCL_CLUSTER_ID_GEN_ON_OFF & 0x00ff),
  		(ZCL_CLUSTER_ID_GEN_ON_OFF & 0xff00) >> 8,
  		0x06, //Data Len
  		addrMode,
  		0x00, //0x00 ZCL frame control field.  not specific to a cluster (i.e. a SCL founadation command)
  		transSeqNumber++,
  		ZCL_CMD_READ,
  		(ATTRID_ON_OFF & 0x00ff),
  		(ATTRID_ON_OFF & 0xff00) >> 8,
  		0x00       //FCS - fill in later
  	};

  	calcFcs(cmd, sizeof(cmd));

    write(serialPortFd,cmd,sizeof(cmd));
    tcflush(serialPortFd, TCOFLUSH);
}

/*********************************************************************
 * @fn      zllSocGetLevel
 *
 * @brief   Send the get level command to a ZLL light.
 *
 * @param   dstAddr - Nwk Addr or Group ID of the Light(s) to be sent the command.
 * @param   endpoint - endpoint of the Light.
 * @param   addrMode - Unicast or Group cast.
 *
 * @return  none
 */
void zllSocGetLevel(u16 dstAddr, u8 endpoint, u8 addrMode)
{
  	u8 cmd[] = {
  		0xFE,
  		13,   /*RPC payload Len */
  		0x29, /*MT_RPC_CMD_AREQ + MT_RPC_SYS_APP */
  		0x00, /*MT_APP_MSG  */
  		0x0B, /*Application Endpoint */
  		(dstAddr & 0x00ff),
  		(dstAddr & 0xff00) >> 8,
  		endpoint, /*Dst EP */
  		(ZCL_CLUSTER_ID_GEN_LEVEL_CONTROL & 0x00ff),
  		(ZCL_CLUSTER_ID_GEN_LEVEL_CONTROL & 0xff00) >> 8,
  		0x06, //Data Len
  		addrMode,
  		0x00, //0x00 ZCL frame control field.  not specific to a cluster (i.e. a SCL founadation command)
  		transSeqNumber++,
  		ZCL_CMD_READ,
  		(ATTRID_LEVEL_CURRENT_LEVEL & 0x00ff),
  		(ATTRID_LEVEL_CURRENT_LEVEL & 0xff00) >> 8,
  		0x00       //FCS - fill in later
  	};

  	calcFcs(cmd, sizeof(cmd));

    write(serialPortFd,cmd,sizeof(cmd));
    tcflush(serialPortFd, TCOFLUSH);
}

/*********************************************************************
 * @fn      zllSocGetHue
 *
 * @brief   Send the get hue command to a ZLL light.
 *
 * @param   dstAddr - Nwk Addr or Group ID of the Light(s) to be sent the command.
 * @param   endpoint - endpoint of the Light.
 * @param   addrMode - Unicast or Group cast.
 *
 * @return  none
 */
void zllSocGetHue(u16 dstAddr, u8 endpoint, u8 addrMode)
{
  	u8 cmd[] = {
  		0xFE,
  		13,   /*RPC payload Len */
  		0x29, /*MT_RPC_CMD_AREQ + MT_RPC_SYS_APP */
  		0x00, /*MT_APP_MSG  */
  		0x0B, /*Application Endpoint */
  		(dstAddr & 0x00ff),
  		(dstAddr & 0xff00) >> 8,
  		endpoint, /*Dst EP */
  		(ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL & 0x00ff),
  		(ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL & 0xff00) >> 8,
  		0x06, //Data Len
  		addrMode,
  		0x00, //0x00 ZCL frame control field.  not specific to a cluster (i.e. a SCL founadation command)
  		transSeqNumber++,
  		ZCL_CMD_READ,
  		(ATTRID_LIGHTING_COLOR_CONTROL_CURRENT_HUE & 0x00ff),
  		(ATTRID_LIGHTING_COLOR_CONTROL_CURRENT_HUE & 0xff00) >> 8,
  		0x00       //FCS - fill in later
  	};

  	calcFcs(cmd, sizeof(cmd));

    write(serialPortFd,cmd,sizeof(cmd));
    tcflush(serialPortFd, TCOFLUSH);
}

/*********************************************************************
 * @fn      zllSocGetSat
 *
 * @brief   Send the get saturation command to a ZLL light.
 *
 * @param   dstAddr - Nwk Addr or Group ID of the Light(s) to be sent the command.
 * @param   endpoint - endpoint of the Light.
 * @param   addrMode - Unicast or Group cast.
 *
 * @return  none
 */
void zllSocGetSat(u16 dstAddr, u8 endpoint, u8 addrMode)
{
  	u8 cmd[] = {
  		0xFE,
  		13,   /*RPC payload Len */
  		0x29, /*MT_RPC_CMD_AREQ + MT_RPC_SYS_APP */
  		0x00, /*MT_APP_MSG  */
  		0x0B, /*Application Endpoint */
  		(dstAddr & 0x00ff),
  		(dstAddr & 0xff00) >> 8,
  		endpoint, /*Dst EP */
  		(ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL & 0x00ff),
  		(ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL & 0xff00) >> 8,
  		0x06, //Data Len
  		addrMode,
  		0x00, //0x00 ZCL frame control field.  not specific to a cluster (i.e. a SCL founadation command)
  		transSeqNumber++,
  		ZCL_CMD_READ,
  		(ATTRID_LIGHTING_COLOR_CONTROL_CURRENT_SATURATION & 0x00ff),
  		(ATTRID_LIGHTING_COLOR_CONTROL_CURRENT_SATURATION & 0xff00) >> 8,
  		0x00       //FCS - fill in later
  	};

  	calcFcs(cmd, sizeof(cmd));

    write(serialPortFd,cmd,sizeof(cmd));
    tcflush(serialPortFd, TCOFLUSH);
}

/*********************************************************************
 * @fn      zllSocEndDevBind
 *
 * @brief   Send the get saturation command to a ZLL light.
 *
 * @param   dstAddr - Nwk Addr or Group ID of the Light(s) to be sent the command.
 * @param   endpoint - endpoint of the Light.
 * @param   addrMode - Unicast or Group cast.
 *
 * @return  none
 */
void zllSocEndDevBind(u16 dstAddr, u8 endpoint, u8 addrMode)
{
  	u8 cmd[30];
  	int i;
  	cmd[0] = 0xFE;
  	gw_app_cmd_t *pCmd = (gw_app_cmd_t*)(&cmd[1]);
  	pCmd->len = 15;
  	pCmd->cmd0 = 0x49;
  	pCmd->cmd1 = 0x00;
  	pCmd->data.ctrlCmd.endpoint = 0xB;
    pCmd->data.ctrlCmd.clusterID = 0xffff;
    pCmd->data.ctrlCmd.dataLen = 3;
    pCmd->data.ctrlCmd.cmdID = ZLL_CTRL_CMD_END_DEV_BIND;

    //for(i=0; i<pCmd->len+1; i++) {
    //    printf("0x%x ", cmd[i]);
    //}
    //printf("\n");

    write(serialPortFd,cmd,sizeof(cmd));
    tcflush(serialPortFd, TCOFLUSH);
}

