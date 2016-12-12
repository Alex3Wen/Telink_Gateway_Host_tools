

/**********************************************************************
 * INCLUDES
 */
#include <stdio.h>

#include "types.h"
#include "appCmd.h"
#include "server.h"
#include "socCmd.h"
#include "nodes.h"

/**********************************************************************
 * LOCAL CONSTANTS
 */

/* None */

/**********************************************************************
 * LOCAL TYPES
 */

/* None */


/**********************************************************************
 * LOCAL VARIABLES
 */
/* None */


/**********************************************************************
 * LOCAL FUNCTIONS
 */
void app_lightCmdHandler(gw_lightCmd_t* cmd);
void app_levelCmdHandler(gw_levelCmd_t* cmd);
void app_groupCmdHandler(gw_groupCmd_t* cmd);
void app_queryCmdHandler(void);

/*********************************************************************
 * @fn      app_cmdHandler
 *
 * @brief   handle the received commands from Apps
 *
 * @param   buf - the recevied command
 * @param   len - the length of received command
 *
 * @return  none
 */
void app_cmdHandler(u8* buf, u8 len)
{
    u8 i;
    //printf("received App command:\n");
    //for (i = 0; i < len; i++) {
    //    printf("0x%x ", buf[i]);
    //}
    //printf("\n");


    /* Filter the error commands */
    if (buf[0] != APP_CMD_SOF) {
        return;
    }

    switch (buf[1]) {
    case CMD_HEART_BEAT:
        break;

    case CMD_QUERY_REQ:
        app_queryCmdHandler();
        break;

    case CMD_GROUP:
        app_groupCmdHandler((gw_groupCmd_t*)buf);
        break;

    case CMD_BIND:
        app_bindCmdHandler((gw_bindCmd_t*)buf);
        break;

    case CMD_LEAVE_NWK:
        break;

    case CMD_LIGHT:
        app_lightCmdHandler((gw_lightCmd_t*)buf);
        break;

    case CMD_LEVEL:
        app_levelCmdHandler((gw_levelCmd_t*)buf);
        break;

    case CMD_CLOSE:
        exit(0);
        break;

    default:
        break;


    }



}


 /*********************************************************************
 * @fn      app_sendDeviceReportCmd
 *
 * @brief   send the report command to Apps
 *
 * @param   type - the device type newly joined device
 * @param   nwkAddr - the network address of newly joined device
 * @param   extAddr - the extended address of newly joined device
 *
 * @return  none
 */
void app_sendDeviceReportCmd(u8 type, u16 nwkAddr, u8*extAddr)
{
    int i;
    int clients_fd[MAX_SOCKET_NUM];
    int clients_num;
    u8 buf[20];
    gw_reportCmd_t* p = (gw_reportCmd_t*)buf;

    p->sof = APP_CMD_SOF;
    p->cmd = CMD_REPORT;
    p->devType = type;
    p->nwkAddr = nwkAddr;
    memcpy(p->extAddr, extAddr, 8);

    /* Send the report command to all connected Apps */
    socketPool_get(clients_fd, &clients_num);
    for(i = 0; i < clients_num; i++) {
        server_send(clients_fd[i], buf, sizeof(gw_reportCmd_t));
    }

}


void app_sendGroupRspCmd(u16 nwkAddr, u16 groupID, u8 opcode, u8 status)
{
    int i;
    int clients_fd[MAX_SOCKET_NUM];
    int clients_num;
    u8 buf[20];
    gw_groupRspCmd_t* p = (gw_groupRspCmd_t*)buf;

    p->sof = APP_CMD_SOF;
    p->cmd = CMD_GROUP_RSP;
    p->nwkAddr = nwkAddr;
    p->opCode = opcode;
    p->status = status;
    p->groupId = groupID;

    /* Send the report command to all connected Apps */
    socketPool_get(clients_fd, &clients_num);
    for(i = 0; i < clients_num; i++) {
        server_send(clients_fd[i], buf, sizeof(gw_groupRspCmd_t));
    }
}


/*********************************************************************
 * @fn      app_lightCmdHandler
 *
 * @brief   parse the received lighting command and send to the specified node
 *
 * @param   cmd - the recevied light command
 *
 * @return  none
 */
void app_lightCmdHandler(gw_lightCmd_t* cmd)
{
    u8 endpoint = 0xB;
    u16 dstAddr = cmd->addr;
    u8 addrMode = cmd->addrMode;
    u8 opCode = cmd->opCode;

    printf("0x%x,  0x%x,  0x%x\n", dstAddr, addrMode, opCode);

    /* Get enpoint from node list */
    if (cmd->addrMode == ADDR_MODE_SHORT_ADDR) {
        //endpoint = node_getEndpoint(cmd->addr);
    }

    zllSocSetState(opCode, dstAddr, endpoint, addrMode);
}



/*********************************************************************
 * @fn      app_levelCmdHandler
 *
 * @brief   parse the received level command and send to the specified node
 *
 * @param   cmd - the recevied level command
 *
 * @return  none
 */
void app_levelCmdHandler(gw_levelCmd_t* cmd)
{
    u8 endpoint = 0xB;

    /* Get enpoint from node list */
    if (cmd->addrMode == ADDR_MODE_SHORT_ADDR) {
        //endpoint = node_getEndpoint(cmd->addr);
    }

    zllSocSetLevel(cmd->level, cmd->transTime, cmd->addr, endpoint, cmd->addrMode);
}


/*********************************************************************
 * @fn      app_groupCmdHandler
 *
 * @brief   parse the received group command and send to the specified node
 *
 * @param   cmd - the recevied group command
 *
 * @return  none
 */
void app_groupCmdHandler(gw_groupCmd_t* cmd)
{
    u8 endpoint = 0xB;

    /* Get enpoint from node list */
    //if (cmd->addrMode == ADDR_MODE_SHORT_ADDR) {
    //    //endpoint = node_getEndpoint(cmd->nwkAddr);
    //}

    if (cmd->opCode == GROUP_OPCODE_ADD) {
        zllSocAddGroup(cmd->groupId, cmd->nwkAddr, endpoint, 0x02);
    }

}

/*********************************************************************
 * @fn      app_queryCmdHandler
 *
 * @brief   search the nodes in node list and send to App through report command
 *
 * @param   none
 *
 * @return  none
 */
void app_queryCmdHandler(void)
{
    int i;
    nodeInfo_t *entry;

    for(i = 0; i < MAX_NODE_NUM; i++) {
        entry = nodes_get(i);
        if (entry->nwkAddr != EMPTY_NODE_NWK_ADDR) {
            app_sendDeviceReportCmd(entry->devType, entry->nwkAddr, entry->extAddr);
        }
    }
}


void app_bindCmdHandler(gw_bindCmd_t* cmd)
{
    zllSocDemoBind(cmd->addrMode, cmd->addr);
}

