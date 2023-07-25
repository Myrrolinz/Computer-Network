#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include"server.h"
#include"externTools.h"
#pragma comment(lib, "ws2_32.lib")
#include<winsock2.h>
#include<WS2tcpip.h>
#include<iostream>
#include<fstream>
#include<sstream>
#include<vector>
using namespace std;

// һЩ�����ʹ�õ���ȫ�ֱ���
static SOCKET serverSocket, clientSocket; // ��������socket
static SOCKADDR_IN serverAddr, clientAddr; // ������ṹ�������ص�IP��ַ�Ͷ˿ں�
static int sockaddrLen = sizeof(serverAddr);

static const int MAX_WAIT_TIME = 1000; // �������ȴ�ʱ�䣬20ms

// �������ֵ�ʱ���Ӧ��flag
const unsigned char SHAKE1 = 0x01;
const unsigned char SHAKE2 = 0x02;
const unsigned char SHAKE3 = 0x03;
// ���λ��ֵ�ʱ���flag
const unsigned char WAVE1 = 0x04;
const unsigned char WAVE2 = 0x05;
// ����ʱʹ�õĶ˿�
const int CLIENT_PORT = 12880;
const int SERVER_PORT = 12660;

static char ServerIP[16] = "127.0.0.1"; // ������IP��ַ
static char ClientIP[16]; // client��IP

static unsigned int currentNum = 0; // ��ǰ��Ҫ���յ�������
static unsigned int divenum = 64; // ����������������ʾ�յ���ô��İ��ͻ�һ��ack


static bool shakeHand() {
	// ����
	//Package p;
	//Package sendP;
	char recvShakeBuf[4];
	while (1) {
		memset(recvShakeBuf, 0, 4);
		// ͣ�Ȼ��ƽ������ݰ�
		while (recvfrom(serverSocket, recvShakeBuf, 4, 0, (sockaddr*)&clientAddr, &sockaddrLen) == SOCKET_ERROR) {

		}

		//// �����յ������ݰ�У����Ƿ���ȷ
		//// �ȶ��յ������ݽ���ת��
		//p = *((Package*)recvBuf);
		// �����Ϣ
		cout << "�յ��������ӵ��������ݰ�Ϊ��\n";
		// packageOutput(p);
		cout << "SYN: " << int(recvShakeBuf[1]) << "\tFIN: " << int(recvShakeBuf[2]) << "\tSHAKE: " << int(recvShakeBuf[3]) << "\tcheckSum: " << int(recvShakeBuf[0]) << endl;
		// ��һ��
		if (newGetChecksum(recvShakeBuf,4)==0 && recvShakeBuf[1] == 0x01 && recvShakeBuf[2] == 0x00 && recvShakeBuf[3] == SHAKE1) {
			// �յ����ְ���ȷ
			cout << "���ְ���ȷ\n";
			// �����ڶ������ְ�
			char secondShake[4];
			secondShake[1] = 0x01;
			secondShake[2] = 0x00;
			secondShake[3] = SHAKE2;
			secondShake[0] = newGetChecksum(secondShake+1, 4-1);
			// ���͵ڶ������ְ�
			sendto(serverSocket, secondShake, 4, 0, (SOCKADDR*)&clientAddr, sockaddrLen);
			// �������
			cout << "SYN: " << int(secondShake[1]) << "\tFIN: " << int(secondShake[2]) << "\tSHAKE: " << int(secondShake[3]) << "\tcheckSum: " << int(secondShake[0]) << endl;

			int beginTime = clock();

			// ��ոոս��յİ����ȴ�����������
			memset(recvShakeBuf, 0, 4);

			while (recvfrom(serverSocket, recvShakeBuf, 4, 0, (sockaddr*)&clientAddr, &sockaddrLen) == SOCKET_ERROR) {
				if (clock() - beginTime > MAX_WAIT_TIME) {
					cout << "������ȴ�ʱ�䣬�ش�:";
					sendto(serverSocket, secondShake, 4, 0, (SOCKADDR*)&clientAddr, sockaddrLen);
					beginTime = clock();
					cout << "SYN: " << int(secondShake[1]) << "\tFIN: " << int(secondShake[2]) << "\tSHAKE: " << int(secondShake[3]) << "\tcheckSum: " << int(secondShake[0]) << endl;
				}
			}			

			cout << "�յ����ݰ�Ϊ��\n";
			cout << "SYN: " << int(recvShakeBuf[1]) << "\tFIN: " << int(recvShakeBuf[2]) << "\tSHAKE: " << int(recvShakeBuf[3]) << "\tcheckSum: " << int(recvShakeBuf[0]) << endl;

			if (newGetChecksum(recvShakeBuf, 4)==0 && recvShakeBuf[1] == 0x01 && recvShakeBuf[2] == 0x00 && recvShakeBuf[3] == SHAKE3) {
				// �յ����ְ���ȷ
				cout << "\n���ӽ����ɹ�!\n\n";
				return true;
			}
		}
		else {
			cout << "�յ������ݰ����ԣ����µȴ�����\n";
			continue;
		}
	}
}

