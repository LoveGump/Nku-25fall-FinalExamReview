# socket编程

UDP服务的应用程序编写步骤

<img src="/Users/gump/Library/Mobile Documents/com~apple~CloudDocs/大三下/计算机网络/image-20260110183547161.png" alt="image-20260110183547161" style="zoom:50%;" />

TCP服务的应用程序编写步骤

​	<img src="/Users/gump/Library/Mobile Documents/com~apple~CloudDocs/大三下/计算机网络/image-20260110183651685.png" alt="image-20260110183651685" style="zoom:50%;" />

## win中进行字节序转换的函数：

| **函数** | **功能**                                  |
| -------- | ----------------------------------------- |
| htons()  | Host to Network Short → 16 位（端口号）   |
| htonl()  | Host to Network Long → 32 位（IPv4 地址） |
| ntohs()  | Network to Host Short → 16 位             |
| ntohl()  | Network to Host Long → 32 位              |



## WSAStartup

```c
int WSAAPI WSAStartup(					// 成功返回0 ，否则会错误代码
		WORD wVersionRequested,			// 调用者希望使用的最高版本
		LPWSADATA lpWSAData,				// 可用socket的详细信息
)
```

<img src="/Users/gump/Library/Application Support/typora-user-images/image-20260110184124780.png" alt="image-20260110184124780" style="zoom:50%;" />

使用示例：

```c
#include <winsock2.h>
int main(){
    WSADATA wsaData;
    int ret = WSAStartup(MAKEWPRD(2,2),&wsaData);
    if (ret != 0){
       printf("WSAStartup failed: %d\n", ret);
        return 1;
    }
      // 这里开始才能安全地使用 socket
    SOCKET s = socket(AF_INET, SOCK_DGRAM, 0);

    // 用完之后
    closesocket(s);
  	WSACleanup();
}
MAKEWORD(low, high)；// MAKEWORD 只看低 8 位 对于MAKEWORD(2,2) 就是0x0202；
  // MAKEWORD(2, 1)   转为16进制就是 0x0102 
  
```



## WSACleanup

功能：结束使用socket，释放socket DLL资源

调用失败之后可以利用WSAGetLsatError 获取详细的错误信息

## socket

```c
SOCKET WSAAPI socket(
	int af,
  int type,
  int protocol
);
```

功能：创建一个socket，并绑定到一个特定的传输层服务；

参数：

- af：地址类型。AF_INET、AF_INET6等
- type：服务类型。SOCKET_STREAM、SOCKET_DGRAM等
- Protocol：协议。IPPROTO_TCP、IPPROTO_UDP、IPPROTO_ICMP。如果是0，则由系统自动选择

返回：

- 正确：0
- 错误INVALID_SOCKET。可以通过WSAGetLastError获取错误详情

```c++
SOCKET s = socket(AF_INET, SOCK_STREAM, 0);
if (s == INVALID_SOCKET) {
    std::cout << "socket 创建失败，错误码：" << WSAGetLastError() << std::endl;
}
```

## bind

```c
int bind(
	SOCKET s,
	const sockaddr *addr,
	int namelen
);
```

功能：将一个本地地址绑定到指定的Socket

参数：

- s ：socket描述符
- addr：地址。包括IP地址和端口号。如为INADDR_ANY和ip6addr_any，则由系统自动分配。
- namelen：地址长度。通常为sockaddr结构的长度

返回：

- 正确：0
- 错误：SOCKET_ERROE。`WSAGetLastError` 获取详情。

示例

```c++
SOCKET s = socket(AF_INET, SOCK_STREAM, 0); // TCP socket

struct sockaddr_in addr;
addr.sin_family = AF_INET;
addr.sin_port = htons(9000);               // 绑定端口 9000
addr.sin_addr.s_addr = INADDR_ANY;         // 接收本机所有网卡的连接

if (bind(s, (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
    printf("bind failed: %d\n", WSAGetLastError());
    closesocket(s);
    WSACleanup();
    return 1;
}
```

> 让一个 socket 有一个固定地址和端口，好让系统知道谁负责接收对应的数据。

## listen

```c++
int WSAAPI listen(
	SOCKET s,
	int backlod
);
```

