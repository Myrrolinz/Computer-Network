#pragma once
#pragma warning(disable:4996)
// ����ws32.lib�⵽����Ŀ��
#pragma comment(lib,"ws2_32.lib")
#include<WinSock2.h>
#include<WS2tcpip.h>
#include<winsock.h>
#include<stdio.h>
#include<iostream>
#include<cstring>

// server����Ҫ���ܺ���
int serverMain();
// ѡ����Ҫ����ͨѶ�Ķ˿ڣ�Ϊ0��ѡĬ��ֵ
int getPort();