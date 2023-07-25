#include"server.h"
#include"client.h"
using namespace std;

// 部分可变化的全局量，支持自定义
// 服务器端口号
u_short ServerPort = 12260;
// 服务器IP地址
char ServerIP[16];
// 定义缓冲区大小
const int SBUF_SIZE = 2048;
// 监听最大队列数
const int ListrnMax = 5;
bool Sconnected = false;

// 全局变量声明
SOCKET ServerSocket, ClientSocket;// sever服务器用来接收，client用来存放收到的请求
SOCKADDR_IN ServerAddr, ClientAddr;// 用来存放IP地址和端口
int snaddr = sizeof(SOCKADDR_IN);
char SsendBuf[SBUF_SIZE];// 发送缓冲区
char SinputBuf[SBUF_SIZE];// 输入缓冲区
char SrecvBuf[SBUF_SIZE];//接收内容缓冲区
char hello[SBUF_SIZE] = "Sconnected! Here is a message that server want to say hello.\0";

int getPort() {
	cout << "Please input your port to connect with others which is in range(12000,16000).\n";
	cout << "If you wanna use default port, please input 0\n";
	u_short tempPort;
	cin >> tempPort;
	while (tempPort < 12000 || tempPort>16000) {
		if (tempPort == 0) {
			return ServerPort;
		}
		cout << "Invalid input. Please input in range(12000,16000) or 0.\n";
		cin >> tempPort;
	}
	return tempPort;
}

void getServerIP(char* p) {
	cout << "Please use 'ipconfig' to get your ip and input\n";
	char ip[16];
	cin.getline(ip, sizeof(ip));
	int index = 0;
	while (ip[index] != '\0') {
		p[index] = ip[index];
		index++;
	}
}

DWORD WINAPI recvMessage(LPVOID) {
	while (1) {
		if (!Sconnected)
			break;
		::memset(SrecvBuf, 0, SBUF_SIZE);
		int result = recv(ClientSocket, SrecvBuf, SBUF_SIZE, 0); //recv(SOCKET s, char* buf, int len, int flags) flags一般设置为0
		//if (result = 0)
		//	cout << "Connection closed" << endl;
		if (result < 0) {
			//cout << "recv failed:" << WSAGetLastError() << endl;
			//cout << "Connect failed. Error code is " << SOCKET_ERROR << ".\n";
		}
		else {
			if (isRecvQuit(SrecvBuf)) {
				cout << "Client has closed this session, bye.\n";
				Sconnected = false;
				//break;
			}
			else
				cout << "From Client, at " << SrecvBuf << endl;
		}
	}
	return 0;
}

int serverMain() {

	// 加载socket库
	WSADATA wsadata;
	if (WSAStartup(MAKEWORD(2, 2), &wsadata)) {//声明使用socket2.2版本 makeword是将两个byte型合并成一个word型，一个在高8位(b)，一个在低8位(a)
		//如果加载出错，暂停服务，输出出错信息
		cout << "载入socket失败\n";
		system("pause");
		return -1;
	}

	// 创建socket，参数顺序:IPv4，流式套接字，指定协议
	ServerSocket = socket(AF_INET, SOCK_STREAM, 0);

	// 初始化地址信息
	// inet_addr用来将IP地址转换成网络字节序
	ServerAddr.sin_family = AF_INET;
	getServerIP(ServerIP);
	inet_pton(AF_INET, ServerIP, &ServerAddr.sin_addr.S_un.S_addr);//将IP地址从字符串格式转换成网络地址格式，支持Ipv4和Ipv6.
	//inet_pton src:是个指针，指向保存IP地址字符串形式的字符串。dst:指向存放网络地址的结构体的首地址
	ServerPort = 12260; // ServerPort = getPort();
	//cin.get();
	ServerAddr.sin_port = htons(ServerPort);//将一个无符号短整型的主机数值转换为网络 字节顺序，即大尾顺序(big-endian)

	// 绑定socket和地址
	bind(ServerSocket, (SOCKADDR*)&ServerAddr, sizeof(SOCKADDR));

	cout << "Server start success. Your Server IP is: " << ServerIP << "\tYour port is: " << ServerPort << endl;
	// 开始循环监听
	while (1) {
		cout << "Witing for connection......\n";
		// 设置队列长度为5
		listen(ServerSocket, ListrnMax);

		// accept新来的请求，并用一个socket存放新创建的socket
		ClientSocket = accept(ServerSocket, (SOCKADDR*)&ClientAddr, &snaddr);//SOCKADDR_IN强制转换成SOCKADDR
		// 检查新创建的socket是否有效
		if (ClientSocket != INVALID_SOCKET) {
			// 连接成功
			cout << "客户端连接成功\n";
			Sconnected = true;
			// 告诉客户端连接成功
			// 获取当前时间
			time_t now = time(0);
			// 转换成字符串形式
			char* dt = ctime(&now);
			memset(SsendBuf, 0, SBUF_SIZE);
			strcat(SsendBuf, dt);
			strcat(SsendBuf, hello);//建立连接成功，发送hello信息
			send(ClientSocket, SsendBuf, SBUF_SIZE, 0);
			//delete[] dt;
			memset(SsendBuf, 0, SBUF_SIZE);
			HANDLE recv_thread = CreateThread(NULL, NULL, recvMessage, NULL, 0, NULL);
			if (recv_thread == 0) {
				cout << "Cteate thread failed. Execution stop.\n";
				return -1;
			}
			else {
				while (1) {
					if (!Sconnected)
						break;
					memset(SinputBuf, '\0', SBUF_SIZE);
					cin.getline(SinputBuf, sizeof(SinputBuf));

					// 获取当前时间
					time_t now = time(0);
					// 转换成字符串形式
					char* dt = ctime(&now);

					memset(SsendBuf, 0, SBUF_SIZE);
					strcat(SsendBuf, dt);
					strcat(SsendBuf, SinputBuf);

					send(ClientSocket, SsendBuf, sizeof(SsendBuf), 0);
					if (isQuit(SinputBuf, getLen(SinputBuf))) {
						cout << "You stopped this session.\n";
						Sconnected = false;
						CloseHandle(recv_thread);
						closesocket(ClientSocket);
						break;
					}
				}
			}
		}
		else {
			cout << "Connect failed.\n";
		}
	}

	closesocket(ServerSocket);
	closesocket(ClientSocket);

	WSACleanup();
	cout << "Server power off.\n";
	return 0;
}