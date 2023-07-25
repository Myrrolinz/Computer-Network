#include"client.h"
#include"externTools.h"
#pragma comment(lib, "ws2_32.lib")
#include<winsock2.h>
#include<WS2tcpip.h>
#include<iostream>
#include<fstream>
#include<stdio.h>
#include <io.h>
#include<mutex>
#include<time.h>

using namespace std;

// 全局变量
static SOCKET sendSocket, recvSocket; // 前面的socket用来发送，后面的用来接收
static SOCKADDR_IN serverAddr, clientAddr;
static int socketAddrLen = sizeof(serverAddr);
static const unsigned int MAX_WAIT_TIME = 100000; // 设置最大的等待时间
static string path = "./test/*";

static double sendFileTime; // 设置一个全局变量，用来计算吞吐率，这里是时间
static double Throughput; // 吞吐率

static int WINDOW_SIZE = 64; // 设置最大的窗口大小

// 三次握手的时候对应的flag
const unsigned char SHAKE1 = 0x01;
const unsigned char SHAKE2 = 0x02;
const unsigned char SHAKE3 = 0x03;
// 四次挥手的时候的flag
const unsigned char WAVE1 = 0x04;
const unsigned char WAVE2 = 0x05;
// 传输时使用的端口
const int CLIENT_PORT = 12880;
const int SERVER_PORT = 12770;

/* 
要发送的数据包的样子
data[1024]
check, seq[3], len[2],isLast, data[1017]
0      1-3     4-5    6       7-1023
发送的第一个包是名称的长度

接收方返回的数据包的样子
ans[4]
check, ack[3]
0      1-3  

协议使用：GBN
SR
*/
static int dataLen = 1017; // 设置每个数据包中的最大数据长度

/// <summary>
/// 停等机制向服务器握手
/// </summary>
void connet2Server() {
	// 三次握手，建立连接
	// 创建一个要发送的数据包，并初始化为空
	
	// 数据包：check, SYN，FIN，SHAKE/WAVE
	char shakeChar[4];
	shakeChar[1] = 0x01;
	shakeChar[2] = 0x00;
	shakeChar[3] = SHAKE1;
	shakeChar[0] = newGetChecksum(shakeChar + 1, 3);

	// 发送shake1
	sendto(sendSocket, shakeChar, 4, 0, (SOCKADDR*)&serverAddr, socketAddrLen);
	cout<< "开始建立连接，请求建立连接的数据包为（第一次握手）：\n";
	cout << "SYN: " << int(shakeChar[1]) << "\tFIN: " << int(shakeChar[2]) << "\tSHAKE: " << int(shakeChar[3]) << "\tcheckSum: " << int(shakeChar[0]) << endl;

	int beginTime = clock(); // 开始计时
	char recvShakeBuf[4];
	memset(recvShakeBuf, 0, 4);

	while((recvfrom(sendSocket, recvShakeBuf, 4, 0, (SOCKADDR*)&serverAddr, &socketAddrLen))==SOCKET_ERROR){
		if (clock() - beginTime > MAX_WAIT_TIME) {
			cout << "超过最长等待时间，重传:";
			sendto(sendSocket, shakeChar, 4, 0, (SOCKADDR*)&serverAddr, socketAddrLen);
			beginTime = clock();
			cout << "SYN: " << int(shakeChar[1]) << "\tFIN: " << int(shakeChar[2]) << "\tSHAKE: " << int(shakeChar[3]) << "\tcheckSum: " << int(shakeChar[0]) << endl;
		}
	}
	// 收到数据包
	cout << "收到数据包为：\n";
	cout << "SYN: " << int(recvShakeBuf[1]) << "\tFIN: " << int(recvShakeBuf[2]) << "\tSHAKE: " << int(recvShakeBuf[3]) << "\tcheckSum: " << int(recvShakeBuf[0]) << endl;

	if (newGetChecksum(recvShakeBuf, 4)==0 && recvShakeBuf[1] == 0x01 && recvShakeBuf[2] == 0x00 && recvShakeBuf[3] == SHAKE2) {
		// 收到握手包正确
		// 发送第三次握手包的内容
		memset(shakeChar, 0, 4);
		shakeChar[1] = 0x01;
		shakeChar[2] = 0x00;
		shakeChar[3] = SHAKE3;
		shakeChar[0] = newGetChecksum(shakeChar+1, 3);
		sendto(sendSocket, shakeChar, 4, 0, (SOCKADDR*)&serverAddr, socketAddrLen);
		cout << "发送第三次握手数据包：\n";
		cout << "SYN: " << int(shakeChar[1]) << "\tFIN: " << int(shakeChar[2]) << "\tSHAKE: " << int(shakeChar[3]) << "\tcheckSum: " << int(shakeChar[0]) << endl;
		cout << "\n连接建立成功\n\n";
	}
}

