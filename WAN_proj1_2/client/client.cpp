#include "RPCSocket.h"
#include "RPCSocket.cpp"
#include <iostream>
#include <cstdlib>
#include <list>
#include <map>
#include <pthread.h>
#include <unistd.h>
#include <cstring>
#include <fstream>
#include <vector>
#include <algorithm>
#include <limits>
#include <sys/time.h>
using namespace std;
string NamePath ="";
string ServerPath = "";
const unsigned int RECEIVE_BUFFER_SIZE = 1024;


string NSAddress ="";
unsigned short NSPort;
unsigned short GlobalPort;
string Gport;
string serverAddress;
unsigned short serverPort;
string serport;
string GlobalAddress;

string IPlookup();
void fetchRouters(string, unsigned short);
int registerInRouter(string, unsigned short, unsigned short);
void *receive_part(void *);
int index(string, string, unsigned short);


struct timeval start,end;
double t1,t2;
		
//Returns The IP Address Of The Machine
string IPlookup()
{
	  struct addrinfo hints, *res;
	  int errcode;
	  char addrstr[100];
	  void *ptr;
		string rd;
	  char hostname[1024];

	  hostname[1023] = '\0';
	  gethostname(hostname, 1023);

	  memset (&hints, 0, sizeof (hints));
	  hints.ai_family = PF_UNSPEC;
	  hints.ai_socktype = SOCK_STREAM;
	  hints.ai_flags = AI_CANONNAME;

	  errcode = getaddrinfo (hostname, NULL, &hints, &res);
	  if (errcode != 0)
	  {
	  	perror ("getaddrinfo");
	  	return "0.0.0.0";
	  }

	printf ("Host: %s\n", hostname);
 	while (res)
    	{
      		inet_ntop (res->ai_family, res->ai_addr->sa_data, addrstr, 100);

     		switch (res->ai_family)
       		{
			case AF_INET:
			  ptr = &((struct sockaddr_in *) res->ai_addr)->sin_addr;
			  break;
			case AF_INET6:
			  ptr = &((struct sockaddr_in6 *) res->ai_addr)->sin6_addr;
			  break;
        	}
	      inet_ntop (res->ai_family, ptr, addrstr, 100);
	      printf ("IPv%d address: %s (%s)\n", res->ai_family == PF_INET6 ? 6 : 4,
		      addrstr, res->ai_canonname);
      		res = res->ai_next;
		rd =string(addrstr);
		cout<<rd;
   	 }
	return rd;

}

//Function To fetch All The Registered Routers From The Name Server
void fetchRouters(string dirAdd, unsigned short dirPort)
{

	try{
		//cout<<"about to connect";
		ClientSocket sock(dirAdd, dirPort);
		string str = "EndNeighbor:";
		const char* registrationInfo = str.c_str();
		int echoStringLen = strlen(registrationInfo);
		sock.sendData(registrationInfo, echoStringLen);
		
		//cout<<"\n sent data\n";
		char neighborBuffer[RECEIVE_BUFFER_SIZE];
		int recvMsgSize = 0;
		string receivedData ="";
		char echoBuffer[RECEIVE_BUFFER_SIZE];
		string rd;
		while (true) 
		{
			//cout<<"created socket4";
			recvMsgSize = sock.receiveData(echoBuffer,RECEIVE_BUFFER_SIZE);
			rd = string(echoBuffer);
			//cout<<"\n"<<rd<<"\n";
			break;
		}
		//cout<<"\nreceived data\n";
		
		//cout<<"\n printing the string\n";
		//cout<<rd;
		char *recv = (char*)rd.c_str();
		string part ="";
		for (int i=1; recv[i]!='$'; ++i)
		{
			
			if(recv[i] != '*')
			{
				part += recv[i];
				//cout<<recv[i];
			}
			else
			{
				string word ="";
				
				int index = part.find("IP_")+3;
				int length = part.find(":Port_") - index;
				string IP = part.substr(index,length);
				int index2 = part.find(":Port_")+6;
				length = part.find(":Func_") - index2;
				string port = part.substr(index2,length);
				//cout<<"\nPort: "<<port;
				//cout<<",IP: "<<IP;
				
					
				const char* const_port = port.c_str();
	
				unsigned short u_port = (unsigned short) strtoul(const_port, NULL, 0);


				cout<<"\nIP Address: "<<IP<<", Port: "<<port<<"\n\n";

				part = "";
				

			}
		}		

	}
	catch (SRPCSocketException &e) 
	{
			cerr << e.info() << endl;
			exit(1);
	}
}

