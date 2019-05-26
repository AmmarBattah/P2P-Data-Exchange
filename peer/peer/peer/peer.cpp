#define _CRT_SECURE_NO_WARNINGS
#include <winsock.h>
#pragma comment(lib, "Ws2_32.lib")
#include <stdio.h>
#include <iostream>
#include <string.h>
#include <windows.h>
#include<string>
#include <fstream>
#include <sys/types.h>
#include <sys/stat.h>
using namespace std;

#include<string>

#define HOSTNAME_LENGTH 20
#define RESP_LENGTH 40
#define FILENAME_LENGTH 20
#define REQUEST_PORT 5001
#define BUFFER_LENGTH 40
#define TRACE 0
#define MSGHDRSIZE 8 //Message Header Size
int n = 0;//keep count of peers
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


typedef struct
{
	Type type;
	int  length; //length of effective bytes in the buffer
	char buffer[BUFFER_LENGTH];
} Msg; //message format used for sending and receiving

/*typedef struct
{
	char hostname[HOSTNAME_LENGTH];//retrieved automatically by function
	bool status;//is ready to exchange files or not
	string files[100];//files peer has
	double speed;
	int requests;//keeps track of number of requests
	int s;//number of files
}peer;*/
class TcpClient
{
	int sock;                    /* Socket descriptor */
	struct sockaddr_in ServAddr; /* server socket address */
	unsigned short ServPort;     /* server port */
	Req * reqp,* reqp2;               /* pointer to request */
	Resp * respp;          /* pointer to response*/
	Msg smsg, rmsg;               /* receive_message and send_message */
	int result;
	WSADATA wsadata;
	struct _stat stat_buf;

public:
	TcpClient(){}
	void run(int argc, char * argv[]);
	~TcpClient();
	int msg_recv(int, Msg *);
	int msg_send(int, Msg *);
	unsigned long ResolveName(char name[]);
	void err_sys(char * fmt, ...);

};


