#include"server.h"
#include<iostream>
using namespace std;

int main() {
	cout << "/***************************/\n";
	cout << "/******\t������ 3.3\t************/\n";
	cout << "/***************************/\n";

	// ����������
	if (serverMain() == 0) {
		cout << "������������\n";
	}
	else {
		cout << "��������쳣\n";
	}
	system("pause");

	return 0;
}