static void waveHand() {
	// ���֣������ξ�����
	char recvWaveBuf[4];
	while (true) {
		while (recvfrom(serverSocket, recvWaveBuf, 4, 0, (sockaddr*)&clientAddr, &sockaddrLen) == SOCKET_ERROR) {

		}

		cout << "�յ��Ͽ����ӵ��������ݰ�Ϊ��\n";
		cout << "SYN: " << int(recvWaveBuf[1]) << "\tFIN: " << int(recvWaveBuf[2]) << "\tSHAKE: " << int(recvWaveBuf[3]) << "\tcheckSum: " << int(recvWaveBuf[0]) << endl;

		// ��һ��
		if (newGetChecksum(recvWaveBuf, 4)==0 && recvWaveBuf[1] == 0x00 && recvWaveBuf[2] == 0x01 && recvWaveBuf[3] == WAVE1) {
			// �յ����ְ���ȷ
			cout << "���ְ���ȷ\n";
			// �����ڶ��λ��ְ�
			char secondShake[4];
			secondShake[1] = 0x00;
			secondShake[2] = 0x01;
			secondShake[3] = WAVE2;
			secondShake[0] = newGetChecksum(secondShake+1, 4-1);
			// ���͵ڶ��λ��ְ�
			sendto(serverSocket, secondShake, 4, 0, (SOCKADDR*)&clientAddr, sockaddrLen);
			// �������
			cout << "���͸��ͻ��˵ĵڶ��λ��֣�\n";
			cout << "SYN: " << int(secondShake[1]) << "\tFIN: " << int(secondShake[2]) << "\tSHAKE: " << int(secondShake[3]) << "\tcheckSum: " << int(secondShake[0]) << endl;

			memset(secondShake, 0, 4);
			memset(recvWaveBuf, 0, 4);
			break;
		}
	}
}