//Function To Connect To The Edge Router
int registerInRouter(string dirAdd, unsigned short dirPort,unsigned short port)
{
	int flag = 0;
	for(int i = 0; i<3; i++)
	{
		flag = 0;

		try{
			ClientSocket sock(dirAdd, dirPort);

			char buffer[32];
			snprintf(buffer, 32, "%hu",port);
			string sport(buffer);

			string str = "EdgeRegister:Port_"+sport+"*";
			const char* registrationInfo = str.c_str();
			int echoStringLen = strlen(registrationInfo);
			sock.sendData(registrationInfo, echoStringLen);

			char neighborBuffer[RECEIVE_BUFFER_SIZE];
			int recvMsgSize = 0;
			string receivedData ="";
			char echoBuffer[RECEIVE_BUFFER_SIZE];
			string rd;
			try
			{
				while (true) 
				{
					//cout<<"created socket4";
					recvMsgSize = sock.receiveData2(echoBuffer,RECEIVE_BUFFER_SIZE);
					rd = string(echoBuffer);
					//cout<<"\n"<<rd<<"\n";
					break;
				}

				cout<<"Received Data:"<<rd<<'\n';

				break;
			}
			catch (SRPCSocketException &e) 
			{
				cout<<"\n TimeoUT FAILED\n";
				cerr << e.info() << endl;
				flag = 1;
				continue;

			}

		}
		catch (SRPCSocketException &e) 
		{
				cerr << e.info() << endl;
				if(flag == 1)
				{
					cout<<"Chosen Edge Router Out Of Reach";
					return -1;				
				}
				
			//	exit(1);
		}
	}

	return 1;

}

//Function To Receive Acknowledgement From The Server
void *receive_part(void *args)
{
	try
	{
		ServerSocket servSock(&GlobalPort);
		ClientSocket *sock;
		while(1)
		{
			int costcount = 0;
			sock = servSock.accept();
			string callerAddress;
			try {
				callerAddress = sock->getPeerAddress() ;
				cout << "\n\nCaller's address:"<< callerAddress;
			} catch (SRPCSocketException e) {
				cerr << "Unable to get peer address" << endl;
			}
			try {
				unsigned short callerPort = sock->getPeerPort();
				cout << "\t   Caller's port:" << callerPort;
			} catch (SRPCSocketException e) {
				cerr << "Unable to get peer port" << endl;
			}
			
			char echoBuffer[RECEIVE_BUFFER_SIZE]="";
			int recvMsgSize;
			string receivedData = "";
			while ((recvMsgSize = sock->receiveData(echoBuffer,RECEIVE_BUFFER_SIZE)) > 0) 
			{
				string rd(echoBuffer);
				cout<<"\nreceived:"<<rd<<"\n";
				receivedData = receivedData + rd.substr(0,recvMsgSize);
				break;
			}
			if(gettimeofday(&end,NULL))
				printf("Time Failed");
			t1+= start.tv_sec + (start.tv_usec/1000000.0);
			t2+= end.tv_sec + (end.tv_usec/1000000.0);

			cout<<"Time Taken:"<<(t2-t1)<<"seconds";

		}
	
		
	}
	catch(SRPCSocketException &e)
	{
		cerr << e.info() << endl;
		exit(1);	
	}

}