void TcpClient::run(int argc, char * argv[])
{
	if (argc != 4)
		err_sys("usage: client servername filename request/register\n ");

	//initilize winsocket
	if (WSAStartup(0x0202, &wsadata) != 0)
	{
		WSACleanup();
		err_sys("Error in starting WSAStartup()\n");
	}


	reqp = (Req *)smsg.buffer;

	//Display name of local host and copy it to the req
	if (gethostname(reqp->hostname, HOSTNAME_LENGTH) != 0) //get the hostname
		err_sys("can not get the host name,program exit");
	strcpy(reqp->filename, argv[2]);
	//printf("%s%s\n", " Request initiated by ", reqp->hostname);
	cout << "Request initiated by " << reqp->hostname << "to find file " << reqp->filename << endl;

	strcpy(reqp->filename, argv[2]);
	//cout << "1.Register new peer\n 2.Request a file\n";
	if (strcmp(argv[3], "register") == 0)
		smsg.type = REGISTER;
	else if (strcmp(argv[3], "request") == 0)
		smsg.type = REQUEST;
	else err_sys("Wrong request type\n");
	//Create the socket
	if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) //create the socket 
		err_sys("Socket Creating Error");

	//connect to the server
	ServPort = REQUEST_PORT;
	memset(&ServAddr, 0, sizeof(ServAddr));     /* Zero out structure */
	ServAddr.sin_family = AF_INET;             /* Internet address family */
	ServAddr.sin_addr.s_addr = ResolveName(argv[1]);   /* Server IP address */
	ServAddr.sin_port = htons(ServPort); /* Server port */
	if (connect(sock, (struct sockaddr *) &ServAddr, sizeof(ServAddr)) < 0)
		err_sys("Socket Creating Error");

	//send out the message
	smsg.length = sizeof(Req);
	fprintf(stdout, "Sent request to %s, waiting...\n", argv[1]);
	if (msg_send(sock, &smsg) != sizeof(Req))//sending request
		err_sys("Sending req packet error.,exit");
	if (smsg.type == REGISTER){/////////////////////////////////////////////////////////register peer
		cout << "Please enter the speed of transmission\n";//prompt peer to provide speed
		double speed;
		cin >> speed;
		respp = (Resp *)smsg.buffer;
		//smsg.type = RESP;
		smsg.length = sizeof(Resp);
		sprintf(respp->response, "%f, ", speed);//put speed to the repponse structure
		if (msg_send(sock, &smsg) != smsg.length)//send speed
			err_sys("send Response failed,exit");
		memset(smsg.buffer, 0, sizeof(smsg.buffer));
		cout << "what is the number of files on the peer?\n";//prompt peer to provide number of files
		int nofiles;
		cin >> nofiles;
		sprintf(respp->response, "%d, ", nofiles);//put speed to the response structure
		if (msg_send(sock, &smsg) != smsg.length)//send number of files
			err_sys("send Response failed,exit");
		memset(smsg.buffer, 0, sizeof(smsg.buffer));
		for (int i = 0; i < nofiles; i++){//loop to enter files
			cout << "Enter filename " << i + 1 << " :\n";//prompt peer to provide file name
			char fname[RESP_LENGTH];
			cin >> fname;
			sprintf(respp->response, "%s", fname);//put file name to the repponse structure
			if (msg_send(sock, &smsg) != smsg.length)//send file name
				err_sys("send Response failed,exit");
			memset(smsg.buffer, 0, sizeof(smsg.buffer));
		}
		cout << "You have been registered to the network\n\n\n";//notify peer that he has been registered
	}
	else if (smsg.type == REQUEST){////////////////////////////////////////////////////////////////////peer request
			cout << "Would you like to automatically connect to the fastest peer?[y/n]\n";//ask user if he wants to automatically connect to fastest peer
			char am;
			cin >> am;//save option
			if (msg_recv(sock, &rmsg) != rmsg.length)//receive number of peers that have the requested file
				err_sys("Receive Req error,exit");
			respp = (Resp *)rmsg.buffer;
			int nopeers = stoi(respp->response);//save number of peers
			memset(rmsg.buffer, 0, sizeof(rmsg.buffer));
			if (nopeers == 0){//if no matches then stops
				cout << "No matches, cant download file\n";
			}
			else{//if there are matches
				cout << "There are " << nopeers << " peers that have the file\n";//display number of peer matches
				double speedmax;//variable to hold fastest speed
				string hostmax;//variable to hold hostname with fastest speed
				char cstr[RESP_LENGTH] = "";
				for (int i = 0; i < nopeers; i++){//loop to receive matched peers and their speeds while also keeping track of host with fastest speed
					if (msg_recv(sock, &rmsg) != rmsg.length)//receive hostname
						err_sys("Receive Req error,exit");
					strcpy(cstr, respp->response);//save hostname
					cout << i + 1 << ". " << cstr << endl;//display hostname
					memset(rmsg.buffer, 0, sizeof(rmsg.buffer));
					if (msg_recv(sock, &rmsg) != rmsg.length)//receive hostspeed
						err_sys("Receive Req error,exit");
					double speed = stod(respp->response);//save speed
					memset(rmsg.buffer, 0, sizeof(rmsg.buffer));
					if (i == 0){// if first host then set it to host with max speed
						speedmax = speed;
						hostmax = (string)cstr;
					}
					if (speed > speedmax){//if current host faster then max host, then current host becomes max host
						speedmax = speed;
						hostmax = (string)cstr;
					}

				}
				memset(smsg.buffer, 0, sizeof(smsg.buffer));
				smsg.type = REQUEST;
				smsg.length = sizeof(Req);
				reqp2 = (Req *)smsg.buffer;
				const char *hostmax1 = (hostmax.c_str());
				if (am == 'y'){//if peer chose the automatic option
					cout << "You are being connected to the fastest peer " << hostmax << endl;
					strcpy(reqp2->hostname, hostmax1);//save hostname to request structure
				}
				else{//if host selectedd manual option
					cout << "Please enter the hostname you would like to download from\n";
					cin >> reqp2->hostname;//save hostname to request structure
				}
				char host[RESP_LENGTH];
				strcpy(host, reqp2->hostname);//saved to a variable to be used by function later on
				cout << host;
				strcpy(reqp2->filename, argv[2]);//save requestedd file to request structure
				//assignment 1 code + new socket connection
				//Create the socket
				if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) //create the socket 
					err_sys("Socket Creating Error");

				//connect to the server
				ServPort = REQUEST_PORT;
				memset(&ServAddr, 0, sizeof(ServAddr));     /* Zero out structure */
				ServAddr.sin_family = AF_INET;             /* Internet address family */
				ServAddr.sin_addr.s_addr = ResolveName(host);   /* Server IP address */
				ServAddr.sin_port = htons(ServPort); /* Server port */
				if (connect(sock, (struct sockaddr *) &ServAddr, sizeof(ServAddr)) < 0)
					err_sys("Socket Creating Error");


				fprintf(stdout, "Sent request to %s to get file %s, waiting...\n", reqp2->hostname, reqp->filename);
				if (msg_send(sock, &smsg) != sizeof(Req))//sending request
					err_sys("Sending req packet error.,exit");

				ofstream fout((reqp2->filename), std::ofstream::binary);//save to file in binary format
				if (msg_recv(sock, &rmsg) != rmsg.length)//receive file size
					err_sys("recv response error,exit");
				respp = (Resp *)rmsg.buffer;
				int size = stoi(respp->response);//save file size
				while (size > 0)//keep looping till all bytes written to file
				{
					if (size >= BUFFER_LENGTH){//if the data left is greater than buffer keep writing to file
						if (msg_recv(sock, &rmsg) != rmsg.length)//receive chunk
							err_sys("recv response error,exit");
						respp = (Resp *)rmsg.buffer;
						fout.write(respp->response, BUFFER_LENGTH);//write chunk to file
						size -= BUFFER_LENGTH;
					}else{//if the data left is less than buffer keep writing to file but decrement by the data left not buffer size
						if (msg_recv(sock, &rmsg) != rmsg.length)
							err_sys("recv response error,exit");
						respp = (Resp *)rmsg.buffer;
						fout.write(respp->response, size);
						size = 0;
					}
				}
				fout.close();
				printf("File was recieved from %s \n\n\n", reqp2->hostname);
			}
	}
	closesocket(sock);

}
TcpClient::~TcpClient()
{
	/* When done uninstall winsock.dll (WSACleanup()) and exit */
	WSACleanup();
}


