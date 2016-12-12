

/**********************************************************************
 * INCLUDES
 */
#include <stdio.h>
#include "types.h"
#include "socCmd.h"
#include "server.h"

/**********************************************************************
 * LOCAL CONSTANTS
 */

#define MAX_CONSOLE_CMD_LEN   50

/**********************************************************************
 * LOCAL TYPES
 */



/**********************************************************************
 * LOCAL VARIABLES
 */
static u16 savedNwkAddr;
static u8 savedAddrMode;
static u8 savedEp;
static u8 savedValue;
static u16 savedTransitionTime;
static u16 savedGroupId;



/**********************************************************************
 * LOCAL FUNCTIONS
 */
void getConsoleCommandParams(char* cmdBuff, u16 *nwkAddr, u8 *addrMode, u8 *ep, u8 *value, u16 *transitionTime, u16 *groupId);
u32 getParam( char *cmdBuff, char *paramId, u32 *paramInt);

void processConsoleCommand(void)
{
    char cmdBuff[MAX_CONSOLE_CMD_LEN];
    u32 bytesRead;
    u16 nwkAddr;
    u8 addrMode;
    u8 endpoint;
    u8 value;
    u16 transitionTime;
    u16 groupId;

    //read stdin
    bytesRead = read(0, cmdBuff, (MAX_CONSOLE_CMD_LEN-1));
    cmdBuff[bytesRead] = '\0';

    getConsoleCommandParams(cmdBuff, &nwkAddr, &addrMode, &endpoint, &value, &transitionTime, &groupId);

    if((strstr(cmdBuff, "touchlink")) != 0) {
        zllSocTouchLink();
        printf("touchlink command executed\n\n");
    }
    else if((strstr(cmdBuff, "sendresettofn")) != 0) {
        //sending of reset to fn must happen within a touchlink
        zllSocTouchLink();
        printf("press a key when device identyfies\n");
        getc(stdin);
        zllSocSendResetToFn();
    }
    else if((strstr(cmdBuff, "resettofn")) != 0) {
        zllSocResetToFn();
        printf("resettofn command executed\n\n");
    }
    else if((strstr(cmdBuff, "setonoff")) != 0) {
        zllSocSetState(value, nwkAddr, endpoint, addrMode);
        printf("setstate command executed with params: \n");
        printf("    Network Addr    :0x%04x\n    End Point       :0x%02x\n    Addr Mode       :0x%02x\n    Value           :0x%02x\n\n",
            nwkAddr, endpoint, addrMode, value);
    } else if((strstr(cmdBuff, "setlevel")) != 0) {
        zllSocSetLevel(value, transitionTime, nwkAddr, endpoint, addrMode);
        printf("setlevel command executed with params: \n");
        printf("    Network Addr    :0x%04x\n    End Point       :0x%02x\n    Addr Mode       :0x%02x\n    Value           :0x%02x\n    Transition Time :0x%04x\n\n",
          nwkAddr, endpoint, addrMode, value, transitionTime);
    } else if((strstr(cmdBuff, "sethue")) != 0) {
        zllSocSetHue(value, transitionTime, nwkAddr, endpoint, addrMode);
        printf("sethue command executed with params: \n");
        printf("    Network Addr    :0x%04x\n    End Point       :0x%02x\n    Addr Mode       :0x%02x\n    Value           :0x%02x\n    Transition Time :0x%04x\n\n",
          nwkAddr, endpoint, addrMode, value, transitionTime);
    } else if((strstr(cmdBuff, "setsat")) != 0) {
        zllSocSetSat(value, transitionTime, nwkAddr, endpoint, addrMode);
        printf("setsat command executed with params: \n");
        printf("    Network Addr    :0x%04x\n    End Point       :0x%02x\n    Addr Mode       :0x%02x\n    Value           :0x%02x\n    Transition Time :0x%04x\n\n",
          nwkAddr, endpoint, addrMode, value, transitionTime);
    } else if((strstr(cmdBuff, "getstate")) != 0) {
        zllSocGetState(nwkAddr, endpoint, addrMode);
        printf("getstate command executed wtih params: \n");
        printf("    Network Addr    :0x%04x\n    End Point       :0x%02x\n    Addr Mode       :0x%02x\n\n",
          nwkAddr, endpoint, addrMode);
    } else if((strstr(cmdBuff, "getlevel")) != 0) {
        zllSocGetLevel(nwkAddr, endpoint, addrMode);
        printf("getlevel command executed with params: \n");
        printf("    Network Addr    :0x%04x\n    End Point       :0x%02x\n    Addr Mode       :0x%02x\n\n",
          nwkAddr, endpoint, addrMode);
    } else if((strstr(cmdBuff, "gethue")) != 0) {
        zllSocGetHue(nwkAddr, endpoint, addrMode);
        printf("gethue command executed with params: \n");
        printf("    Network Addr    :0x%04x\n    End Point       :0x%02x\n    Addr Mode       :0x%02x\n\n",
          nwkAddr, endpoint, addrMode);
    } else if((strstr(cmdBuff, "getsat")) != 0) {
        zllSocGetSat(nwkAddr, endpoint, addrMode);
        printf("getsat command executed with params: \n");
        printf("    Network Addr    :0x%04x\n    End Point       :0x%02x\n    Addr Mode       :0x%02x\n\n",
          nwkAddr, endpoint, addrMode);
    } else if((strstr(cmdBuff, "getnodes")) != 0) {
        //send the get nodes command to zc.
        zllSocGetNodes();
    } else if((strstr(cmdBuff, "addgroup")) != 0) {
        zllSocAddGroup(groupId, nwkAddr, endpoint, addrMode);
        printf("addgroup command executed with params: \n");
        printf("    Network Addr    :0x%04x\n    End Point       :0x%02x\n    Addr Mode       :0x%02x\n    GroupID       :0x%2x\n",
          nwkAddr, endpoint, addrMode, groupId);
    } else if((strstr(cmdBuff, "setbind")) != 0) {
        if (addrMode == 1) {
            zllSocDemoBind(addrMode, nwkAddr);
        } else {
            zllSocDemoBind(addrMode, nwkAddr);
        }

        printf("setbind command executed with params: \n");
        printf("    Network Addr    :0x%04x\n    End Point       :0x%02x\n    Addr Mode       :0x%02x\n    GroupID       :0x%2x\n",
          nwkAddr, endpoint, addrMode, groupId);
    } else if((strstr(cmdBuff, "resetflash")) != 0) {
        zllSocFlashReset(nwkAddr, endpoint, addrMode);
        printf("reset command executed with params: \n");
        printf("    Network Addr    :0x%04x\n    End Point       :0x%02x\n    Addr Mode       :0x%02x\n\n",
          nwkAddr, endpoint, addrMode);
    } else if((strstr(cmdBuff, "enddevbind")) != 0) {
        zllSocEndDevBind(nwkAddr, endpoint, addrMode);
    } else if((strstr(cmdBuff, "selectlight")) != 0) {
        zllSocIdentify(transitionTime, nwkAddr, endpoint, addrMode);
        printf("identify command executed with params: \n");
        printf("    Network Addr    :0x%04x\n    End Point       :0x%02x\n    Addr Mode       :0x%02x\n    Value           :0x%02x\n\n",
            nwkAddr, endpoint, addrMode, value);
    } else if((strstr(cmdBuff, "exit")) != 0) {
        printf("Closing. \n");
        socClose();
        server_close();
        exit(0);
    } else {
        printf("invalid command\n\n");
        //commandUsage();
    }

}

