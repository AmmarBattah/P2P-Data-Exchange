#pragma once

#ifndef SER_TCP_H
#define SER_TCP_H
#include <winsock.h>
#pragma comment(lib, "Ws2_32.lib")
#include <iostream>
using namespace std;
#include <windows.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <process.h>
#include <fstream>
#include <sstream>
#include <string>
#define HOSTNAME_LENGTH 20
#define RESP_LENGTH 40
#define FILENAME_LENGTH 20
#define REQUEST_PORT 5001
#define BUFFER_LENGTH 40 
#define MAXPENDING 10
#define MSGHDRSIZE 8 //Message Header Size


int n = 0;//keep count of number of peers

typedef enum{
	REGISTER, REQUEST, RESP //Message type
} Type;

typedef struct
{
	char hostname[HOSTNAME_LENGTH];
	char filename[FILENAME_LENGTH];
} Req;  //request

typedef struct
{
	char response[RESP_LENGTH];
} Resp; //response

struct peer
{
	char hostname[HOSTNAME_LENGTH];//retrieved automatically by function
	bool status;//is ready to exchange files or not
	double speed;
	int requests;//keeps track of number of requests
	int s;//number of files
	string files[100];//files peer has
};

typedef struct
{
	Type type;
	int  length; //length of effective bytes in the buffer
	char buffer[BUFFER_LENGTH];
} Msg; //message format used for sending and receiving


class TcpServer
{
	int serverSock, clientSock;     /* Socket descriptor for server and client*/
	struct sockaddr_in ClientAddr; /* Client address */
	struct sockaddr_in ServerAddr; /* Server address */
	unsigned short ServerPort;     /* Server port */
	int clientLen;            /* Length of Server address data structure */
	char servername[HOSTNAME_LENGTH];
	

public:
	TcpServer();
	~TcpServer();
	void TcpServer::start();
};

class TcpThread :public Thread
{

	int cs;
public:
	TcpThread(int clientsocket) :cs(clientsocket)
	{}
	virtual void run();
	int msg_recv(int, Msg *);
	int msg_send(int, Msg *);
	unsigned long ResolveName(char name[]);
	static void err_sys(char * fmt, ...);
};

#endif