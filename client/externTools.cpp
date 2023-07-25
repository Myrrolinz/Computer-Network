#include"externTools.h"
#include<iostream>
#include<cstring>
#include<io.h>
#include<stdio.h>

unsigned int getChecksum(Package p) {
	unsigned int sum = 0;
	// 这里的校验和仅仅是简单的将内部除了message之外的所有数据加起来
	sum = sum + p.len + p.flag + p.num + p.SYN + p.FIN + p.ACK + p.isLast + p.fileName + p.fileTpye;
	return sum;
}

void packageOutput(Package p) {
	cout << "messageLen: " << p.len << "\tflag: " << p.flag << "\tnum(seq): " << p.num<< "\tisLast: "<< p.isLast
		<< "\nSYN: " << p.SYN << "\tFIN: " << p.FIN << "\tACK: " << p.ACK
		<< "\nchecksum: " << p.checksum << endl;
}

void getIP(char* p) {
	// 获取本次服务时server的IP地址
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

	// 建立句柄
	long handle;
	struct _finddata_t fileinfo;
	
	// 第一个文件
	handle = _findfirst(path.c_str(), &fileinfo);
	// 判断句柄状态
	if (handle == -1) {
		cout << "文件名称出现错误\n";
		return;
	}
	else {
		cout << "测试文件夹下目录列表为：\n";
		int tempNumOfFile = 0;
		do
		{
			//找到的文件的文件名
			if (fileinfo.name[0] == '.')
				continue;
			cout << ++tempNumOfFile << ": " << fileinfo.name << endl;

		} while (!_findnext(handle, &fileinfo));
		_findclose(handle);
		return;
	}
}

/// <summary>
/// 计算校验和，从第1位开始算，一直算到最后一位
/// </summary>
/// <param name="pac">计算的包</param>
/// <param name="len">包的长度</param>
/// <returns>返回校验和</returns>
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