void getConsoleCommandParams(char* cmdBuff, u16 *nwkAddr, u8 *addrMode, u8 *ep, u8 *value, u16 *transitionTime, u16 *groupId)
{
  //set some default values
  uint32_t tmpInt;

  if( getParam( cmdBuff, "-n", &tmpInt) )
  {
    savedNwkAddr = (u16) tmpInt;
  }
  if( getParam( cmdBuff, "-m", &tmpInt) )
  {
    savedAddrMode = (u8) tmpInt;
  }
  if( getParam( cmdBuff, "-e", &tmpInt) )
  {
    savedEp = (u8) tmpInt;
  }
  if( getParam( cmdBuff, "-v", &tmpInt) )
  {
    savedValue = (u8) tmpInt;
  }
  if( getParam( cmdBuff, "-t", &tmpInt) )
  {
    savedTransitionTime = (u16) tmpInt;
  }
  if( getParam( cmdBuff, "-g", &tmpInt) )
  {
    savedGroupId = (u16) tmpInt;
  }

  *nwkAddr = savedNwkAddr;
  *addrMode = savedAddrMode;
  *ep = savedEp;
  *value = savedValue;
  *transitionTime = savedTransitionTime;
  *groupId = savedGroupId;

  return;
}

u32 getParam( char *cmdBuff, char *paramId, u32 *paramInt)
{
  char* paramStrStart;
  char* paramStrEnd;
  //0x1234+null termination
  char paramStr[7];
  u32 rtn = 0;

  memset(paramStr, 0, sizeof(paramStr));
  paramStrStart = strstr(cmdBuff, paramId);

  if( paramStrStart )
  {
    //index past the param idenentifier "-?"
    paramStrStart+=2;
    //find the the end of the param text
    paramStrEnd = strstr(paramStrStart, " ");
    if( paramStrEnd )
    {
      if(paramStrEnd-paramStrStart > (sizeof(paramStr)-1))
      {
        //we are not on the last param, but the param str is too long
        strncpy( paramStr, paramStrStart, (sizeof(paramStr)-1));
        paramStr[sizeof(paramStr)-1] = '\0';
      }
      else
      {
        //we are not on the last param so use the " " as the delimiter
        strncpy( paramStr, paramStrStart, paramStrEnd-paramStrStart);
        paramStr[paramStrEnd-paramStrStart] = '\0';
      }
    }

    else
    {
      //we are on the last param so use the just go the the end of the string
      //(which will be null terminate). But make sure that it is not bigger
      //than our paramStr buffer.
      if(strlen(paramStrStart) > (sizeof(paramStr)-1))
      {
        //command was entered wrong and paramStrStart will over flow the
        //paramStr buffer.
        strncpy( paramStr, paramStrStart, (sizeof(paramStr)-1));
        paramStr[sizeof(paramStr)-1] = '\0';
      }
      else
      {
        //Param is the correct size so just copy it.
        strcpy( paramStr, paramStrStart);
        paramStr[strlen(paramStrStart)-1] = '\0';
      }
    }

    //was the param in hex or dec?
    if(strstr(paramStr, "0x"))
    {
      //convert the hex value to an int.
      sscanf(paramStr, "0x%x", paramInt);
    }
    else
    {
      //assume that it ust be dec and convert to int.
      sscanf(paramStr, "%d", paramInt);
    }

    //paramInt was set
    rtn = 1;

  }

  return rtn;
}

