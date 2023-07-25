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

// ȫ�ֱ���
static SOCKET sendSocket, recvSocket; // ǰ���socket�������ͣ��������������
static SOCKADDR_IN serverAddr, clientAddr;
static int socketAddrLen = sizeof(serverAddr);
static const unsigned int MAX_WAIT_TIME = 100000; // �������ĵȴ�ʱ��
static string path = "./test/*";

static double sendFileTime; // ����һ��ȫ�ֱ������������������ʣ�������ʱ��
static double Throughput; // ������

static int WINDOW_SIZE = 64; // �������Ĵ��ڴ�С

// �������ֵ�ʱ���Ӧ��flag
const unsigned char SHAKE1 = 0x01;
const unsigned char SHAKE2 = 0x02;
const unsigned char SHAKE3 = 0x03;
// �Ĵλ��ֵ�ʱ���flag
const unsigned char WAVE1 = 0x04;
const unsigned char WAVE2 = 0x05;
// ����ʱʹ�õĶ˿�
const int CLIENT_PORT = 12880;
const int SERVER_PORT = 12770;

/* 
Ҫ���͵����ݰ�������
data[1024]
check, seq[3], len[2],isLast, data[1017]
0      1-3     4-5    6       7-1023
���͵ĵ�һ���������Ƶĳ���

���շ����ص����ݰ�������
ans[4]
check, ack[3]
0      1-3  

Э��ʹ�ã�GBN
SR
*/
static int dataLen = 1017; // ����ÿ�����ݰ��е�������ݳ���

/// <summary>
/// ͣ�Ȼ��������������
/// </summary>
void connet2Server() {
	// �������֣���������
	// ����һ��Ҫ���͵����ݰ�������ʼ��Ϊ��
	
	// ���ݰ���check, SYN��FIN��SHAKE/WAVE
	char shakeChar[4];
	shakeChar[1] = 0x01;
	shakeChar[2] = 0x00;
	shakeChar[3] = SHAKE1;
	shakeChar[0] = newGetChecksum(shakeChar + 1, 3);

	// ����shake1
	sendto(sendSocket, shakeChar, 4, 0, (SOCKADDR*)&serverAddr, socketAddrLen);
	cout<< "��ʼ�������ӣ����������ӵ����ݰ�Ϊ����һ�����֣���\n";
	cout << "SYN: " << int(shakeChar[1]) << "\tFIN: " << int(shakeChar[2]) << "\tSHAKE: " << int(shakeChar[3]) << "\tcheckSum: " << int(shakeChar[0]) << endl;

	int beginTime = clock(); // ��ʼ��ʱ
	char recvShakeBuf[4];
	memset(recvShakeBuf, 0, 4);

	while((recvfrom(sendSocket, recvShakeBuf, 4, 0, (SOCKADDR*)&serverAddr, &socketAddrLen))==SOCKET_ERROR){
		if (clock() - beginTime > MAX_WAIT_TIME) {
			cout << "������ȴ�ʱ�䣬�ش�:";
			sendto(sendSocket, shakeChar, 4, 0, (SOCKADDR*)&serverAddr, socketAddrLen);
			beginTime = clock();
			cout << "SYN: " << int(shakeChar[1]) << "\tFIN: " << int(shakeChar[2]) << "\tSHAKE: " << int(shakeChar[3]) << "\tcheckSum: " << int(shakeChar[0]) << endl;
		}
	}
	// �յ����ݰ�
	cout << "�յ����ݰ�Ϊ��\n";
	cout << "SYN: " << int(recvShakeBuf[1]) << "\tFIN: " << int(recvShakeBuf[2]) << "\tSHAKE: " << int(recvShakeBuf[3]) << "\tcheckSum: " << int(recvShakeBuf[0]) << endl;

	if (newGetChecksum(recvShakeBuf, 4)==0 && recvShakeBuf[1] == 0x01 && recvShakeBuf[2] == 0x00 && recvShakeBuf[3] == SHAKE2) {
		// �յ����ְ���ȷ
		// ���͵��������ְ�������
		memset(shakeChar, 0, 4);
		shakeChar[1] = 0x01;
		shakeChar[2] = 0x00;
		shakeChar[3] = SHAKE3;
		shakeChar[0] = newGetChecksum(shakeChar+1, 3);
		sendto(sendSocket, shakeChar, 4, 0, (SOCKADDR*)&serverAddr, socketAddrLen);
		cout << "���͵������������ݰ���\n";
		cout << "SYN: " << int(shakeChar[1]) << "\tFIN: " << int(shakeChar[2]) << "\tSHAKE: " << int(shakeChar[3]) << "\tcheckSum: " << int(shakeChar[0]) << endl;
		cout << "\n���ӽ����ɹ�\n\n";
	}
}

