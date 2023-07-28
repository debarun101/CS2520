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
#include <limits>
#include <fstream>
#include <sys/time.h>

const unsigned int RECEIVE_BUFFER_SIZE = 1024;

typedef struct LSD
{
	string sourceIP;
	unsigned short sourcePort;
	string destinationIP;
	unsigned short destinationPort;
	unsigned short cost;
	unsigned int sequence;
	unsigned int age;
	int no_of_links;
	
}LSD;

typedef map <string, LSD> LinkData;

LinkData Database;
		/*for (int i=1; recv[i]!='$'; ++i)
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
				cout<<"\nPort: "<<port;
				cout<<",IP: "<<IP;
				
					
				const char* const_port = port.c_str();
	
				unsigned short u_port = (unsigned short) strtoul(const_port, NULL, 0);

				if((strcmp(IP.c_str(), IPaddress.c_str())!=0) || (u_port != inport))
				{

					cout<<"\nshort port:"<<u_port<<"\n";
					//string key = IP+port;
				
					neighbor ne;
					ne.IP = IP;
					ne.port = u_port;
					ne.SendN = 0;
					ne.AckN = 0;
					ne.SendC = 0;
					ne.AckC = 0;
					ne.Cost = 0;
					//ne.nullify = 0;
					neighbor_table[key++] = ne;
				
				
					cout<<"\npart: "<<part<<"\n";
				}
				part = "";
				

			}
			//cout<<"\n";
			//cout<<"\npart:"<<part;
			//cout << rd.at(i);
		}	*/

//LSA:myIP_127.0.0.1:myport_4009*IP_127.0.0.1:Port_3007:Cost_25*IP_127.0.0.1:Port_3008:Cost_30*#Sequence_1#Age_1$

void addToLSD(string sourceIP, unsigned short sourcePort, string stport, string LSAPacket, unsigned int Sequence, unsigned int Age)
{

	char *recv = (char*)LSAPacket.c_str();
	cout<<"Const char";
	int iter = LSAPacket.find('*');
	string part ="";
	for(int i = iter+1; recv[i] != '#'; i++)
	{
		cout<<"\ni="<<i<<",recv"<<recv[i]<<"\n";

		if(recv[i]!='*')
		{
			part+= recv[i];
		}
		
		else
		{
			int indexIp = part.find("IP_")+3;
			int length = part.find(":Port_") - indexIp;
			string destinationIP = part.substr(indexIp, length);

			int indexPort = part.find("Port_")+5;
			length = part.find(":Cost_") - indexPort;
			string port = part.substr(indexPort, length);
		 	unsigned short destinationPort = (unsigned short) strtoul(port.c_str(), NULL, 0);
			int indexCost = part.find("Cost_")+5;
			length = 2;
			string cost = part.substr(indexCost, length);
			unsigned short destinationCost = (unsigned short)strtoul(cost.c_str(), NULL, 0);
				
			string map_key = sourceIP+":"+stport+"*"+destinationIP+":"+port;

			Database[map_key].sourceIP = sourceIP;
			Database[map_key].sourcePort = sourcePort;
			Database[map_key].destinationIP = destinationIP;
			Database[map_key].destinationPort = destinationPort;
			Database[map_key].cost = destinationCost;
			Database[map_key].sequence = Sequence;
			Database[map_key].age = Age;

			part ="";	
				

		}
	} 

}

void printLSD()
{
	for(LinkData::iterator it = Database.begin(); it != Database.end(); it++)
		{
			cout<<"\nmap key->"<<it->first<<"=="<<"sIp:"<<it->second.sourceIP<<",sPort:"<<it->second.sourcePort<<",dIp:"<<it->second.destinationIP<<",dport: "<<it->second.destinationPort<<"cost: "<<it->second.cost<<",sequence: "<<it->second.sequence<<",age: "<<it->second.age<<'\n';

		} 


}
/*
string map_key = "0:"+Gport+"*"+neighbor_table[i].IP+":"+port;
Database[map_key].sourceIP ="0";
Database[map_key].sourcePort = GlobalPort;
Database[map_key].destinationIP = neighbor_table[i].IP;
Database[map_key].destinationPort = neighbor_table[i].port;
Database[map_key].cost = neighbor_table[i].Cost;
Database[map_key].sequence = GlobalSequence+1;
Database[map_key].age = GlobalAge+1;
*/