功能：socket进入监听状态，监听远程==连接==是否到来

参数：

- s：socket描述符
- backlog：连接等待队列的最大长度

返回：

- 正确：0
- 错误：`SOCKET_ERROE`。`WSAGetLastError` 获取详情。

使用方式：==流方式，Server端==

示例：

```c
SOCKET server = socket(AF_INET, SOCK_STREAM, 0);

struct sockaddr_in addr;
addr.sin_family = AF_INET;
addr.sin_port = htons(9000);
addr.sin_addr.s_addr = INADDR_ANY; // 

bind(server, (struct sockaddr*)&addr, sizeof(addr));

// 进入监听状态
if (listen(server, 5) == SOCKET_ERROR) {
    printf("listen failed: %d\n", WSAGetLastError());
}
```

- 调用 listen 后，socket **变成监听 socket**

- 不能再直接 send() 或 recv()
- 只能通过 accept() 拿到客户端连接

## connect

```c
int WSASPI connet(
	SOCKET s,
	const sockaddr *name,
	int namelen
);
```

功能：向一个特定的socket发送连接请求

参数：

- s
- name：地址。包括IP和端口号
- name：地址长度。通常也为`sockaddr`的长度

返回：

- 正确：0
- 错误：`SOCKET_ERROE`。`WSAGetLastError` 获取详情。

使用方式：==流方式、client端==

示例：

```c
SOCKET s = socket(AF_INET, SOCK_STREAM, 0);

struct sockaddr_in serverAddr;
serverAddr.sin_family = AF_INET;
serverAddr.sin_port = htons(9000);                 // 服务器端口
inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr); // 将ip地址转为二进制的函数
// serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
// serverAddr.sin_addr.s_addr = htonl(0x7f000001);  // 127.0.0.1 保证字节序


if (connect(s, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
    printf("connect failed: %d\n", WSAGetLastError());
    closesocket(s);
    WSACleanup();
    return 1;
}

printf("Connected to server!\n");
```

## accept

```c
SOCKET WSAAPI accept(
	SOCKET s,
	sockaddr *addr,	// 这里可以为null 表示不保留客户端的信息
  int *addrlen
);
```

功能：接受一个特定socket请求等待队列中的连接请求

参数：

- s：socket描述符
- addr，返回远程端地址 
- **addrlen** → 初始化为 struct sockaddr 的大小，返回客户端地址实际长度

返回：

- 正确：返回新连接的socket的描述符。
- 错误：`INVALID_SOCKET`。可通过`WSAGetLAstError`获取错误详情

使用：==流方式、Server端==。通常运行后进入==阻塞==状态，直到连接请求的到来。

示例：

```c
SOCKET server = socket(AF_INET, SOCK_STREAM, 0);

bind(server, (struct sockaddr*)&addr, sizeof(addr)); // 绑定地址
listen(server, 5);

struct sockaddr_in clientAddr;
int clientAddrLen = sizeof(clientAddr);

// 阻塞等待客户端连接
SOCKET client = accept(server, (struct sockaddr*)&clientAddr, &clientAddrLen);
if (client == INVALID_SOCKET) {
    printf("accept failed: %d\n", WSAGetLastError());
} else {
    printf("Client connected!\n");
}
```

## sendto

```c
int WSAAPI sendto(
	SOCKET s,						// socket描述
	const char *buf,		// 要发送的数据缓冲区
	int len,						// 发送数据的长度
	int flags,					// 对调用的处理方式，OOB等
	const sockaddr *to,	// 目标socket地址
	int tolen						// 目标地址长度
	);
```

功能：向指定的目的地址发送数据

返回：

- 正确：返回实际发送的==字节数==
- 错误：`SOCKET_ERROE`。`WSAGetLastError` 获取详情。

使用：==数据报方式==

示例

```c
SOCKET udpSock = socket(AF_INET, SOCK_DGRAM, 0);

struct sockaddr_in serverAddr;
serverAddr.sin_family = AF_INET;
serverAddr.sin_port = htons(9000);
inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);

const char *msg = "Hello UDP server";

sendto(udpSock, msg, strlen(msg), 0,
       (struct sockaddr*)&serverAddr, sizeof(serverAddr));
```