static void recvFile() {
	// �����ļ�
	// ����������
	char recvBuf[1024];
	memset(recvBuf, 0, 1024);

	// �����洢ѡ�񲿷ֵ�vec
	vector<char*> srUnsure;

	// ��������ʹ�õĻ�����
	char sendBuf[4];
	memset(sendBuf, 0, 4);

	// ��¼���ֺ����ֳ���
	unsigned short int nameLen = 0;
	char* fileName;

	// ��¼�յ���seq
	unsigned int seq;

	// ͣ�ȣ�������
	while (true) {
		// ���յ��ĵ�һ����������
		while (recvfrom(serverSocket, recvBuf, 1024, 0, (sockaddr*)&clientAddr, &sockaddrLen) == SOCKET_ERROR) {

		}

		// ����seq
		seq = 0;
		seq = unsigned int(recvBuf[1]);
		seq = (seq << 8) + unsigned int(recvBuf[2]);
		seq = (seq << 8) + unsigned int(recvBuf[3]);

		cout << "From C\t�յ������ݰ�Ϊ;\n";
		cout << "seq: " << seq << "\nlen��: " << int(recvBuf[4]) << "\tlen��: " << int(recvBuf[5])
			<< "\tchecksum: " << int(recvBuf[0]) << endl; 

		if (newGetChecksum(recvBuf, 1024)==0) {
			if (seq == currentNum) {
				// ��¼����
				// �������ֵĳ���
				unsigned short int temp = unsigned short int(recvBuf[4]);
				nameLen = (temp << 8) + unsigned short int(recvBuf[5]);
				// ��������
				fileName = new char[nameLen];
				for (int i = 0; i < nameLen; i++)
					fileName[i] = recvBuf[i + 7];

				// �鿴�Ƿ���Ҫ�ش�
				if (currentNum != 0 && currentNum % divenum == 0) {
					// ����Ҫ���ص����ݰ�
					sendBuf[1] = (currentNum >> 16) & 0xFF;
					sendBuf[2] = (currentNum >> 8) & 0xFF;
					sendBuf[3] = currentNum & 0xFF;
					sendBuf[0] = newGetChecksum(sendBuf + 1, 3);

					// �������ݰ�
					sendto(serverSocket, sendBuf, 4, 0, (SOCKADDR*)&clientAddr, sockaddrLen);

					cout << "TO C\t����ack���ݰ�\n";
					cout << "checksum: " << int(sendBuf[0]) << "\tack: " << currentNum << endl;
				}
				// �ڴ������ݰ�+1
				currentNum += 1;

				break;
			}
		}
	}

	// �����ֽ��д���
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
	cout << "�յ����ļ�����·��Ϊ��" << fileNameN << endl;

	ofstream toWriteFile;
	toWriteFile.open(fileNameN, ios::binary);
	if (!toWriteFile) {
		cout << "�ļ�����ʧ��\n";
		return;
	}

	// ��ʼ��ʱ
	int beginTime = clock();

	while (true) {
		while (recvfrom(serverSocket, recvBuf, 1024, 0, (sockaddr*)&clientAddr, &sockaddrLen) == SOCKET_ERROR) {
			if (clock() - beginTime > MAX_WAIT_TIME) {
				cout << "�������ȴ�ʱ�䣬�ش���";
				sendBuf[3] = currentNum-1 & 0xFF;
				sendBuf[2] = (currentNum-1 >> 8) & 0xFF;
				sendBuf[1] = (currentNum-1 >> 16) & 0xFF;
				sendBuf[0] = newGetChecksum(sendBuf + 1, 3);

				sendto(serverSocket, sendBuf, 4, 0, (SOCKADDR*)&clientAddr, sockaddrLen);
				cout << "checksum: " << int(sendBuf[0]) << "\tack: " << (currentNum-1) << endl;
				beginTime = clock();
			}
		}
		// ����seq
		seq = 0;
		seq = unsigned int(recvBuf[1]);
		seq = (seq << 8) + (unsigned int(recvBuf[2]) & 0xff);
		seq = (seq << 8) + (unsigned int(recvBuf[3]) & 0xff);

		cout << "From C\t�յ������ݰ�Ϊ;\n";
		cout << "seq: " << seq << "\tlen��: " << int(recvBuf[4]) << "\tlen��: " << int(recvBuf[5]) << "\tisLast: " << int(recvBuf[6]) << "\tchecksum: " << int(recvBuf[0]) << endl;

		if (newGetChecksum(recvBuf, 1024) == 0) { // У����ȷ
			if (currentNum == seq) {
				// ��ȡҪ��ȡ���ַ����ĳ���
				unsigned short int temp = unsigned short int(recvBuf[4]);
				temp = (temp << 8) + (unsigned short int(recvBuf[5]) & 0x00FF);

				// д���ļ�
				for (int i = 0; i < temp; i++) {
					toWriteFile << recvBuf[7 + i];
				}
				cout << "delivered:\t" << currentNum << endl;
				currentNum++;

				bool hasLargerThen = true;
				// ��vec����û�б������ģ�����У�����ѭ��д
				
				while (hasLargerThen && srUnsure.size()) {
					bool tempFlag = false;
					for (int tempPoint = 0; tempPoint < srUnsure.size(); tempPoint++) {
						// ��ȡseq
						unsigned int vecSeq = unsigned int(srUnsure[tempPoint][1] & 0xff);
						vecSeq = (vecSeq << 8) + unsigned int(srUnsure[tempPoint][2] & 0xff);
						vecSeq = (vecSeq << 8) + unsigned int(srUnsure[tempPoint][3] & 0xff);
						if (vecSeq == currentNum) {
							// ��ȡҪ��ȡ���ַ����ĳ���
							unsigned short int temp = unsigned short int(srUnsure[tempPoint][4]);
							temp = (temp << 8) + (unsigned short int(srUnsure[tempPoint][5]) & 0x00FF);
							//cout << "temp���ȣ�" << temp << endl << endl;

							// д���ļ�
							for (int i = 0; i < temp; i++) {
								toWriteFile << srUnsure[tempPoint][7 + i];
							}
							cout << "delivered:\t" << currentNum << endl;
							// ָ�����
							currentNum += 1;

							// vec�е���
							srUnsure.erase(srUnsure.begin() + tempPoint);
							tempPoint -= 1;
						}
						else if (vecSeq > currentNum) {
							hasLargerThen = true;
							tempFlag = true;
							break;
						}
						else if (vecSeq < currentNum) {
							// vec�е���
							srUnsure.erase(srUnsure.begin() + tempPoint);
							tempPoint -= 1;
						}
					}
					if (tempFlag)
						continue;
					else
						hasLargerThen = false;
				}
				

				// �鿴�Ƿ���Ҫ�ش�ack
				if (currentNum % divenum == 0 || int(recvBuf[6]) == 1) {
					// û���𻵣����ض�Ӧack��Ȼ��ack��ת
					sendBuf[1] = (currentNum-1 >> 16) & 0xFF;
					sendBuf[2] = (currentNum-1 >> 8) & 0xFF;
					sendBuf[3] = (currentNum-1) & 0xFF;
					sendBuf[0] = newGetChecksum(sendBuf + 1, 3);

					// �����ļ�
					sendto(serverSocket, sendBuf, 4, 0, (SOCKADDR*)&clientAddr, sockaddrLen);
				}

				beginTime = clock();

				cout << "TO C\t��ȷ�������ݰ�\n";
				cout << "checksum: " << int(sendBuf[0]) << "\tack: " << currentNum-1 << endl;

				// ȷ�ϵ�ǰ�յ��������һ��
				if (int(recvBuf[6]) == 1)
					break;
			}
			else {
				if (seq > currentNum) {
					// ���뵽vec��
					srUnsure.push_back(recvBuf);
				}
				else {
					// ���𻵣�������ȷ�Ϲ������һ��ack �ظ�ack������
					//sendBuf[1] = (currentNum-1 >> 16) & 0xFF;
					//sendBuf[2] = (currentNum-1 >> 8) & 0xFF;
					//sendBuf[3] = (currentNum-1) & 0xFF;
					//sendBuf[0] = newGetChecksum(sendBuf + 1, 3);

					//sendto(serverSocket, sendBuf, 4, 0, (SOCKADDR*)&clientAddr, sockaddrLen);

					//beginTime = clock();
					//cout << "TO C\t���յ������ݰ����𻵣�������Ҫ��ack\n";
					//cout << "check: " << int(sendBuf[0]) << "\tack: " << (currentNum-1) << endl;
					//continue;
				}
			}
		}
		else {
			// ���𻵣�����֮ǰ�Ѿ�ȷ�Ϲ���ack
			sendBuf[1] = (currentNum-1 >> 16) & 0xFF;
			sendBuf[2] = (currentNum-1 >> 8) & 0xFF;
			sendBuf[3] = (currentNum-1) & 0xFF;
			sendBuf[0] = newGetChecksum(sendBuf + 1, 3);

			sendto(serverSocket, sendBuf, 4, 0, (SOCKADDR*)&clientAddr, sockaddrLen);

			beginTime = clock();
			cout << "TO C\t���յ������ݰ����𻵣�������Ҫ��ack\n";
			cout << "check: " << int(sendBuf[0]) << "\tack: " << (currentNum-1) << endl;
			continue;
		}
	}

	// �ļ�������ϣ��ر��ļ�
	toWriteFile.close();

	cout << "�ļ�����ɹ�\n";
}