/// <summary>
/// ������������ļ�ʹ�õĺ�������������
/// </summary>
/// <param name="fileName">Ҫ������ļ���</param>
/// <param name="fileNameLen">�ļ����ĳ���</param>
static void sendFile2Server(char* fileName, unsigned short int fileNameLen) {
	// ����������ֽ��д���
	char filePath[] = "./test/";
	char* fileNameN = new char[7 + fileNameLen + 1];
	for (int i = 0; i < 7 + fileNameLen + 1; i++) {
		if (i < 7)
			fileNameN[i] = filePath[i];
		else
			fileNameN[i] = fileName[i - 7];
	}
	fileNameN[7 + fileNameLen + 1] = '\0';

	// ��ȡ�ļ����Զ�������ʽ���룩
	ifstream toSendFile(fileNameN, ifstream::in | ios::binary);

	// �����ļ��ĳ���
	toSendFile.seekg(0, toSendFile.end);
	unsigned int fileLen = toSendFile.tellg();
	toSendFile.seekg(0, toSendFile.beg);
	cout << "����õ�Ҫ���͵��ļ�����Ϊ��" << fileLen << endl;

	// ������Ҫ��װ�ɶ��ٸ���
	unsigned int totalNum = fileLen / dataLen;
	if (fileLen % dataLen > 0)
		totalNum += 1;
	// �ټ�һ����ʾ�ļ����Ƶİ�
	totalNum += 1;
	cout << "һ����Ҫ��װ��" << totalNum << "����\n";

	// ��ʱ������Ϊ-1��ʱ���ʾ��ʱ��ֹͣ��
	int timer = -1;

	// ������ʱ������
	char* tempBuf = new char[fileLen];
	// ���ļ�װ�ص�������
	toSendFile.read(tempBuf, fileLen);

	// ���ڵ������
	unsigned int leftWindow = 0; 
	// ��һ��Ҫ���͵�
	unsigned int curr = 0;

	// ��ʼ��װ���ݰ�
	// ���ļ�ȫ����װ���Ժ��ֱ�ӷ���������ÿ�η�֮ǰ��װ
	char** sendPackage = new char*[totalNum];
	for (int tmp = 0; tmp < totalNum; tmp++) {
		*(sendPackage + tmp) = new char[1024];
		memset(sendPackage[tmp], 0, 1024);
	}

	// �����ļ������������
	sendPackage[0][3] = 0x00; 
	for (unsigned short int i = 0; i < fileNameLen; i++) {
		sendPackage[0][i + 7] = fileName[i];
	}
	sendPackage[0][5] = unsigned char(fileNameLen & 0x00FF); //len��
	sendPackage[0][4] = unsigned char((fileNameLen >> 8) & 0x00FF); //len��
	sendPackage[0][0] = newGetChecksum(sendPackage[0]+1, 1024-1); //check

	// ��ʼ���ļ�������д��
	for (int tmp = 1; tmp < totalNum; tmp++) {
		// ���ñ�־λ seq
		sendPackage[tmp][3] = tmp & 0xFF;
		sendPackage[tmp][2] = (tmp >> 8) & 0xFF;
		sendPackage[tmp][1] = (tmp >> 16) & 0xFF;

		// ����isLastλ
		if (tmp == totalNum - 1)
			sendPackage[tmp][6] = 0x01;
		else
			sendPackage[tmp][6] = 0x00;

		// װ����һ�����ļ�
		unsigned short int actualLen = 0;
		for (int tempPoint = 0; (tempPoint < dataLen) && ((tmp - 1) * dataLen + tempPoint) < fileLen; tempPoint++) {
			sendPackage[tmp][7 + tempPoint] = tempBuf[(tmp - 1) * dataLen + tempPoint];
			actualLen += 1;
		}

		// ���ó���
		sendPackage[tmp][5] = unsigned char(actualLen & 0x00FF);
		sendPackage[tmp][4] = unsigned char((actualLen >> 8) & 0x00FF);

		// ����У���
		sendPackage[tmp][0] = newGetChecksum(sendPackage[tmp] + 1, 1024 - 1);
	}
	// ---------�����ļ�ȫ���������

	// ����һ��sr������������ʼ�����������ʶ�Ƿ�acked
	bool* srBuf = new bool[totalNum] ;
	for (int i = 0; i < totalNum; i++)
		srBuf[i] = false;

	// �ӿ�ʼ���͵�һ�����ݰ����￪ʼ������Ҫͳ�Ƶ�ʱ��
	sendFileTime = clock();

	while (leftWindow != totalNum) {
		// forѭ�����ʹ�����ô������ݰ������ڴ�СΪWINDOW_SIZE
		// ���ڵ�ǰ��Ҫ���͵����ݰ��ļ��㹫ʽ: curr = leftWindow + willSendInWindow
		for (; (curr < leftWindow + WINDOW_SIZE) && (curr < totalNum); curr++) {
			// �������ݰ�
			sendto(sendSocket, (char*)sendPackage[curr], 1024, 0, (SOCKADDR*)&serverAddr, socketAddrLen);
			if (curr == leftWindow)
				// ��ʱ����ʼ��ʱ
				timer = clock();
			// ��������Ϣ
			cout << "��ǰ�������ݰ�Ϊ��\nseq: " << curr << "\nlen��: " << int(sendPackage[curr][4]) << "\tlen��: " << int(sendPackage[curr][5])
				<< "\tchecksum: " << int(sendPackage[curr][0]) << endl << "ʣ�ര�ڴ�С��" << (leftWindow + WINDOW_SIZE - curr) << endl;
		}

		// ����һ��������Żظ��Ļ�����������Ϊ4
		char recvBuf[4];
		// ѭ���������ݰ��������´������
		while (recvfrom(sendSocket, recvBuf, 4, 0, (SOCKADDR*)&serverAddr, &socketAddrLen) == SOCKET_ERROR) {
			// ��ʱ�ش�
			if ((timer != -1) && (clock() - timer > MAX_WAIT_TIME)) {
				cout << "==================������ȴ�ʱ�䣬�ش�:\n";
				int tempBase = leftWindow;
				for (; tempBase < curr; tempBase++) {
					if (!srBuf[tempBase]) { //���ش�δȷ�ϵ����ݱ�
						// �������ݰ�
						sendto(sendSocket, (char*)sendPackage[tempBase], 1024, 0, (SOCKADDR*)&serverAddr, socketAddrLen);
						// ��������Ϣ
						cout << "��ǰ�������ݰ�Ϊ��\nseq: " << tempBase << "\nlen��: " << int(sendPackage[tempBase][4]) << "\tlen��: " << int(sendPackage[tempBase][5])
							<< "\tchecksum: " << int(sendPackage[tempBase][0]) << endl << "ʣ����Ҫ�ش��İ�����Ϊ��" << (curr - tempBase - 1) << endl << endl;
					}
					// ���¿�ʼ��ʱ
					timer = clock();
				}
			}
		}
		// ��ʾ�յ�����Ч�����ݰ�����ʼ������ݰ�����ȷ��

		// ������������
		unsigned int answerNum;
		answerNum = unsigned short int(recvBuf[1]) & 0xFF;
		answerNum = (answerNum << 8) + (unsigned short int(recvBuf[2]) & 0xFF);
		answerNum = (answerNum << 8) + (unsigned short int(recvBuf[3]) & 0xFF);
		cout << "FROM S:\nchecksum: " << int(recvBuf[0]) << "\tack: " << answerNum << endl;

		// ���checksum
		if ((newGetChecksum(recvBuf, 4) == 0) && (answerNum >= leftWindow)) {
			if (answerNum == curr-1) {
				// bool������£�����Ϊtrue����ʾ�Ѿ�������
				for (int i = leftWindow; i < answerNum; i++)
					srBuf[i] = true;
				// ���ڵ�����ƶ�
				leftWindow = answerNum + 1;

				// �ж��Ƿ���Ҫ����timer
				if (leftWindow == curr)
					timer = -1;
				else
					timer = clock();
			}
			else if (answerNum < curr-1) {
				// bool������£�����Ϊtrue����ʾ�Ѿ�������
				for (int i = leftWindow; i < answerNum; i++)
					srBuf[i] = true;
				// ���ڵ�����ƶ�
				leftWindow = answerNum + 1;
				// �ش�leftWindow
				//sendto(sendSocket, (char*)sendPackage[leftWindow], 1024, 0, (SOCKADDR*)&serverAddr, socketAddrLen);
				//// ��������Ϣ
				//cout << "���ش�leftwindow��\t��ǰ�������ݰ�Ϊ��\nseq: " << leftWindow << "\nlen��: " << int(sendPackage[leftWindow][4]) << "\tlen��: " << int(sendPackage[leftWindow][5])
				//	<< "\tchecksum: " << int(sendPackage[leftWindow][0]) << endl << "\nʣ����Ҫ�ش��İ�����Ϊ��" << (curr - leftWindow - 1) << endl << endl;
				//// ���¿�ʼ��ʱ
				//timer = clock();
			}
		}
		else {
			// Ҫô���յ��İ����ˣ�Ҫô��ack�������֮ǰ�ģ���ʱ����Ҫ��������Щȫ���ش�
			cout << "==================�յ������ݰ������ش�:\n";
			//int tempBase = leftWindow;
			int tempBase = answerNum + 1;
			for (; tempBase < curr; tempBase++) {
				if (!srBuf[tempBase]) {
					// �������ݰ�
					sendto(sendSocket, (char*)sendPackage[tempBase], 1024, 0, (SOCKADDR*)&serverAddr, socketAddrLen);
					// ��������Ϣ
					cout << "��ǰ�������ݰ�Ϊ��\nseq: " << tempBase << "\nlen��: " << int(sendPackage[tempBase][4]) << "\tlen��: " << int(sendPackage[tempBase][5])
						<< "\tchecksum: " << int(sendPackage[tempBase][0]) << endl << "\nʣ����Ҫ�ش��İ�����Ϊ��" << (curr - tempBase - 1) << endl << endl;
				}
			}
			//���¿�ʼ��ʱ
			timer = clock();
		}
	}

	sendFileTime = clock() - sendFileTime;
	// �������
	sendFileTime = sendFileTime / CLOCKS_PER_SEC;
	cout << "���δ�������ʱ��Ϊ��" << sendFileTime << " s." << endl;
	// ����������
	Throughput = fileLen / sendFileTime;
}

