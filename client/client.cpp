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

// һЩ��ȫ�ֱ���
static SOCKET sendSocket, recvSocket; // ǰ���socket�������ͣ��������������
static SOCKADDR_IN serverAddr, clientAddr;
static int socketAddrLen = sizeof(serverAddr);
static const unsigned int MAX_WAIT_TIME = 1000; // �������ĵȴ�ʱ��
static int seq = 0;
static string path = "./test/*";

static double sendFileTime; // ����һ��ȫ�ֱ������������������ʣ�������ʱ��
static double Throughput;


// �������ֵ�ʱ���Ӧ��flag
const unsigned char SHAKE1 = 0x01;
const unsigned char SHAKE2 = 0x02;
const unsigned char SHAKE3 = 0x03;
// �Ĵλ��ֵ�ʱ���flag
const unsigned char WAVE1 = 0x04;
const unsigned char WAVE2 = 0x05;
// ����ʱʹ�õĶ˿�
const int CLIENT_PORT = 12880;
const int SERVER_PORT = 12660;

/* Ҫ���͵����ݰ�������
data[1024]
check	|seq	|len(2)	|isLast	|data(1019)
������������������������������������������
0		|1		|2-3	|4		|5-1023
���͵ĵ�һ���������Ƶĳ���

���շ����ص����ݰ�������
check	|ack
������������������
0		|1   
*/