无连接，不需要先进行connect 每次都需要制定ip地址和端口号，不用确保数据一定到达。

## recvfrom（阻塞接收）

```c
int WSAAPI recvfrom(
	SOCKET s,					// socket描述符
	char *buf,				// 接受数据的缓冲区
	int len,					// 接收缓冲区的长度
	int flags,				// 对调用的处理方式， OOB
 	sockaddr *addr,		// 源socket的地址
	int *fromlen			// 源地址的长度
);
```

功能：从特定的目的地址接受数据

返回：

- 正确：接收到的字节数
- 错误错误：`SOCKET_ERROE`。`WSAGetLastError` 获取详情。

使用：==数据报方式==

示例：

```c
SOCKET udpSock = socket(AF_INET, SOCK_DGRAM, 0);

struct sockaddr_in serverAddr;
serverAddr.sin_family = AF_INET;
serverAddr.sin_port = htons(9000);
inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);

const char *msg = "Hello UDP server";

sendto(udpSock, msg, strlen(msg), 0,
       (struct sockaddr*)&serverAddr, sizeof(serverAddr));
```

## send

```c
int WSAAPI send(
	SOCKET s,
	const char *buf,	// 发动数据缓存区
	int len,					// 发送数据的长度
	int flags
);
```

功能：向socket发送数据

返回：

- 正确：返回实际发送的==字节数==
- 错误：`SOCKET_ERROE`。`WSAGetLastError` 获取详情。

使用：==流方式==，已经建立连接。

```c
const char *msg = "Hello, server!";
int bytesSent = send(sock, msg, (int)strlen(msg), 0);

if (bytesSent == SOCKET_ERROR) {
    printf("send failed: %d\n", WSAGetLastError());
} else {
    printf("Sent %d bytes\n", bytesSent);
}
// 数据很大，循环发送
int totalSent = 0;
int msgLen = strlen(msg);
while (totalSent < msgLen) {
    int sent = send(sock, msg + totalSent, msgLen - totalSent, 0);
    if (sent == SOCKET_ERROR) {
        printf("send failed: %d\n", WSAGetLastError());
        break;
    }
    totalSent += sent;
}
```



## recv（阻塞接收）

- **调用函数后，如果缓冲区没有数据，函数会一直等待**，直到有数据到达或者连接关闭（TCP）或错误发生。
- 阻塞模式是 socket 的默认状态，无论是 TCP（recv）还是 UDP（recvfrom）都是如此。

```c
int recv(
    SOCKET s,       // 已连接的 socket
    char *buf,      // 数据接收缓冲区
    int len,        // 缓冲区长度
    int flags       // 标志，通常为 0
);
```

返回：

- 正确：接收到的字节数
- 错误错误：`SOCKET_ERROE`。`WSAGetLastError` 获取详情。

使用：==流方式==

```c
// 已经建立过连接
char buffer[512];
int bytesReceived = recv(sock, buffer, sizeof(buffer) - 1, 0);
if (bytesReceived > 0) {
    buffer[bytesReceived] = '\0';  // null-terminate
    printf("Received: %s\n", buffer);
} else if (bytesReceived == 0) {
    printf("Server closed connection\n");
} else {
    printf("recv failed: %d\n", WSAGetLastError());
}
```

## closesocket

```c
int WSAAPI closesocket(
	socket s       // socket描述符 
);
```

功能：关闭一个socket

返回：

- 正确：接收到的字节数
- 错误错误：`SOCKET_ERROE`。`WSAGetLastError` 获取详情。

- **TCP**：
  - closesocket 会先发 FIN 给对方，进入 **四次挥手关闭连接**
  - 确保数据尽量发送完毕
- **UDP**：
  - 没有连接，直接释放 socket 资源即可

## SOCKETADDR 和SOCKETADDR_IN的结构

