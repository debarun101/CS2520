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


const unsigned int RECEIVE_BUFFER_SIZE = 1024;

void workerSetup(unsigned short dport)
{

	try
	{
		ServerSocket servSock(dport);
		ClientSocket *sock;
		sock = servSock.accept();
		int i =0;
		while(i<6)
		{
			
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
				if(requestType == "Cost")
				{
					char * response = "CostAck:";
					int len = strlen(response);
					sock->sendData(response, len);
					
					cout<<"\nCost ack sent \n";
					break;
				}
				
			}
			i++;

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

	unsigned short dport = (unsigned short) strtoul(argv[1], NULL, 0);
	//cout<<"created socket6";
	//printf("%hu",dport);

	

	workerSetup(dport);
	return 0;
}
