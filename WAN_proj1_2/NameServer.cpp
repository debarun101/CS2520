#include "RPCSocket.h"
#include "RPCSocket.cpp"
#include <iostream>
#include <cstdlib>
#include <list>
#include <map>
#include "DirectoryService.h"
#include <pthread.h>
#include <unistd.h>
#include <cstring>
#include <fstream>
#include <vector>
#include <algorithm>

using namespace std;

const unsigned int RECEIVE_BUFFER_SIZE = 1024;
const unsigned short DIRECTORY_SERVICE_PORT = 2500;//Ports below 1024 can only be used by administrator.

DirectoryService::DirectoryService() {
	
}

DirectoryService::~DirectoryService() {
	
}



typedef struct router
{
	string IP;
	unsigned short port;
	string func;

}router;

typedef map<string, router> RouterList;
string GlobalAddress;
unsigned short GlobalPort;


RouterList India;
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
	ofstream myfile("NameConfig");
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
void workerSetup()
{

	try
	{
		ServerSocket servSock(&GlobalPort);
		ClientSocket *sock;
		advertise();
		while(1)
		{
			sock = servSock.accept();
			string callerAddress;
			try {
				callerAddress = sock->getPeerAddress() ;
				//cout << "\n\nCaller's address:"<< callerAddress;
			} catch (SRPCSocketException e) {
				cerr << "Unable to get peer address" << endl;
			}
			try {
				unsigned short callerPort = sock->getPeerPort();
				//cout << "\t   Caller's port:" << callerPort;
			} catch (SRPCSocketException e) {
				cerr << "Unable to get peer port" << endl;
			}
			
			char echoBuffer[RECEIVE_BUFFER_SIZE];
			int recvMsgSize;
			string receivedData = "";
			while ((recvMsgSize = sock->receiveData(echoBuffer,RECEIVE_BUFFER_SIZE)) > 0) 
			{
				string rd(echoBuffer);
				//cout<<rd<<endl;
				receivedData = receivedData + rd.substr(0,recvMsgSize);
				int index = receivedData.find(":");
				string requestType = receivedData.substr(0,index) ;
				//cout << "Request Type:" << requestType << endl;
				
				if(requestType == "routerRegister")
				{
					//cout << "Received Data:" << receivedData << endl;
					int index2 = receivedData.find("Port_") + 5; //length of "Port_" is 5
					int length = receivedData.find(":") - index2;
					string port = receivedData.substr(index2,length);

					int index3 = receivedData.find("Func_")+5; // length of string "func_" is 5
					length = receivedData.find("*") - index3;
					string func =  receivedData.substr(index3,length);

					//cout <<"Port:" << port << endl;				
					//cout << "Functionality_"<<func<<"\n\n";
					const char* const_port = port.c_str();
	
					unsigned short u_port = (unsigned short) strtoul(const_port, NULL, 0);
					
					//Forming the key of the router list
					
					string key ="";
					key = callerAddress+port;

					//Forming the value of the routerList

					router value;
					value.IP = callerAddress;
					value.port = u_port;
					value.func = func;

					India[key] = value;
					cout<<"\nbegin India =======\n";
					for (RouterList::iterator p = India.begin();p != India.end(); ++p) 
					{
						
						cout<<"\nkey:"<<p->first <<",IP:"<<p->second.IP<<",Port:"<<p->second.port<<",functionality:"<<p->second.func<<":\n";
					
					}
					cout<<"\n end India =======\n";
					/*
					workerDet info;
					info.address = callerAddress;
					info.port = port;
					++worker_id;
					workerList[worker_id] = info;
					//cout<<"index: "<<worker_id<<" address: "<<workerList[worker_id].address<<"port: "<<workerList[worker_id].port<<endl;
					*/
					
					char * response = "successRegister";
					int len = strlen(response);
					sock->sendData(response, len);
				}
				
				else if(requestType == "neighborList")
				{
					string resolveSend = "";
					for (RouterList::iterator p = India.begin();p != India.end(); ++p) 
					{
						resolveSend += "*IP_"+p->second.IP+":";
						char buffer1[16];
						snprintf(buffer1,16, "%hu",p->second.port);
						string temp1(buffer1);

						resolveSend += "Port_"+temp1+":Func_"+p->second.func;
					}
					resolveSend += "*$";

					cout<<"\n\n"<<"resolve string: "<<resolveSend<<endl;

					const char* sendData = resolveSend.c_str();
					int len = strlen(sendData);
					sock->sendData(sendData, len);
					
				}
				else if(requestType == "EndNeighbor")
				{
					string resolveSend = "";
					for (RouterList::iterator p = India.begin();p != India.end(); ++p) 
					{
						resolveSend += "*IP_"+p->second.IP+":";
						char buffer1[16];
						snprintf(buffer1,16, "%hu",p->second.port);
						string temp1(buffer1);

						resolveSend += "Port_"+temp1+":Func_"+p->second.func;
					}
					resolveSend += "*$";

					cout<<"\n\n"<<"resolve string: "<<resolveSend<<endl;

					const char* sendData = resolveSend.c_str();
					int len = strlen(sendData);
					sock->sendData(sendData, len); 


				}
				else if(requestType == "DeadNeighbor")
				{
					int index2 = receivedData.find("IP_") + 3; 
					int length = receivedData.find(":Port_") - index2;
					string IP = receivedData.substr(index2,length);

					int index3 = receivedData.find("Port_")+5; 
					length = receivedData.find("*") - index3;
					string port =  receivedData.substr(index3,length);

					//cout <<"Port:" << port << endl;				
					//cout << "Functionality_"<<func<<"\n\n";
					const char* const_port = port.c_str();
	
					unsigned short u_port = (unsigned short) strtoul(const_port, NULL, 0);
					
					//Forming the key of the router list
					
					string key = IP+port;
					cout<<"IP cancel:"<<IP<<", port cancel:"<<port<<"\n"; 
					//India.erase(key);
					for (RouterList::iterator p = India.begin();p != India.end(); ++p) 
					{
						if(strcmp(p->second.IP.c_str(),IP.c_str()) == 0 && p->second.port == u_port)
						{
							India.erase(p);
						}
					}
					


					cout<<"\nAfter Cancel, begin India =======\n";
					for (RouterList::iterator p = India.begin();p != India.end(); ++p) 
					{
						
						cout<<"\nkey:"<<p->first <<",IP:"<<p->second.IP<<",Port:"<<p->second.port<<",functionality:"<<p->second.func<<":\n";
					
					}
					cout<<"\n end India =======\n";
					
					
					char * response = "successCancel";
					int len = strlen(response);
					sock->sendData(response, len);
					

				
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
int main(int argc, char *argv[])
{
	GlobalAddress = IPlookup();

	//unsigned short dport = (unsigned short) strtoul(argv[1], NULL, 0);
	workerSetup();
	return 0;
}


