#include "RPCSocket.h"
#include "RPCSocket.cpp"
#include "TCPServerConnector.h"
#include <iostream>
#include <cstdlib>
#include <list>
#include <map>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <cstring>
#include "ServerRegister.h"
#include <fstream>

using namespace std;

const unsigned int RECEIVE_BUFFER_SIZE = 1024;
const unsigned short DIRECTORY_SERVICE_PORT = 2500;
const string DIRECTORY_SERVICE_ADDRESS = "127.0.0.1";// 10.0.0.43
const unsigned short WORKER_SERVICE_PORT = 1030;

ServerRegister::ServerRegister() {
	
}

ServerRegister::~ServerRegister() {
	
}

void registerMaster(unsigned short inport, string dirAdd, unsigned short dirPort, string function)
{
	try{
		//cout<<"about to connect";
		ClientSocket sock(dirAdd, dirPort); //Client Socket created
		//cout<<"created socket";
		//string strPort = std::to_string(WORKER_SERVICE_PORT);
		char buffer[32];
		snprintf(buffer, 32, "%hu",inport); //converting the port from unsigned short to char*
		//cout<<"created socket2";
		string temp(buffer); //converting port from char* to string
		string str = "routerRegister:Port_"+ temp+":Func_"+function+"*"; 
		const char *registrationInfo = str.c_str();
		int echoStringLen = strlen(registrationInfo);

		sock.sendData(registrationInfo, echoStringLen);
		
		char echoBuffer[RECEIVE_BUFFER_SIZE];
		int recvMsgSize = 0;

		string receivedData;
		//cout<<"created socket3";
		//receiving ACKNOWLEDGEMENT from Name Server if it was successfully registered or not
		while (true) 
		{
			//cout<<"created socket4";
			recvMsgSize = sock.receiveData(echoBuffer,RECEIVE_BUFFER_SIZE);
			string rd(echoBuffer);
			receivedData = receivedData+rd.substr(0,recvMsgSize);
			if(receivedData=="successRegister")
			{
				cout << "Successfully registered in the directory service. Return message1:" << receivedData <<"\n";
				//setupServer(SERVER_SERVICE_PORT);
				//return 1;
			}
			else
			{
				cout << "Failed to register in the directory service. Return message0:" << receivedData <<"\n";
				//return 0;
			}
			//Close the session regardless of the returned result.
			break;
		}
		//cout<<"created socket5";
		/*
		str = "neighborList:";
		registrationInfo = str.c_str();
		echoStringLen = strlen(registrationInfo);
		sock.sendData(registrationInfo, echoStringLen);

		char neighborBuffer[RECEIVE_BUFFER_SIZE];
		recvMsgSize = 0;
		receivedData ="";

		while (true) 
		{
			//cout<<"created socket4";
			recvMsgSize = sock.receiveData(echoBuffer,RECEIVE_BUFFER_SIZE);
			string rd(echoBuffer);
			cout<<"\n"<<rd<<"\n";
			break;
		}*/		
		
		
	}
	catch (SRPCSocketException &e) 
	{
			cerr << e.info() << endl;
			exit(1);
	}
}
void fetchRouters(unsigned short inport, string dirAdd, unsigned short dirPort)
{

	try{
		//cout<<"about to connect";
		ClientSocket sock(dirAdd, dirPort);
		string str = "neighborList:";
		const char* registrationInfo = str.c_str();
		int echoStringLen = strlen(registrationInfo);
		sock.sendData(registrationInfo, echoStringLen);

		char neighborBuffer[RECEIVE_BUFFER_SIZE];
		int recvMsgSize = 0;
		string receivedData ="";
		char echoBuffer[RECEIVE_BUFFER_SIZE];
		string rd;
		while (true) 
		{
			//cout<<"created socket4";
			recvMsgSize = sock.receiveData(echoBuffer,RECEIVE_BUFFER_SIZE);
			rd= string(echoBuffer);
			cout<<"\n"<<rd<<"\n";
			break;
		}	
		cout<<"\n printing the string";
		for (int i=1; i<rd.length(); ++i)
		{
			string part ="";
			while(rd.at(i) != '*')
			{
				part += rd.at(i);
			}
			cout<<"\npart:"<<part<<"\n";
			//cout << rd.at(i);
		}		
		

	}
	catch (SRPCSocketException &e) 
	{
			cerr << e.info() << endl;
			exit(1);
	}
}


void configure(unsigned short port, string daddress, unsigned short dport)
{
	
	registerMaster(port, daddress, dport,"main");
	fetchRouters(port, daddress, dport);
	
	

} 

int main(int argc, char *argv[])
{
	//cout<<"created socket5";
	unsigned short port = (unsigned short) strtoul(argv[1], NULL, 0);
//	cout<<"created socket6";
	printf("%hu",port);

	string daddress = argv[2];

	unsigned short dport = (unsigned short) strtoul(argv[3], NULL, 0);	
	
	configure(port,daddress, dport);
	
	//respondMaster(port);
	return 0;
}
