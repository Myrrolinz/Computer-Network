#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include"server.h"
#include"externTools.h"
#pragma comment(lib, "ws2_32.lib")
#include<winsock2.h>
#include<WS2tcpip.h>
#include<iostream>
#include<fstream>
#include<sstream>

using namespace std;

// 一些后面会使用到的全局变量
static SOCKET serverSocket, clientSocket; // 服务器的socket
static SOCKADDR_IN serverAddr, clientAddr; // 用这个结构来存放相关的IP地址和端口号
static int sockaddrLen = sizeof(serverAddr);

static const int MAX_WAIT_TIME = 1000; // 设置最大等待时间

// 三次握手的时候对应的flag
const unsigned char SHAKE1 = 0x01;
const unsigned char SHAKE2 = 0x02;
const unsigned char SHAKE3 = 0x03;
// 四次挥手的时候的flag
const unsigned char WAVE1 = 0x04;
const unsigned char WAVE2 = 0x05;
// 传输时使用的端口
const int CLIENT_PORT = 12880;
const int SERVER_PORT = 12660;

static char ServerIP[16] = "127.0.0.1"; // 服务器IP地址
static char ClientIP[16]; // client的IP

static bool shakeHand() {
	// 握手
	//Package p;
	//Package sendP;
	char recvShakeBuf[4];
	while (1) {
		memset(recvShakeBuf, 0, 4);
		// 停等机制接收数据包
		while (recvfrom(serverSocket, recvShakeBuf, 4, 0, (sockaddr*)&clientAddr, &sockaddrLen) == SOCKET_ERROR) {

		}

		// 检查接收到的数据包校验和是否正确
		// 先对收到的数据进行转换
		//p = *((Package*)recvBuf);
		// 输出信息
		cout << "收到建立连接的请求，数据包为：\n";
		// packageOutput(p);
		cout << "SYN: " << int(recvShakeBuf[1]) << "\tFIN: " << int(recvShakeBuf[2]) << "\tSHAKE: " << int(recvShakeBuf[3]) << "\tcheckSum: " << int(recvShakeBuf[0]) << endl;
		// 第一次
		if (newGetChecksum(recvShakeBuf,4)==0 && recvShakeBuf[1] == 0x01 && recvShakeBuf[2] == 0x00 && recvShakeBuf[3] == SHAKE1) {
			// 收到握手包正确
			cout << "握手包正确\n";
			// 创建第二次握手包
			char secondShake[4];
			secondShake[1] = 0x01;
			secondShake[2] = 0x00;
			secondShake[3] = SHAKE2;
			secondShake[0] = newGetChecksum(secondShake+1, 4-1);
			// 发送第二次握手包
			sendto(serverSocket, secondShake, 4, 0, (SOCKADDR*)&clientAddr, sockaddrLen);
			// 输出内容
			cout << "SYN: " << int(secondShake[1]) << "\tFIN: " << int(secondShake[2]) << "\tSHAKE: " << int(secondShake[3]) << "\tcheckSum: " << int(secondShake[0]) << endl;

			int beginTime = clock();

			// 清空刚刚接收的包，等待第三次握手
			memset(recvShakeBuf, 0, 4);

			while (recvfrom(serverSocket, recvShakeBuf, 4, 0, (sockaddr*)&clientAddr, &sockaddrLen) == SOCKET_ERROR) {
				if (clock() - beginTime > MAX_WAIT_TIME) {
					cout << "超过最长等待时间，重传:";
					sendto(serverSocket, secondShake, 4, 0, (SOCKADDR*)&clientAddr, sockaddrLen);
					beginTime = clock();
					cout << "SYN: " << int(secondShake[1]) << "\tFIN: " << int(secondShake[2]) << "\tSHAKE: " << int(secondShake[3]) << "\tcheckSum: " << int(secondShake[0]) << endl;
				}
			}
			

			cout << "收到数据包为：\n";
			cout << "SYN: " << int(recvShakeBuf[1]) << "\tFIN: " << int(recvShakeBuf[2]) << "\tSHAKE: " << int(recvShakeBuf[3]) << "\tcheckSum: " << int(recvShakeBuf[0]) << endl;

			if (newGetChecksum(recvShakeBuf, 4)==0 && recvShakeBuf[1] == 0x01 && recvShakeBuf[2] == 0x00 && recvShakeBuf[3] == SHAKE3) {
				// 收到握手包正确
				cout << "\n连接建立成功\n\n";
				return true;
			}
		}
		else {
			cout << "收到的数据包有误，重新等待连接\n";
			continue;
		}

	}
}

