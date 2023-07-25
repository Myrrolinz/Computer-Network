#include"client.h"
#include<iostream>
#include<cstring>
#include<ctime>
#include<processthreadsapi.h>

// 部分可变化的全局量，支持自定义
// 客户端端口号
u_short ClientPort = 12870;
// 本地IP地址
const char* LocalIP = "127.0.0.1";
// 定义缓冲区大小
const int CBUF_SIZE = 2048;
bool connected = false;

// 全局变量声明
SOCKET RemoteSocket, LocalhostSocket;// sever服务器用来接收，client用来存放收到的请求
SOCKADDR_IN RemoteAddr, LocahhostAddr;// 用来存放IP地址和端口
int cnaddr = sizeof(SOCKADDR_IN);
char CsendBuf[CBUF_SIZE];// 发送缓冲区
char CinputBuf[CBUF_SIZE];// 输入内容缓冲区
char CrecvBuf[CBUF_SIZE];//接收内容缓冲区

char* getIP() {
	char* DstIP = new char[16];
	cin >> DstIP;
	return DstIP;
}

unsigned int getLen(char* a) {
	unsigned int index = 0;
	while (a[index] != '\0')
		index++;
	return index;
}

DWORD WINAPI recvMessagec(LPVOID) {
	while (1) {
		::memset(CrecvBuf, 0, CBUF_SIZE);//清空接收内容缓冲区
		//recv(SOCKET s, char* buf, int len, int flags) flags一般设置为0
		if (recv(LocalhostSocket, CrecvBuf, CBUF_SIZE, 0) < 0) {//使用recv函数接受接收信息
			cout << "Connect failed. Error code is " << SOCKET_ERROR << ".\n";
			system("pause");
		}
		else {
			if (isRecvQuit(CrecvBuf)) {//server quit
				cout << "Server has stopped this session, bye.\n";
				connected = false;
				break;
				ExitThread(1);
			}
			else
				cout << "From Server, at " << CrecvBuf << endl;//显示接收的消息
		}
	}
	return 0;
}

bool isQuit(char* a, unsigned int len) {
	if (len == 4 && a[0] == 'q' && a[1] == 'u' && a[2] == 'i' && a[3] == 't')
		return true;
	else
		return false;
}

bool isRecvQuit(char* a) {
	int len = 0;
	while (a[len] != '\0')
		len++;
	if (len == 29 && a[25] == 'q' && a[26] == 'u' && a[27] == 'i' && a[28] == 't')
		return true;
	else
		return false;
}

int clientMain() {
	// 加载socket库
	WSADATA wsadata;
	if (WSAStartup(MAKEWORD(2, 2), &wsadata) != 0) {
		//如果加载出错，暂停服务，输出出错信息
		cout << "载入socket失败\n";
		system("pause");
		return -1;
	}

	// 创建socket，参数顺序:IPV4，流式套接字，指定协议
	LocalhostSocket = socket(AF_INET, SOCK_STREAM, 0);
	// 检查socket创建是否成功
	if (LocalhostSocket == INVALID_SOCKET) {
		cout << "Socket创建失败，错误代码为：" << WSAGetLastError() << endl;
		WSACleanup();
		return -2;
	}

	// 套接字绑定本地地址信息
	LocahhostAddr.sin_family = AF_INET;
	LocahhostAddr.sin_port = htons(ClientPort);
	inet_pton(AF_INET, LocalIP, &LocahhostAddr.sin_addr.S_un.S_addr);


	// 服务器（目标地址信息）
	RemoteAddr.sin_family = AF_INET;
	cout << "Which IP would you wanna connect:\n";
	char* DstIP = getIP();
	cin.get();
	RemoteAddr.sin_port = htons(12260);
	inet_pton(AF_INET, DstIP, &RemoteAddr.sin_addr.S_un.S_addr);//将一个无符号短整型的主机数值转换为网络 字节顺序，即大尾顺序(big-endian)


	// 连接远程服务器
	if (connect(LocalhostSocket, (SOCKADDR*)&RemoteAddr, sizeof(RemoteAddr)) != SOCKET_ERROR) {
		// 连接成功
		// 接收信息
		HANDLE recv_thread = CreateThread(NULL, NULL, recvMessagec, NULL, 0, NULL);
		connected = true;

		while (1) {
			if (!connected)
				break;
			memset(CinputBuf, 0, CBUF_SIZE);
			cin.getline(CinputBuf, CBUF_SIZE);

			// 获取当前时间
			time_t now = time(0);
			// 转换成字符串形式
			char* dt = ctime(&now);

			memset(CsendBuf, 0, CBUF_SIZE);
			strcat(CsendBuf, dt);
			strcat(CsendBuf, CinputBuf);
				
			send(LocalhostSocket, CsendBuf, CBUF_SIZE, 0);
			if (isQuit(CinputBuf, getLen(CinputBuf))) {
				cout << "Quit, session closed.\n";
				connected = false;
				closesocket(LocalhostSocket);
				closesocket(RemoteSocket);
				WSACleanup();
				return 0;
			}
			memset(CsendBuf, '\0', CBUF_SIZE);
			memset(CinputBuf, '\0', CBUF_SIZE);
		}
	}
	else {
		cout << "Connect failed. Error code is " << SOCKET_ERROR << ".\n";
		return 0;
	}

	closesocket(LocalhostSocket);
	closesocket(RemoteSocket);
	WSACleanup();
	system("pause");
	return 0;
}