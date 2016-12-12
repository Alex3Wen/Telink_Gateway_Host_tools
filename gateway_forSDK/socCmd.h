#ifndef  __SOC_CMD_H__
#define  __SOC_CMD_H__

#include "types.h"

#pragma pack(1)

/*********************************************************************
 * CONSTANTS
 */

/** @addtogroup zll_ctrl_command_id ZLL Control Command ID
 * @{
 */
#define ZLL_CTRL_CMD_TOUCHLINK                          0x01
#define ZLL_CTRL_CMD_RESET_TO_FN                        0x02
#define ZLL_CTRL_CMD_CH_CHANNEL                         0x03
#define ZLL_CTRL_CMD_JOIN_HA                            0x04
#define ZLL_CTRL_CMD_PERMIT_JOIN                        0x05
#define ZLL_CTRL_CMD_SEND_RESET_TO_FN                   0x06
#define ZLL_CTRL_CMD_DEV_ANN_IND                        0x07
#define ZLL_CTRL_CMD_GET_NODES                          0x08
#define ZLL_CTRL_CMD_END_DEV_BIND                       0x09
#define ZLL_CTRL_CMD_DEMO_BIND                          0x0A
/** @} end of group zll_ctrl_command_id */


/** @addtogroup zcl_foundation_cmdid ZCL Foundation Command ID
 * @{
 */
#define ZCL_CMD_READ                                    0x00
#define ZCL_CMD_READ_RSP                                0x01
#define ZCL_CMD_WRITE                                   0x02
#define ZCL_CMD_WRITE_UNDIVIDED                         0x03
#define ZCL_CMD_WRITE_RSP                               0x04
/** @} end of group zcl_foundation_cmdid */


/** @addtogroup zcl_cluster_id ZCL General Cluster ID
 * @{
 */
#define ZCL_CLUSTER_ID_GEN_IDENTIFY                     0x0003
#define ZCL_CLUSTER_ID_GEN_GROUPS                       0x0004
#define ZCL_CLUSTER_ID_GEN_SCENES                       0x0005
#define ZCL_CLUSTER_ID_GEN_ON_OFF                       0x0006
#define ZCL_CLUSTER_ID_GEN_LEVEL_CONTROL                0x0008
/** @} end of group zll_ctrl_command_id */

/*********************************************************************
 * ENUMS
 */



/*********************************************************************
 * TYPES
 */
typedef struct {
	u8 endpoint;                      //!< This is the application endpoint of the application, it should be set to 0x0B
	u8 reserved0[3];                  //!< All reserved bytes should be set to 0x00
	u16 clusterID;                    //!< should be set to 0xFFFF
	u8 dataLen;                       //!< This should be 6 + the number of parameters in the MT_APP Command
	u8 reserved1[3];                  //!< All reserved bytes should be set to 0x00
	u8 cmdID;                         //!< @ref zll_ctrl_command_id
	u8 reserved2[2];                  //!< All reserved bytes should be set to 0x00
	u8 payload[1];
} ctrl_cmd_t;


typedef struct {
	u8 endpoint;                     //!< This is the application endpoint of the application, it should be set to 0x0B
	u16 dstNwkAddr;                  //!< Network Address (or groupId depending on address mmode) of the device to send the ZCL message to
	u8 dstEndpoint;                  //!< End Point of the device to send the ZCL message to
	u16 clusterID;                   //!< Cluster ID of the ZCL Command to be sent
	u8 dataLen;                      //!< This should be 6 + the number of parameters in the ZCL Command
	u8 addrMode;                     //!< The address mode of the ZCL message
	u8 zclFrameCtrl;                 //!< ZCL Frame Control Field
	u8 zclTransSeqNo;                //!< The transaction ID should be incremented for each ZCL message
	u8 cmdID;                        //!< Command ID of the ZCL Command to be sent
	u8 payload[1];                   //!< Payload of the ZCL Command to be sent
} data_cmd_t;


typedef struct {
	u8 len;
	u8 cmd0;                         //!< Always be 0x49
	u8 cmd1;                         //!< Always be 0x00
	union {
		ctrl_cmd_t ctrlCmd;
		data_cmd_t dataCmd;
	} data;
} gw_app_cmd_t;


/*********************************************************************
 * Public Functions
 */
int  socOpen(char *devicePath);
void socClose(void);
void processSocCmd(void);

void zllSocSetState(u8 state, u16 dstAddr, u8 endpoint, u8 addrMode);
void zllSocSetLevel(u8 level, u16 time, u16 dstAddr, u8 endpoint, u8 addrMode);
void zllSocSetHue(u8 hue, u16 time, u16 dstAddr, u8 endpoint, u8 addrMode);
void zllSocAddGroup(u16 groupId, u16 dstAddr, u8 endpoint, u8 addrMode);
void zllSocDemoBind(u8 addrMode, u16 addr);

#endif  /* __SOC_CMD_H__ */