```c
typedef struct sockaddr(
		u_short sa_family;	// 协议族AF_INET、AF_INET6 等
  	char sa_data[14];		// 保存 IP + 端口等信息
) SOCKADDR,*PSOCKADDR,*LPSOCKADDR;

// SOCKADDR_IN 是 IPv4 地址专用结构，比 SOCKADDR 可读性更强：
typedef struct sockaddr_in(
	short sin_family,				// AF_INET
  u_short sin_port,				// 端口号（网络字节序）
  struct in_addr sin_addr;// IPv4 地址
  char sin_zero[8];				// 填充对齐，通常置 0
)SOCKADDR_IN,*PSOCKADDR_IN,*LPSOCKADDR_IN;
```

In_addr结构

```c
struct in_addr {
    union {
        struct {
            u_char s_b1; 	// 第一个字节
            u_char s_b2;	// 第二个字节 
            u_char s_b3;	// ...
            u_char s_b4;
        } S_un_b;
        struct {
            u_short s_w1;	// 前两个字节高16位
            u_short s_w2;	// 低 16 位
        } S_un_w;
        u_long S_addr;
    } S_un;
};
```

## CreateThread

```c
HANDLE CreateThread(
    LPSECURITY_ATTRIBUTES   lpThreadAttributes, // 返回句柄能否被继承。 NULL表示不能被继承
    SIZE_T                  dwStackSize,        // 堆栈初始大小，0 表示默认
    LPTHREAD_START_ROUTINE  lpStartAddress,     // 新线程的开始执行地址。自己线程函数的开始地址
    LPVOID                  lpParameter,        // 传给线程函数的参数
    DWORD                   dwCreationFlags,    // 创建标志，0 表示立即运行
    LPDWORD                 lpThreadId          // 返回线程 ID，可为 NULL
);
```

示例：

```c
#include <windows.h>
#include <iostream>

// 线程函数，必须符合 DWORD WINAPI(LPVOID) 原型
DWORD WINAPI ThreadFunc(LPVOID lpParam) {
    int* pValue = static_cast<int*>(lpParam);
    std::cout << "Hello from thread! Parameter = " << *pValue << std::endl;

    // 可以执行任意任务，比如循环打印
    for (int i = 0; i < 5; ++i) {
        std::cout << "Thread running: i = " << i << std::endl;
        Sleep(500); // 暂停 500 毫秒
    }

    return 0; // 返回值可用作 ExitCode
}

int main() {
    int threadParam = 42; // 线程参数

    // 创建线程
    DWORD threadId;
    HANDLE hThread = CreateThread(
        nullptr,       // 默认安全属性
        0,             // 默认栈大小
        ThreadFunc,    // 线程函数
        &threadParam,  // 参数传递
        0,             // 创建后立即运行
        &threadId      // 返回线程ID
    );

    if (hThread == nullptr) {
        std::cerr << "CreateThread failed, error: " << GetLastError() << std::endl;
        return 1;
    }

    std::cout << "Main thread: Waiting for child thread to finish..." << std::endl;

    // 等待线程结束
    WaitForSingleObject(hThread, INFINITE);

    std::cout << "Main thread: Child thread finished!" << std::endl;

    // 关闭线程句柄，释放资源
    CloseHandle(hThread);

    return 0;
}
```

## 代码示例

### UDP客户端

```c++
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>

#pragma comment(lib, "ws2_32.lib")

int main() {
    WSADATA wsaData;
    WORD wVersionRequested = MAKEWORD(2, 2);

    // 初始化 Winsock
    if (WSAStartup(wVersionRequested, &wsaData) != 0) {
        std::cerr << "WSAStartup failed!" << std::endl;
        return 1;
    }

    // 创建 UDP socket
    SOCKET sockClient = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockClient == INVALID_SOCKET) {
        std::cerr << "socket failed: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return 1;
    }

    // 设置服务器地址
    sockaddr_in addrSrv;
    addrSrv.sin_family = AF_INET;
    addrSrv.sin_port = htons(9000);  // 服务器端口
    inet_pton(AF_INET, "127.0.0.1", &addrSrv.sin_addr); // 服务器 IP

    // 发送数据
    const char* sendBuf = "Hello UDP Server!";
    int sendlen = (int)strlen(sendBuf);
    int addrLen = sizeof(addrSrv);

    if (sendto(sockClient, sendBuf, sendlen, 0, (sockaddr*)&addrSrv, addrLen) == SOCKET_ERROR) {
        std::cerr << "sendto failed: " << WSAGetLastError() << std::endl;
        closesocket(sockClient);
        WSACleanup();
        return 1;
    }

    std::cout << "Data sent to server." << std::endl;

    // 接收数据
    char recvBuf[50];
    int recvlen = recvfrom(sockClient, recvBuf, sizeof(recvBuf) - 1, 0,
                           (sockaddr*)&addrSrv, &addrLen);
    if (recvlen == SOCKET_ERROR) {
        std::cerr << "recvfrom failed: " << WSAGetLastError() << std::endl;
    } else {
        recvBuf[recvlen] = '\0'; // 字符串结束符
        std::cout << "Received from server: " << recvBuf << std::endl;
    }

    // 关闭 socket
    closesocket(sockClient);
    WSACleanup();

    return 0;
}
```