/// <summary>
/// 向服务器传输文件使用的函数，滑动窗口
/// </summary>
/// <param name="fileName">要传输的文件名</param>
/// <param name="fileNameLen">文件名的长度</param>
static void sendFile2Server(char* fileName, unsigned short int fileNameLen) {
	// 对输入的名字进行处理
	char filePath[] = "./test/";
	char* fileNameN = new char[7 + fileNameLen + 1];
	for (int i = 0; i < 7 + fileNameLen + 1; i++) {
		if (i < 7)
			fileNameN[i] = filePath[i];
		else
			fileNameN[i] = fileName[i - 7];
	}
	fileNameN[7 + fileNameLen + 1] = '\0';

	// 读取文件（以二进制形式读入）
	ifstream toSendFile(fileNameN, ifstream::in | ios::binary);

	// 计算文件的长度
	toSendFile.seekg(0, toSendFile.end);
	unsigned int fileLen = toSendFile.tellg();
	toSendFile.seekg(0, toSendFile.beg);
	cout << "计算得到要发送的文件长度为：" << fileLen << endl;

	// 计算需要封装成多少个包
	unsigned int totalNum = fileLen / dataLen;
	if (fileLen % dataLen > 0)
		totalNum += 1;
	// 再加一个表示文件名称的包
	totalNum += 1;
	cout << "一共需要封装成" << totalNum << "个包\n";

	// 计时器，当为-1的时候表示计时器停止。
	int timer = -1;

	// 创建临时缓冲区
	char* tempBuf = new char[fileLen];
	// 将文件装载到缓冲区
	toSendFile.read(tempBuf, fileLen);

	// 窗口的最左边
	unsigned int leftWindow = 0; 
	// 下一个要发送的
	unsigned int curr = 0;

	// 开始封装数据包
	// 将文件全部封装，以后就直接发，而不是每次发之前封装
	char** sendPackage = new char*[totalNum];
	for (int tmp = 0; tmp < totalNum; tmp++) {
		*(sendPackage + tmp) = new char[1024];
		memset(sendPackage[tmp], 0, 1024);
	}

	// 设置文件名包体的内容
	sendPackage[0][3] = 0x00; 
	for (unsigned short int i = 0; i < fileNameLen; i++) {
		sendPackage[0][i + 7] = fileName[i];
	}
	sendPackage[0][5] = unsigned char(fileNameLen & 0x00FF); //len高
	sendPackage[0][4] = unsigned char((fileNameLen >> 8) & 0x00FF); //len低
	sendPackage[0][0] = newGetChecksum(sendPackage[0]+1, 1024-1); //check

	// 开始对文件整体进行打包
	for (int tmp = 1; tmp < totalNum; tmp++) {
		// 设置标志位 seq
		sendPackage[tmp][3] = tmp & 0xFF;
		sendPackage[tmp][2] = (tmp >> 8) & 0xFF;
		sendPackage[tmp][1] = (tmp >> 16) & 0xFF;

		// 设置isLast位
		if (tmp == totalNum - 1)
			sendPackage[tmp][6] = 0x01;
		else
			sendPackage[tmp][6] = 0x00;

		// 装载下一部分文件
		unsigned short int actualLen = 0;
		for (int tempPoint = 0; (tempPoint < dataLen) && ((tmp - 1) * dataLen + tempPoint) < fileLen; tempPoint++) {
			sendPackage[tmp][7 + tempPoint] = tempBuf[(tmp - 1) * dataLen + tempPoint];
			actualLen += 1;
		}

		// 设置长度
		sendPackage[tmp][5] = unsigned char(actualLen & 0x00FF);
		sendPackage[tmp][4] = unsigned char((actualLen >> 8) & 0x00FF);

		// 计算校验和
		sendPackage[tmp][0] = newGetChecksum(sendPackage[tmp] + 1, 1024 - 1);
	}
	// ---------至此文件全部打包结束

	// 创建一个sr缓冲区，并初始化。该数组标识是否acked
	bool* srBuf = new bool[totalNum] ;
	for (int i = 0; i < totalNum; i++)
		srBuf[i] = false;

	// 从开始发送第一个数据包这里开始计算需要统计的时间
	sendFileTime = clock();

	while (leftWindow != totalNum) {
		// for循环发送窗口那么多个数据包，窗口大小为WINDOW_SIZE
		// 对于当前需要发送的数据包的计算公式: curr = leftWindow + willSendInWindow
		for (; (curr < leftWindow + WINDOW_SIZE) && (curr < totalNum); curr++) {
			// 发送数据包
			sendto(sendSocket, (char*)sendPackage[curr], 1024, 0, (SOCKADDR*)&serverAddr, socketAddrLen);
			if (curr == leftWindow)
				// 计时器开始计时
				timer = clock();
			// 输出相关信息
			cout << "当前发送数据包为：\nseq: " << curr << "\nlen高: " << int(sendPackage[curr][4]) << "\tlen低: " << int(sendPackage[curr][5])
				<< "\tchecksum: " << int(sendPackage[curr][0]) << endl << "剩余窗口大小：" << (leftWindow + WINDOW_SIZE - curr) << endl;
		}

		// 创建一个用来存放回复的缓冲区，长度为4
		char recvBuf[4];
		// 循环接收数据包，并更新窗口左端
		while (recvfrom(sendSocket, recvBuf, 4, 0, (SOCKADDR*)&serverAddr, &socketAddrLen) == SOCKET_ERROR) {
			// 超时重传
			if ((timer != -1) && (clock() - timer > MAX_WAIT_TIME)) {
				cout << "==================超过最长等待时间，重传:\n";
				int tempBase = leftWindow;
				for (; tempBase < curr; tempBase++) {
					if (!srBuf[tempBase]) { //仅重传未确认的数据报
						// 发送数据包
						sendto(sendSocket, (char*)sendPackage[tempBase], 1024, 0, (SOCKADDR*)&serverAddr, socketAddrLen);
						// 输出相关信息
						cout << "当前发送数据包为：\nseq: " << tempBase << "\nlen高: " << int(sendPackage[tempBase][4]) << "\tlen低: " << int(sendPackage[tempBase][5])
							<< "\tchecksum: " << int(sendPackage[tempBase][0]) << endl << "剩余需要重传的包个数为：" << (curr - tempBase - 1) << endl << endl;
					}
					// 重新开始计时
					timer = clock();
				}
			}
		}
		// 表示收到了有效的数据包，开始检查数据包的正确性

		// 计算回来的序号
		unsigned int answerNum;
		answerNum = unsigned short int(recvBuf[1]) & 0xFF;
		answerNum = (answerNum << 8) + (unsigned short int(recvBuf[2]) & 0xFF);
		answerNum = (answerNum << 8) + (unsigned short int(recvBuf[3]) & 0xFF);
		cout << "FROM S:\nchecksum: " << int(recvBuf[0]) << "\tack: " << answerNum << endl;

		// 检查checksum
		if ((newGetChecksum(recvBuf, 4) == 0) && (answerNum >= leftWindow)) {
			if (answerNum == curr-1) {
				// bool数组更新，设置为true，表示已经传到了
				for (int i = leftWindow; i < answerNum; i++)
					srBuf[i] = true;
				// 窗口的左边移动
				leftWindow = answerNum + 1;

				// 判断是否需要更新timer
				if (leftWindow == curr)
					timer = -1;
				else
					timer = clock();
			}
			else if (answerNum < curr-1) {
				// bool数组更新，设置为true，表示已经传到了
				for (int i = leftWindow; i < answerNum; i++)
					srBuf[i] = true;
				// 窗口的左边移动
				leftWindow = answerNum + 1;
				// 重传leftWindow
				//sendto(sendSocket, (char*)sendPackage[leftWindow], 1024, 0, (SOCKADDR*)&serverAddr, socketAddrLen);
				//// 输出相关信息
				//cout << "【重传leftwindow】\t当前发送数据包为：\nseq: " << leftWindow << "\nlen高: " << int(sendPackage[leftWindow][4]) << "\tlen低: " << int(sendPackage[leftWindow][5])
				//	<< "\tchecksum: " << int(sendPackage[leftWindow][0]) << endl << "\n剩余需要重传的包个数为：" << (curr - leftWindow - 1) << endl << endl;
				//// 重新开始计时
				//timer = clock();
			}
		}
		else {
			// 要么是收到的包错了，要么是ack的序号是之前的，此时就需要把现在这些全都重传
			cout << "==================收到的数据包有误，重传:\n";
			//int tempBase = leftWindow;
			int tempBase = answerNum + 1;
			for (; tempBase < curr; tempBase++) {
				if (!srBuf[tempBase]) {
					// 发送数据包
					sendto(sendSocket, (char*)sendPackage[tempBase], 1024, 0, (SOCKADDR*)&serverAddr, socketAddrLen);
					// 输出相关信息
					cout << "当前发送数据包为：\nseq: " << tempBase << "\nlen高: " << int(sendPackage[tempBase][4]) << "\tlen低: " << int(sendPackage[tempBase][5])
						<< "\tchecksum: " << int(sendPackage[tempBase][0]) << endl << "\n剩余需要重传的包个数为：" << (curr - tempBase - 1) << endl << endl;
				}
			}
			//重新开始计时
			timer = clock();
		}
	}

	sendFileTime = clock() - sendFileTime;
	// 计算成秒
	sendFileTime = sendFileTime / CLOCKS_PER_SEC;
	cout << "本次传输所用时间为：" << sendFileTime << " s." << endl;
	// 计算吞吐率
	Throughput = fileLen / sendFileTime;
}

