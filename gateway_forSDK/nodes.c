

/**********************************************************************
 * INCLUDES
 */

#include "appCmd.h"
#include "nodes.h"

#include <stdlib.h>
#include <unistd.h>

/**********************************************************************
 * LOCAL CONSTANTS
 */

/* None */

/**********************************************************************
 * LOCAL TYPES
 */

typedef struct {
	nodeInfo_t nodeTbl[MAX_NODE_NUM];
	u8 curNodeNum;
} node_ctrl_t;


/**********************************************************************
 * LOCAL VARIABLES
 */
node_ctrl_t node_vs;
node_ctrl_t *node_v = &node_vs;


/**********************************************************************
 * LOCAL FUNCTIONS
 */


/*********************************************************************
 * @fn      nodes_reset
 *
 * @brief   Reset the node lists
 *
 * @param   none
 *
 * @return  none
 */
void nodes_reset(void)
{
    int i = 0;
	node_v->curNodeNum = 0;

	for(i = 0; i < MAX_NODE_NUM; i++) {
        memset(&(node_v->nodeTbl[i]), INVALID_NODE_INFO, sizeof(nodeInfo_t));
        node_v->nodeTbl[i].fInGroup = FALSE;
	}
}

/*********************************************************************
 * @fn      nodes_search
 *
 * @brief   Search node through specified network address and extended address
 *
 * @param   nwkAddr
 * @param   extAddr
 *
 * @return  none
 */
nodeInfo_t* nodes_search(u16 nwkAddr, u8* extAddr)
{
	int i = 0;
	for(i = 0; i < MAX_NODE_NUM; i++) {
		if (node_v->nodeTbl[i].nwkAddr == nwkAddr) {
			if (0 == memcmp(extAddr, node_v->nodeTbl[i].extAddr, 8)) {
				return &node_v->nodeTbl[i];
			}
		}
	}
	return NULL;
}

/*********************************************************************
 * @fn      nodes_add
 *
 * @brief   Add node to the node list
 *
 * @param   nwkAddr
 * @param   extAddr
 * @param   capability
 * @param   devID
 * @param   endpoint
 *
 * @return  none
 */
void nodes_add(u16 nwkAddr, u8* extAddr, u8 capability, u16 devID, u8 endpoint)
{
	nodeInfo_t *entry;
	u8 empty[8] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

	if (NULL != nodes_search(nwkAddr, extAddr)) {
		return;
	}

	/* Not Found */
	entry = nodes_search(EMPTY_NODE_NWK_ADDR, empty);
	if (!entry) {
		return;
	}

	/* Fill to an empty entry */
	memcpy((u8*)&entry->nwkAddr, (u8*)&nwkAddr, 2);
	memcpy(entry->extAddr, extAddr, 8);
	entry->capability = capability;
	entry->devId = devID;
	entry->fInGroup = 0;
	entry->endpoint = endpoint;


	switch (devID) {
    case HA_DEV_ONOFF_LIGHT:
    case HA_DEV_DIMMABLE_LIGHT:
    case HA_DEV_COLOR_DIMMABLE_LIGHT:
    case 0x0210:
        entry->devType = DEV_TYPE_LIGHT;
        break;

    case HA_DEV_ONOFF_SWITCH:
    case HA_DEV_ONOFF_LIGHT_SWITCH:
    case HA_DEV_DIMMER_SWITCH:
    case HA_DEV_COLOR_DIMMER_SWITCH:
        entry->devType = DEV_TYPE_ONOFF_SWITCH;
        break;

    default:
        entry->devType = DEV_TYPE_UNKNOWN;
	}

	node_v->curNodeNum++;
}

/*********************************************************************
 * @fn      nodes_get
 *
 * @brief   Get specified node through index
 *
 * @param   none
 *
 * @return  Number of current nodes
 */
nodeInfo_t* nodes_get(u8 index)
{
	return &node_v->nodeTbl[index];
}

/*********************************************************************
 * @fn      nodes_curNum
 *
 * @brief   Get current nodes number
 *
 * @param   none
 *
 * @return  Number of current nodes
 */
u8 nodes_curNum(void)
{
	return node_v->curNodeNum;
}

#if 0
void nodes_writeToFile(void)
{
    int i;
    nodeInfo_t *entry;

    for(i = 0; i < MAX_NODE_NUM; i++) {
        entry = nodes_get(i);
        if (entry->nwkAddr != EMPTY_NODE_NWK_ADDR) {
            (entry->devType, entry->nwkAddr, entry->extAddr);
        }
    }
}


void nodes_writeEntryToFile(nodeInfo_t *entry)
{
    FILE* fp;

    if ( NULL == (fp=fopen("gateway.txt", "a+")) ) {
        printf("the file %s opened failed!\n", filename);
        return;
    }

    fprintf(fp, "[node]\n");
    fprintf(fp, "devType = %d\n", devType);
    fprintf(fp, "nwkAddr = %d\n", nwk)
}



void nodes_readFromFile(void)
{

}

#endif
