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

string NamePath = "";
const unsigned int RECEIVE_BUFFER_SIZE = 1024;

unsigned short GlobalPort;
string GlobalAddress;
unsigned long doc_id =0;

string EdgeRIP;
unsigned short EdgeRPort;


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
void advertise()
{
	ofstream myfile("ServerConfig");
	//Reference: www.cplusplus.com/doc/tutorial/files/
	if(myfile.is_open())
	{
		string IP = "IP:"+GlobalAddress+"*\n";
		myfile<<IP;
		
		char buffer[32];
		snprintf(buffer, 32, "%hu",GlobalPort);
		string port(buffer);
			
		string actport = "Port:"+port+"*\n";
		myfile<<actport;
			
		myfile.close();

	}
}

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
int registerInRouter(string dirAdd, unsigned short dirPort,unsigned short port)
{
	int flag = 0;
	for(int i = 0; i<3; i++)
	{
		flag = 0;

		try
		{
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


void* receive_part(void *args)
{
	try
	{
		ServerSocket servSock(&GlobalPort);
		ClientSocket *sock;
		advertise();
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
				int index = receivedData.find(":");
				string requestType = receivedData.substr(0,index) ;
				//cout << "Request Type:" << requestType << endl;
				
				if(requestType == "FileUpload")
				{
					
					int index2 = receivedData.find("IP_") + 3; 
					int length = receivedData.find(":Port_") - index2;
					string IP = receivedData.substr(index2,length); //server IP

					int index3 = receivedData.find("Port_")+5; 
					length = receivedData.find("*") - index3;
					string port =  receivedData.substr(index3,length);//server Port

					cout<<"\nDestination IP:"<<IP<<"\n";
					cout <<"Destination Port:" << port <<"\n";				
					//cout << "Functionality_"<<func<<"\n\n";
					const char* const_port = port.c_str();
	
					unsigned short u_port = (unsigned short) strtoul(const_port, NULL, 0); //server port

					int index11 = receivedData.find("myPort_")+7;
					length = receivedData.find(":IP_") - index11;
					string clientPort = receivedData.substr(index11,length); 
					
					unsigned short c_port = (unsigned short) strtoul(clientPort.c_str(), NULL, 0); 

					cout<<"\nclient port: "<<c_port;

					//Get The Client IP
					int index12 = receivedData.find("myip_")+5;
					length = receivedData.find(":myPort_") - index12;
					string ClientIP = receivedData.substr(index12, length);

					cout<<"\nClient IP:"<<ClientIP<<"\n";

					//check if the server is reachable in the Routing Table
					
					//ask to send file
					char * response = "SendFile:";
					int len = strlen(response);
					sock->sendData(response, len);

					//Receive the file
					char buffer10[64];
					++doc_id;
					snprintf(buffer10, 64, "%lu",doc_id);
					string temp1(buffer10);
					fstream myfile(temp1.c_str(),ios::out);
					cout<<"\nfile opened\n";
		    			if(NULL == myfile)
		    			{
						printf("Error opening file");
									//return 1;
		    			}

					int count =0;
					char frBuff[RECEIVE_BUFFER_SIZE]="";				
					int bytesReceived = 0;
					cout<<"about to receive";
					while((bytesReceived = sock->receiveData(frBuff,RECEIVE_BUFFER_SIZE)) > 0)
		    			{
						count++;
						cout<<"Bytes received: "<<bytesReceived<<endl;    
						myfile.write(frBuff, bytesReceived);
						memset (frBuff,0,RECEIVE_BUFFER_SIZE);
						if(bytesReceived < RECEIVE_BUFFER_SIZE)
							break;
						//cout<< "written";
		       			}
						//cout<<"hello there";
					myfile.close();
					cout<<"count="<<count;
					//TODO
					if(count == 0)
					{
						
						break;
					}					
					else
					{
						//Send Acknowledgement To Client
						try
						{
							ClientSocket sock1(EdgeRIP, EdgeRPort);	
							string Msg = "FileAck:IP_"+ClientIP+":Port_"+clientPort+"*";
							cout<<"message sent"<<Msg;
							const char *Info = Msg.c_str();
							int echoStringLen = strlen(Info);
							sock1.sendData(Info, echoStringLen);
						}
						catch (SRPCSocketException &e) 
						{
							cerr << e.info() << endl;
							//exit(1);
						}
						break;
					}
					break;
					
				}

			}

		}
	}
	catch(SRPCSocketException &e)
	{
		cerr << e.info() << endl;
		exit(1);	
	}


}

void readConfig()
{
	ifstream infile("myconfig");
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
		
		
	}
}
string NameIP;
unsigned short NamePort;
void readNameConfig()
{
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
			NameIP = line.substr(index+1,length);
			cout<<"\nstring NameIP:"<<NameIP<<"\n";
			
			
		}
		else if(type == "Port")
		{
			int index2 = line.find(":");
			int length = line.find("*") - index2-1;
			string port = line.substr(index+1,length);
			cout<<"\nstring update:"<<port<<"\n";
			NamePort = (unsigned short) strtoul(port.c_str(), NULL, 0);
			cout<<"Integer Name Port"<<NamePort<<"\n";
		}
		
	}
}


int main(int argc, char *argv[])
{
	//cout<<"created socket5";
	//unsigned short port = (unsigned short) strtoul(argv[1], NULL, 0);
//	cout<<"created socket6";
	//printf("%hu",port);
	//GlobalPort = port;
	//string daddress = argv[2];
	readConfig();
	readNameConfig();
	GlobalAddress = IPlookup();
	//GlobalAddress = "127.0.0.1";
	pthread_t tn[10];
	pthread_create(&tn[0],NULL,receive_part,NULL);
	unsigned short dport = (unsigned short) strtoul(argv[3], NULL, 0);
	while(1)
	{
	
		cout<<"\n------------------************************************--------------------\n";
		cout<<"\n                       A Simple Routing Protocol                          \n";
		cout<<"\n------------------************************************--------------------\n";
		cout<<"\n                              ---SERVER---\n";
		cout<<"\nChoose an Edge Router To Connect To\n";
		
		fetchRouters(NameIP,NamePort); 

		cout<<"======================================================";

		cout<<"\nEnter The IP Address Of Your Choice:\n";
		string IPadd;
		getline(cin, IPadd);
		EdgeRIP  = IPadd;	
		
		string edgePort;
		cout<<"\nEnter The Port No. Of Your Choice:\n";
		getline(cin, edgePort);

		unsigned short edPort = (unsigned short) strtoul(edgePort.c_str(), NULL, 0);
		EdgeRPort = edPort;

		cout<<"\nYour Choice: IP: "<<IPadd<<", Port: "<<edPort<<'\n';
	
		if(registerInRouter(IPadd, edPort, GlobalPort) == -1)
		{
			continue;
		}
		break;
		
		
	}
	
	pthread_join(tn[0],NULL);
	cout<<"\nSend a file to the server(Y/N)\n";
	
	return 0;
}