### UDP服务端

```c++
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>

#pragma comment(lib, "ws2_32.lib")

int main() {
    WSADATA wsaData;
    WORD wVersionRequested = MAKEWORD(2, 2);

    // 初始化 Winsock
    if (WSAStartup(wVersionRequested, &wsaData) != 0) {
        std::cerr << "WSAStartup failed!" << std::endl;
        return 1;
    }

    // 创建 UDP socket
    SOCKET sockSrv = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockSrv == INVALID_SOCKET) {
        std::cerr << "socket failed: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return 1;
    }

    // 设置服务器地址并绑定
    sockaddr_in addrSrv;
    addrSrv.sin_family = AF_INET;
    addrSrv.sin_port = htons(9000);  // 服务器端口
  	// INADDR_ANY在网络中的字段为 0.0.0.0 可以转为网络序，也可不进行转换
    addrSrv.sin_addr.s_addr = INADDR_ANY;        // 绑定所有网卡

    if (bind(sockSrv, (sockaddr*)&addrSrv, sizeof(addrSrv)) == SOCKET_ERROR) {
        std::cerr << "bind failed: " << WSAGetLastError() << std::endl;
        closesocket(sockSrv);
        WSACleanup();
        return 1;
    }

    std::cout << "UDP server is running..." << std::endl;

    char recvBuf[50];
    const char* sendBuf = "Message received!";
    int addrLen;
    sockaddr_in addrClient;

    while (true) {
        addrLen = sizeof(addrClient);
        // 接收客户端消息
        int recvlen = recvfrom(sockSrv, recvBuf, sizeof(recvBuf) - 1, 0,
                               (sockaddr*)&addrClient, &addrLen);
        if (recvlen == SOCKET_ERROR) {
            std::cerr << "recvfrom failed: " << WSAGetLastError() << std::endl;
            continue;
        }

        recvBuf[recvlen] = '\0'; // 字符串结束符
        std::cout << "Received from client: " << recvBuf << std::endl;

        // 回复客户端
        int sendlen = (int)strlen(sendBuf);
        if (sendto(sockSrv, sendBuf, sendlen, 0,
                   (sockaddr*)&addrClient, addrLen) == SOCKET_ERROR) {
            std::cerr << "sendto failed: " << WSAGetLastError() << std::endl;
        }
    }

    // 永远不会到这里，但正确写法应该关闭 socket
    closesocket(sockSrv);
    WSACleanup();

    return 0;
}
```

### TCP客户端

