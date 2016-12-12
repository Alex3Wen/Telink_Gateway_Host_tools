#ifndef  __SERVER_H__
#define  __SERVER_H__

/*********************************************************************
 * CONSTANTS
 */
#define MAX_SOCKET_NUM              10

/*********************************************************************
 * ENUMS
 */



/*********************************************************************
 * TYPES
 */

typedef struct {
    int sockets[MAX_SOCKET_NUM];
    int curNum;
} socketPool_t;



/*********************************************************************
 * Public Functions
 */
int  server_init(void);
int  server_open(void);
void server_close(void);
void processTcpCmd(int socket);
void server_acceptNewConn(void);

void socketPool_get(int* retSocks, int* number);

#endif  /* __SERVER_H__ */
