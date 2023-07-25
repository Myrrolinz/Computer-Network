#ifndef _TOOLS
#define _TOOLS

using namespace std;

static const int BUF_SIZE = 1024;

struct Package {
	// ���ݰ��Ľṹ
	unsigned int len; // ��ǰ���ݰ���message���ֵĳ���
	unsigned int flag; // flag��Ӧ��������λ��ֺ��Ĵ����֣�����������Ĵ����ļ�������Ͳ��ÿ�������Ϊ0
	unsigned int num; // num��ʾ���ݰ��ı�ţ�Ҳ����seq
	int isLast; // һ����־λ����ʾ��ǰ�ǲ������һ�����ݰ�
	int SYN;
	int FIN;
	unsigned int ACK;
	unsigned int checksum;
	unsigned int total; // ��ʾ���δ��͵��ļ��ܹ��ж��ٸ����ݰ�
	int fileName; // ����ļ������֣�������������ű�ʾ
	int fileTpye; // ����ļ������ͣ�0��jpg��1��txt
	char message[BUF_SIZE];
	//char rawData[sizeof(unsigned int) * 4 + sizeof(bool) * 3 + BUF_SIZE]; // ����ط�������ų�ʼʱ��Զ�˽�����������
};

// һЩ�����õ��ĺ���
extern unsigned int getChecksum(Package); // ����У���
extern void packageOutput(Package); // ����һ����ʽ������е�����
extern void getIP(char*); // ��ȡ�����IP�ĵ�ַ������������Ľ�������
extern void getFileList(); // ��ȡ�����������ļ��б�
extern unsigned char newGetChecksum(char*, int); // Char�����м���У���

// �������ֵ�ʱ���Ӧ��flag
extern const unsigned char SHAKE1;
extern const unsigned char SHAKE2;
extern const unsigned char SHAKE3;
// 2�λ��ֵ�ʱ���flag
extern const unsigned char WAVE1;
extern const unsigned char WAVE2;

// ˫�����õĶ˿�
extern const int CLIENT_PORT;
extern const int SERVER_PORT;

#endif