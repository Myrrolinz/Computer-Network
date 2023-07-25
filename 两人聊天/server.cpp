#include"server.h"
#include"client.h"
using namespace std;

// ���ֿɱ仯��ȫ������֧���Զ���
// �������˿ں�
u_short ServerPort = 12260;
// ������IP��ַ
char ServerIP[16];
// ���建������С
const int SBUF_SIZE = 2048;
// ������������
const int ListrnMax = 5;
bool Sconnected = false;

// ȫ�ֱ�������
SOCKET ServerSocket, ClientSocket;// sever�������������գ�client��������յ�������
SOCKADDR_IN ServerAddr, ClientAddr;// �������IP��ַ�Ͷ˿�
int snaddr = sizeof(SOCKADDR_IN);
char SsendBuf[SBUF_SIZE];// ���ͻ�����
char SinputBuf[SBUF_SIZE];// ���뻺����
char SrecvBuf[SBUF_SIZE];//�������ݻ�����
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
		int result = recv(ClientSocket, SrecvBuf, SBUF_SIZE, 0); //recv(SOCKET s, char* buf, int len, int flags) flagsһ������Ϊ0
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

	// ����socket��
	WSADATA wsadata;
	if (WSAStartup(MAKEWORD(2, 2), &wsadata)) {//����ʹ��socket2.2�汾 makeword�ǽ�����byte�ͺϲ���һ��word�ͣ�һ���ڸ�8λ(b)��һ���ڵ�8λ(a)
		//������س�����ͣ�������������Ϣ
		cout << "����socketʧ��\n";
		system("pause");
		return -1;
	}

	// ����socket������˳��:IPv4����ʽ�׽��֣�ָ��Э��
	ServerSocket = socket(AF_INET, SOCK_STREAM, 0);

	// ��ʼ����ַ��Ϣ
	// inet_addr������IP��ַת���������ֽ���
	ServerAddr.sin_family = AF_INET;
	getServerIP(ServerIP);
	inet_pton(AF_INET, ServerIP, &ServerAddr.sin_addr.S_un.S_addr);//��IP��ַ���ַ�����ʽת���������ַ��ʽ��֧��Ipv4��Ipv6.
	//inet_pton src:�Ǹ�ָ�룬ָ�򱣴�IP��ַ�ַ�����ʽ���ַ�����dst:ָ���������ַ�Ľṹ����׵�ַ
	ServerPort = 12260; // ServerPort = getPort();
	//cin.get();
	ServerAddr.sin_port = htons(ServerPort);//��һ���޷��Ŷ����͵�������ֵת��Ϊ���� �ֽ�˳�򣬼���β˳��(big-endian)

	// ��socket�͵�ַ
	bind(ServerSocket, (SOCKADDR*)&ServerAddr, sizeof(SOCKADDR));

	cout << "Server start success. Your Server IP is: " << ServerIP << "\tYour port is: " << ServerPort << endl;
	// ��ʼѭ������
	while (1) {
		cout << "Witing for connection......\n";
		// ���ö��г���Ϊ5
		listen(ServerSocket, ListrnMax);

		// accept���������󣬲���һ��socket����´�����socket
		ClientSocket = accept(ServerSocket, (SOCKADDR*)&ClientAddr, &snaddr);//SOCKADDR_INǿ��ת����SOCKADDR
		// ����´�����socket�Ƿ���Ч
		if (ClientSocket != INVALID_SOCKET) {
			// ���ӳɹ�
			cout << "�ͻ������ӳɹ�\n";
			Sconnected = true;
			// ���߿ͻ������ӳɹ�
			// ��ȡ��ǰʱ��
			time_t now = time(0);
			// ת�����ַ�����ʽ
			char* dt = ctime(&now);
			memset(SsendBuf, 0, SBUF_SIZE);
			strcat(SsendBuf, dt);
			strcat(SsendBuf, hello);//�������ӳɹ�������hello��Ϣ
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

					// ��ȡ��ǰʱ��
					time_t now = time(0);
					// ת�����ַ�����ʽ
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