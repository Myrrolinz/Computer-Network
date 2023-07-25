#include"externTools.h"
#include<iostream>
#include<cstring>
#include<io.h>
#include<stdio.h>


unsigned int getChecksum(Package p) {
	unsigned int sum = 0;
	// �����У��ͽ����Ǽ򵥵Ľ��ڲ�����message֮����������ݼ�����
	sum = sum + p.len + p.flag + p.num + p.SYN + p.FIN + p.ACK + p.isLast + p.fileName + p.fileTpye;
	return sum;
}

void packageOutput(Package p) {
	cout << "messageLen: " << p.len << "\tflag: " << p.flag << "\tnum(seq): " << p.num << "\tisLast: " << p.isLast
		<< "\nSYN: " << p.SYN << "\tFIN: " << p.FIN << "\tACK: " << p.ACK
		<< "\nchecksum: " << p.checksum << endl;
}

void getIP(char* p) {
	// ��ȡ���η���ʱserver��IP��ַ
	cout << "Please use 'ipconfig' to get your ip and input\n";
	char ip[16];
	cin.getline(ip, sizeof(ip));
	int index = 0;
	while (ip[index] != '\0') {
		p[index] = ip[index];
		index++;
	}
}

void getFileList() {
	string path = "./test/*";

	// �������
	long handle;
	struct _finddata_t fileinfo;

	// ��һ���ļ�
	handle = _findfirst(path.c_str(), &fileinfo);
	// �жϾ��״̬
	if (handle == -1) {
		cout << "�ļ����Ƴ��ִ���\n";
		return;
	}
	else {
		cout << "�����ļ�����Ŀ¼�б�Ϊ��\n";
		int tempNumOfFile = 0;
		do
		{
			//�ҵ����ļ����ļ���
			if (fileinfo.name[0] == '.')
				continue;
			cout << ++tempNumOfFile << ": " << fileinfo.name << endl;

		} while (!_findnext(handle, &fileinfo));
		_findclose(handle);
		return;
	}
}

/// <summary>
/// ����У��ͣ��ӵ�1λ��ʼ�㣬һֱ�㵽���һλ
/// </summary>
/// <param name="pac">����İ�</param>
/// <param name="len">���ĳ���</param>
/// <returns>����У���</returns>
unsigned char newGetChecksum(char* package, int len) {
	if (len == 0) {
		return ~(0);
	}
	unsigned int sum = 0;
	int i = 0;
	while (len--) {
		sum += (unsigned char)package[i++];
		if (sum & 0xFF00) {
			sum &= 0x00FF;
			sum++;
		}
	}
	return ~(sum & 0x00FF);
}