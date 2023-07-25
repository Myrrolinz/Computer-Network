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

// ȫ�ֱ�������
SOCKET RemoteSocket, LocalhostSocket;// sever�������������գ�client��������յ�������
SOCKADDR_IN RemoteAddr, LocahhostAddr;// �������IP��ַ�Ͷ˿�
int cnaddr = sizeof(SOCKADDR_IN);
char CsendBuf[CBUF_SIZE];// ���ͻ�����
char CinputBuf[CBUF_SIZE];// �������ݻ�����
char CrecvBuf[CBUF_SIZE];//�������ݻ�����
char userName[CBUF_SIZE];// �û�������
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

void showList(char* usrlist) {//��ӡ�����û���
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
		else if(isHello(CrecvBuf))//�û����ӳɹ� Connected!
			cout << CrecvBuf << endl;
		else if(CrecvBuf[0] == 'I' && CrecvBuf[1] == 'n' && CrecvBuf[2] == 'p')//input destination error
			cout << CrecvBuf << endl;
		else if(CrecvBuf[0] == 'u')//getUsrList
			showList(CrecvBuf);
		else if (isServerQuit(CrecvBuf)){//server quit
			cout << "Server has stopped this chat room, bye.\n";
			break;
		}
		else if (isClientQuit(CrecvBuf)){//����������
			int len = int(CrecvBuf[1]);
			char* quitUsr = new char[len + 1];
			for (int i = 0; i < len; i++)
				quitUsr[i] = CrecvBuf[i + 2];
			quitUsr[len] = '\0';
			cout << "User: " << quitUsr << " has left from sesstion.\n";
		}
		else {//�յ���ĳ�˷�������Ϣ����ӡ��Դ�û���ʱ�䡢��Ϣ
			char timeString[26];//ʱ��
			int index = 0;
			for (; index < 25; index++)
				timeString[index] = CrecvBuf[index];
			timeString[index] = '\0';
			int len = int(CrecvBuf[index]);
			index += 1;
			char* nameString = new char[len + 1];//��Դ�û�
			for (; index < 26 + len; index++)
				nameString[index - 26] = CrecvBuf[index];
			nameString[index - 26] = '\0';
			while (CrecvBuf[index] == '\0')
				index++;
			cout << "From user: " << nameString << "\tTime: " << timeString;
			while (CrecvBuf[index] != '\0') {//��ӡ��Ϣ
				cout << CrecvBuf[index];
				index++;
			}
			cout << endl;
		}
	}
	return 0;
}

bool ismyQuit(char* a) {//��������
	if (a[0] == 'q' && a[1] == 'u' && a[2] == 'i' && a[3] == 't')
		return true;
	else
		return false;
}

bool isClientQuit(char* a) {//ĳ������
	if (a[0] == 'q')
		return true;
	else
		return false;
}

bool isHello(char* a) {//���ӳɹ�
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
	inet_pton(AF_INET, DstIP, &RemoteAddr.sin_addr.S_un.S_addr);

	// ��ȡ�û������Լ�����
	cout << "Please input your name which will be displayed in our chat room.\n";
	char inputName[CBUF_SIZE];
	cin.getline(inputName, CBUF_SIZE);
	userName[0] = char(getLen(inputName));//userName[0]Ϊ������Ϣ��ǿ��ת��Ϊchar
	int indexOfInput = 0;
	while (inputName[indexOfInput] != '\0') {
		userName[indexOfInput + 1] = inputName[indexOfInput];
		indexOfInput++;
	}
	userName[indexOfInput + 2] = '\0';
	//��װ�û���Ϣ
	char* sendName = new char[int(userName[0]) + 4];//����ǿ��ת��
	sendName[0] = 'n';
	sendName[1] = userName[0];
	int l = userName[0];
	for (int ind = 0; ind < l; ind++)
		sendName[2 + ind] = userName[1 + ind];
	sendName[l + 3] = '\0';

	// ����Զ�̷�����
	if (connect(LocalhostSocket, (SOCKADDR*)&RemoteAddr, sizeof(RemoteAddr)) != SOCKET_ERROR) {
		// ���ӳɹ�
		// ����һ���Լ�����Ϣ�����֣�
		send(LocalhostSocket, sendName, int(userName[0] + 2), 0);
		// ������Ϣ
		HANDLE recv_thread = CreateThread(NULL, NULL, recvMessagec, NULL, 0, NULL);

		while (1) {//���߳����ڷ�����Ϣ
			memset(CinputBuf, 0, CBUF_SIZE);
			char input[CBUF_SIZE];
			cin.getline(input, CBUF_SIZE);

			if (!strcmp(input, "getUsrList\0")) {//�������getUsrList
				send(LocalhostSocket, input, 11, 0);
			}
			else {//�������quit
				if (ismyQuit(input)) {
					char tempQuit[CBUF_SIZE];
					memset(tempQuit, 0, CBUF_SIZE);
					tempQuit[0] = 'q';
					strcat(tempQuit, userName);
					strcat(tempQuit, "quit\0");
					send(LocalhostSocket, tempQuit, sizeof(tempQuit), 0);
					cout << "Thanks for using. Bye.\n";
					closesocket(LocalhostSocket);//�ر�socket
					closesocket(RemoteSocket);
					WSACleanup();
					system("pause");
					return 0;
				}
				else {
					int i = 0;
					while (input[i] != ':'&&input[i]!='\0')
						i++;
					
					char* dstUser = new char[i + 2];//��ȡĿ�ĵ�name
					dstUser[0] = char(i);
					for (int j = 1; j <= i; j++)
						dstUser[j] = input[j - 1];
					dstUser[i + 1] = '\0';

					i += 1;
					int j = i;
					while (input[i] != '\0') {
						CinputBuf[i - j] = input[i];//��ȡ����name�������������Ϣ
						i += 1;
					}
					CinputBuf[i - j] = '\0';

					// ��ȡ��ǰʱ��
					time_t now = time(0);
					// ת�����ַ�����ʽ
					char* dt = ctime(&now);

					memset(CsendBuf, 0, CBUF_SIZE);
					// ����ʱ����Ϣ
					strcat(CsendBuf, dt);
					// ����Ŀ��������Ϣ
					strcat(CsendBuf, dstUser);
					// �����Լ�������Ϣ
					strcat(CsendBuf, userName);
					// �������������
					strcat(CsendBuf, CinputBuf);
					// ������Ϣ
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