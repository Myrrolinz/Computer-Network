#ifndef _TOOLS
#define _TOOLS

using namespace std;

static const int BUF_SIZE = 1024;

struct Package {
	// 数据包的结构
	unsigned int len; // 当前数据包中message部分的长度
	unsigned int flag; // flag对应下面的三次挥手和四次握手；如果是正常的传输文件，这里就不用看，设置为0
	unsigned int num; // num表示数据包的编号，也就是seq
	int isLast; // 一个标志位，表示当前是不是最后一个数据包
	int SYN;
	int FIN;
	unsigned int ACK;
	unsigned int checksum;
	unsigned int total; // 表示本次传送的文件总共有多少个数据包
	int fileName; // 存放文件的名字，这里简单起见用序号表示
	int fileTpye; // 存放文件的类型，0是jpg，1是txt
	char message[BUF_SIZE];
	//char rawData[sizeof(unsigned int) * 4 + sizeof(bool) * 3 + BUF_SIZE]; // 这个地方用来存放初始时从远端接收来的数据
};

// 一些都会用到的函数
extern unsigned int getChecksum(Package); // 计算校验和
extern void packageOutput(Package); // 按照一定格式输出包中的内容
extern void getIP(char*); // 获取输入的IP的地址，用来给后面的进行设置
extern void getFileList(); // 获取测试样例的文件列表
extern unsigned char newGetChecksum(char*, int); // Char类型中计算校验和

// 三次握手的时候对应的flag
extern const unsigned char SHAKE1;
extern const unsigned char SHAKE2;
extern const unsigned char SHAKE3;
// 2次挥手的时候的flag
extern const unsigned char WAVE1;
extern const unsigned char WAVE2;

// 双方会用的端口
extern const int CLIENT_PORT;
extern const int SERVER_PORT;

#endif