void TcpClient::err_sys(char * fmt, ...) //from Richard Stevens's source code
{
	perror(NULL);
	va_list args;
	va_start(args, fmt);
	fprintf(stderr, "error: ");
	vfprintf(stderr, fmt, args);
	fprintf(stderr, "\n");
	va_end(args);
	exit(1);
}

unsigned long TcpClient::ResolveName(char name[])
{
	struct hostent *host;            /* Structure containing host information */

	if ((host = gethostbyname(name)) == NULL)
		err_sys("gethostbyname() failed");

	/* Return the binary, network byte ordered address */
	return *((unsigned long *)host->h_addr_list[0]);
}

/*
msg_recv returns the length of bytes in the msg_ptr->buffer,which have been recevied successfully.
*/
int TcpClient::msg_recv(int sock, Msg * msg_ptr)
{
	int rbytes, n;

	for (rbytes = 0; rbytes<MSGHDRSIZE; rbytes += n)
	if ((n = recv(sock, (char *)msg_ptr + rbytes, MSGHDRSIZE - rbytes, 0)) <= 0)
		err_sys("Recv MSGHDR Error");

	for (rbytes = 0; rbytes<msg_ptr->length; rbytes += n)
	if ((n = recv(sock, (char *)msg_ptr->buffer + rbytes, msg_ptr->length - rbytes, 0)) <= 0)
		err_sys("Recevier Buffer Error");

	return msg_ptr->length;
}

/* msg_send returns the length of bytes in msg_ptr->buffer,which have been sent out successfully
*/
int TcpClient::msg_send(int sock, Msg * msg_ptr)
{
	int n;
	if ((n = send(sock, (char *)msg_ptr, MSGHDRSIZE + msg_ptr->length, 0)) != (MSGHDRSIZE + msg_ptr->length))
		err_sys("Send MSGHDRSIZE+length Error");
	return (n - MSGHDRSIZE);

}

int main(int argc, char *argv[]) //argv[1]=servername argv[2]=filename argv[3]=time/size
{



	TcpClient * tc = new TcpClient();
	tc->run(argc, argv);
	system("pause");
	return 0;
}