static void waveHand() {
	// 挥手，挥两次
	char recvWaveBuf[4];
	while (true) {
		while (recvfrom(serverSocket, recvWaveBuf, 4, 0, (sockaddr*)&clientAddr, &sockaddrLen) == SOCKET_ERROR) {

		}

		cout << "收到断开连接的请求，数据包为：\n";
		cout << "SYN: " << int(recvWaveBuf[1]) << "\tFIN: " << int(recvWaveBuf[2]) << "\tSHAKE: " << int(recvWaveBuf[3]) << "\tcheckSum: " << int(recvWaveBuf[0]) << endl;

		// 第一次
		if (newGetChecksum(recvWaveBuf, 4)==0 && recvWaveBuf[1] == 0x00 && recvWaveBuf[2] == 0x01 && recvWaveBuf[3] == WAVE1) {
			// 收到挥手包正确
			cout << "挥手包正确\n";
			// 创建第二次挥手包
			char secondShake[4];
			secondShake[1] = 0x00;
			secondShake[2] = 0x01;
			secondShake[3] = WAVE2;
			secondShake[0] = newGetChecksum(secondShake+1, 4-1);
			// 发送第二次挥手包
			sendto(serverSocket, secondShake, 4, 0, (SOCKADDR*)&clientAddr, sockaddrLen);
			// 输出内容
			cout << "发送给客户端的第二次挥手：\n";
			cout << "SYN: " << int(secondShake[1]) << "\tFIN: " << int(secondShake[2]) << "\tSHAKE: " << int(secondShake[3]) << "\tcheckSum: " << int(secondShake[0]) << endl;

			memset(secondShake, 0, 4);
			memset(recvWaveBuf, 0, 4);
			break;
		}
	}
}

static void recvFile() {
	// 接收文件
	// 初始ack
	char ack = 0x00;
	// 创建缓冲区
	char recvBuf[1024];
	memset(recvBuf, 0, 1024);
	// 创建发送使用的缓冲区
	char sendBuf[2];
	memset(sendBuf, 0, 2);
	// 记录名字和名字长度
	unsigned short int nameLen = 0;
	char* fileName;
	// 停等，等名字
	while (true) {
		// 接收到的第一个包是名字
		while (recvfrom(serverSocket, recvBuf, 1024, 0, (sockaddr*)&clientAddr, &sockaddrLen) == SOCKET_ERROR) {

		}
		cout << "From C\t收到的数据包为;\n";
		cout << "seq: " << int(recvBuf[1]) << "\tlen高: " << int(recvBuf[2]) << "\tlen低: " << int(recvBuf[3]) << "\tisLast: " << int(recvBuf[4]) << "\tchecksum: " << int(recvBuf[0]) << endl;

		if (newGetChecksum(recvBuf, 1024)==0 && recvBuf[1] == ack) {
			// 记录名称
			// 计算名字的长度
			unsigned short int temp = unsigned short int(recvBuf[2]);
			nameLen = (temp << 8) + unsigned short int(recvBuf[3]);
			fileName = new char[nameLen];
			for (int i = 0; i < nameLen; i++)
				fileName[i] = recvBuf[i + 5];

			// 没有损坏，返回对应ack，然后ack翻转
			sendBuf[1] = ack;
			sendBuf[0] = newGetChecksum(sendBuf+1, 1);
			sendto(serverSocket, sendBuf, 2, 0, (SOCKADDR*)&clientAddr, sockaddrLen);
			cout << "TO C\t正确返回数据包\n";
			cout << "checksum: " << int(sendBuf[0]) << "\tack: " << int(sendBuf[1]) << endl;

			// ack翻转
			if (ack == 0x00)
				ack = 0x01;
			else
				ack = 0x00;
			break;
		}
		else {
			// 有损坏，返回错误的ack
			// 返回ack1
			sendBuf[1] = 0x02;
			sendBuf[0] = newGetChecksum(sendBuf+1, 2-1);
			sendto(serverSocket, sendBuf, 2, 0, (SOCKADDR*)&clientAddr, sockaddrLen);
			cout << "TO C\t接收到的数据包有损坏，返回错误ack\n";
			cout << "checksum: " << int(sendBuf[0]) << "\tack: " << int(sendBuf[1]) << endl;
			continue;
		}
	}

	// 对名字进行处理
	char filePath[] = "C:\\Users\\11955\\Desktop\\test\\";
	char* fileNameN = new char[28 + nameLen + 1];
	for (int i = 0; i < 28 + nameLen; i++) {
		if (i < 28)
			fileNameN[i] = filePath[i];
		else {
			fileNameN[i] = fileName[i - 28];
		}
	}
	fileNameN[28 + nameLen] = '\0';

	//cout << fileNameN << endl;
	ofstream toWriteFile;
	toWriteFile.open(fileNameN, ios::binary);
	if (!toWriteFile) {
		cout << "文件创建失败\n";
	}

	int beginTime = clock();

	// 接下来停等接收正式的数据包
	int currentNum = 0;
	while (true) {
		while (recvfrom(serverSocket, recvBuf, 1024, 0, (sockaddr*)&clientAddr, &sockaddrLen) == SOCKET_ERROR) {
			if (clock() - beginTime > MAX_WAIT_TIME) {
				cout << "超过最大等待时间，重传：";
				sendBuf[1] =(~(ack) & 0x01);
				sendBuf[0] = newGetChecksum(sendBuf + 1, 1);
				sendto(serverSocket, sendBuf, 2, 0, (SOCKADDR*)&clientAddr, sockaddrLen);
				cout << "checksum: " << int(sendBuf[0]) << "\tack: " << int(sendBuf[1]) << endl;
				beginTime = clock();
			}
		}
		cout << "From C\t收到的数据包为;\n";
		cout << "seq: " << int(recvBuf[1]) << "\tlen高: " << int(recvBuf[2]) << "\tlen低: " << int(recvBuf[3]) << "\tisLast: " << int(recvBuf[4])<<"\tchecksum: " << int(recvBuf[0]) <<endl;
			
		if (newGetChecksum(recvBuf, 1024)==0 && recvBuf[1] == ack) {
			// 将内容输入到文件中
			unsigned short int temp = unsigned short int(recvBuf[2]);
			//cout << "temp高位：" << temp << endl;
			temp = (temp << 8) + (unsigned short int(recvBuf[3]) & 0x00FF);
			//cout << "temp<<8" << (unsigned short int(recvBuf[2]) << 8) << endl;
			//cout << "temp低位: " << unsigned short int(recvBuf[3]) << endl;
			cout << "temp长度：" << temp << endl << endl;
			for (int i = 0; i < temp; i++) {
				toWriteFile << recvBuf[5 + i];
				//cout<< recvBuf[5 + i];
			}

			currentNum += 1;

			// 没有损坏，返回对应ack，然后ack翻转
			sendBuf[1] = ack;
			sendBuf[0] = newGetChecksum(sendBuf+1, 1);
			sendto(serverSocket, sendBuf, 2, 0, (SOCKADDR*)&clientAddr, sockaddrLen);
			beginTime = clock();
			cout << "TO C\t正确返回数据包\n";
			cout << "checksum: " << int(sendBuf[0]) << "\tack: " << int(sendBuf[1]) << endl;

			// ack翻转
			if (ack == 0x00)
				ack = 0x01;
			else
				ack = 0x00;

			/* cout << "\n判断是不是最后一个\n";
			cout << (int(recvBuf[4]) == 1) << endl << endl;*/
			// 确认当前收到的是最后一个
			if (int(recvBuf[4]) == 1)
				break;
		}
		else {
			// 有损坏，返回错误的ack
			// 返回ack1
			sendBuf[1] = 0x02;
			sendBuf[0] = newGetChecksum(sendBuf+1, 2-1);
			sendto(serverSocket, sendBuf, 2, 0, (SOCKADDR*)&clientAddr, sockaddrLen);
			beginTime = clock();
			cout << "TO C\t接收到的数据包有损坏，返回错误ack\n";
			cout << "check: " << int(sendBuf[0]) << "\tack: " << int(sendBuf[1]) << endl;
			continue;
		}
	}

	// 文件接收完毕，关闭文件
	toWriteFile.close();

	cout << "文件保存成功\n";
}

