#include"client.h"
#include<iostream>
#include<cstring>
#include<ctime>
#include<processthreadsapi.h>

// ���ֿɱ仯��ȫ������֧���Զ���
// �ͻ��˶˿ں�
u_short ClientPort = 12870;
// ����IP��ַ
const char* LocalIP = "127.0.0.1";
// ���建������С
const int CBUF_SIZE = 2048;
bool connected = false;

// ȫ�ֱ�������
SOCKET RemoteSocket, LocalhostSocket;// sever�������������գ�client��������յ�������
SOCKADDR_IN RemoteAddr, LocahhostAddr;// �������IP��ַ�Ͷ˿�
int cnaddr = sizeof(SOCKADDR_IN);
char CsendBuf[CBUF_SIZE];// ���ͻ�����
char CinputBuf[CBUF_SIZE];// �������ݻ�����
char CrecvBuf[CBUF_SIZE];//�������ݻ�����

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
		::memset(CrecvBuf, 0, CBUF_SIZE);//��ս������ݻ�����
		//recv(SOCKET s, char* buf, int len, int flags) flagsһ������Ϊ0
		if (recv(LocalhostSocket, CrecvBuf, CBUF_SIZE, 0) < 0) {//ʹ��recv�������ܽ�����Ϣ
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
				cout << "From Server, at " << CrecvBuf << endl;//��ʾ���յ���Ϣ
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
	// ����socket��
	WSADATA wsadata;
	if (WSAStartup(MAKEWORD(2, 2), &wsadata) != 0) {
		//������س�����ͣ�������������Ϣ
		cout << "����socketʧ��\n";
		system("pause");
		return -1;
	}

	// ����socket������˳��:IPV4����ʽ�׽��֣�ָ��Э��
	LocalhostSocket = socket(AF_INET, SOCK_STREAM, 0);
	// ���socket�����Ƿ�ɹ�
	if (LocalhostSocket == INVALID_SOCKET) {
		cout << "Socket����ʧ�ܣ��������Ϊ��" << WSAGetLastError() << endl;
		WSACleanup();
		return -2;
	}

	// �׽��ְ󶨱��ص�ַ��Ϣ
	LocahhostAddr.sin_family = AF_INET;
	LocahhostAddr.sin_port = htons(ClientPort);
	inet_pton(AF_INET, LocalIP, &LocahhostAddr.sin_addr.S_un.S_addr);


	// ��������Ŀ���ַ��Ϣ��
	RemoteAddr.sin_family = AF_INET;
	cout << "Which IP would you wanna connect:\n";
	char* DstIP = getIP();
	cin.get();
	RemoteAddr.sin_port = htons(12260);
	inet_pton(AF_INET, DstIP, &RemoteAddr.sin_addr.S_un.S_addr);//��һ���޷��Ŷ����͵�������ֵת��Ϊ���� �ֽ�˳�򣬼���β˳��(big-endian)


	// ����Զ�̷�����
	if (connect(LocalhostSocket, (SOCKADDR*)&RemoteAddr, sizeof(RemoteAddr)) != SOCKET_ERROR) {
		// ���ӳɹ�
		// ������Ϣ
		HANDLE recv_thread = CreateThread(NULL, NULL, recvMessagec, NULL, 0, NULL);
		connected = true;

		while (1) {
			if (!connected)
				break;
			memset(CinputBuf, 0, CBUF_SIZE);
			cin.getline(CinputBuf, CBUF_SIZE);

			// ��ȡ��ǰʱ��
			time_t now = time(0);
			// ת�����ַ�����ʽ
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