//Function To Send A File To The Server
int index(string filename, string daddress, unsigned short dport)
{
	try
	{
		cout<<"\nEdge router IP:"<<daddress<<", Edge router port:"<<dport<<"\n";
		ClientSocket sock(daddress, dport);
		//Message Sent To Edge Router Before Transferring A File
		string Msg = "FileUpload:myip_"+GlobalAddress+":myPort_"+Gport+":IP_"+serverAddress+":Port_"+serport+"*";
		cout<<"message sent"<<Msg;
		const char *Info = Msg.c_str();
		int echoStringLen = strlen(Info);
		sock.sendData(Info, echoStringLen);

		int index12 = Msg.find(":myip_")+6;
		int length = Msg.find(":myPort_") - index12;
		string ClientIP = Msg.substr(index12, length);
		cout<<"\nClient IP:"<<ClientIP;


		char echoBuffer[RECEIVE_BUFFER_SIZE];
		int recvMsgSize = 0;
		string receivedData = "";
		while (true) 
		{
			//cout<<"Entered While: About to Receive"<<endl;
			recvMsgSize = sock.receiveData(echoBuffer,RECEIVE_BUFFER_SIZE);
			cout<<"received message size:"<<recvMsgSize<<"\n";
			string rd(echoBuffer);
			
			receivedData = receivedData+rd.substr(0,recvMsgSize);
			
			cout<<"\n Result :"<<receivedData<<"\n";
			break;
		}
		int index = receivedData.find(":");
		string requestType = receivedData.substr(0,index) ;
		cout<<"\nRequest type:"<<requestType;
		
		//Receives Acknowledegement From The Edge Router To Send A File
		if(requestType =="SendFile")
		{
			t1 = 0.0;
			t2 = 0.0;

			if(gettimeofday(&start,NULL))
				printf("Time Failed");


			/*string msg1 = "This is a test";
			const char *Info1 = msg1.c_str();
			int echoStringLen1 = strlen(Info1);
			sock.sendData(Info1, echoStringLen1);*/

			cout<<"\n sending file"<<endl;
			FILE *myfile;
			myfile = fopen(filename.c_str(),"rb");
			if(!myfile)
			{
				cout<<"No such file exists\n";
				return -1;
				
			}
			while(1)
			{
				//cout<<" stuck inside";
		
				char buff[16]={0};
				int nread = fread(buff,1,16,myfile);
				//cout<<"Bytes read: "<<nread<<"\n";  
				// If read was success, send data. 
				if(nread > 0)
				{
					//printf("Sending \n");
					sock.sendDataFile(buff, nread);
		     				//send(connfd, buff, nread, 0);
				}

				if (nread < 16)
				{
					if (feof(myfile))
			       			printf("End of file.. File successfully uploaded\n");
					if (ferror(myfile))
			       			printf("Error reading\n");
					break;
				}
			
		
			}
			fclose(myfile);
			
			//receive_part(servSock, sock);

			/*char echoBuffer30[RECEIVE_BUFFER_SIZE];
			int recvMsgSize30 = 0;
			string receivedData30 = "";
			while (true) 
			{
				//cout<<"Entered While: About to Receive"<<endl;
				recvMsgSize30 = sock.receiveData(echoBuffer30, RECEIVE_BUFFER_SIZE);
				cout<<"received message size:"<<recvMsgSize30<<"\n";
				string rd10(echoBuffer30);
			
				receivedData30 = receivedData30+rd10.substr(0,recvMsgSize30);
			
				cout<<"\n Result :"<<receivedData30<<"\n";
				break;
			}*/
				

			

		}
		//If No Acknowledgement Is Recceived, Then Don't Send The File
		else
		{
			return -1;

		}
		


		
		
	}
	catch (SRPCSocketException &e) 
	{
			cerr << e.info() << endl;
			exit(1);
	}
	return 1;

}

void readConfig()
{
	ifstream infile("clientconfig");
	string line;
	while(getline(infile, line))
	{
		cout<<'\n'<<line<<'\n';	
		int index = line.find(":");
		string type = line.substr(0, index);
		
		if(type == "NameServer")
		{
			int index2 = line.find(":");
			int length = line.find("*") - index2-1;
			NamePath = line.substr(index+1,length);
			cout<<"\nstring NamePath:"<<NamePath<<"\n";
		}
		else if(type == "Server")
		{
			
			int index2 = line.find(":");
			int length = line.find("*") - index2-1;
			ServerPath = line.substr(index+1,length);
			cout<<"\nstring ServerPath:"<<ServerPath<<"\n";
		}
		
		
	}
}


void readNameConfig()
{
	//ifstream infile("/home/debarun/WAN_proj1/NameConfig");
	ifstream infile(NamePath.c_str());
	string line;
	while(getline(infile, line))
	{
		cout<<'\n'<<line<<'\n';	
		int index = line.find(":");
		string type = line.substr(0, index);
		if(type == "IP")
		{
			int index2 = line.find(":");
			int length = line.find("*") - index2-1;
			NSAddress = line.substr(index+1,length);
			cout<<"\nNameServer IP:"<<NSAddress<<"\n";
			
			
		}
		else if(type == "Port")
		{
			int index2 = line.find(":");
			int length = line.find("*") - index2-1;
			string port = line.substr(index+1,length);
			
			cout<<"\nString update:"<<port<<"\n";
			NSPort = (unsigned short) strtoul(port.c_str(), NULL, 0);
			
			cout<<"Name Server Port"<<NSPort;
		}
		
	}
}

