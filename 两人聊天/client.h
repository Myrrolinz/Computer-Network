#pragma once
#pragma warning(disable:4996)
// 链接ws32.lib库到此项目中
#pragma comment(lib,"ws2_32.lib")
#include<WinSock2.h>
#include<WS2tcpip.h>
#include<stdio.h>
#include<cstring>
using namespace std;

// client主功能函数
int clientMain();
// 获取IP地址
char* getIP();
unsigned int getLen(char*);
bool isQuit(char*, unsigned int);
char* mergeTime(char*);
void showMergeMessage(char*);
bool isRecvQuit(char*);