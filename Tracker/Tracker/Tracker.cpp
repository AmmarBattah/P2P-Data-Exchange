#pragma once

#define _CRT_SECURE_NO_WARNINGS

#include "Thread.h"
#include "Tracker.h"

peer p[200];
TcpServer::TcpServer()
{
	WSADATA wsadata;
	if (WSAStartup(0x0202, &wsadata) != 0)
		TcpThread::err_sys("Starting WSAStartup() error\n");

	//Display name of local host
	if (gethostname(servername, HOSTNAME_LENGTH) != 0) //get the hostname
		TcpThread::err_sys("Get the host name error,exit");

	printf("Server: %s waiting to be contacted for Register/Request ...\n", servername);


	//Create the server socket
	if ((serverSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
		TcpThread::err_sys("Create socket error,exit");

	//Fill-in Server Port and Address info.
	ServerPort = REQUEST_PORT;
	memset(&ServerAddr, 0, sizeof(ServerAddr));      /* Zero out structure */
	ServerAddr.sin_family = AF_INET;                 /* Internet address family */
	ServerAddr.sin_addr.s_addr = htonl(INADDR_ANY);  /* Any incoming interface */
	ServerAddr.sin_port = htons(ServerPort);         /* Local port */

	//Bind the server socket
	if (bind(serverSock, (struct sockaddr *) &ServerAddr, sizeof(ServerAddr)) < 0)
		TcpThread::err_sys("Bind socket error,exit");

	//Successfull bind, now listen for Server requests.
	if (listen(serverSock, MAXPENDING) < 0)
		TcpThread::err_sys("Listen socket error,exit");
}

TcpServer::~TcpServer()
{
	WSACleanup();
}


void TcpServer::start()
{
	for (;;) /* Run forever */
	{
		/* Set the size of the result-value parameter */
		clientLen = sizeof(ClientAddr);

		/* Wait for a Server to connect */
		if ((clientSock = accept(serverSock, (struct sockaddr *) &ClientAddr,
			&clientLen)) < 0)
			TcpThread::err_sys("Accept Failed ,exit");

		/* Create a Thread for this new connection and run*/
		TcpThread * pt = new TcpThread(clientSock);
		pt->start();
	}
}

//////////////////////////////TcpThread Class //////////////////////////////////////////
void TcpThread::err_sys(char * fmt, ...)
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
unsigned long TcpThread::ResolveName(char name[])
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
int TcpThread::msg_recv(int sock, Msg * msg_ptr)
{
	int rbytes, n;

	for (rbytes = 0; rbytes < MSGHDRSIZE; rbytes += n)
	if ((n = recv(sock, (char *)msg_ptr + rbytes, MSGHDRSIZE - rbytes, 0)) <= 0)
		err_sys("Recv MSGHDR Error");

	for (rbytes = 0; rbytes < msg_ptr->length; rbytes += n)
	if ((n = recv(sock, (char *)msg_ptr->buffer + rbytes, msg_ptr->length - rbytes, 0)) <= 0)
		err_sys("Recevier Buffer Error");

	return msg_ptr->length;
}

/* msg_send returns the length of bytes in msg_ptr->buffer,which have been sent out successfully
*/
int TcpThread::msg_send(int sock, Msg * msg_ptr)
{
	int n;
	if ((n = send(sock, (char *)msg_ptr, MSGHDRSIZE + msg_ptr->length, 0)) != (MSGHDRSIZE + msg_ptr->length))
		err_sys("Send MSGHDRSIZE+length Error");
	return (n - MSGHDRSIZE);

}

void TcpThread::run() //cs: Server socket
{
	Resp * respp;//a pointer to response
	Req * reqp; //a pointer to the Request Packet
	Msg smsg, rmsg; //send_message receive_message
	struct _stat stat_buf;
	int result;

	if (msg_recv(cs, &rmsg) != rmsg.length)//receive type of request
		err_sys("Receive Req error,exit");

	reqp = (Req *)rmsg.buffer;
	if (rmsg.type == REGISTER){
		cout << "User / " << reqp->hostname << " \ Registering to network as peer " << n + 1 << endl;
		strcpy(p[n].hostname, reqp->hostname);//get hostname
		cout << p[n].hostname << "is the Host\n";
		p[n].status = true;//is on since registering
		p[n].requests = 0;//no requests yet
		p[n].s = 0;//zero files
		if (msg_recv(cs, &rmsg) != rmsg.length)//receive link speed
			err_sys("Receive Req error,exit");
		rmsg.type = RESP;
		rmsg.length = sizeof(Resp);
		respp = (Resp *)rmsg.buffer;
		p[n].speed = stod(respp->response);//save speed
		memset(rmsg.buffer, 0, sizeof(rmsg.buffer));
		if (msg_recv(cs, &rmsg) != rmsg.length)//receive number of files
			err_sys("Receive Req error,exit");
		int nofiles = stoi(respp->response);
		p[n].s = nofiles;//save number of files
		for (int i = 0; i < nofiles; i++){//loop till u get all files
			memset(rmsg.buffer, 0, sizeof(rmsg.buffer));
			if (msg_recv(cs, &rmsg) != rmsg.length)
				err_sys("Receive Req error,exit");
			char cstr[RESP_LENGTH] = "";//char pointer to get the response 
			sprintf(cstr, "%s", respp->response);
			p[n].files[i] = (string)cstr;//copy response to struct
			cout << "File " << p[n].files[i] << " Received\n";
		}
		memset(rmsg.buffer, 0, sizeof(rmsg.buffer));
		cout << p[n].hostname << " has been registered with attributes:\n";//display attributes of registered peer
		cout << "Has " << p[n].s << " files which are: \n";
		for (int i = 0; i < p[n].s; i++){
			cout << i + 1 << ". " << p[n].files[i] << endl;
		}
		cout << "With speed of " << p[n].speed << "\n\n\n\n";
		n++;//number of peers incremented
	} else if (rmsg.type == REQUEST){//peer requests file
		smsg.type = RESP;
		smsg.length = sizeof(Resp);
		respp = (Resp *)smsg.buffer;
		cout << "User \"" << reqp->hostname << "\" requested  file " << reqp->filename << " to be downloaded. \n";
		int fp[50];
		int k = 0;//keep track of peer matches
		char cstr[RESP_LENGTH] = "";//char pointer to get the response 
		sprintf(cstr, "%s", reqp->filename);//save requested file name
		string filename = (string)cstr;
		for (int i = 0; i < n; i++){//go through each peer
			for (int j = 0; j < p[i].s; j++){//go through each file on peer
				if (p[i].files[j] == filename) {//if requestedd file matches file on a certain peer
					fp[k] = i;//save the index of peer in the struct array
					k++;//update index that keeps track of indexes
					break;//asumming that there is one copy of the file...no need to search other files
				}
			}
		}
		cout << "There are " << k << " matches, out of " << n << " total peers\n";//display number of matches
		sprintf(respp->response, "%d, ", k);
		if (msg_send(cs, &smsg) != smsg.length)// send out number of peer matches
			err_sys("send Respose failed,exit");
		memset(smsg.buffer, 0, sizeof(smsg.buffer));
		if (k != 0){
			for (int e = 0; e < k; e++){
				int r = fp[e];
				cout << p[r].hostname << endl;
				sprintf(respp->response, "%s", p[r].hostname);
				if (msg_send(cs, &smsg) != smsg.length)// send out peers that have specified file
					err_sys("send Respose failed,exit");
				memset(smsg.buffer, 0, sizeof(smsg.buffer));
				sprintf(respp->response, "%f", p[r].speed);
				if (msg_send(cs, &smsg) != smsg.length)// send out speed of each peer
					err_sys("send Respose failed,exit");
				memset(smsg.buffer, 0, sizeof(smsg.buffer));
			}
			char cstr[RESP_LENGTH] = "";//char pointer to get the response 
			sprintf(cstr, "%s", reqp->hostname);
			string hostname = (string)cstr;
			for (int i = 0; i < n; i++){//loop to  to update the number of requests of requesting host
				if (p[i].hostname == hostname){//if saved hostname matches current requesting host,update requests variable
					p[i].files[(p[i].s)] = filename;
					p[i].requests++;
					cout << reqp->hostname << " Has now requested " << p[i].requests << " times.\n";//display number of requests
					cout << p[i].files[(p[i].s)] << " will be added to list of files available at " << hostname << "\n\n\n\n\n";
					p[i].s++;//update number of files
					break;
				}
			}
		}
	}else{
		cout << "Wrong operation\n";
	}
	closesocket(cs);

}



////////////////////////////////////////////////////////////////////////////////////////

int main(void)
{

	TcpServer ts;
	ts.start();
	system("pause");
	return 0;
}
