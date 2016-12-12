#ifndef  __APP_CMD_H__
#define  __APP_CMD_H__

#include "types.h"

#pragma pack(1)

/*********************************************************************
 * CONSTANTS
 */

#define MAX_APP_PACKET_LEN          50

#define APP_CMD_SOF                 0xA3

/*********************************************************************
 * ENUMS
 */

/*
 * Definition for Command ID
 */
enum {
    /* General Command ID */
	CMD_HEART_BEAT,
	CMD_REPORT,
	CMD_QUERY_REQ,
	CMD_QUERY_RSP,
	CMD_LEAVE_NWK,
	CMD_BIND,
	CMD_GROUP,
	CMD_GROUP_RSP,

	/* Device Command ID */
	CMD_LIGHT,
	CMD_LEVEL,


	/* Close Gateway */
	CMD_CLOSE,

};


/*
 * Definition for Device Type
 */
enum {
	DEV_TYPE_GW,
	DEV_TYPE_LIGHT,
	DEV_TYPE_PIR_SENSOR,
	DEV_TYPE_ONOFF_SWITCH,
	DEV_TYPE_UNKNOWN,
};


enum {
    GROUP_OPCODE_ADD,
    GROUP_OPCODE_REMOVE,
};


/*
 * Definition for heart beat command
 */
typedef struct {
    u8 sof;
	u8 cmd;
	u8 hbCnt;
} gw_hbCmd_t;


/*
 * Definition Group command format
 */
typedef struct {
    u8 sof;
    u8 cmd;
    u16 nwkAddr;
    u8 opCode;
    u16 groupId;
} gw_groupCmd_t;

/*
 * Definition Group Response command format
 */
typedef struct {
    u8 sof;
    u8 cmd;
    u16 nwkAddr;
    u8 opCode;
    u8 status;
    u16 groupId;
} gw_groupRspCmd_t;


/*
 * Definition Bind command format
 */
typedef struct {
    u8 sof;
    u8 cmd;
    u8 addrMode;
    u16 addr;
} gw_bindCmd_t;


/*
 * Definition report command format
 */
typedef struct gw_reportCmd_tag{
	u8 sof;
	u8 cmd;
	u8 devType;
	u16 nwkAddr;
	u8 extAddr[8];
} gw_reportCmd_t;


/*
 * Definiton for light on-off command
 */
typedef struct gw_lgihtCmd_tag {
	u8 sof;
	u8 cmd;
	u8 addrMode;
	u16 addr;
	u8 opCode; // on / off / toggle
} gw_lightCmd_t;

/*
 *  Definiton for light level command
 */
typedef struct gw_levelCmd_tag {
    u8 sof;
    u8 cmd;
    u8 addrMode;
    u16 addr;
    u8 opCode;
    u8 level;
    u16 transTime;
} gw_levelCmd_t;



/*********************************************************************
 * TYPES
 */



/*********************************************************************
 * Public Functions
 */

void app_cmdHandler(u8* buf, u8 len);

void app_sendDeviceReportCmd(u8 type, u16 nwkAddr, u8*extAddr);


#endif  /* __APP_CMD_H__ */