/// <summary>
/// ����ʹ�õĺ���������ʹ�õ���ͣ�Ȼ���
/// </summary>
static void sayGoodBye2Server() {
	// ���λ��֣��Ͽ�����

	// ���ݰ���check, SYN��FIN��SHAKE/WAVE
	char shakeChar[4];
	shakeChar[1] = 0x00;
	shakeChar[2] = 0x01;
	shakeChar[3] = WAVE1;
	shakeChar[0] = newGetChecksum(shakeChar + 1, 4 - 1);
	// ����WAVE1
	sendto(sendSocket, shakeChar, 4, 0, (SOCKADDR*)&serverAddr, socketAddrLen);
	cout << "�Ͽ����ӣ��������ݰ�Ϊ����һ�λ��֣���\n";
	cout << "SYN: " << int(shakeChar[1]) << "\tFIN: " << int(shakeChar[2]) << "\tSHAKE: " << int(shakeChar[3]) << "\tcheckSum: " << int(shakeChar[0]) << endl;
	
	// ��ʼ��ʱ
	int beginTime = clock();

	// ͣ�ȣ��ȷ������Ļ���
	char recvWaveBuf[4];
	while (true) {
		while ((recvfrom(sendSocket, recvWaveBuf, 4, 0, (SOCKADDR*)&serverAddr, &socketAddrLen))==SOCKET_ERROR) {
			if (clock() - beginTime > MAX_WAIT_TIME) {
				cout << "������ȴ�ʱ�䣬�ش�:";
				sendto(sendSocket, shakeChar, 4, 0, (SOCKADDR*)&serverAddr, socketAddrLen);
				beginTime = clock();
				cout << "SYN: " << int(shakeChar[1]) << "\tFIN: " << int(shakeChar[2]) << "\tSHAKE: " << int(shakeChar[3]) << "\tcheckSum: " << int(shakeChar[0]) << endl;
			}
		}
		if (newGetChecksum(recvWaveBuf, 4)==0 && recvWaveBuf[1] == 0x00 && recvWaveBuf[2] == 0x01 && recvWaveBuf[3] == WAVE2) {
			// �յ����ְ���ȷ
			cout << "�յ����Է������ĵڶ��λ��֣�\n";
			cout << "SYN: " << int(recvWaveBuf[1]) << "\tFIN: " << int(recvWaveBuf[2]) << "\tSHAKE: " << int(recvWaveBuf[3]) << "\tcheckSum: " << int(recvWaveBuf[0]) << endl;
			memset(recvWaveBuf, 0, 4);
			memset(shakeChar, 0, 4);
			break;
		}
		else {
			cout << "����������ʧ�ܣ��ش��������ݰ�\n";
			sendto(sendSocket, shakeChar, 4, 0, (SOCKADDR*)&serverAddr, socketAddrLen);
			cout << "�Ͽ����ӣ��������ݰ�Ϊ����һ�λ��֣���\n";
			cout << "SYN: " << int(shakeChar[1]) << "\tFIN: " << int(shakeChar[2]) << "\tSHAKE: " << int(shakeChar[3]) << "\tcheckSum: " << int(shakeChar[0]) << endl;
		}
	}
}

