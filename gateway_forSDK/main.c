/**********************************************************************
 * INCLUDES
 */
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <poll.h>

#include "socCmd.h"
#include "server.h"
#include "nodes.h"

/**********************************************************************
 * LOCAL CONSTANTS
 */

#define MAX_POLL_FD_NUM           10

/**********************************************************************
 * LOCAL TYPES
 */

/* None */


/**********************************************************************
 * LOCAL VARIABLES
 */

struct pollfd pollFds[MAX_POLL_FD_NUM];


/**********************************************************************
 * LOCAL FUNCTIONS
 */
void usage( char* exeName );


/**********************************************************************
 * FUNCTIONS Implementation
 */

int main(int argc, char* argv[])
{
    int retval = 0;
    int soc_fd;
    int server_fd;
    int clients_fd[10];
    int clients_num = 0;
    int i;

    printf("%s -- %s %s\n", argv[0], __DATE__, __TIME__ );

    if( argc != 2 ) {
        usage(argv[0]);
        printf("attempting to use /dev/ttyACM0\n");
        soc_fd = socOpen( "/dev/ttyACM0" );
    } else {
        soc_fd = socOpen( argv[1] );
    }

    if( soc_fd == -1 ) {
        exit(-1);
    }

    nodes_reset();
    server_init();
    server_fd = server_open();
    if( server_fd == -1 ) {
        exit(-1);
    }

    //zllSocRegisterCallbacks( zllSocCbs );

    /* set some default values */
    //savedNwkAddr = 0x0002;
    //savedAddrMode = 0x02;
    //savedEp = 0x0b;
    //savedValue = 0x0;
    //savedTransitionTime = 0x1;

    while(1) {
        //set the zllSoC serial port FD in the poll file descriptors
        pollFds[0].fd = soc_fd;
        pollFds[0].events = POLLIN;

        //set the stdin FD in the poll file descriptors
        pollFds[1].fd = 0; //stdin
        pollFds[1].events = POLLIN;

        //set the Tcp Server FD in the poll file descriptors
        pollFds[2].fd = server_fd; //stdin
        pollFds[2].events = POLLIN;

        clients_num = 0;
        socketPool_get(clients_fd, &clients_num);

        for(i = 3; i < 3 + clients_num; i++) {
            pollFds[i].fd = clients_fd[i-3];
            pollFds[i].events = POLLIN;
        }

        poll(pollFds, 3 + clients_num, -1);

        //did the poll unblock because of the zllSoC serial?
        if(pollFds[0].revents) {
            processSocCmd();
        }
        //did the poll unblock because of the stdin?
        else if(pollFds[1].revents) {
            processConsoleCommand();
        }
        else if (pollFds[2].revents) {
            server_acceptNewConn();
        }
        else {
            for(i = 3; i < 3 + clients_num; i++) {
                if (pollFds[i].revents) {
                    processTcpCmd(clients_fd[i-3]);
                }
            }

        }

        usleep(10);
    }

    return retval;

}


void usage( char* exeName )
{
    printf("Usage: ./%s <port>\n", exeName);
    printf("Eample: ./%s /dev/ttyACM0\n", exeName);
}