```c++
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>

#pragma comment(lib, "ws2_32.lib")

int main() {
    WSADATA wsaData;
    WORD wVersionRequested = MAKEWORD(2, 2);

    // 初始化 Winsock
    if (WSAStartup(wVersionRequested, &wsaData) != 0) {
        std::cerr << "WSAStartup failed!" << std::endl;
        return 1;
    }

    // 创建 TCP socket
    SOCKET sockClient = socket(AF_INET, SOCK_STREAM, 0);
    if (sockClient == INVALID_SOCKET) {
        std::cerr << "socket failed: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return 1;
    }

    // 设置服务器地址
    sockaddr_in addrSrv;
    addrSrv.sin_family = AF_INET;
    addrSrv.sin_port = htons(9000); // 服务器端口
    inet_pton(AF_INET, "127.0.0.1", &addrSrv.sin_addr); // 服务器 IP

    // 连接服务器
    if (connect(sockClient, (sockaddr*)&addrSrv, sizeof(addrSrv)) == SOCKET_ERROR) {
        std::cerr << "connect failed: " << WSAGetLastError() << std::endl;
        closesocket(sockClient);
        WSACleanup();
        return 1;
    }

    std::cout << "Connected to server!" << std::endl;

    // 发送数据
    const char* sendBuf = "Hello TCP Server!";
    if (send(sockClient, sendBuf, (int)strlen(sendBuf), 0) == SOCKET_ERROR) {
        std::cerr << "send failed: " << WSAGetLastError() << std::endl;
    } else {
        std::cout << "Sent: " << sendBuf << std::endl;
    }

    // 接收数据
    char recvBuf[50];
    int bytesReceived = recv(sockClient, recvBuf, sizeof(recvBuf) - 1, 0);
    if (bytesReceived > 0) {
        recvBuf[bytesReceived] = '\0'; // 字符串结束符
        std::cout << "Received: " << recvBuf << std::endl;
    } else if (bytesReceived == 0) {
        std::cout << "Server closed connection." << std::endl;
    } else {
        std::cerr << "recv failed: " << WSAGetLastError() << std::endl;
    }

    // 关闭 socket
    closesocket(sockClient);
    WSACleanup();

    return 0;
}
```
### TCP服务端

```c
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>

#pragma comment(lib, "ws2_32.lib")

int main() {
    WSADATA wsaData;
    WORD wVersionRequested = MAKEWORD(2, 2);

    // 初始化 Winsock
    if (WSAStartup(wVersionRequested, &wsaData) != 0) {
        std::cerr << "WSAStartup failed!" << std::endl;
        return 1;
    }

    // 创建 TCP socket
    SOCKET sockSrv = socket(AF_INET, SOCK_STREAM, 0);
    if (sockSrv == INVALID_SOCKET) {
        std::cerr << "socket failed: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return 1;
    }

    // 设置服务器地址并绑定
    sockaddr_in addrSrv;
    addrSrv.sin_family = AF_INET;
    addrSrv.sin_port = htons(9000);          // 服务器端口
    addrSrv.sin_addr.s_addr = INADDR_ANY;    // 监听所有网卡

    if (bind(sockSrv, (sockaddr*)&addrSrv, sizeof(addrSrv)) == SOCKET_ERROR) {
        std::cerr << "bind failed: " << WSAGetLastError() << std::endl;
        closesocket(sockSrv);
        WSACleanup();
        return 1;
    }

    // 开始监听
    if (listen(sockSrv, 5) == SOCKET_ERROR) {
        std::cerr << "listen failed: " << WSAGetLastError() << std::endl;
        closesocket(sockSrv);
        WSACleanup();
        return 1;
    }

    std::cout << "TCP server is running, waiting for clients..." << std::endl;

    // 循环接收客户端连接
    while (true) {
        sockaddr_in addrClient;
        int addrLen = sizeof(addrClient);

        SOCKET sockConn = accept(sockSrv, (sockaddr*)&addrClient, &addrLen);
        if (sockConn == INVALID_SOCKET) {
            std::cerr << "accept failed: " << WSAGetLastError() << std::endl;
            continue;
        }

        std::cout << "Client connected!" << std::endl;

        // 接收客户端数据
        char recvBuf[50];
        int bytesReceived = recv(sockConn, recvBuf, sizeof(recvBuf) - 1, 0);
        if (bytesReceived > 0) {
            recvBuf[bytesReceived] = '\0';
            std::cout << "Received from client: " << recvBuf << std::endl;

            // 回复客户端
            const char* sendBuf = "Message received!";
            send(sockConn, sendBuf, (int)strlen(sendBuf), 0);
        } else if (bytesReceived == 0) {
            std::cout << "Client disconnected." << std::endl;
        } else {
            std::cerr << "recv failed: " << WSAGetLastError() << std::endl;
        }

        // 关闭客户端 socket
        closesocket(sockConn);
    }

    // 关闭服务器 socket（通常不会到这里，因为循环永远运行）
    closesocket(sockSrv);
    WSACleanup();

    return 0;
}
```