/// <summary>
/// 挥手使用的函数，这里使用的是停等机制
/// </summary>
static void sayGoodBye2Server() {
	// 两次挥手，断开连接

	// 数据包：check, SYN，FIN，SHAKE/WAVE
	char shakeChar[4];
	shakeChar[1] = 0x00;
	shakeChar[2] = 0x01;
	shakeChar[3] = WAVE1;
	shakeChar[0] = newGetChecksum(shakeChar + 1, 4 - 1);
	// 发送WAVE1
	sendto(sendSocket, shakeChar, 4, 0, (SOCKADDR*)&serverAddr, socketAddrLen);
	cout << "断开连接，发送数据包为（第一次挥手）：\n";
	cout << "SYN: " << int(shakeChar[1]) << "\tFIN: " << int(shakeChar[2]) << "\tSHAKE: " << int(shakeChar[3]) << "\tcheckSum: " << int(shakeChar[0]) << endl;
	
	// 开始计时
	int beginTime = clock();

	// 停等，等服务器的挥手
	char recvWaveBuf[4];
	while (true) {
		while ((recvfrom(sendSocket, recvWaveBuf, 4, 0, (SOCKADDR*)&serverAddr, &socketAddrLen))==SOCKET_ERROR) {
			if (clock() - beginTime > MAX_WAIT_TIME) {
				cout << "超过最长等待时间，重传:";
				sendto(sendSocket, shakeChar, 4, 0, (SOCKADDR*)&serverAddr, socketAddrLen);
				beginTime = clock();
				cout << "SYN: " << int(shakeChar[1]) << "\tFIN: " << int(shakeChar[2]) << "\tSHAKE: " << int(shakeChar[3]) << "\tcheckSum: " << int(shakeChar[0]) << endl;
			}
		}
		if (newGetChecksum(recvWaveBuf, 4)==0 && recvWaveBuf[1] == 0x00 && recvWaveBuf[2] == 0x01 && recvWaveBuf[3] == WAVE2) {
			// 收到握手包正确
			cout << "收到来自服务器的第二次挥手：\n";
			cout << "SYN: " << int(recvWaveBuf[1]) << "\tFIN: " << int(recvWaveBuf[2]) << "\tSHAKE: " << int(recvWaveBuf[3]) << "\tcheckSum: " << int(recvWaveBuf[0]) << endl;
			memset(recvWaveBuf, 0, 4);
			memset(shakeChar, 0, 4);
			break;
		}
		else {
			cout << "服务器挥手失败，重传挥手数据包\n";
			sendto(sendSocket, shakeChar, 4, 0, (SOCKADDR*)&serverAddr, socketAddrLen);
			cout << "断开连接，发送数据包为（第一次挥手）：\n";
			cout << "SYN: " << int(shakeChar[1]) << "\tFIN: " << int(shakeChar[2]) << "\tSHAKE: " << int(shakeChar[3]) << "\tcheckSum: " << int(shakeChar[0]) << endl;
		}
	}
}

