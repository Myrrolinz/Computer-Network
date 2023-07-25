#pragma once
#pragma warning(disable:4996)
// 链接ws32.lib库到此项目中
#pragma comment(lib,"ws2_32.lib")
#include<WinSock2.h>
#include<WS2tcpip.h>
#include<stdio.h>
#include<winsock.h>
#include<iostream>
#include<cstring>

// server端主要功能函数
int serverMain();
// 选择想要用来通讯的端口，为0则选默认值
int getPort();