//Gets The Server IP and POrt From The Advertised file
void readServerConfig()
{
	//ifstream infile("/home/debarun/WAN_proj1/NameConfig");
	ifstream infile(ServerPath.c_str());
	string line;
	while(getline(infile, line))
	{
		cout<<'\n'<<line<<'\n';	
		int index = line.find(":");
		string type = line.substr(0, index);
		if(type == "IP")
		{
			int index2 = line.find(":");
			int length = line.find("*") - index2-1;
			serverAddress = line.substr(index+1,length);
			cout<<"\nServer IP:"<<serverAddress<<"\n";
			
			
		}
		else if(type == "Port")
		{
			int index2 = line.find(":");
			int length = line.find("*") - index2-1;
			serport = line.substr(index+1,length);
			
			cout<<"\nString Server POrt:"<<serport<<"\n";
			serverPort = (unsigned short) strtoul(serport.c_str(), NULL, 0);
			
			cout<<"\nUnsigned Short Server Port"<<serverPort<<"\n";
		}
		
	}
}


//Main Function... Contains The UI.
int main(int argc, char *argv[])
{

	pthread_t tn[10];
	GlobalAddress = IPlookup();
	//GlobalAddress = "127.0.0.1";
	
	pthread_create(&tn[0],NULL,receive_part,NULL);
	sleep(3);
	//cout<<"created socket5";
	char buffer[32];
	snprintf(buffer, 32, "%hu",GlobalPort);
	string temp(buffer);
	Gport = temp;

	//unsigned short port = (unsigned short) strtoul(argv[1], NULL, 0);
	//cout<<"created socket6";
	//printf("%hu",port);
	//GlobalPort = port;
	readConfig();
	readNameConfig();
	readServerConfig();

	/*string daddress = argv[1];

	unsigned short dport = (unsigned short) strtoul(argv[2], NULL, 0);

	string serveradd = argv[3];
	
	serport  = argv[4];
	unsigned short serverport = (unsigned short) strtoul(argv[4], NULL, 0);

	serverAddress = serveradd;
	serverPort = serverport;

	*/
	
	string edgePort;
	string IPadd;
	unsigned short edPort;
	//The User Interface
	while(1)
	{
	
		cout<<"\n------------------************************************--------------------\n";
		cout<<"\n                       A Simple Routing Protocol                          \n";
		cout<<"\n------------------************************************--------------------\n";
		cout<<"\n                              ---CLIENT---\n";
		cout<<"\n\nChoose an Edge Router To Connect To:\n";
		cout<<"\n-------------------"; 
		cout<<"\nRegistered Routers:";
		cout<<"\n-------------------"; 
		fetchRouters(NSAddress,NSPort);
		cout<<"\n--------------------------------------\n";
		cout<<"--------------------------------------\n";

		cout<<"\nEnter the IP ADDRESS:\n";
		
		getline(cin, IPadd);
	
		//string edgePort;
		cout<<"\nEnter The PORT NUMBER:\n";
		getline(cin, edgePort);

		edPort = (unsigned short) strtoul(edgePort.c_str(), NULL, 0);
		//cout<<"\nYour Choice: IP: "<<IPadd<<", Port: "<<edPort<<'\n';
	
		if(registerInRouter(IPadd, edPort, GlobalPort) == -1)
		{
			continue;
		}
		break;
	}

	cout<<"\nTransfer a file to the server now(Y/N)??\n";
	string choice;
	getline(cin, choice);
	while(1)
	{
		if(choice == "Y" || choice == "y")
		{
			cout<<"\n** Enter the NUMBER of files you want to upload: ";
			int no;
			cin>>no;
			if (cin.fail()) {
	   			cout<<"\nPlease enter a number.. Enter again"; //Not an int.
				cin.clear();
	       			std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
				std::cin >> no;
				//continue;
			}
			
			std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
			int i =0;
			

			while(i<no)
			{
				cout<<"\n\nEnter the FILE NAME you want to Upload:";
				string file;
				getline(cin,file);
				//cout<<"filename:"<<file;
				if(file !="")
				{	while(1)
					{		
						int ret = index(file, IPadd, edPort);
						if(ret == -1)
						{
							cout<<"\nWAIT and TRY again(Y/N)??\n";
							string ch;
							getline(cin,ch);
							if(ch == "Y" || ch == "y")
								continue;
							else 
								break;
						}
						else
							break;
						
					
					}
				}

					i++;	
			}

		}
		else 
			break;
	}

	pthread_join(tn[0],NULL);
	/*pthread_t tn[10];
	pthread_create(&tn[0],NULL,receive_part,NULL);
	pthread_join(tn[0],NULL);*/

	
	
	return 0;
}