/// <summary>
/// main
/// </summary>
/// <returns></returns>
int sendMain() {
	// 获取socket库，开始网络编程
	WSADATA wsadata;
	if (WSAStartup(MAKEWORD(2, 2), &wsadata)) {
		//如果加载出错，暂停服务，输出出错信息
		cout << "载入socket失败\n";
		return -1;
	}
	
	// 对sendSocket进行相关设置
	sendSocket = socket(AF_INET, SOCK_DGRAM, 0);
	if (sendSocket == INVALID_SOCKET) {
		cout << "socket创建失败\n";
		return -1;
	}
	
	// 将服务器的套接字和ip、port绑定
	serverAddr.sin_family = AF_INET;
	// ************需要输入的是服务器（也就是要发送的意定方）的IP
	char IP[16];
	cout << "输入要发送的服务器（server）的IP：\n";
	getIP(IP);
	inet_pton(AF_INET, IP, &serverAddr.sin_addr.s_addr);
	serverAddr.sin_port = htons(SERVER_PORT);

	// 对自己的进行处理
	clientAddr.sin_family = AF_INET;
	// ************输入自己（client）的IP
	cout << "输入客户端（自己）的IP：\n";
	getIP(IP);
	inet_pton(AF_INET, IP, &clientAddr.sin_addr.s_addr);
	clientAddr.sin_port = htons(CLIENT_PORT);

	// 对recvSocket进行处理
	recvSocket = socket(AF_INET, SOCK_DGRAM, 0);

	// 设置超时的时间
	setsockopt(recvSocket, SOL_SOCKET, SO_RCVTIMEO, (char*)&MAX_WAIT_TIME, sizeof(MAX_WAIT_TIME));

	int isBind = bind(recvSocket, (SOCKADDR*)&clientAddr, sizeof(clientAddr));
	// 判断是否绑定成功
	if (isBind == SOCKET_ERROR) {
		cout << "客户端的recvSocket绑定失败，错误代码为：" << WSAGetLastError();
		closesocket(recvSocket);
		WSACleanup();
		return -1;
	}

	// 和server建立连接
	connet2Server();
	
	// 显示文件列表
	getFileList();

	// 获取要发送的文件名
	cout << "输入想要传输的文件名：";
	char tempName[20];
	cin.getline(tempName, 20);

	// 计算文件名的长度
	unsigned short int fileNameLen = 0;
	while (tempName[fileNameLen] != '\0')
		fileNameLen += 1;

	// 发送文件
	sendFile2Server(tempName, fileNameLen);

	Sleep(600);

	// 挥手
	sayGoodBye2Server();
	
	// 输出吞吐率
	cout << "本次传输吞吐率为：" << Throughput << " Kb/s" << endl;

	return 0;
}