int serverMain() {
	// ����socket��
	WSADATA wsadata;
	if (WSAStartup(MAKEWORD(2, 2), &wsadata)) {
		//������س�����ͣ�������������Ϣ
		cout << "����socketʧ��\n";
		return -1;
	}

	// ��ʼ����Ϣ
	// server
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(SERVER_PORT);
	serverAddr.sin_addr.s_addr = inet_addr(ServerIP);
	// client
	clientAddr.sin_family = AF_INET;
	clientAddr.sin_port = htons(CLIENT_PORT);

	serverSocket = socket(AF_INET, SOCK_DGRAM, 0);
	if (serverSocket == INVALID_SOCKET) {
		cout << "�׽��ִ���ʧ��\n";
		WSACleanup();
		return -1;
	}

	//setsockopt(serverSocket, SOL_SOCKET, SO_RCVTIMEO, (char*)&TIMEOUT, sizeof(TIMEOUT));
	// �����Լ��ģ��Լ���IP��ֱ�ӹ̶���127��
	inet_pton(AF_INET, ServerIP, &serverAddr.sin_addr.s_addr);

	// ��ȡclient��IP
	cout << "��ȡclient��IP\n";
	getIP(ClientIP);
	inet_pton(AF_INET, ClientIP, &clientAddr.sin_addr.s_addr);

	setsockopt(serverSocket, SOL_SOCKET, SO_RCVTIMEO, (char*)&MAX_WAIT_TIME, sizeof(MAX_WAIT_TIME));

	// ��socket�͵�ַ
	if (bind(serverSocket, (SOCKADDR*)&serverAddr, sizeof(SOCKADDR)) == SOCKET_ERROR) {
		cout << "bindʧ��\n";
		WSACleanup();
		return -1;
	}

	cout << "�����������ɹ����ȴ����ӡ���\n";

	// �ȴ���������
	// ��������
	shakeHand();

	// �����ļ�
	recvFile();

	// ���ݴ�����ϣ��ȴ���һ������
	cout << "���ݴ�����ɣ��ȴ��ͻ����˳�����\n";
	// ���λ��֣�����
	waveHand();
	cout << "���ֽ�����BYE\n";

	return 0;
}