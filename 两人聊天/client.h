#pragma once
#pragma warning(disable:4996)
// ����ws32.lib�⵽����Ŀ��
#pragma comment(lib,"ws2_32.lib")
#include<WinSock2.h>
#include<WS2tcpip.h>
#include<stdio.h>
#include<cstring>
using namespace std;

// client�����ܺ���
int clientMain();
// ��ȡIP��ַ
char* getIP();
unsigned int getLen(char*);
bool isQuit(char*, unsigned int);
char* mergeTime(char*);
void showMergeMessage(char*);
bool isRecvQuit(char*);