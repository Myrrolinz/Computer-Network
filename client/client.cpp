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

// 一些个全局变量
static SOCKET sendSocket, recvSocket; // 前面的socket用来发送，后面的用来接收
static SOCKADDR_IN serverAddr, clientAddr;
static int socketAddrLen = sizeof(serverAddr);
static const unsigned int MAX_WAIT_TIME = 1000; // 设置最大的等待时间
static int seq = 0;
static string path = "./test/*";

static double sendFileTime; // 设置一个全局变量，用来计算吞吐率，这里是时间
static double Throughput;


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

/* 要发送的数据包的样子
data[1024]
check	|seq	|len(2)	|isLast	|data(1019)
—————————————————————
0		|1		|2-3	|4		|5-1023
发送的第一个包是名称的长度

接收方返回的数据包的样子
check	|ack
—————————
0		|1   
*/

// 三次握手，建立连接
void connet2Server() {
	// 创建一个要发送的数据包，并初始化
	// 数据包：check, SYN，FIN，SHAKE/WAVE
	char shakeChar[4];
	shakeChar[1] = 0x01;
	shakeChar[2] = 0x00;
	shakeChar[3] = SHAKE1;
	shakeChar[0] = newGetChecksum(shakeChar+1, 3);

	// 发送shake1
	sendto(sendSocket, shakeChar, 4, 0, (SOCKADDR*)&serverAddr, socketAddrLen);
	cout<< "开始建立连接，请求建立连接的数据包为（第一次握手）：\n";
	cout << "SYN: " << int(shakeChar[1]) << "\tFIN: " << int(shakeChar[2]) << "\tSHAKE: " << int(shakeChar[3]) << "\tcheckSum: " << int(shakeChar[0]) << endl;

	// 开始计时
	int beginTime = clock();

	// 接收缓冲区
	char recvShakeBuf[4];
	memset(recvShakeBuf, 0, 4);

	//超时重传
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

	// 收到shake2正确，则发送shake3
	if (newGetChecksum(recvShakeBuf, 4)==0 && recvShakeBuf[1] == 0x01 && recvShakeBuf[2] == 0x00 && recvShakeBuf[3] == SHAKE2) {
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

static void sendFile2Server(char* fileName, unsigned short int len) {
	// 对输入的名字进行处理
	char filePath[] = "./test/";
	char* fileNameN = new char[7 + len + 1];
	for (int i = 0; i < 7 + len + 1; i++) {
		if (i < 7)
			fileNameN[i] = filePath[i];
		else
			fileNameN[i] = fileName[i - 7];
	}
	fileNameN[7 + len + 1] = '\0';

	// 读取文件（以二进制形式读入）
	ifstream toSendFile(fileNameN, ifstream::in | ios::binary);

	// 计算文件的长度
	toSendFile.seekg(0, toSendFile.end);// seekg（）是对输入文件定位，它有两个参数：第一个参数是偏移量，第二个参数是基地址。
	// tellg（）函数不需要带参数，它返回当前定位指针的位置，也代表着输入流的大小。
	unsigned int fileLen = toSendFile.tellg();// 文件长度
	toSendFile.seekg(0, toSendFile.beg);
	cout << "计算得到要发送的文件长度为：" << fileLen << endl;

	// 计算需要封装成多少个包
	unsigned int totalNum = fileLen / 1019;
	if (fileLen % 1019 > 0)
		totalNum += 1;
	// 再加一个表示文件名称的包
	totalNum += 1;
	cout << "一共需要封装成" << totalNum << "个包\n";

	// 创建临时缓冲区
	char* tempBuf = new char[fileLen];
	// 将图片装载到缓冲区
	toSendFile.read(tempBuf, fileLen);

	char seq = 0x00;// 从0号数据包开始，每次发送之后进行翻转
	unsigned int currentNum = 0; //记录已经成功发送了几个数据包，从0开始

	//// 开始封装数据包
	// 首先发送的是表示文件名称的数据包
	char sendPackage[1024];
	memset(sendPackage, 0, 1024);
	// 设置包体的内容
	sendPackage[1] = seq;
	for (unsigned short int i = 0; i < len; i++) {
		sendPackage[i + 5] = fileName[i];
	}
	sendPackage[3] = unsigned char(len & 0x00FF);
	sendPackage[2] = unsigned char((len >> 8) & 0x00FF);
	sendPackage[0] = newGetChecksum(sendPackage+1, 1024-1);

	// 从开始发送第一个数据包这里开始计算时间
	sendFileTime = clock();

	// 发送第一个数据包
	sendto(sendSocket, sendPackage, 1024, 0, (SOCKADDR*)&serverAddr, socketAddrLen);
	cout << "当前发送数据包为：\nseq: " << int(sendPackage[1]) << "\tlen高: " << int(sendPackage[2]) << "\tlen低: " << int(sendPackage[3])
		<< "\tchecksum: " << int(sendPackage[0]) << endl;

	int beginTime = clock();
	// 创建一个临时的包，用来存放内容，可以考虑改成2
	char recvShakeBuf[2];
	// 停等接收
	while (true) {
		while (recvfrom(sendSocket, recvShakeBuf, 2, 0, (SOCKADDR*)&serverAddr, &socketAddrLen) == SOCKET_ERROR) {
			if (clock() - beginTime > MAX_WAIT_TIME) {
				cout << "超过最长等待时间，重传:";
				sendto(sendSocket, sendPackage, 1024, 0, (SOCKADDR*)&serverAddr, socketAddrLen);
				beginTime = clock();
				cout << "当前发送数据包为：\nseq: " << int(sendPackage[1]) << "\tlen高: " << int(sendPackage[2]) << "\tlen低: " << int(sendPackage[3])
					<< "\tchecksum: " << int(sendPackage[0]) << endl;
				// cout << "SYN: " << int(sendPackage[1]) << "\tFIN: " << int(sendPackage[2]) << "\tSHAKE: " << int(sendPackage[3]) << "\tcheckSum: " << int(sendPackage[0]) << endl;
			}
		}
		cout << "FROM S:\nack: " << int(recvShakeBuf[1]) << "\tchecksum: " << int(recvShakeBuf[0]) << endl;
		// 检查收到的包是否正确
		if (newGetChecksum(recvShakeBuf, 2)==0 && recvShakeBuf[1] == seq) {
			// 正确，打包下一部分发送
			// 记录的数量+1
			currentNum += 1;
			if (currentNum == totalNum) {
				cout << "文件传输完成\n";
				break;
			}
			// seq翻转
			if (seq == 0x00)
				seq = 0x01;
			else
				seq = 0x00;

			// 清空发送缓存
			memset(sendPackage, 0, 1024);
			// 设置标志位
			sendPackage[1] = seq;
			// 装载下一部分文件
			unsigned short int actualLen = 0;
			for (int tempPoint = 0; (tempPoint < 1019) && ((currentNum - 1) * 1019 + tempPoint)<fileLen; tempPoint++) {
				sendPackage[5 + tempPoint] = tempBuf[(currentNum - 1) * 1019 + tempPoint];
				actualLen += 1;
			}
			// 设置长度
			sendPackage[3] = unsigned char(actualLen & 0x00FF);
			sendPackage[2] = unsigned char((actualLen >> 8) & 0x00FF);

			// 设置isLast
			if (currentNum == totalNum - 1)
				sendPackage[4] = 0x01;
			sendPackage[0] = newGetChecksum(sendPackage + 1, 1024 - 1);
			sendto(sendSocket, sendPackage, 1024, 0, (SOCKADDR*)&serverAddr, socketAddrLen);
			beginTime = clock();
			cout << "当前发送数据包为：\nseq: " << int(sendPackage[1]) << "\tlen高: " << int(sendPackage[2]) << "\tlen低: " << int(sendPackage[3])
				<< "\tchecksum: " << int(sendPackage[0]) << endl;
		}
		else {
			// 错误，重传
			cout << "收到错误的ack，重新发送\n";
			sendto(sendSocket, sendPackage, 1024, 0, (SOCKADDR*)&serverAddr, socketAddrLen);
			beginTime = clock();
			cout << "当前发送数据包为：\nseq: " << int(sendPackage[1]) << "\tlen高: " << int(sendPackage[2]) << "\tlen低: " << int(sendPackage[3])
				<< "\tchecksum: " << int(sendPackage[0]) << endl;
		}
	}

	sendFileTime = clock() - sendFileTime;
	// 计算成秒
	sendFileTime = sendFileTime / CLOCKS_PER_SEC;
	cout << "本次传输所用时间为：" << sendFileTime << " s." << endl;
	// 计算吞吐率
	Throughput = fileLen / sendFileTime;
}

static void sayGoodBye2Server() {
	// 两次挥手，断开连接

	// 数据包：check, SYN，FIN，SHAKE/WAVE
	char shakeChar[4];
	shakeChar[1] = 0x00;
	shakeChar[2] = 0x01;
	shakeChar[3] = WAVE1;
	shakeChar[0] = newGetChecksum(shakeChar+1, 4-1);
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
	cout << "输入想要传输的文件名：";
	char tempName[20];
	cin.getline(tempName, 20);

	unsigned short int fileNameLen = 0;
	while (tempName[fileNameLen] != '\0')
		fileNameLen += 1;
	// 发送文件
	sendFile2Server(tempName, fileNameLen);

	// 挥手
	sayGoodBye2Server();
	
	// 输出吞吐率
	cout << "本次传输吞吐率为：" << Throughput << endl;
	
	return 0;
}