### 多线程TCP服务端

```c
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <iostream>

#pragma comment(lib, "ws2_32.lib")

// 每个客户端线程处理函数
DWORD WINAPI ClientHandler(LPVOID lpParam) {
    SOCKET ClientSocket = (SOCKET)lpParam;

    // 接收客户端数据
    char recvBuf[50];
    int bytesReceived = recv(ClientSocket, recvBuf, sizeof(recvBuf) - 1, 0);
    if (bytesReceived > 0) {
        recvBuf[bytesReceived] = '\0';
        std::cout << "Received from client: " << recvBuf << std::endl;

        // 回复客户端
        const char* sendBuf = "Message received!";
        send(ClientSocket, sendBuf, (int)strlen(sendBuf), 0);
    } else if (bytesReceived == 0) {
        std::cout << "Client disconnected." << std::endl;
    } else {
        std::cerr << "recv failed: " << WSAGetLastError() << std::endl;
    }

    // 关闭客户端 socket
    closesocket(ClientSocket);
    return 0;
}

int main() {
    WSADATA wsaData;
    WORD wVersionRequested = MAKEWORD(2, 2);

    // 初始化 Winsock
    if (WSAStartup(wVersionRequested, &wsaData) != 0) {
        std::cerr << "WSAStartup failed!" << std::endl;
        return 1;
    }

    // 创建 TCP socket
    SOCKET sockSrv = socket(AF_INET, SOCK_STREAM, 0);
    if (sockSrv == INVALID_SOCKET) {
        std::cerr << "socket failed: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return 1;
    }

    // 设置服务器地址并绑定
    sockaddr_in addrSrv;
    addrSrv.sin_family = AF_INET;
    addrSrv.sin_port = htons(9000);         // 服务器端口
    addrSrv.sin_addr.s_addr = INADDR_ANY;   // 监听所有网卡

    if (bind(sockSrv, (sockaddr*)&addrSrv, sizeof(addrSrv)) == SOCKET_ERROR) {
        std::cerr << "bind failed: " << WSAGetLastError() << std::endl;
        closesocket(sockSrv);
        WSACleanup();
        return 1;
    }

    // 开始监听
    if (listen(sockSrv, 5) == SOCKET_ERROR) {
        std::cerr << "listen failed: " << WSAGetLastError() << std::endl;
        closesocket(sockSrv);
        WSACleanup();
        return 1;
    }

    std::cout << "TCP server running, waiting for clients..." << std::endl;

    // 循环接受客户端连接
    while (true) {
        sockaddr_in addrClient;
        int addrLen = sizeof(addrClient);

        SOCKET ClientSocket = accept(sockSrv, (sockaddr*)&addrClient, &addrLen);
        if (ClientSocket == INVALID_SOCKET) {
            std::cerr << "accept failed: " << WSAGetLastError() << std::endl;
            continue;
        }

        std::cout << "Client connected!" << std::endl;

        // 为每个客户端创建线程
        DWORD threadId;
        HANDLE hThread = CreateThread(
            nullptr,            // 默认安全属性
            0,                  // 默认栈大小
            ClientHandler,      // 线程函数
            (LPVOID)ClientSocket, // 参数传递客户端 socket
            0,                  // 创建后立即运行
            &threadId
        );

        if (hThread == nullptr) {
            std::cerr << "CreateThread failed: " << GetLastError() << std::endl;
            closesocket(ClientSocket);
        } else {
            CloseHandle(hThread); // 不再需要保存句柄
        }
    }

    // 永远不会到这里，通常需要信号或条件退出循环
    closesocket(sockSrv);
    WSACleanup();

    return 0;
}
```

## 应用层

### HTTP/1.1 为什么会发生头阻塞

HTTP/1.1是基于文本的协议，请求和响应报文都没有标识符，在同一 TCP 连接上传递的请求和响应的报文，都只能有序地进行匹配。在停止等待机制中，发出一个请求报文之后，需要等待响应报文才能发送下一个请求报文；在流水线机制中，响应报文的返回顺序必须与请求报文的发送顺序一致，才能实现请求报文和响应报文的匹配。