// �������֣���������
void connet2Server() {
	// ����һ��Ҫ���͵����ݰ�������ʼ��
	// ���ݰ���check, SYN��FIN��SHAKE/WAVE
	char shakeChar[4];
	shakeChar[1] = 0x01;
	shakeChar[2] = 0x00;
	shakeChar[3] = SHAKE1;
	shakeChar[0] = newGetChecksum(shakeChar+1, 3);

	// ����shake1
	sendto(sendSocket, shakeChar, 4, 0, (SOCKADDR*)&serverAddr, socketAddrLen);
	cout<< "��ʼ�������ӣ����������ӵ����ݰ�Ϊ����һ�����֣���\n";
	cout << "SYN: " << int(shakeChar[1]) << "\tFIN: " << int(shakeChar[2]) << "\tSHAKE: " << int(shakeChar[3]) << "\tcheckSum: " << int(shakeChar[0]) << endl;

	// ��ʼ��ʱ
	int beginTime = clock();

	// ���ջ�����
	char recvShakeBuf[4];
	memset(recvShakeBuf, 0, 4);

	//��ʱ�ش�
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

	// �յ�shake2��ȷ������shake3
	if (newGetChecksum(recvShakeBuf, 4)==0 && recvShakeBuf[1] == 0x01 && recvShakeBuf[2] == 0x00 && recvShakeBuf[3] == SHAKE2) {
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

static void sendFile2Server(char* fileName, unsigned short int len) {
	// ����������ֽ��д���
	char filePath[] = "./test/";
	char* fileNameN = new char[7 + len + 1];
	for (int i = 0; i < 7 + len + 1; i++) {
		if (i < 7)
			fileNameN[i] = filePath[i];
		else
			fileNameN[i] = fileName[i - 7];
	}
	fileNameN[7 + len + 1] = '\0';

	// ��ȡ�ļ����Զ�������ʽ���룩
	ifstream toSendFile(fileNameN, ifstream::in | ios::binary);

	// �����ļ��ĳ���
	toSendFile.seekg(0, toSendFile.end);// seekg�����Ƕ������ļ���λ������������������һ��������ƫ�������ڶ��������ǻ���ַ��
	// tellg������������Ҫ�������������ص�ǰ��λָ���λ�ã�Ҳ�������������Ĵ�С��
	unsigned int fileLen = toSendFile.tellg();// �ļ�����
	toSendFile.seekg(0, toSendFile.beg);
	cout << "����õ�Ҫ���͵��ļ�����Ϊ��" << fileLen << endl;

	// ������Ҫ��װ�ɶ��ٸ���
	unsigned int totalNum = fileLen / 1019;
	if (fileLen % 1019 > 0)
		totalNum += 1;
	// �ټ�һ����ʾ�ļ����Ƶİ�
	totalNum += 1;
	cout << "һ����Ҫ��װ��" << totalNum << "����\n";

	// ������ʱ������
	char* tempBuf = new char[fileLen];
	// ��ͼƬװ�ص�������
	toSendFile.read(tempBuf, fileLen);

	char seq = 0x00;// ��0�����ݰ���ʼ��ÿ�η���֮����з�ת
	unsigned int currentNum = 0; //��¼�Ѿ��ɹ������˼������ݰ�����0��ʼ

	//// ��ʼ��װ���ݰ�
	// ���ȷ��͵��Ǳ�ʾ�ļ����Ƶ����ݰ�
	char sendPackage[1024];
	memset(sendPackage, 0, 1024);
	// ���ð��������
	sendPackage[1] = seq;
	for (unsigned short int i = 0; i < len; i++) {
		sendPackage[i + 5] = fileName[i];
	}
	sendPackage[3] = unsigned char(len & 0x00FF);
	sendPackage[2] = unsigned char((len >> 8) & 0x00FF);
	sendPackage[0] = newGetChecksum(sendPackage+1, 1024-1);

	// �ӿ�ʼ���͵�һ�����ݰ����￪ʼ����ʱ��
	sendFileTime = clock();

	// ���͵�һ�����ݰ�
	sendto(sendSocket, sendPackage, 1024, 0, (SOCKADDR*)&serverAddr, socketAddrLen);
	cout << "��ǰ�������ݰ�Ϊ��\nseq: " << int(sendPackage[1]) << "\tlen��: " << int(sendPackage[2]) << "\tlen��: " << int(sendPackage[3])
		<< "\tchecksum: " << int(sendPackage[0]) << endl;

	int beginTime = clock();
	// ����һ����ʱ�İ�������������ݣ����Կ��Ǹĳ�2
	char recvShakeBuf[2];
	// ͣ�Ƚ���
	while (true) {
		while (recvfrom(sendSocket, recvShakeBuf, 2, 0, (SOCKADDR*)&serverAddr, &socketAddrLen) == SOCKET_ERROR) {
			if (clock() - beginTime > MAX_WAIT_TIME) {
				cout << "������ȴ�ʱ�䣬�ش�:";
				sendto(sendSocket, sendPackage, 1024, 0, (SOCKADDR*)&serverAddr, socketAddrLen);
				beginTime = clock();
				cout << "��ǰ�������ݰ�Ϊ��\nseq: " << int(sendPackage[1]) << "\tlen��: " << int(sendPackage[2]) << "\tlen��: " << int(sendPackage[3])
					<< "\tchecksum: " << int(sendPackage[0]) << endl;
				// cout << "SYN: " << int(sendPackage[1]) << "\tFIN: " << int(sendPackage[2]) << "\tSHAKE: " << int(sendPackage[3]) << "\tcheckSum: " << int(sendPackage[0]) << endl;
			}
		}
		cout << "FROM S:\nack: " << int(recvShakeBuf[1]) << "\tchecksum: " << int(recvShakeBuf[0]) << endl;
		// ����յ��İ��Ƿ���ȷ
		if (newGetChecksum(recvShakeBuf, 2)==0 && recvShakeBuf[1] == seq) {
			// ��ȷ�������һ���ַ���
			// ��¼������+1
			currentNum += 1;
			if (currentNum == totalNum) {
				cout << "�ļ��������\n";
				break;
			}
			// seq��ת
			if (seq == 0x00)
				seq = 0x01;
			else
				seq = 0x00;

			// ��շ��ͻ���
			memset(sendPackage, 0, 1024);
			// ���ñ�־λ
			sendPackage[1] = seq;
			// װ����һ�����ļ�
			unsigned short int actualLen = 0;
			for (int tempPoint = 0; (tempPoint < 1019) && ((currentNum - 1) * 1019 + tempPoint)<fileLen; tempPoint++) {
				sendPackage[5 + tempPoint] = tempBuf[(currentNum - 1) * 1019 + tempPoint];
				actualLen += 1;
			}
			// ���ó���
			sendPackage[3] = unsigned char(actualLen & 0x00FF);
			sendPackage[2] = unsigned char((actualLen >> 8) & 0x00FF);

			// ����isLast
			if (currentNum == totalNum - 1)
				sendPackage[4] = 0x01;
			sendPackage[0] = newGetChecksum(sendPackage + 1, 1024 - 1);
			sendto(sendSocket, sendPackage, 1024, 0, (SOCKADDR*)&serverAddr, socketAddrLen);
			beginTime = clock();
			cout << "��ǰ�������ݰ�Ϊ��\nseq: " << int(sendPackage[1]) << "\tlen��: " << int(sendPackage[2]) << "\tlen��: " << int(sendPackage[3])
				<< "\tchecksum: " << int(sendPackage[0]) << endl;
		}
		else {
			// �����ش�
			cout << "�յ������ack�����·���\n";
			sendto(sendSocket, sendPackage, 1024, 0, (SOCKADDR*)&serverAddr, socketAddrLen);
			beginTime = clock();
			cout << "��ǰ�������ݰ�Ϊ��\nseq: " << int(sendPackage[1]) << "\tlen��: " << int(sendPackage[2]) << "\tlen��: " << int(sendPackage[3])
				<< "\tchecksum: " << int(sendPackage[0]) << endl;
		}
	}

	sendFileTime = clock() - sendFileTime;
	// �������
	sendFileTime = sendFileTime / CLOCKS_PER_SEC;
	cout << "���δ�������ʱ��Ϊ��" << sendFileTime << " s." << endl;
	// ����������
	Throughput = fileLen / sendFileTime;
}

static void sayGoodBye2Server() {
	// ���λ��֣��Ͽ�����

	// ���ݰ���check, SYN��FIN��SHAKE/WAVE
	char shakeChar[4];
	shakeChar[1] = 0x00;
	shakeChar[2] = 0x01;
	shakeChar[3] = WAVE1;
	shakeChar[0] = newGetChecksum(shakeChar+1, 4-1);
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
	cout << "������Ҫ������ļ�����";
	char tempName[20];
	cin.getline(tempName, 20);

	unsigned short int fileNameLen = 0;
	while (tempName[fileNameLen] != '\0')
		fileNameLen += 1;
	// �����ļ�
	sendFile2Server(tempName, fileNameLen);

	// ����
	sayGoodBye2Server();
	
	// ���������
	cout << "���δ���������Ϊ��" << Throughput << endl;
	
	return 0;
}