/// <summary>
/// main
/// </summary>
/// <returns></returns>
int sendMain() {
	// ��ȡsocket�⣬��ʼ������
	WSADATA wsadata;
	if (WSAStartup(MAKEWORD(2, 2), &wsadata)) {
		//������س�����ͣ�������������Ϣ
		cout << "����socketʧ��\n";
		return -1;
	}
	
	// ��sendSocket�����������
	sendSocket = socket(AF_INET, SOCK_DGRAM, 0);
	if (sendSocket == INVALID_SOCKET) {
		cout << "socket����ʧ��\n";
		return -1;
	}
	
	// �����������׽��ֺ�ip��port��
	serverAddr.sin_family = AF_INET;
	// ************��Ҫ������Ƿ�������Ҳ����Ҫ���͵��ⶨ������IP
	char IP[16];
	cout << "����Ҫ���͵ķ�������server����IP��\n";
	getIP(IP);
	inet_pton(AF_INET, IP, &serverAddr.sin_addr.s_addr);
	serverAddr.sin_port = htons(SERVER_PORT);

	// ���Լ��Ľ��д���
	clientAddr.sin_family = AF_INET;
	// ************�����Լ���client����IP
	cout << "����ͻ��ˣ��Լ�����IP��\n";
	getIP(IP);
	inet_pton(AF_INET, IP, &clientAddr.sin_addr.s_addr);
	clientAddr.sin_port = htons(CLIENT_PORT);

	// ��recvSocket���д���
	recvSocket = socket(AF_INET, SOCK_DGRAM, 0);

	// ���ó�ʱ��ʱ��
	setsockopt(recvSocket, SOL_SOCKET, SO_RCVTIMEO, (char*)&MAX_WAIT_TIME, sizeof(MAX_WAIT_TIME));

	int isBind = bind(recvSocket, (SOCKADDR*)&clientAddr, sizeof(clientAddr));
	// �ж��Ƿ�󶨳ɹ�
	if (isBind == SOCKET_ERROR) {
		cout << "�ͻ��˵�recvSocket��ʧ�ܣ��������Ϊ��" << WSAGetLastError();
		closesocket(recvSocket);
		WSACleanup();
		return -1;
	}

	// ��server��������
	connet2Server();
	
	// ��ʾ�ļ��б�
	getFileList();

	// ��ȡҪ���͵��ļ���
	cout << "������Ҫ������ļ�����";
	char tempName[20];
	cin.getline(tempName, 20);

	// �����ļ����ĳ���
	unsigned short int fileNameLen = 0;
	while (tempName[fileNameLen] != '\0')
		fileNameLen += 1;

	// �����ļ�
	sendFile2Server(tempName, fileNameLen);

	Sleep(600);

	// ����
	sayGoodBye2Server();
	
	// ���������
	cout << "���δ���������Ϊ��" << Throughput << " Kb/s" << endl;

	return 0;
}