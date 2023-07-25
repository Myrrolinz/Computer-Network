#include"server.h"
#include<iostream>
using namespace std;

int main() {
	cout << "/***************************/\n";
	cout << "/******\t服务器 3.3\t************/\n";
	cout << "/***************************/\n";

	// 启动服务器
	if (serverMain() == 0) {
		cout << "服务正常结束\n";
	}
	else {
		cout << "服务出现异常\n";
	}
	system("pause");

	return 0;
}