int serverMain() {
	// 加载socket库
	WSADATA wsadata;
	if (WSAStartup(MAKEWORD(2, 2), &wsadata)) {
		//如果加载出错，暂停服务，输出出错信息
		cout << "载入socket失败\n";
		return -1;
	}

	// 初始化信息
	// server
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(SERVER_PORT);
	serverAddr.sin_addr.s_addr = inet_addr(ServerIP);
	// client
	clientAddr.sin_family = AF_INET;
	clientAddr.sin_port = htons(CLIENT_PORT);

	serverSocket = socket(AF_INET, SOCK_DGRAM, 0);
	if (serverSocket == INVALID_SOCKET) {
		cout << "套接字创建失败\n";
		WSACleanup();
		return -1;
	}

	//setsockopt(serverSocket, SOL_SOCKET, SO_RCVTIMEO, (char*)&TIMEOUT, sizeof(TIMEOUT));
	// 创建自己的，自己的IP就直接固定成127了
	// cout << "请输入服务器（本机）的IP：\n";
	// getIP(ServerIP);
	inet_pton(AF_INET, ServerIP, &serverAddr.sin_addr.s_addr);

	// 获取client的IP
	cout << "获取client的IP\n";
	getIP(ClientIP);
	inet_pton(AF_INET, ClientIP, &clientAddr.sin_addr.s_addr);

	setsockopt(serverSocket, SOL_SOCKET, SO_RCVTIMEO, (char*)&MAX_WAIT_TIME, sizeof(MAX_WAIT_TIME));

	// 绑定socket和地址
	if (bind(serverSocket, (SOCKADDR*)&serverAddr, sizeof(SOCKADDR)) == SOCKET_ERROR) {
		cout << "bind失败\n";
		WSACleanup();
		return -1;
	}

	cout << "服务器启动成功，等待连接……\n";

	// 等待连接请求
	// 三次握手
	shakeHand();

	// 接收文件
	recvFile();

	// 数据传输完毕，等待下一步操作
	cout << "数据传输完成，等待客户端退出……\n";
	// 两次挥手，结束
	waveHand();
	cout << "挥手结束，BYE\n";

	return 0;
}