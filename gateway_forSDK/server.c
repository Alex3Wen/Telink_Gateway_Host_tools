/**********************************************************************
 * INCLUDES
 */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>

#include "server.h"
#include "appCmd.h"

/**********************************************************************
 * LOCAL CONSTANTS
 */

#define TCP_SERVER_LISTHEN_PORT    16000

#define SOCKET_NOT_FOUND           -1
#define INVALID_SOCKET             -1
#define EMPTY_SOCKET_VAL           -1

/**********************************************************************
 * LOCAL TYPES
 */

typedef struct {
    int tcp_server_sock;
    struct sockaddr_in tcp_server_listenAddr;
    socketPool_t sockPool;
} server_ctrl_t;


/**********************************************************************
 * LOCAL VARIABLES
 */

server_ctrl_t server_vars;
server_ctrl_t* server_v = &server_vars;


/**********************************************************************
 * LOCAL FUNCTIONS
 */
int socketPool_search(int sock);
void socketPool_add(int newSock);
void socketPook_del(int delSock);


/**********************************************************************
 * FUNCTIONS IMPLEMEMTATION
 */

int server_init(void)
{
    /* Init socket pool */
    memset(server_v->sockPool.sockets, 0xff, sizeof(int) * MAX_SOCKET_NUM);
    server_v->sockPool.curNum = 0;
}

 /*********************************************************************
 * @fn      server_open
 *
 * @brief   open the tcp server
 *
 * @param   none
 *
 * @return  the opened socket
 */
int server_open(void)
{
    const int on = 1;

    /* TCP Server */
    if (-1 == (server_v->tcp_server_sock = socket(AF_INET, SOCK_STREAM, 0))) {
        perror("socket create failed.");
        return -1;
    }

    bzero((void*)&server_v->tcp_server_listenAddr, sizeof(struct sockaddr_in));

    server_v->tcp_server_listenAddr.sin_family = AF_INET;
    server_v->tcp_server_listenAddr.sin_port = htons(TCP_SERVER_LISTHEN_PORT);
    server_v->tcp_server_listenAddr.sin_addr.s_addr = INADDR_ANY;//inet_addr(LOCAL_RECV_ADDR);

    /* set socket non-block */
    if(-1 == fcntl(server_v->tcp_server_sock, F_SETFL, O_NONBLOCK)) {
        perror("fcntl set failed.");
        return -1;
    }

    setsockopt(server_v->tcp_server_sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    if(-1 == bind(server_v->tcp_server_sock, (struct sockaddr*)&server_v->tcp_server_listenAddr, sizeof(struct sockaddr_in))) {
        perror("bind socket failed.");
        return -1;
    }

    if(-1 == listen(server_v->tcp_server_sock, 255)) {
        perror("socket listen failed.");
        return -1;
    }

    return server_v->tcp_server_sock;
}


 /*********************************************************************
 * @fn      server_close
 *
 * @brief   close the tcp server
 *
 * @param   none
 *
 * @return  none
 */
void server_close(void)
{
    close(server_v->tcp_server_sock);
}


/*********************************************************************
 * @fn      server_acceptNewConn
 *
 * @brief   handle new client accept command
 *
 * @param   none
 *
 * @return  none
 */
void server_acceptNewConn(void)
{
    int tempSock;
    struct linger m_sLinger;
    struct sockaddr_in fromAddr;
    int fromLen = sizeof(struct sockaddr_in);
    u8 extAddr[8] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};

    if (-1 == (tempSock = accept(server_v->tcp_server_sock, (struct sockaddr*)(&fromAddr), &fromLen))) {
        printf("net_recv: accept error! errno = %d\n", errno);
        return;
    }

    m_sLinger.l_onoff = 1;
    m_sLinger.l_linger = 0;
    setsockopt(tempSock, SOL_SOCKET, SO_LINGER, (const char*)&m_sLinger,sizeof(m_sLinger));

    /* Add to socket pool */
    socketPool_add(tempSock);


    /* for test */
    //app_sendDeviceReportCmd(DEV_TYPE_LIGHT, 0x0001, extAddr);

}

/*********************************************************************
 * @fn      server_send
 *
 * @brief   send data to connected client
 *
 * @param   none
 *
 * @return  none
 */
void server_send(int clientSock, u8* buf, u8 len)
{
    /* send TCP message */
    int i;
    printf("send to App:\n");
    for (i = 0; i < len; i++) {
        printf("0x%x ", buf[i]);
    }
    printf("\n");

    if (-1 == send(clientSock, buf, len, 0)) {
        printf("send error! errno = %d\n", errno);
        return;
    }
}

 /*********************************************************************
 * @fn      processTcpCmd
 *
 * @brief   handle the tcp command send from client
 *
 * @param   none
 *
 * @return  none
 */
void processTcpCmd(int clientSocket)
{
    int fromLen = 0;
    int recvLen = 0;
    u8 buf[50];


    fromLen = sizeof(struct sockaddr_in);
    recvLen = recv(clientSocket, buf, MAX_APP_PACKET_LEN, 0);

    if(recvLen == 0) {
        /* Disconnected */
        close(clientSocket);
        socketPool_del(clientSocket);
        return;
    }

    if (recvLen > 0) {
        app_cmdHandler(buf, recvLen);
    }
}


 /*********************************************************************
 * @fn      socketPool_search
 *
 * @brief   search the specified socket from pool
 *
 * @param   sock - The socket to be searched
 *
 * @return  the index of search result
 */
int socketPool_search(int sock)
{
    int i;
    for(i = 0; i < MAX_SOCKET_NUM; i++) {
        if (server_v->sockPool.sockets[i] == sock) {
            return i;
        }
    }
    return SOCKET_NOT_FOUND;
}

 /*********************************************************************
 * @fn      socketPool_add
 *
 * @brief   add the specified socket to the pool
 *
 * @param   newSock - The socket to add
 *
 * @return  none
 */
void socketPool_add(int newSock)
{
    int index = socketPool_search(newSock);
    if (SOCKET_NOT_FOUND != index) {
        return;
    }

    index = socketPool_search(EMPTY_SOCKET_VAL);
    if (SOCKET_NOT_FOUND == index) {
        /* Table already full */
        return;
    }

    server_v->sockPool.sockets[index] = newSock;
    server_v->sockPool.curNum++;
}

 /*********************************************************************
 * @fn      socketPool_del
 *
 * @brief   delete the specified socket from pool
 *
 * @param   delSock - The socket to delete
 *
 * @return  none
 */
void socketPool_del(int delSock)
{
    int index = socketPool_search(delSock);
    if (SOCKET_NOT_FOUND == index) {
        return;
    }

    server_v->sockPool.sockets[index] = INVALID_SOCKET;
    server_v->sockPool.curNum--;
}

/*********************************************************************
 * @fn      socketPool_get
 *
 * @brief   get all the clients socket
 *
 * @param   retSocks - The return value of all clients socket
 * @param   number - The returned clients number
 *
 * @return  none
 */
void socketPool_get(int* retSocks, int* number)
{
    int i;
    *number = 0;
    for(i = 0; i < MAX_SOCKET_NUM; i++) {
        if (server_v->sockPool.sockets[i] != INVALID_SOCKET) {
            retSocks[(*number)++] = server_v->sockPool.sockets[i];
        }
    }
}







