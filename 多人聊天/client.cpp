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

// 全局变量声明
SOCKET RemoteSocket, LocalhostSocket;// sever服务器用来接收，client用来存放收到的请求
SOCKADDR_IN RemoteAddr, LocahhostAddr;// 用来存放IP地址和端口
int cnaddr = sizeof(SOCKADDR_IN);
char CsendBuf[CBUF_SIZE];// 发送缓冲区
char CinputBuf[CBUF_SIZE];// 输入内容缓冲区
char CrecvBuf[CBUF_SIZE];//接收内容缓冲区
char userName[CBUF_SIZE];// 用户的名字
//char ErrorMessage[] = "Input destination user error, please check your input.\nPlease use 'getUsrList' to get users.\0";

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

void showList(char* usrlist) {//打印在线用户名
	int totalLen = getLen(usrlist);
	int posi = 1;
	int number = 1;
	while (posi<totalLen) {
		int len = int(usrlist[posi]);
		posi += 1;
		int lastPost = posi + len;
		cout << number << ". ";
		number += 1;
		for (; posi < lastPost; posi+=1)
			cout << usrlist[posi];
		cout << endl;
	}
}

DWORD WINAPI recvMessagec(LPVOID) {
	while (1) {
		::memset(CrecvBuf, 0, CBUF_SIZE);
		if (recv(LocalhostSocket, CrecvBuf, CBUF_SIZE, 0) < 0) {
			cout << "Connect failed. Error code is " << SOCKET_ERROR << ".\n";
			system("pause");
		}
		else if(isHello(CrecvBuf))//用户连接成功 Connected!
			cout << CrecvBuf << endl;
		else if(CrecvBuf[0] == 'I' && CrecvBuf[1] == 'n' && CrecvBuf[2] == 'p')//input destination error
			cout << CrecvBuf << endl;
		else if(CrecvBuf[0] == 'u')//getUsrList
			showList(CrecvBuf);
		else if (isServerQuit(CrecvBuf)){//server quit
			cout << "Server has stopped this chat room, bye.\n";
			break;
		}
		else if (isClientQuit(CrecvBuf)){//有人下线了
			int len = int(CrecvBuf[1]);
			char* quitUsr = new char[len + 1];
			for (int i = 0; i < len; i++)
				quitUsr[i] = CrecvBuf[i + 2];
			quitUsr[len] = '\0';
			cout << "User: " << quitUsr << " has left from sesstion.\n";
		}
		else {//收到了某人发来的消息，打印来源用户、时间、消息
			char timeString[26];//时间
			int index = 0;
			for (; index < 25; index++)
				timeString[index] = CrecvBuf[index];
			timeString[index] = '\0';
			int len = int(CrecvBuf[index]);
			index += 1;
			char* nameString = new char[len + 1];//来源用户
			for (; index < 26 + len; index++)
				nameString[index - 26] = CrecvBuf[index];
			nameString[index - 26] = '\0';
			while (CrecvBuf[index] == '\0')
				index++;
			cout << "From user: " << nameString << "\tTime: " << timeString;
			while (CrecvBuf[index] != '\0') {//打印消息
				cout << CrecvBuf[index];
				index++;
			}
			cout << endl;
		}
	}
	return 0;
}

bool ismyQuit(char* a) {//本人下线
	if (a[0] == 'q' && a[1] == 'u' && a[2] == 'i' && a[3] == 't')
		return true;
	else
		return false;
}

bool isClientQuit(char* a) {//某人下线
	if (a[0] == 'q')
		return true;
	else
		return false;
}

bool isHello(char* a) {//连接成功
	if (a[25]=='C' && a[26]=='o' && a[27] == 'n' && a[28] == 'n' )
		return true;
	else
		return false;
}

bool isServerQuit(char* a) {
	int len = 0;
	while (a[len] != '\0')
		len++;
	if (len == 30 && int(a[25])==40 && a[26] == 'q' && a[27] == 'u' && a[28] == 'i' && a[29] == 't')
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
	inet_pton(AF_INET, DstIP, &RemoteAddr.sin_addr.S_un.S_addr);

	// 获取用户名称以及长度
	cout << "Please input your name which will be displayed in our chat room.\n";
	char inputName[CBUF_SIZE];
	cin.getline(inputName, CBUF_SIZE);
	userName[0] = char(getLen(inputName));//userName[0]为长度信息，强制转换为char
	int indexOfInput = 0;
	while (inputName[indexOfInput] != '\0') {
		userName[indexOfInput + 1] = inputName[indexOfInput];
		indexOfInput++;
	}
	userName[indexOfInput + 2] = '\0';
	//封装用户信息
	char* sendName = new char[int(userName[0]) + 4];//类型强制转换
	sendName[0] = 'n';
	sendName[1] = userName[0];
	int l = userName[0];
	for (int ind = 0; ind < l; ind++)
		sendName[2 + ind] = userName[1 + ind];
	sendName[l + 3] = '\0';

	// 连接远程服务器
	if (connect(LocalhostSocket, (SOCKADDR*)&RemoteAddr, sizeof(RemoteAddr)) != SOCKET_ERROR) {
		// 连接成功
		// 发送一条自己的信息（名字）
		send(LocalhostSocket, sendName, int(userName[0] + 2), 0);
		// 接收信息
		HANDLE recv_thread = CreateThread(NULL, NULL, recvMessagec, NULL, 0, NULL);

		while (1) {//本线程用于发送消息
			memset(CinputBuf, 0, CBUF_SIZE);
			char input[CBUF_SIZE];
			cin.getline(input, CBUF_SIZE);

			if (!strcmp(input, "getUsrList\0")) {//输入的是getUsrList
				send(LocalhostSocket, input, 11, 0);
			}
			else {//输入的是quit
				if (ismyQuit(input)) {
					char tempQuit[CBUF_SIZE];
					memset(tempQuit, 0, CBUF_SIZE);
					tempQuit[0] = 'q';
					strcat(tempQuit, userName);
					strcat(tempQuit, "quit\0");
					send(LocalhostSocket, tempQuit, sizeof(tempQuit), 0);
					cout << "Thanks for using. Bye.\n";
					closesocket(LocalhostSocket);//关闭socket
					closesocket(RemoteSocket);
					WSACleanup();
					system("pause");
					return 0;
				}
				else {
					int i = 0;
					while (input[i] != ':'&&input[i]!='\0')
						i++;
					
					char* dstUser = new char[i + 2];//获取目的地name
					dstUser[0] = char(i);
					for (int j = 1; j <= i; j++)
						dstUser[j] = input[j - 1];
					dstUser[i + 1] = '\0';

					i += 1;
					int j = i;
					while (input[i] != '\0') {
						CinputBuf[i - j] = input[i];//获取除了name以外的真正的消息
						i += 1;
					}
					CinputBuf[i - j] = '\0';

					// 获取当前时间
					time_t now = time(0);
					// 转换成字符串形式
					char* dt = ctime(&now);

					memset(CsendBuf, 0, CBUF_SIZE);
					// 加入时间信息
					strcat(CsendBuf, dt);
					// 加入目标名字信息
					strcat(CsendBuf, dstUser);
					// 加入自己名字信息
					strcat(CsendBuf, userName);
					// 加入输入的内容
					strcat(CsendBuf, CinputBuf);
					// 发送消息
					send(LocalhostSocket, CsendBuf, CBUF_SIZE, 0);

					memset(CsendBuf, '\0', CBUF_SIZE);
					memset(CinputBuf, '\0', CBUF_SIZE);
				}
			}
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