###  HTTP/2.0 的改进

1）流与二进制分帧

- 流：在一个tcp连接上可以并发地抽象出多个双向流（也可以看成虚拟通道），每个流都有一个唯一的流标识符
- 二进制分帧：基于文本的方式对HTTP报文进行切割，并封装成一个或者多个二进制帧

2）多路复用

- 多个流的帧可以**交错发送在同一个 TCP 连接**上。
- 不同流的响应不再受其他流阻塞，解决了 HTTP/1.1 的 HOL 阻塞问题。

### 报文压缩

静态表、动态表、哈夫曼编码

### 服务器推送：

允许服务器在没有明确请求的情况下向客户端推送额外的资源。

### 最长前缀匹配的依据

依据就是匹配前缀的位数：

1. 将目标地址与每条路由前缀做 **按位比较**（从高位开始）。
2. 找出匹配的最长连续前缀长度。
3. 使用前缀长度最长的路由作为最终下一跳。

### 若链路上有多条流共享带宽（统计多路复用），端到端时延最主要受哪一类时延主导？简述原因

端到端时延最主要受排队时延主导，端到端时延通常包括

$D_{e2e} = D_{\text{处理}} + D_{\text{排队}} + D_{\text{传输}} + D_{\text{传播}}$​ 

处理时延：一般很短；传输时延：通常固定，由数据大小决定；传播时延：固定。

在统计多路复用环境下，由于流量突发性和竞争带宽，报文在队列中等待转发的时间变化最大，因此 排队时延成为端到端时延的主导因素。

### CDN（内容分发网络）重定向的两种方式简单描述

#### **基于 DNS 的重定向（DNS-based redirection）**

- **原理**：用户访问域名时，DNS 解析阶段返回 **就近节点的 IP 地址**
- **特点**：DNS中的CNAME类型的资源记录
  - 透明给客户端，浏览器直接连接返回的 IP
  - 基于 **DNS 解析** 做负载均衡和地理就近选择
  - 缺点：DNS 缓存可能导致短时间内无法灵活调整节点

**流程简化**：

```
用户浏览器 → DNS 查询 → CDN DNS 返回最近节点 IP → 直接访问该节点
```

<img src="/Users/gump/Library/Application Support/typora-user-images/image-20260111135012503.png" alt="image-20260111135012503" style="zoom:50%;" />

#### 基于 HTTP  的重定向（HTTP-based redirection）

- **原理**：用户请求到源站或 CDN 边缘节点，服务器返回 **HTTP 3xx 重定向响应**，指向离用户更近的 CDN 节点
- **特点**：权威域名服务器中A类型的资源记录
  - 依赖 HTTP 协议，客户端会按响应去访问新的节点
  - 更灵活，可以做精细化策略（根据用户状态、时间、业务等）
  - 增加一次请求响应开销

**流程简化**：

```
用户请求 → 源站/边缘节点 → 返回 302/307 → 用户浏览器访问指定节点
```

<img src="/Users/gump/Library/Application Support/typora-user-images/image-20260111135221398.png" alt="image-20260111135221398" style="zoom:50%;" />

### DHCP UDP67端口

DHCP除了提供可用的ip地址之外，还要提供网络掩码、IP地址租期、默认路由器的IP地址、域名服务器的IP地址等消息。客户端在IP地址租期过半时，需要重新发送DHCPrequest进行续租。客户端也可以使用DHCPrelease随时终止IP地址的租用。

DHCP报文的采用广播的方式进行传输，所封装的IP地址为255.255.255.255（受限广播地址）包含该地址的IP数据包不会被路由器转发，传播范围为请求主机所在的物理网络。

## TCP协议的局限性

1. TCP连接不支持多路径传输和网络接口之间的动态切换
   TCP<源IP，源端口，目的IP，目的端口>任意一元改变，都不能持续这个连接
2. TCP叠加安全传输层协议（TLS）会引入较大的握手延时。
   在http和tcp之间添加的tls层，保证安全
3. TCP确认重传机制及按序交付会形成队头阻塞
   可以同时建立多个tcp连接来缓解（后面还有别的策略）