int main(int argc, char *argv[])
{
	//cout<<"created socket5";
	//unsigned short port = (unsigned short) strtoul(argv[1], NULL, 0);
	//cout<<"created socket6";
	//printf("%hu",port);

	//string daddress = argv[2];

	//unsigned short dport = (unsigned short) strtoul(argv[3], NULL, 0);	
	//cost(port, daddress, dport);

	cout<<"\nHI\n";
	//addToLSD("127.0.0.1", 4009,"4009","LSA:myIP_127.0.0.1:myport_4009*IP_127.0.0.1:Port_3007:Cost_25*IP_127.0.0.1:Port_3008:Cost_30*#Sequence_1#Age_1$",1,1);

	//printLSD();
	

	string destKey = "127.0.0.1:2000*";
	int index = destKey.find(':');
	string destinationIP = destKey.substr(0, index);
	int length = destKey.find('*') - (index+1);
	string destinationPort = destKey.substr(index+1, length);
	unsigned short uport = (unsigned short) strtoul(destinationPort.c_str(), NULL, 0);	
	

	cout<<"\nDestination IP:"<<destinationIP;
	cout<<"\nDestination Port:"<<destinationPort<<"\n";
	cout<<"\n Short Port->"<<uport<<"\n";
	
	//respondMaster(port);
	return 0;
}



/*void cost(unsigned short inport, string dirAdd, unsigned short dirPort)
{
		struct timeval start,end;
		double t1,t2;
		t1 = 0.0;
		t2 = 0.0;
		ClientSocket sock(dirAdd,dirPort);
		cout<<"created client socket\n";

		//Send Cost Message
		
		//sock.sendData(registrationInfo, echoStringLen);

		double cost[5];

		for(int i = 0; i<= 5; i++)
		{
			string str = "Cost:";
			const char* registrationInfo = str.c_str();
			int echoStringLen = strlen(registrationInfo);
			cout<<"\nmessage sent size: "<<echoStringLen;
			cout<<"\ni="<<i<<"\n";
			if(gettimeofday(&start,NULL))
				printf("Time Failed");
			sock.sendData(registrationInfo, echoStringLen);
			cout<<"data sent";
			int recvMsgSize = 0;
			string receivedData ="";
			char echoBuffer[10];
			string rd;

			while (true) 
			{
				//cout<<"created socket4";
				recvMsgSize = sock.receiveData(echoBuffer,RECEIVE_BUFFER_SIZE);
				rd = string(echoBuffer);
				cout<<"\n"<<rd<<"\n";
				break;
			}
			//recvMsgSize = sock.receiveData(echoBuffer,10);
			//rd = string(echoBuffer);
			//cout<<"\nReceived: "<<rd<<'\n';
			
			if(gettimeofday(&end,NULL))
				printf("Time Failed");
			cout<<"\ngot timeof day \n";
			t1+= start.tv_sec + (start.tv_usec/1000000.0);
			t2+= end.tv_sec + (end.tv_usec/1000000.0);
			cout<<"\ngot timeof day2 \n";
			if(i>0)
			{
				cost[i] = 0.9*(cost[i-1])+0.1*(t2-t1);
			}
			else
				cost[i] = t2-t1;
			cout<<"RTT"<<i<<"::"<<(t2-t1);

			
		}
		double RTT = (t2-t1)/5;
		cout<<"\nRTT: "<<RTT<<"\n";

		cout<<"\nsmoothed cost: "<<cost[4];
}*/
