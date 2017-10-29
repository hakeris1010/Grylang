#ifndef GRYLSOCKS_H_DEFINED
#define GRYLSOCKS_H_DEFINED

// The header which defines system.
#include "systemcheck.h"

/* ======== Socket headers - OS dependent. ======== //
 *  Included here, to allow user to use Native Socket API alongside with GrylSocks.
 */ 
#if defined _GRYLTOOL_WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <windows.h>

#elif defined _GRYLTOOL_POSIX
    #include <unistd.h>
    #include <arpa/inet.h>  //close
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netdb.h>
    #include <netinet/in.h>
    #include <sys/time.h>   //FD_SET, FD_ISSET, FD_ZERO macros
    #include <errno.h>

    #define INVALID_SOCKET -1
    #define SOCKET_ERROR   -1
    
    typedef int SOCKET;

#endif // __linux__ || __WIN32

#define GSOCK_DEFAULT_BUFLEN 1500

/*! The socket data structure
 *  - Encapsulates a socket, a buffer of an initial size of GSOCK_DEFAULT_BUFLEN, and flags. 
 *  - Use for more convenience when transferring a buffer of each socket.
 */
typedef struct
{
    SOCKET sock;
    char dataBuff[GSOCK_DEFAULT_BUFLEN];
    char flags;
    short checksum;
} GSOCKSocketStruct;

int gsockGetLastError();
int gsockInitSocks();
int gsockErrorCleanup(SOCKET sock, struct addrinfo* addrin, const char* msg, char cleanupEverything, int retval);
int gsockCloseSocket(SOCKET sock);
void gsockSockCleanup();

SOCKET gsockConnectSocket(const char* address, const char* port, int family, int socktype, int protocol, int flags);
SOCKET gsockListenSocket(int port, const char* localBindAddr, int family, int socktype, int protocol, int flags);

int gsockReceive(SOCKET sock, char* buff, size_t bufsize, int flags);
int gsockSend(SOCKET sock, const char* buff, size_t bufsize, int flags); 

#endif
