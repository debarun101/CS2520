#include "RPCSocket.h"
#include "RPCSocket.cpp"
#include <iostream>
#include <cstdlib>
#include <list>
#include <map>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <cstring>
#include <fstream>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <vector>
#include <limits.h>
#define Max 999999999
#define POLY 0x8408

using namespace std;




pthread_mutex_t count_mutex;
pthread_mutex_t alive_mutex;
pthread_mutex_t cost_mutex;
pthread_mutex_t list_mutex;
pthread_mutex_t LSD_mutex;
pthread_mutex_t LSA_mutex;
pthread_mutex_t RT_mutex;

unsigned long doc_id = 0;
const unsigned int RECEIVE_BUFFER_SIZE = 1024;
int key =0;
int current = 0;
int alive_current = 0;
int no_of_neighbors = 0;
int neighbor_index =0;
unsigned int MaxAge = 1000;
int Debug;


unsigned short GlobalPort;
string Gport;
string GlobalAddress;
string NSAddress;
unsigned short NSPort;

unsigned int GlobalSequence = 0;
unsigned int GlobalAge = 0;

int Pe = 10;
int Pc = 10;


int HelloInterval = 30;

int UpdateInterval = 60;

//structure to hold blacklist routers
typedef struct blacklist
{
	string IP;
	unsigned short port;
}blacklist;
blacklist bl[100];
int blkey =0;

string NamePath;
string NameIp;
unsigned short NamePort;

//structure to hold the features of a neighbor in the neighbor table
typedef struct neighbor
{
	string IP;
	unsigned short port;
	int SendN;
	int AckN;
	int SendC;
	int AckC;
	unsigned short Cost;
	int endSystem;
	
}neighbor;

//neighbor_table is an array that stores the details of all the neighbors in the array
neighbor neighbor_table[100];

//struct that contains the parameters to be passed to a new thread
typedef struct parameters
{
	string IP;
	unsigned short port;
}parameters;


//struct that contains the contents of LSD



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

typedef struct incoming_list
{
	string LSAPacket;
	string sourceIP;
	unsigned short sourcePort;
	

}incoming_list;

typedef vector<incoming_list> InList;

InList vect;
 
//LSA_Status Table
typedef struct Status
{
	int Send;
	int Ack;
}Status;

typedef map<string, map<string, Status> > LSA_Status;
LSA_Status LStat;


//Routing Table

typedef struct route_table
{
	string destinationIP;
	unsigned short destinationPort;

	string nexthopIP;
	unsigned short nexthopPort;

	int Cost;

}routing;

typedef map<string, routing> Route_Table;
Route_Table Rtable;




typedef map <string, int> RID;
RID Rome;

typedef map <int, string> Inverse;
Inverse trick;


//function prototypes

void registerMaster(unsigned short, string, unsigned short, string);
void fetchRouters(string, unsigned short, string, unsigned short);
void* alive_call(void*);
void alive(string,unsigned short);
void* cost(void* );
void* neighbor_acq(void*);
void* receive_part(void*);
void configure(unsigned short, string, unsigned short);
void eraseList(string, unsigned short,string);
void printLSD();
void printIncomingList();
void addToLSD(string , unsigned short, string, string, unsigned int, unsigned int);
void printIncomingList();
void dijkstra(int **, int, int);


//the checksum function

unsigned short crc16(char *data_p, unsigned short length)
{
      unsigned char i;
      unsigned int data;
      unsigned int crc = 0xffff;

      if (length == 0)
            return (~crc);

      do
      {
            for (i=0, data=(unsigned int)0xff & *data_p++;
                 i < 8; 
                 i++, data >>= 1)
            {
                  if ((crc & 0x0001) ^ (data & 0x0001))
                        crc = (crc >> 1) ^ POLY;
                  else  crc >>= 1;
            }
      } while (--length);

      crc = ~crc;
      data = crc;
      crc = (crc << 8) | (data >> 8 & 0xff);

      return (crc);
}


//get the IP of the machine

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

//function to print the contents of LSD
void printLSD()
{

	cout<<"\n ======  Link State Database ======= \n";
	for(LinkData::iterator it = Database.begin(); it != Database.end(); it++)
		{
			cout<<"\nmap key->"<<it->first<<"=="<<"source_Ip:"<<it->second.sourceIP<<",source_Port:"<<it->second.sourcePort<<",dest_Ip:"<<it->second.destinationIP<<",dest_port: "<<it->second.destinationPort<<"Cost: "<<it->second.cost<<",Sequence #: "<<it->second.sequence<<",Age: "<<it->second.age<<'\n';

		} 


}

void printIncomingList()
{
	cout<<"\n=============List Of LSAs=====================\n";
	for(int i =0; i< vect.size();i++)
	{
		cout<<"\nList"<<i<<": LSA packet: "<<vect[i].LSAPacket<<": Source Port:"<<vect[i].sourcePort<<"Source IP"<<vect[i].sourceIP<<"\n";

	}

}

void printNeighbor()
{
	cout<<"\n====== Neighbor table ======\n";

	for(int h = 0; h<key; h++)
	{
	
		cout<<"IP: "<<neighbor_table[h].IP<<",Port: "<<neighbor_table[h].port<<"\n";//<<",sendN: "<<neighbor_table[h].SendN<<"\n";
	}

		
	cout<<"\n+++++++++++++++++++++++++++++\n";

}
void printmyNeighbors()
{
	cout<<"\n====== Neighbor table ======\n";

	for(int h = 0; h<key; h++)
	{
		if(neighbor_table[h].AckN >0)
			cout<<"IP: "<<neighbor_table[h].IP<<",Port: "<<neighbor_table[h].port<<",sendN: "<<neighbor_table[h].SendN<<"\n";
	}

		
	cout<<"\n+++++++++++++++++++++++++++++\n";


}

void printRouting()
{
	cout<<"\n========= Routing Table ========\n";

	for(Route_Table::iterator it = Rtable.begin(); it != Rtable.end(); it++)
	{
			
		cout<<"\nKey: "<<it->first<<", Destination IP:"<<it->second.destinationIP<<", Destination Port: "<<it->second.destinationPort<<", Next Hop IP"<<it->second.nexthopIP<<", Next hop port:"<<it->second.nexthopPort<<", Cost:"<<it->second.Cost<<"\n";
      	}

	cout<<"\n+++++++++++++++++++++++++++++++++\n";


}

void *Print(void* args)
{
	while(1)
	{
		cout<<"\n======= Present Status =======\n";
		string choice;
		cout<<"\n.. Routing Table(Press 'R')\n";
		cout<<".. Neighbor List(Press 'N'\n";
		cout<<".. Link State Database(Press 'LD')\n";
		cout<<".. LSA List(Press 'L')\n\n";
		getline(cin, choice);
		if(choice == "N" || choice == "n")
			//sleep(20);
		{	
			printmyNeighbors();
		}
		else if(choice == "L" || choice == "l")
		{
	      		pthread_mutex_lock(&list_mutex);
			printIncomingList();
			pthread_mutex_unlock(&list_mutex);
		}
		else if (choice == "LD" || choice == "ld")
		{
			pthread_mutex_lock(&LSD_mutex);
			printLSD();
			pthread_mutex_unlock(&LSD_mutex);
		}
		else if (choice == "R" || choice == "r")
		{
			pthread_mutex_lock(&RT_mutex);
			printRouting();
			pthread_mutex_unlock(&RT_mutex);
		}
		else
		{
			cout<<"\nWrong Choice! Try Again...";
		}
	}
}

//Utility Function For Dijkstra
int minDistance(int dist[], bool sptSet[], int count)
{
   // Initialize min value
   int min = INT_MAX, min_index;
 
	   for (int v = 0; v < count; v++)
	     if (sptSet[v] == false && dist[v] <= min)
		{
		 
	        	min = dist[v], min_index = v;
	 	
		 }
 
   return min_index;
}

//dijkstra's algorithm
void dijkstra(int **graph, int src, int count)
{
	if(Debug == 1)
	cout<<"\nEntered Dijkstra\n";
	
	int *nextHop = new int[count]; // output array, nextHop[i] will hold the next hop that src to i;
	
	int **index = new int*[count];
	for(int i = 0; i<count; i++)
		index[i] = new int[count];

	int **index1 = new int*[count];
	for(int i = 0; i<count; i++)
		index1[i] = new int[count];
	
	int *index2 = new int[count];
	int hop;
	int source;


     int *dist = new int[count];     // The output array.  dist[i] will hold the shortest
                      // distance from src to i
 
     bool *sptSet = new bool[count]; // sptSet[i] will true if vertex i is included in shortest
                     // path tree or shortest distance from src to i is finalized
 
     // Initialize all distances as INFINITE and stpSet[] as false
	if(Debug == 1)
		cout<<"\ncame here 1\n";
     for (int i = 0; i < count; i++){
        dist[i] = INT_MAX, sptSet[i] = false,nextHop[i] = Max;
		for(int j= 0; j < count; j++){
			index[i][j] = 0;
		}
	}



     // Distance of source vertex from itself is always 0
     dist[src] = 0;
	 nextHop[src] = 0;
	 index[src][src] = 1;
	 index1[src][src] = 1;
     // Find shortest path for all vertices
	if(Debug == 1)
	cout<<"\ncame here 2\n";
     for (int c = 0; c < count-1; c++)
     {
       // Pick the minimum distance vertex from the set of vertices not
       // yet processed. u is always equal to src in first iteration.
       int u = minDistance(dist, sptSet,count);
       // Mark the picked vertex as processed
       sptSet[u] = true;
       // Update dist value of the adjacent vertices of the picked vertex.
	if(Debug == 1)
		cout<<"\ncame here 3\n";

       for (int v = 0; v < count; v++){
        
         if (!sptSet[v] && graph[u][v] && dist[u] != INT_MAX 
                                       && dist[u]+graph[u][v] < dist[v]){									   
            dist[v] = dist[u] + graph[u][v];
			//index is the graph of shortest path
			index[u][v] = dist[v];
			for(int i=0;i<count;i++)
			{
				for(int j=0;j<count;j++)
				{
					if(index[j][i]!=dist[i]){
						index[j][i] = 0;
					}
				}
			}
			index[src][src]=1;
			}//end index
		}
     }

	if(Debug == 1)
		cout<<"\ncame here 4\n";
	 //convert it to one dems array
	 for(int m=0;m<count;m++)
	 {
		 for(int n=0;n<count;n++)
		 {
			 if(index[n][m]!=0)
			 {
				 index2[m]=n;
				 
			 }
		 }
	 }

	if(Debug == 1)
	{
		cout<<"\ncame here 5\n";
		cout<<"count: "<<count<<"\n";
		cout<<"\nvalues of index2[]\n"; 
	}
	for(int i=0;i<count;i++)
	{
		if(Debug == 1)		
			cout<<index2[i]<<",";
	}
	if(Debug == 1)
		cout<<"\nvalues of index[][]\n";
	 for(int m=0;m<count;m++)
	 {
		 for(int n=0;n<count;n++)
		 {
			if(Debug == 1)
			 	cout<<index[m][n]<<",";
		 }
	 }
	if(Debug == 1)
		cout<<"\n";
		
	 for(int i=0;i<count;i++)
	 {
		 int m;
	if(Debug == 1)
		cout<<"\ninside for\n";
		 if(index2[i]==src)
		 {
			if(Debug == 1)
			{
				cout<<"\nindex2[i] ="<<index2[i];
				cout<<"inside if\n";
			}
			 nextHop[i]=i;
		 }
		 else
		 {
			if(Debug == 1)
			cout<<"inside else";
			 m=index2[i];
			 if(index2[m]==src)
			 {
				if(Debug == 1)				
				cout<<"\ninside sub if";

				nextHop[i]=m;
				if(Debug == 1)
				cout<<"\nnext hop:"<<m;
			 }
			if(Debug == 1)
			cout<<"\ninside sub else";
			int tmp1, tmp2;		
			 while(index2[m]!=src)
			 {
				tmp1 = index2[m];
				 m=index2[m];
				tmp2 = index2[m];
				if(tmp1 == tmp2)
					break;
					
			 }

			if(Debug == 1)
			cout<<"\nnext hop:"<<m;	
			 nextHop[i]=m;
			
		 }
	 }
	//if(Debug == 1)
		cout<<"\ncame here 6\n";
		cout<<"count: "<<count<<"\n";
		cout<<"\n\n======= INSIDE DIJKSTRA:::Adjacency Matrix ========\n";
		for(int i = 0; i< count; i++)
		{
			for(int j = 0; j< count; j++)
			{
				cout<<graph[i][j]<<'\t';
			}
			cout<<"\n";

		}

	printf("Destination \t next hop\t Distance from Source\n ");
	
	for (int i = 0; i < count; i++)
	{
		//if(dist[i]<6000)
			printf("%d \t\t %d\t\t\t %d\n", i, nextHop[i], dist[i]);
	}

	//Filling the Routing Table
	pthread_mutex_lock(&RT_mutex);
	for(Route_Table::iterator it = Rtable.begin(); it != Rtable.end(); it++)
	{
		Rtable.erase(it);

	}
	
	for (int i = 0; i < count; i++)
	{
		//if(dist[i]<6000)
		//{
			string destKey = trick[i];

			int index = destKey.find(':');
			string destinationIP = destKey.substr(0, index);
			int length = destKey.find('*') - (index+1);
			string destinationPort = destKey.substr(index+1, length);
			unsigned short destUport = (unsigned short) strtoul(destinationPort.c_str(), NULL, 0);

			int kk = nextHop[i];
			string hopString = trick[kk];
			index = hopString.find(':');
			string nexthopIP = hopString.substr(0, index);
			length = hopString.find('*') - (index+1);
			string nexthopPort = hopString.substr(index+1, length);
			unsigned short nexthopUport = (unsigned short) strtoul(nexthopPort.c_str(), NULL, 0);
		
			//pthread_mutex_lock(&RT_mutex);
			routing Rn;
			Rn.destinationIP = destinationIP;
			Rn.destinationPort = destUport;
			Rn.nexthopIP = nexthopIP;
			Rn.nexthopPort = nexthopUport;
			Rn.Cost = dist[i];
			 

			Rtable[destKey] = Rn;
		//}
		//pthread_mutex_unlock(&RT_mutex);

	}
	pthread_mutex_unlock(&RT_mutex);
}


void *Routing_Table(void *args)
{

	while(1)
	{
		//Erase the original contents of Rome and trick
		if(Debug == 1)
			cout<<"\nerase\n";
		for(RID::iterator it = Rome.begin(); it != Rome.end(); it++)
		{
			Rome.erase(it);

		}

		for(Inverse::iterator it = trick.begin(); it != trick.end(); it++)
		{
			trick.erase(it);

		}
		sleep(UpdateInterval);
		//
		int count = 0;

		if(Debug == 1)
			cout<<"\nReached before scan \n"; 
		pthread_mutex_lock(&LSD_mutex);
		for(LinkData::iterator it = Database.begin(); it != Database.end(); it++)
		{
			
			if(Debug == 1)
			cout<<"\nentered database for\n";
			char buffer[32];
			snprintf(buffer, 32, "%hu",it->second.sourcePort);
			string port(buffer);
			
			
			string key = it->second.sourceIP+":"+port+"*";
			Rome[key]=count++;

			key="";
			char buffer1[32];
			snprintf(buffer1, 32, "%hu",it->second.destinationPort);
			string port1(buffer1);
			key =it->second.destinationIP+":"+port1+"*";
			Rome[key]=count++;

			
		}
		pthread_mutex_unlock(&LSD_mutex);
		count =0;
		if(Debug == 1)
			cout<<"\nwill iterate rome now\n";
		for(RID::iterator it = Rome.begin(); it != Rome.end(); it++)
		{
			string value = it->first;
			trick[count]=value;
			count++;

		}
		if(Debug == 1)
		cout<<"\n trick iterate";
		for(Inverse::iterator it = trick.begin(); it != trick.end(); it++)
		{
			Rome[it->second] = it->first;
		}
		//make adjacency matrix of size count*count
		unsigned short** adjacency = new unsigned short*[count];
		for(int i = 0; i < count; ++i)
		    adjacency[i] = new unsigned short[count];

		//initialize all the elements to 0;
		for(int i =0 ; i< count; i++)
			for(int j = 0; j < count; j++)
				adjacency[i][j] = 0;
		if(Debug == 1)
		cout<<"\n Count value:"<<count<<'\n';
		for(int i = 0; i< count; i++)
		{
			for(int j = 0; j< count; j++)
			{
				string val = trick[i];

				string ip = val.substr(0,val.find(":"));
				
				int portindex = val.find(':')+1;
				int length = val.find("*") - portindex;
				string port = val.substr(portindex, length);
				unsigned short sourcePort = (unsigned short) strtoul(port.c_str(), NULL, 0);

				for(LinkData::iterator it = Database.begin(); it != Database.end(); it++)
				{
					if(strcmp(it->second.sourceIP.c_str(), ip.c_str())==0 && sourcePort == it->second.sourcePort)
					{
						

						char buffer3[32];
						snprintf(buffer3, 32, "%hu",it->second.destinationPort);
						string sport(buffer3);
						string key =  it->second.destinationIP+":"+sport+"*";
						int row =i;
						int column = Rome[key];
						adjacency[row][column] = it->second.cost;
						


					}

				}
			}

		}
		if(Debug == 1)
		cout<<"\ncount="<<count;
		string key = GlobalAddress+":"+Gport+"*";
		int src = Rome[key];
		if(Debug == 1)
		cout<<"\nsource ="<<src;

		int** adjacency1 = new int*[count];
		for(int i = 0; i < count; ++i)
		    adjacency1[i] = new int[count];


		for(int m = 0; m<count; m++)
		{
			for(int n =0; n<count; n++)
			{
				adjacency1[m][n] = adjacency[m][n];
			}

		}
		if(Debug == 1)
		cout<<"\n\n=======Adjacency Matrix ========\n";
		for(int i = 0; i< count; i++)
		{
			for(int j = 0; j< count; j++)
			{
				
				cout<<adjacency[i][j]<<'\t';
			}
			
			cout<<"\n";

		}
		if(Debug == 1)
		{
			cout<<"\n\n======= Copied Adjacency Matrix ========\n";
			for(int i = 0; i< count; i++)
			{
				for(int j = 0; j< count; j++)
				{
					cout<<adjacency1[i][j]<<'\t';
				}
				cout<<"\n";

			}
		}
		//pthread_mutex_lock(&RT_mutex);
		if(count>0 && src >=0)
			dijkstra(adjacency1,src, count);
		//pthread_mutex_unlock(&RT_mutex);
		
		//pthread_mutex_unlock(&LSD_mutex);
		
		cout<<"\n\n=======List of routers ========\n";
		for(Inverse:: iterator it = trick.begin(); it != trick.end(); it++)
		{
			cout<<"\n"<<it->first<<"::"<<it->second<<"\n";
		}
	}

}



//function to register the router to the name server

//function to fetch the IP and Port of all the possible neighbors from the Name Server




//FUNCTION TO GENERATE AND FLOOD LSAs

void *LSA_Gen(void *args)
{
	if(Debug == 1)
	cout<<"\nEntered LSA flooding\n";
	
	while(1)
	{

		
		
		string LSAPacket = "LSA:myIP_"+GlobalAddress+":myport_"+Gport;
		sleep(UpdateInterval);
		//pthread_mutex_lock(&cost_mutex);
		int flag =0;
		int Ncount = 0;
		for(int i = 0; i < key; i++)
		{
		
			if(neighbor_table[i].port !=0 && strcmp(neighbor_table[i].IP.c_str(), "0.0.0.0")!=0 && neighbor_table[i].AckN == 1 && neighbor_table[i].Cost > 0)
			{
							
				flag =1;
				if(Ncount ==0)
				{
					pthread_mutex_lock(&LSD_mutex);
					for(LinkData::iterator it = Database.begin(); it != Database.end(); it++)
					{
						if(strcmp(it->second.sourceIP.c_str(),GlobalAddress.c_str()) == 0 && it->second.sourcePort == GlobalPort)
						{
							GlobalAge = it->second.age;
							Database.erase(it);
						}
					}
					Ncount++;
					pthread_mutex_unlock(&LSD_mutex);
				}
				
				LSAPacket += "*IP_";
				LSAPacket += neighbor_table[i].IP+":Port_";
				
				char buffer[32];
				snprintf(buffer, 32, "%hu",neighbor_table[i].port);
				string port(buffer);
				LSAPacket += port+":Cost_";

				

				char buffer1[32];
				snprintf(buffer1, 32, "%hu",neighbor_table[i].Cost);
				string cost(buffer1);
				LSAPacket += cost;

				
				//Fill the LSD simultaneously
				pthread_mutex_lock(&LSD_mutex);
				string map_key = GlobalAddress+":"+Gport+"*"+neighbor_table[i].IP+":"+port;
				Database[map_key].sourceIP =GlobalAddress;
				Database[map_key].sourcePort = GlobalPort;
				Database[map_key].destinationIP = neighbor_table[i].IP;
				Database[map_key].destinationPort = neighbor_table[i].port;
				Database[map_key].cost = neighbor_table[i].Cost;
				Database[map_key].sequence = GlobalSequence+1;
				Database[map_key].age = GlobalAge+1;
				pthread_mutex_unlock(&LSD_mutex);
				
			}
		
		}
		if(flag == 1)
		{
			GlobalSequence++;
			//GlobalAge++;
		}


		if(flag ==0)
		{
			//erasing the residuals
			for(LinkData::iterator it = Database.begin(); it != Database.end(); it++)
			{

				Database.erase(it);
			}

			for(int i =0; i< vect.size();i++)
			{
				vect.erase(vect.begin()+i);
			}
			continue;

			
		}
		char buffer1[32];
		snprintf(buffer1, 32, "%u",GlobalSequence);
		string sequence(buffer1);
		
		char buffer[32];
		snprintf(buffer, 32, "%u",GlobalAge);
		string age(buffer);		
		
		
		LSAPacket += "*#Sequence_"+sequence+"#Age_"+age+"$";
		if(Debug == 1)
		cout<<"LSA Packet:- "<<LSAPacket;
		

			
		
		if(flag == 1) //i.e. if there is any valid neighbor
		{
			//Erasing the older version from the incoming list and adding the new one
			pthread_mutex_lock(&list_mutex);
			eraseList(GlobalAddress, GlobalPort,LSAPacket);
			pthread_mutex_unlock(&list_mutex);
			
			
		}
		
		
		//Flood the LSAs
		for(int i =0; i< vect.size();i++)
		{
			int ackflag =0;
			int neighbCount = 0;
			//pthread_mutex_lock(&LSA_mutex);
			try
			{
				for(int j = 0; j < key; j++)
				{
					if( neighbor_table[j].port != 0 && strcmp(neighbor_table[j].IP.c_str(),"0.0.0.0") !=0 && neighbor_table[j].endSystem == 0 )
					{
						neighbCount++;
						//calculating probability of dropping a packet

						int v2 = rand()%100 +1;
						//if the random number generated lies within the range of Pc, then we drop the packet
						if(v2 > Pc)
						{
							string IP = neighbor_table[j].IP;
							unsigned short port = neighbor_table[j].port;
					
							ClientSocket sock2(IP,port);
							string str = vect[i].LSAPacket;
							const char* registrationInfo = (vect[i].LSAPacket+"0").c_str();
							unsigned short checksum = crc16((char*)registrationInfo, (unsigned short)strlen(registrationInfo));

							if(Debug == 1)
							cout<<"\nLSA message CRC:"<<checksum<<"\n";
						
							char buffers[32];
							snprintf(buffers, 32, "%hu",checksum);
							string check(buffers);

							str = str+"0|CRC_"+check+"&";
							//Generating Random Number to Determine If there is an error in the packet.
							int v1 = rand()%100+1;
							
							if(Debug == 1)
							cout<<"\nrandom no:"<<v1;
							//v1 = 11;
				
							if(v1 <= Pe)
							{
								for(int i = 0;i< str.length();i++)
								{
									if(str[i] == '$')
									{
										str[i+1]='1';
									}
								}
							}

			
							registrationInfo = str.c_str();
							int echoStringLen = strlen(registrationInfo);
							sock2.sendData(registrationInfo, echoStringLen);
							
							if(Debug == 1)						
							cout<<"\n LSA packet sent to neighbor"<<j<<"\n";
					

							int recvMsgSize = 0;
							string receivedData ="";
							char echoBuffer[RECEIVE_BUFFER_SIZE];
							string rd;
							try
							{
								while (true) 
								{
									//cout<<"created socket4";
									recvMsgSize = sock2.receiveData2(echoBuffer,RECEIVE_BUFFER_SIZE);
									rd = string(echoBuffer);
									if(Debug == 1)
									cout<<"\n"<<rd<<"\n";
									break;
								}
							}
							catch (SRPCSocketException &e) 
							{
								ackflag = 0;
								if(Debug == 1)
								cout<<"\n LSA TimeoUT FAILED\n";
								cerr << e.info() << endl;
								continue;
								pthread_exit(NULL);
				
							}
			
							receivedData = receivedData + rd.substr(0,recvMsgSize);
							int index = receivedData.find(":");
							string requestType = receivedData.substr(0,index) ;
							if(requestType == "LSAAck")
							{
								ackflag += 1;
								if(Debug == 1)
								cout<<"\n LSA acknowledgement received \n";
							}
							else
							{
								//ackflag = 0;
								continue;
							}
						}
						//sock2.close1();
					}
					
				}
				
			}
			catch (SRPCSocketException &e) 
			{
					cerr << e.info() << endl;
					continue;
					//exit(1);
			}
			//to remove residuals from LSA
			if(ackflag == neighbCount)
				vect.erase (vect.begin()+i);
			
			
		}
		

		
	
		
	}
}




void *increaseAge(void* args)
{
	while(1)
	{
		if(Debug == 1)
		cout<<"\n entered Increase Age";
		
		typedef map<int, unsigned int> AgeDiff;
		AgeDiff agdiff;
		pthread_mutex_lock(&LSD_mutex);
		for(LinkData::iterator it = Database.begin(); it != Database.end(); it++)
		{
			int diff = (int)(MaxAge - it->second.age);
			agdiff[diff] = it->second.age;
		
		}

		AgeDiff::iterator p = agdiff.begin();
		int sleeptime = p->first;
		if(Debug == 1)
		cout<<"Sleep time1:"<<sleeptime;
		if(sleeptime >= 20 || sleeptime == 0) 
		{
			sleeptime = 20;
		}
		if(Debug == 1)		
		cout<<"Sleep time2:"<<sleeptime;
		for(LinkData::iterator it = Database.begin(); it != Database.end(); it++)
		{
			it->second.age+= (unsigned int)sleeptime;
			
		}
		
		for(LinkData::iterator it = Database.begin(); it != Database.end(); it++)
		{
			

			if(it->second.age >= MaxAge)
			{
				if(Debug == 1)
				cout<<"\nyay entered MAXage check\n"; 
				if(it->second.sourcePort == GlobalPort && strcmp(it->second.sourceIP.c_str(), GlobalAddress.c_str())==0)
					GlobalAge = 1;

				Database.erase(it); 
				if(Debug == 1)
				cout<<"\n===========ERASED DATABASE ENTRY-----\n";
			}
			
		}
		pthread_mutex_unlock(&LSD_mutex);;

		
		sleep(sleeptime);
	}

}

void eraseList(string IP, unsigned short port, string LSAPacket)
{

	for(int i =0; i< vect.size();i++)
	{
			
		if(strcmp(vect[i].sourceIP.c_str(),IP.c_str())==0 && vect[i].sourcePort == port)
		{
			vect.erase (vect.begin()+i);
		}
	}

	incoming_list in;
	in.LSAPacket = LSAPacket;
	in.sourceIP = IP;
	in.sourcePort = port;
	vect.push_back(in);

}
void addToLSD(string sourceIP, unsigned short sourcePort, string stport, string LSAPacket, unsigned int Sequence, unsigned int Age)
{
	//erase
	for(LinkData::iterator it = Database.begin(); it != Database.end(); it++)
	{
		if(strcmp(it->second.sourceIP.c_str(), sourceIP.c_str())==0 && sourcePort == it->second.sourcePort)
		{
			Database.erase(it); 
		}

	}

	//add


	char *recv = (char*)LSAPacket.c_str();
	//cout<<"Const char";
	int iter = LSAPacket.find('*');
	string part ="";
	for(int i = iter+1; recv[i] != '#'; i++)
	{
		//cout<<"\ni="<<i<<",recv"<<recv[i]<<"\n";

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

void* alive_call(void* args)
{

	cout <<"\nentered alive_call actual\n";

	
		while(1)
		{
			cout<<"\n mera key: "<<key;
			for(int i = 0; i < key; i++)
			{
				cout<<"\n entered for \n";
				cout<<"\ni = "<<i<<"ack: "<<neighbor_table[i].AckN;
				if(neighbor_table[i].AckN == 1 && neighbor_table[i].endSystem == 0)
					alive(neighbor_table[i].IP,neighbor_table[i].port);
			}
			sleep(25);
		}
	


}


void alive(string IP, unsigned short port)
{

	int flag =0;
	for(int i = 0; i< 2; i++)
	{
		try
		{
			flag = 0;
		
			ClientSocket sock1(IP,port);
			cout<<"created client socket\n";
			//Send Alive Message
			string str = "Alive:";
			const char* registrationInfo = str.c_str();
			int echoStringLen = strlen(registrationInfo);
			cout<<"\nmessage sent size: "<<echoStringLen;
			sock1.sendData(registrationInfo, echoStringLen);
			cout<<"\nsent alive message\n";
			int recvMsgSize = 0;
			string receivedData ="";
			char echoBuffer[RECEIVE_BUFFER_SIZE];
			string rd;
			
			try{
				while (true) 
				{
					//cout<<"created socket4";
					recvMsgSize = sock1.receiveData2(echoBuffer,RECEIVE_BUFFER_SIZE);
					rd = string(echoBuffer);
					cout<<"\n"<<rd<<"\n";
					break;
				}
			}
			catch (SRPCSocketException &e) 
			{
				cout<<"\n Alive TimeoUT FAILED\n";
				cerr << e.info() << endl;
				flag = 1;
				//sock1.close1();
				continue;
				//exit(1);
			}
			receivedData = receivedData + rd.substr(0,recvMsgSize);
			int index = receivedData.find(":");
			string requestType = receivedData.substr(0,index) ;
			if(requestType == "AliveAck")
			{
				cout<<"\n Neighbor is confirmed to be alive \n";
			}
			//alive_current++;
		//}

			flag =0;
			cout<<"\nexit alive\n";
			break;
			//sleep(4);
			//pthread_mutex_unlock(&alive_mutex);
			//sock1.close1();
			
		}

		catch (SRPCSocketException &e) 
		{
				cerr << e.info() << endl;
				cout<<"\n TimeoUT FAILED\n";
				cerr << e.info() << endl;
				flag = 1;

				continue;
				//exit(1);
		}

	}

	if(flag == 1)
	{

		cout<<"\nneighbor is dead\n";

		//erase the neighbor from the neighbor table
		//pthread_mutex_lock(&count_mutex);
		//pthread_mutex_lock(&LSA_mutex);
		//pthread_mutex_lock(&cost_mutex);
		for(int mn = 0; mn < key; mn++)
		{
			if(neighbor_table[mn].port == port && strcmp(neighbor_table[mn].IP.c_str(), IP.c_str())==0)
			{
				neighbor_table[mn].port =0;
				neighbor_table[mn].IP = "0.0.0.0";
				neighbor_table[mn].SendN = 0;
				neighbor_table[mn].AckN = 0;
				neighbor_table[mn].SendC = 0;
				neighbor_table[mn].AckC = 0;
				neighbor_table[mn].Cost = 0;
				neighbor_table[mn].endSystem =0;
			}

		}

		//Delete the entry from the LSD
		pthread_mutex_lock(&LSD_mutex);
		for(LinkData::iterator it = Database.begin(); it != Database.end(); it++)
		{
			
			if(strcmp(it->second.sourceIP.c_str(),IP.c_str()) == 0 && it->second.sourcePort == port)
			{

				Database.erase(it);
			}
			
		}
		
		pthread_mutex_unlock(&LSD_mutex);
		//Delete the content in Incoming List

		
			
		pthread_mutex_lock(&list_mutex);
		for(int i =0; i< vect.size();i++)
		{
			
			if(strcmp(vect[i].sourceIP.c_str(),IP.c_str())==0 && vect[i].sourcePort == port)
			{
				vect.erase (vect.begin()+i);
			}
				
		}
		pthread_mutex_unlock(&list_mutex);	
		

		//Delete The Countent In the Routing Table
		pthread_mutex_lock(&RT_mutex);
		for(Route_Table::iterator it = Rtable.begin(); it != Rtable.end(); it++)
		{
			cout<<"\n TO DELETE CONTENT FROM ROUTING TABLE\n";
			cout<<"\nDead Address: "<<IP<<", Dead Port:"<<port<<"\n";
			cout<<"\nRtable Address: "<<it->second.destinationIP<<", RTable Port: "<<it->second.destinationPort<<"\n";
			if(strcmp(it->second.destinationIP.c_str(),IP.c_str())==0 && it->second.destinationPort == port)
			{
				cout<<"\nMatch Founfd\n";
				Rtable.erase(it);
				break;

			} 
			
	      	}
		pthread_mutex_unlock(&RT_mutex);
		// Send request to Name Server to make changes 
		
		try
		{
			ClientSocket sock(NSAddress, NSPort);
			
			char buffer[32];
			snprintf(buffer, 32, "%hu",port);
			string temp(buffer);

			string str = "DeadNeighbor:IP_"+IP+":Port_"+temp+"*";
			
			const char* registrationInfo = str.c_str();
			int echoStringLen = strlen(registrationInfo);
			sock.sendData(registrationInfo, echoStringLen);

			char echoBuffer[RECEIVE_BUFFER_SIZE];
			int recvMsgSize = 0;

			string receivedData;
			
			while (true) 
			{
				//cout<<"created socket4";
				recvMsgSize = sock.receiveData(echoBuffer,RECEIVE_BUFFER_SIZE);
				string rd(echoBuffer);
				receivedData = receivedData+rd.substr(0,recvMsgSize);
				if(receivedData=="successCancel")
				{
					cout << "Successfully Removed" << receivedData <<"\n";
					//setupServer(SERVER_SERVICE_PORT);
					//return 1;
				}
				else
				{
					cout << "Failed to remove" << receivedData <<"\n";
					//return 0;
				}
				//Close the session regardless of the returned result.
				break;
			}

		//cout<<"created socket5";
			sock.close1();

		}
		catch (SRPCSocketException &e) 
		{
			cerr << e.info() << endl;
		}


		//pthread_mutex_unlock(&cost_mutex);
		//pthread_mutex_unlock(&LSA_mutex);
		//pthread_mutex_unlock(&count_mutex);

		//return -1;

	}

	
}

void* cost(void* args)
{
	if(Debug == 1)
	cout<<"\ncost\n";
	

	try
	{
		//pthread_mutex_lock(&cost_mutex);

		parameters* dim = (parameters*) args; 

	   	string IP = dim->IP; 
		unsigned short port = dim->port;
		if(Debug == 1)
		cout<<"\ncost IP:"<<dim->IP<<",port: "<<dim->port<<"\n";

		struct timeval start,end;
		double t1,t2;
		t1 = 0.0;
		t2 = 0.0;
		ClientSocket sock(IP,port);
		if(Debug == 1)
		cout<<"created client socket\n";

		double cost[5];

		for(int i = 0; i<= 5; i++)
		{
			string str = "Cost:";
			const char* registrationInfo = str.c_str();
			int echoStringLen = strlen(registrationInfo);
			//cout<<"\nmessage sent size: "<<echoStringLen;
			//cout<<"\ni="<<i<<"\n";
			if(gettimeofday(&start,NULL))
				printf("Time Failed");
			sock.sendData(registrationInfo, echoStringLen);

			if(Debug == 1)
			cout<<"data sent";
			int recvMsgSize = 0;
			string receivedData ="";
			char echoBuffer[10];
			string rd;

			try{

				while (true) 
				{
					//cout<<"created socket4";
					recvMsgSize = sock.receiveData2(echoBuffer,RECEIVE_BUFFER_SIZE);
					rd = string(echoBuffer);

					if(Debug == 1)
					cout<<"\n"<<rd<<"\n";
					break;
				}
			}
			catch (SRPCSocketException &e) 
			{

				if(Debug == 1)
				cout<<"\n Cost TimeoUT FAILED\n";
				cerr << e.info() << endl;
				//return(NULL);
				//pthread_mutex_unlock(&cost_mutex);
				pthread_exit(NULL);
				
			}
			
			/*recvMsgSize = sock.receiveData(echoBuffer,10);
			rd = string(echoBuffer);
			cout<<"\nReceived: "<<rd<<'\n';*/
			
			if(gettimeofday(&end,NULL))
				printf("Time Failed");
			//cout<<"\ngot timeof day \n";
			t1+= start.tv_sec + (start.tv_usec/1000000.0);
			t2+= end.tv_sec + (end.tv_usec/1000000.0);
			//cout<<"\ngot timeof day2 \n";
			if(i>0)
			{
				cost[i] = 0.9*(cost[i-1])+0.1*(t2-t1);
			}
			else
				cost[i] = t2-t1;
			//cout<<"RTT"<<i<<"::"<<(t2-t1);

			
		}
		double RTT = (t2-t1)/5;
		//cout<<"\nRTT: "<<RTT<<"\n";

		if(Debug == 1)
			cout<<"\nsmoothed cost: "<<cost[4];

		double act_cost = cost[4]*1000;
		unsigned short acost;
		if(act_cost<=0.1)
			acost = 5;
		else if (act_cost>0.1 && act_cost <= 0.2)
			acost =10;
		else if (act_cost>0.2 && act_cost <= 0.3)
			acost = 20;
		else if (act_cost>0.3 && act_cost <= 0.4)
			acost = 25;
		else if (act_cost>0.4 && act_cost <= 0.5)
			acost = 30;
		else if (act_cost>0.5 && act_cost <= 0.6)
			acost = 35;
		else if (act_cost>0.6 && act_cost <= 0.7)
			acost = 40;
		else if (act_cost>0.7 && act_cost <= 0.9)
			acost = 45;
		else if (act_cost>0.9 && act_cost <= 1.1)
			acost = 50;
		else if (act_cost>1.1 && act_cost <= 1.3)
			acost = 55;
		else if (act_cost>1.3 && act_cost <= 1.5)
			acost = 60;
		else if (act_cost>1.5 && act_cost <= 1.8)
			acost = 65;
		else if (act_cost>1.8 && act_cost <= 2.1)
			acost = 70;
		else if (act_cost>2.1)
			acost = 75;
		

		//pthread_mutex_lock(&count_mutex);
		if(Debug == 1)
		cout<<"\n key: "<<key;
		for(int i =0; i< key; i++)
		{

			//cout<<"neighbor table ip:"<<neighbor_table[i].IP<<", my ip: "<<IP<<", neighb port:"<<neighbor_table[i].port<<", my port: "<<port;
			if(neighbor_table[i].port !=0 && strcmp(neighbor_table[i].IP.c_str(), "0.0.0.0")!=0 && neighbor_table[i].endSystem == 0)
				if(neighbor_table[i].port == port && strcmp(neighbor_table[i].IP.c_str(), IP.c_str())==0)
				{
					//cout<<"\nMatch match found\n";
					//cout<<"smoth cost: "<<cost[4];
				
					neighbor_table[i].Cost = acost;
					//cout<<"neighbor table cost: "<<neighbor_table[i].Cost;
				}
		}
		if(Debug == 1)
		{
			cout<<"\n===cost neighbor table ====\n";
			for(int h = 0; h<key; h++)
			{
				cout<<"\nIP: "<<neighbor_table[h].IP<<",Port: "<<neighbor_table[h].port<<",cost: "<<neighbor_table[h].Cost<<"\n";
			}
		}
		
		
		//pthread_mutex_unlock(&count_mutex);

		//pthread_mutex_unlock(&cost_mutex);

		sock.close1();


	}
	catch (SRPCSocketException &e) 
	{
			if(Debug == 1)
			cout<<"\n Cost TimeoUT FAILED\n";
			cerr << e.info() << endl;
			pthread_exit(NULL);
			//exit(1);
	}
	pthread_exit(NULL);
	
}




//Global Variable "no_of_neighbors" stores the total number of neighbors of the router//TODO the selection based on blacklist
void* neighbor_acq(void *args)
{
	if(Debug == 1)
	{
		cout<<"\nentered mutex\n";
		cout<<"\nneighbors size:"<<key<<"\n";
	}
	try
	{
		
		if(key>0 && current < key)
		{
			pthread_mutex_lock(&count_mutex);
			if(Debug == 1)
			cout<<"\nentered if\n";
			string IP = neighbor_table[current].IP;
			if(Debug == 1)
			cout<<"\n got IP\n";
			unsigned short port = neighbor_table[current].port;
			

			if(Debug == 1)
			cout<<"\nIP resolved:"<<IP<<"\n";
			if(Debug == 1)			
			cout<<"\nport resolved:"<<port<<"\n";
			ClientSocket sock(IP,port);
			if(Debug == 1)
			cout<<"created client socket\n";
			//Send Be_Neighbors Request
			
						
			
			string str = "BeNeighbors:Port_"+Gport+"*";
			const char* registrationInfo = str.c_str();
			int echoStringLen = strlen(registrationInfo);
			sock.sendData(registrationInfo, echoStringLen);
			neighbor_table[current].SendN =1;			
		
			if(Debug == 1)
			cout<<"sent be neighbors\n";
			//Receive Acknowledgement
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

					if(Debug == 1)
					cout<<"\n"<<rd<<"\n";
					break;
				}
			}
			catch (SRPCSocketException &e) 
			{
				if(Debug == 1)
				cout<<"\n TimeoUT FAILED\n";
				cerr << e.info() << endl;
				//return(NULL);
				current++;
				pthread_mutex_unlock(&count_mutex);
				pthread_exit(NULL);
				//flag = 1;
				//continue;
				//exit(1);
			}
			
			receivedData = receivedData + rd.substr(0,recvMsgSize);
			int index = receivedData.find(":");
			string requestType = receivedData.substr(0,index) ;

			if(requestType == "BeNeighbors_Confirm")
			{
				
				neighbor_table[current].AckN =1;
				//pthread_mutex_unlock(&count_mutex);

				if(Debug == 1)
				cout<<"\nconfirmed neighbors\n";
				//sending alive messages

				pthread_t tn[2];

				//parameters *par =(parameters*) malloc(sizeof(parameters));
				parameters *par = new parameters;
				//parameters par;
				if(Debug == 1)
				cout<<"\nmalloced\n";
				if(Debug == 1)
				cout<<"\nIP:"<<IP;
				if(Debug == 1)
				cout<<",port:"<<port<<"\n";
				(*par).IP = IP;

				if(Debug == 1)
				cout<<"IP obtained:"<<(*par).IP;
				if(Debug == 1)
				cout<<"\n IP assigned\n";

				
				(*par).port = port;

				if(Debug == 1)
				{
					cout<<"Port Obtained:"<<(*par).port<<"\n";
				
					cout<<"\n port assigned\n";
					//pthread_create(&tn[0],NULL,receive_part,NULL);
					cout<<"\nreached alive call\n";
				}
				//http://stackoverflow.com/questions/6524433/passing-multiple-arguments-to-a-thread-in-c-pthread-create

				pthread_create(&tn[0], NULL, cost, par);
				//pthread_create(&tn[1], NULL,alive_call, par);
				pthread_join(tn[0],NULL);
				//pthread_join(tn[1],NULL);

				if(Debug == 1)
				cout<<"\n\n==== END OF NEIGHBOR_ACQ ========\n\n";
				
			}
			else
			{
				neighbor_table[current].AckN =-1;
			}
			current++;
			sock.close1();
			pthread_mutex_unlock(&count_mutex);
			
			//cout<<"\nit incremented\n";
			
		
		}
		if(Debug == 1)
		cout<<"exit mutex";
		
	
	}

	catch (SRPCSocketException &e) 
	{
			cerr << e.info() << endl;
			if(Debug == 1)
			cout<<"\n TimeoUT FAILED\n";
			current++;
			pthread_mutex_unlock(&count_mutex);
			pthread_exit(NULL);
			//exit(1);
	}
	//cout<<"\nThread Count:"<<count<<"\n";

	pthread_exit(NULL);
	
}



void* receive_part(void *args)
{
	try
	{
		ServerSocket servSock(&GlobalPort);
		if(Debug == 1)
		cout<<"\nGlobal Port: "<<GlobalPort<<"\n";
		ClientSocket *sock;
		while(1)
		{
			int costcount = 0;
			sock = servSock.accept();
			string callerAddress;
			try {
				callerAddress = sock->getPeerAddress() ;
				if(Debug == 1)
				cout << "\n\nCaller's address:"<< callerAddress;
			} catch (SRPCSocketException e) {
				cerr << "Unable to get peer address" << endl;
			}
			try {
				unsigned short callerPort = sock->getPeerPort();
				if(Debug == 1)
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
				if(Debug == 1)
				cout<<"\nreceived:"<<rd<<"\n";
				receivedData = receivedData + rd.substr(0,recvMsgSize);
				int index = receivedData.find(":");
				string requestType = receivedData.substr(0,index) ;
				//cout << "Request Type:" << requestType << endl;
				
				if(requestType == "BeNeighbors")
				{
					int flag = 0;
					int index2 = receivedData.find("Port_")+5;
					int length = receivedData.find("*") - index2;
					string port = receivedData.substr(index2, length);

					const char* const_port = port.c_str();
					unsigned short u_port = (unsigned short) strtoul(const_port, NULL, 0);
			
					//TODO check Blacklist;
					for(int i = 0;i < blkey; i++)
					{
						//check if the requesting neighbor is blacklisted
						if(strcmp(callerAddress.c_str(), bl[i].IP.c_str()) == 0)
						{
							flag = 1;

						}
					}

					if(flag == 0)
					{
						char * response = "BeNeighbors_Confirm:";
						int len = strlen(response);
						sock->sendData(response, len);
					
						
						if(Debug == 1)
						cout<<"\nPort to be inserted:"<<u_port<<"\n";

						pthread_mutex_lock(&count_mutex);

						if(Debug == 1)
						cout<<"\nWILL SEND COST TO NEIGHBOR\n";
						neighbor ne;
						ne.IP = callerAddress;
						ne.port = u_port;
						ne.SendN = 1;
						ne.AckN = 1;
						ne.SendC = 0;
						ne.AckC = 0;
						ne.Cost = 0;
						ne.endSystem =0;
						neighbor_table[key++] = ne;
					
						parameters *par = new parameters;
						(*par).IP = callerAddress;
						(*par).port = u_port;

						pthread_t tn[1];
						pthread_create(&tn[0], NULL, cost, par);
						pthread_join(tn[0],NULL);
						pthread_mutex_unlock(&count_mutex);

						break;
					}
					else if(flag == 1)
					{
						char * response = "BeNeighbors_Reject:";
						int len = strlen(response);
						sock->sendData(response, len);
						break;
					}
						if(Debug == 1)
						cout<<"sent Be_neighbors confirm";
				}
				else if(requestType == "EdgeRegister")//for accepting traffic from an end system
				{
					char * response = "Edge_Confirm:";
					int len = strlen(response);
					sock->sendData(response, len);

					int index2 = receivedData.find("Port_")+5;
					int length = receivedData.find("*") - index2;
					string port = receivedData.substr(index2, length);

					const char* const_port = port.c_str();
					unsigned short u_port = (unsigned short) strtoul(const_port, NULL, 0);
					
					if(Debug == 1)
					cout<<"End System's IP:" <<callerAddress<<"\n";
					if(Debug == 1)
					cout<<"End System's port:"<<u_port<<"\n";
					pthread_mutex_lock(&count_mutex);
					if(Debug == 1)
					cout<<"\nWILL SEND COST TO END SYSTEM\n";
					neighbor ne;
					ne.IP = callerAddress;
					ne.port = u_port;
					ne.SendN = 1;
					ne.AckN = 1;
					ne.SendC = 0;
					ne.AckC = 0;
					ne.Cost = 2;
					ne.endSystem =1;
					neighbor_table[key++] = ne;
					
					break;
					//"BeNeighbors:Port_"+Gport+"*"
					
				}
				else if(requestType == "FileAck") //Forward the ACK to a file to the next hop
				{
					int index2 = receivedData.find("IP_") + 3; 
					int length = receivedData.find(":Port_") - index2;
					string IP = receivedData.substr(index2,length); //server IP

					int index3 = receivedData.find("Port_")+5; 
					length = receivedData.find("*") - index3;
					string port =  receivedData.substr(index3,length);
					if(Debug == 1)
					cout<<"\nDestination IP:"<<IP<<"\n";
					if(Debug == 1)
					cout <<"Destination Port:" << port <<"\n";				
					//cout << "Functionality_"<<func<<"\n\n";
					const char* const_port = port.c_str();
	
					unsigned short u_port = (unsigned short) strtoul(const_port, NULL, 0); //server port

					//check if the server is reachable in the Routing Table
					pthread_mutex_lock(&RT_mutex);
					int Cost = 6000;
					int flag1 = 0;
					string nextIP;
					unsigned short nextPort;
					if(Debug == 1)
					cout<<"=========== Checking if DESTINATION IS AVAILABLE ============="; 
					printRouting();
					for(Route_Table::iterator it = Rtable.begin(); it != Rtable.end(); it++)
					{
						if(Debug == 1)
						cout<<"\nRouter table IP:"<<it->second.destinationIP<<", R Port:"<<it->second.destinationPort<<"\n";
						if(strcmp(it->second.destinationIP.c_str(), IP.c_str()) == 0 && it->second.destinationPort == u_port)
						{
							nextIP = it->second.nexthopIP;
							nextPort = it->second.nexthopPort;
							Cost = it->second.Cost;
							flag1 = 1;
						}
					}
					pthread_mutex_unlock(&RT_mutex);
					//send file ack to the next router
					if(flag1 == 1)
					{
						if(Cost < 5000)
						{						
							try
							{
								ClientSocket sock1(nextIP,nextPort);
								string str = "FileAck:IP_"+IP+":Port_"+port+"*";
								const char *registrationInfo = str.c_str();
								int echoStringLen = strlen(registrationInfo);

								sock1.sendData(registrationInfo, echoStringLen);
							}
							catch(SRPCSocketException &e)
							{
								cerr << e.info() << endl;
										//exit(1);	
							}
							break;
						}
						

					}// TODO Say Destination Not Available 
					
				}
				else if(requestType == "FileUpload")
				{
					
					int index2 = receivedData.find("IP_") + 3; 
					int length = receivedData.find(":Port_") - index2;
					string IP = receivedData.substr(index2,length); //server IP

					int index3 = receivedData.find(":Port_")+6; 
					length = receivedData.find("*") - index3;
					string port =  receivedData.substr(index3,length);
					if(Debug == 1)
					cout<<"\nDestination IP:"<<IP<<"\n";
					if(Debug == 1)
					cout <<"Destination Port:" << port <<"\n";				
					//cout << "Functionality_"<<func<<"\n\n";
					const char* const_port = port.c_str();
	
					unsigned short u_port = (unsigned short) strtoul(const_port, NULL, 0); //server port

					int index11 = receivedData.find("myPort_")+7;
					length = receivedData.find(":IP_") - index11;
					string clientPort = receivedData.substr(index11,length); 
					
					unsigned short c_port = (unsigned short) strtoul(clientPort.c_str(), NULL, 0); 

					int index12 = receivedData.find(":myip_")+6;
					length = receivedData.find(":myPort_") - index12;
					string ClientIP = receivedData.substr(index12, length);

					//check if the server is reachable in the Routing Table
					pthread_mutex_lock(&RT_mutex);
					int Cost = 6000;
					int flag1 = 0;
					string nextIP;
					unsigned short nextPort;
					if(Debug == 1)
					cout<<"=========== Checking if DESTINATION IS AVAILABLE ============="; 
					printRouting();
					for(Route_Table::iterator it = Rtable.begin(); it != Rtable.end(); it++)
					{
						if(Debug == 1)
						cout<<"\nRouter table IP:"<<it->second.destinationIP<<", R Port:"<<it->second.destinationPort<<"\n";
						if(strcmp(it->second.destinationIP.c_str(), IP.c_str()) == 0 && it->second.destinationPort == u_port)
						{
							nextIP = it->second.nexthopIP;
							nextPort = it->second.nexthopPort;
							Cost = it->second.Cost;
							flag1 = 1;
						}
					}
					pthread_mutex_unlock(&RT_mutex);
						if(flag1 == 1)
						{
							if(Debug == 1)
							cout<<"\nYes, destination available\n";
							if(Cost < 5000) //ensuring that the cost is not infinite
							{

								if(Debug == 1)
								cout<<"\n yes, cost less than 5000\n";
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
		    						if(NULL == myfile)
		    						{
									printf("Error opening file");
									//return 1;
		    						}

								int count =0;
								char frBuff[RECEIVE_BUFFER_SIZE]="";				
								int bytesReceived = 0;
								if(Debug == 1)
								cout<<"\nAbout to RECEIVE FILE\n";
								while((bytesReceived = sock->receiveData(frBuff,RECEIVE_BUFFER_SIZE)) > 0)
		    						{
									count++;
									if(Debug == 1)
									cout<<"Bytes received: "<<bytesReceived<<endl;    
									myfile.write(frBuff, bytesReceived);
									memset (frBuff,0,RECEIVE_BUFFER_SIZE);
									if(bytesReceived < RECEIVE_BUFFER_SIZE)
										break;
									//cout<< "written";
		       		
		    						}
								//cout<<"hello there";
								myfile.close();
								//TODO
								if(Debug == 1)
								cout<<"\nCOUNT ="<<count;
								if(count == 0)
									break;
								/*
								if(count == 0)
								{
									string Msg = "FileNotReceived";
									cout<<"message sent"<<Msg;
									const char *Info = Msg.c_str();
									int echoStringLen = strlen(Info);
									sock->sendData(Info, echoStringLen);
									break;
								}					
								else
								{

									string Msg = "FileAck";
									cout<<"message sent"<<Msg;
									const char *Info = Msg.c_str();
									int echoStringLen = strlen(Info);
									sock->sendData(Info, echoStringLen);
									break;
								}
								*/
								
								try{
									ClientSocket sock1(nextIP,nextPort);
									string str = "FileUpload:myip_"+ClientIP+":myPort_"+clientPort+":IP_"+IP+":Port_"+port+"*";
									const char *registrationInfo = str.c_str();
									int echoStringLen = strlen(registrationInfo);

									sock1.sendData(registrationInfo, echoStringLen);

									char echoBuffer3[RECEIVE_BUFFER_SIZE];
									int recvMsgSize3 = 0;
									string receivedData3 = "";
									while (true) 
									{
										//cout<<"Entered While: About to Receive"<<endl;
										recvMsgSize3 = sock1.receiveData(echoBuffer3, RECEIVE_BUFFER_SIZE);
										if(Debug == 1)
										cout<<"received message size:"<<recvMsgSize3<<"\n";
										string rd1(echoBuffer3);
			
										receivedData3 = receivedData3+rd1.substr(0,recvMsgSize3);
										if(Debug == 1)
										cout<<"\n Result :"<<receivedData3<<"\n";
										break;
									}
									int index5 = receivedData3.find(":");
									string requestType1 = receivedData3.substr(0,index5) ;
							if(Debug == 1)									
									cout<<"\nRequest type:"<<requestType1;
									if(requestType1 =="SendFile")
									{
										/*string msg1 = receivedData1;
										const char *Info1 = msg1.c_str();
										int echoStringLen1 = strlen(Info1);
										sock1.sendData(Info1, echoStringLen1);*/

										//send the file now
										FILE *mfile;
										if(Debug == 1)
										cout<<"\nFILE NAME: "<<temp1<<"\n";
										mfile = fopen(temp1.c_str(),"rb");
										if(!mfile)
										{
											if(Debug == 1)
											cout<<"could not open file:\n";
					
										}
										while(1)
										{
											if(Debug == 1)
											cout<<" stuck inside";
			
											char buff[1024]={0};
											int nread = fread(buff,1,1024,mfile);
											if(Debug == 1)
											cout<<"Bytes read: "<<nread<<"\n";  
					 						// If read was success, send data. 
											if(nread > 0)
											{
					     							printf("Sending \n");
												sock1.sendDataFile(buff, nread);
					     							
											}

											if (nread < 1024)
											{
						    						if (feof(mfile))
													printf("End of file\n");
						     						if (ferror(mfile))
													printf("Error reading\n");
						     						break;	
											}
			
		
										}
										
										/*char echoBuffer30[RECEIVE_BUFFER_SIZE];
										int recvMsgSize30 = 0;
										string receivedData30 = "";
										while (true) 
										{
											//cout<<"Entered While: About to Receive"<<endl;
											recvMsgSize30 = sock1.receiveData(echoBuffer30, RECEIVE_BUFFER_SIZE);
											cout<<"received message size:"<<recvMsgSize30<<"\n";
											string rd10(echoBuffer30);
			
											receivedData30 = receivedData30+rd10.substr(0,recvMsgSize30);
			
											cout<<"\n Result :"<<receivedData30<<"\n";
											break;
										}
										const char* response4 = receivedData30.c_str();
										int len4 = strlen(response4);
										sock->sendData(response4, len4);*/
									}
									else
									{
										char * response4 = "NotReachable:";
										int len4 = strlen(response4);
										sock->sendData(response4, len4);
										
									}

									

								}

								catch(SRPCSocketException &e)
								{
									cerr << e.info() << endl;
									//exit(1);	
								}

								break;							
							}
							else
							{
								if(Debug == 1)
								cout<<"\n no, cost more than 5000\n";
								char * response = "NotReachable:";
								int len = strlen(response);
								sock->sendData(response, len);
								break;
							}
		
						}
						else
						{
							if(Debug == 1)
							cout<<"\nNO, destination NOT available\n";
							char * response = "NotReachable:";
							int len = strlen(response);
							sock->sendData(response, len);
							break;
						}

					
					
					break;
					/*typedef struct route_table
					{
						string destinationIP;
						unsigned short destinationPort;

						string nexthopIP;
						unsigned short nexthopPort;

						int Cost;

					}routing;

					typedef map<string, routing> Route_Table;
					Route_Table Rtable;
	
					/*char * response = "SendFile:";
					int len = strlen(response);
					sock->sendData(response, len);*/

					
				}
				else if(requestType == "Alive")
				{
					char * response = "AliveAck:";
					int len = strlen(response);
					sock->sendData(response, len);
					
					cout<<"\nAlive ack sent \n";
					break;
				}
				else if(requestType == "Cost")
				{
					char * response = "CostAck:";
					int len = strlen(response);
					sock->sendData(response, len);
					if(Debug == 1)
					cout<<"\nCost ack sent \n";
					if(costcount==5)
						break;
					costcount++;
					
					continue;
				}
				else if(requestType == "LSA")
				{
					//check conditions whether to accept or reject the LSA
					//LSA:myIP_127.0.0.1:myport_4009*IP_127.0.0.1:Port_3007:Cost_25*#Sequence_1#Age_1$

					//1) check sequence number
					//2)check age.. + other criteria
					int IPindex = receivedData.find("myIP_")+5;
					int length = receivedData.find(":myport_") - IPindex;
					string sourceIP = receivedData.substr(IPindex, length);

					int portindex = receivedData.find("myport_")+7;
					length = receivedData.find("*") - portindex;
					string port = receivedData.substr(portindex, length);				

					unsigned short sourcePort = (unsigned short) strtoul(port.c_str(), NULL, 0);
										

					int seqindex = receivedData.find("#Sequence_")+10;
					length = receivedData.find("#Age_") - seqindex;
					string seq = receivedData.substr(seqindex, length);
				
					unsigned int Sequence = (unsigned int) strtoul(seq.c_str(), NULL, 0);

					int ageindex = 	receivedData.find("#Age_")+5;
					length  = receivedData.find("$") - ageindex;
					string age = receivedData.substr(ageindex, length);				

					unsigned int Age = (unsigned int) strtoul(age.c_str(), NULL, 0);
					if(Debug == 1)
					cout<<"\n RECEIVEED AGE: "<<Age<<"\n";

					//determining the CRC value of correct packet
					int crcindex = receivedData.find("CRC_")+4;
					int length1 = receivedData.find("&") - crcindex;
					string rcrc = receivedData.substr(crcindex, length1);
					if(Debug == 1)
					cout<<"\nstring received crc:"<<rcrc;		
					unsigned short u_crc = (unsigned short) strtoul(rcrc.c_str(), NULL, 0);

					if(Debug == 1)	
					cout<<"Received Short CRC:"<<u_crc;
					
					//calculating the CRC of the packet
					string crcpacket = receivedData.substr(0, receivedData.find("|"));
					
					unsigned short calc_CRC = crc16((char*)crcpacket.c_str(),strlen(crcpacket.c_str()));

					if(Debug == 1)
					cout<<"\nCalculated Short CRC:"<<calc_CRC;
					if(calc_CRC != u_crc)
					{
						char * response = "LSANack:";
						int len = strlen(response);
						sock->sendData(response, len);
						if(Debug == 1)
						cout<<"\nLSA NNack sent\n";
						break;
					}

					string LSAPacket = receivedData.substr(0, receivedData.find("$"))+"$";
					//cout<<
					//cout<<"\nLSA packet is: "<<LSAPacket<<"\n";
					//cout<<"\n Sequence No. is:"<<Sequence<<", Age is :"<<Age<<"\n";
					int presentflag = 0;
					for(LinkData::iterator it = Database.begin(); it != Database.end(); it++)
					{
						if(strcmp(it->second.sourceIP.c_str(), sourceIP.c_str())==0 && sourcePort == it->second.sourcePort)
						{
							presentflag = 1;
							if(Sequence > it->second.sequence)
							{
								//Store the new LSA in the Incoming-List
								pthread_mutex_lock(&list_mutex);
								eraseList(sourceIP,sourcePort,LSAPacket);
								pthread_mutex_unlock(&list_mutex);

								//store the new LSA in the LSD
								pthread_mutex_lock(&LSD_mutex);
								addToLSD(sourceIP,sourcePort,port,LSAPacket,Sequence,Age);
								pthread_mutex_unlock(&LSD_mutex);
								
							}
						}
						

					}
					if(presentflag == 0)
					{
						//store the LSA in the Incoming List
						pthread_mutex_lock(&list_mutex);
						eraseList(sourceIP,sourcePort,LSAPacket);
						pthread_mutex_unlock(&list_mutex);
						
						//store the new LSA in the LSD
						pthread_mutex_lock(&LSD_mutex);
						addToLSD(sourceIP,sourcePort,port,LSAPacket,Sequence,Age);
						pthread_mutex_unlock(&LSD_mutex);
					}
					
					char * response = "LSAAck:";
					int len = strlen(response);
					sock->sendData(response, len);
					
					//printLSD();
					//printIncomingList();

					if(Debug == 1)
					cout<<"\nLSA ack sent\n";
					break;

				}
				
				
			}

		}
		//sock->close1();
	}
	catch(SRPCSocketException &e)
	{
		cerr << e.info() << endl;
		exit(1);	
	}

}
/*typedef struct blacklist
{
	string IP;
	unsigned short port;
}blacklist;
blacklist bl[100];*/

void fetchRouters(string IPaddress, unsigned short inport, string dirAdd, unsigned short dirPort)
{

	//sleep(3);
	if(Debug == 1)
	cout<<"\ninside fetch routers ip:"<<IPaddress<<"port: "<<inport<<"\n";
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
			rd = string(echoBuffer);
			if(Debug == 1)
			cout<<"\n"<<rd<<"\n";
			break;
		}
		
		if(Debug == 1)
		cout<<"\n printing the string\n";
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
				if(Debug == 1)
				cout<<"\nPort: "<<port;
				if(Debug == 1)
				cout<<",IP: "<<IP;
				
					
				const char* const_port = port.c_str();
	
				unsigned short u_port = (unsigned short) strtoul(const_port, NULL, 0);
				int flag = 0;
				//Before Sending Neighbor Request, Check if the neighbor is blacklisted
				for(int i = 0;i < blkey; i++)
				{
					//check if the requesting neighbor is blacklisted
					if(strcmp(IP.c_str(), bl[i].IP.c_str()) == 0 )
					{
						flag = 1;

					}
				}
				//CHECK this condition when you test in ELEMENTSS
				if(Debug == 1)
				cout<<"\nBefore condition check: IP cmp: "<<strcmp(IP.c_str(), IPaddress.c_str())<<", Port CMP= "<<u_port <<","<<inport<<",\n";
				if(Debug == 1)
				cout<<"\nBefore condition chek2: IP1 length:"<<IP.length()<<"IP2 length:"<<IPaddress.length()<<"\n";
				if(Debug == 1)				
				cout<<"IP1: "<<IP<<", "<<"IP2: "<<IPaddress<<"\n";
				if((strcmp(IP.c_str(), IPaddress.c_str())!=0) || (u_port != inport))
				{

					if(flag == 0)
					{
						if(Debug == 1)
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
						ne.endSystem = 0;
						//ne.nullify = 0;
						neighbor_table[key++] = ne;
				
						if(Debug == 1)
						cout<<"\npart: "<<part<<"\n";
					}
				}
				part = "";
				

			}
			
		}
		sock.close1();

	}
	catch (SRPCSocketException &e) 
	{
			cerr << e.info() << endl;
			//exit(1);
	}
}

void registerMaster(unsigned short inport, string dirAdd, unsigned short dirPort, string function)
{
	//sleep(3);
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
				if(Debug == 1)
				cout << "Successfully registered in the directory service. Return message1:" << receivedData <<"\n";
				//setupServer(SERVER_SERVICE_PORT);
				//return 1;
			}
			else
			{
				if(Debug == 1)
				cout << "Failed to register in the directory service. Return message0:" << receivedData <<"\n";
				//return 0;
			}
			//Close the session regardless of the returned result.
			break;
		}

		//cout<<"created socket5";
		sock.close1();
		
	}
	catch (SRPCSocketException &e) 
	{
			cerr << e.info() << endl;
			//exit(1);
	}
}

//Reading From The Configuration File For Basic Setup
void readConfig()
{
	ifstream infile("config");
	string line;
	while(getline(infile, line))
	{
		if(Debug == 1)
		cout<<'\n'<<line<<'\n';	
		int index = line.find(":");
		string type = line.substr(0, index);
		if(type == "HelloInterval")
		{
			int index2 = line.find(":");
			int length = line.find("*") - index2-1;
			string hello = line.substr(index+1,length);
			if(Debug == 1)
			cout<<"\nstring hello:"<<hello<<"\n";
			HelloInterval = (int) strtoul(hello.c_str(), NULL, 0);
			if(Debug == 1)
			cout<<"Integer hello interval->"<<HelloInterval;
			
		}
		else if(type == "UpdateInterval")
		{
			int index2 = line.find(":");
			int length = line.find("*") - index2-1;
			string update = line.substr(index+1,length);
			if(Debug == 1)
			cout<<"\nstring update:"<<update<<"\n";
			UpdateInterval = (int) strtoul(update.c_str(), NULL, 0);
			if(Debug == 1)
			cout<<"Integer update interval->"<<UpdateInterval;
		}
		else if(type == "Pc")
		{
			int index2 = line.find(":");
			int length = line.find("*") - index2-1;
			string pc = line.substr(index+1,length);
			if(Debug == 1)
			cout<<"\nstring pc:"<<pc<<"\n";
			Pc = (int) strtoul(pc.c_str(), NULL, 0);
			if(Debug == 1)
			cout<<"Integer Pc->"<<Pc;
		}
		else if(type == "Pe")
		{
			int index2 = line.find(":");
			int length = line.find("*") - index2-1;
			string pe = line.substr(index+1,length);
			if(Debug == 1)
			cout<<"\nstring pe:"<<pe<<"\n";
			Pe = (int) strtoul(pe.c_str(), NULL, 0);
			if(Debug == 1)
			cout<<"Integer pe->"<<Pe;
		}
		else if(type == "MaxAge")
		{
			int index2 = line.find(":");
			int length = line.find("*") - index2-1;
			string maxage = line.substr(index+1,length);

			if(Debug == 1)
			cout<<"\nstring maxage:"<<maxage<<"\n";
			MaxAge = (int) strtoul(maxage.c_str(), NULL, 0);
			if(Debug == 1)
			cout<<"Integer maxage->"<<MaxAge;
		}
		else if(type == "NameServer")
		{
			int index2 = line.find(":");
			int length = line.find("*") - index2-1;
			NamePath = line.substr(index+1,length);
			if(Debug == 1)
			cout<<"\nstring NamePath:"<<NamePath<<"\n";
		}
		else if(type == "Debug")
		{
			int index2 = line.find(":");
			int length = line.find("*") - index2-1;
			string debug = line.substr(index+1,length);
			if(Debug == 1)
			cout<<"\nstring Debug:"<<debug<<"\n";

			
			Debug = (int) strtoul(debug.c_str(), NULL, 0);

			if(Debug == 1)
			cout<<"Integer Debug->"<<Debug;
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
			if(Debug == 1)
			cout<<"\nString update:"<<port<<"\n";
			NSPort = (unsigned short) strtoul(port.c_str(), NULL, 0);
			if(Debug == 1)
			cout<<"Name Server Port"<<NSPort;
		}
		
	}
}





void configure(string daddress, unsigned short dport)
{

	pthread_t tn[100];
	pthread_create(&tn[0],NULL,receive_part,NULL);	
	sleep(4);
	registerMaster(GlobalPort, daddress, dport,"receive");
	char buffer[32];
	snprintf(buffer, 32, "%hu",GlobalPort);
	string temp(buffer);
	Gport = temp;

	fetchRouters(GlobalAddress,GlobalPort, daddress, dport);

	while(1)
	{
		cout<<"\n------------------************************************--------------------\n";
		cout<<"\n                       A Simple Routing Protocol                          \n";
		cout<<"\n------------------************************************--------------------\n";
		cout<<"\n                              ---ROUTER---\n";
		sleep(2);
		cout<<"\nCONFIGURE this ROUTER ...\n";
		cout<<"...........\n";
		cout<<"Options \n";
		cout<<"...........\n";
		cout<<"\n# View List Of Registered Routers (Press 'ls')\n";
		cout<<"\n# Enter BlackListed Routers (Press 'bl')\n";
		cout<<"\n# Change Max Age Of The Router (Press 'age')\n";
		cout<<"\n# Change The Probability Of Introducing Error(Press 'Pe')\n";
		cout<<"\n# Change The Probability Of Dropping (Press 'Pc')\n";
		cout<<"\n-------------------------------------------------\n";
		cout<<"\n\n\nPress S to start routing ............\n"; 
		cout<<"\n=================================================\n";
		string input;

		getline(cin, input);
	
		if(input == "BL" || input == "bl" ||input == "bL" || input == "Bl")
		{
			while(1)
			{
				cout<<"Enter BLACKLIST Router IP:\n";
				string IP;
				getline(cin, IP);
				cout<<"Enter BLACKLIST Router Port:\n";
				string port;
				getline(cin, port);
				unsigned short u_port = (unsigned short) strtoul(port.c_str(), NULL, 0);
				bl[blkey].IP = IP;
				bl[blkey].port = u_port;
				blkey++;

				cout<<"\nContinue(Y/N)?\n";
				string cont;
				getline(cin, cont);
				if(cont == "Y" || cont == "y")
				{
					continue;
				}
				else
				{
					break;
				}			
			}
		}
		else if(input == "LS" || input == "ls" ||input == "lS" || input == "Ls")
		{
			
		}
		else if(input == "PE" ||input == "Pe" || input == "pe" || input == "pE")
		{
			cout<<"\nEnter Pe (between 0 and 1)";
			string pe;
			getline(cin, pe);
			double perror = (double)strtoul(pe.c_str(), NULL, 0);
			Pe = perror;
		}
		else if(input == "PC" ||input == "Pc" || input == "pc" || input == "pC")
		{
			cout<<"\nEnter Pc (between 0 and 1)";
			string pc;
			getline(cin, pc);
			double perror = (double)strtoul(pc.c_str(), NULL, 0);
			Pc = perror;
		}
		else if(input == "S" || input == "s")
		{
			cout<<"\nBlackListed Routers======";
			for(int i = 0; i < blkey; i++)
			{
				cout<<"\nIP: "<<bl[i].IP<<", Port: "<<bl[i].port<<"\n";
			}
			break;

		}
	}
	
	
	
	//GlobalPort = port;
	
	

		
		
	
	
		

	for(int i = 1; i<(key+1); i++)
	{
		cout<<"\ncreated thread: "<<i<<"\n";
		pthread_create(&tn[i],NULL,neighbor_acq,NULL);
	}

	pthread_create(&tn[key+1],NULL,alive_call,NULL);

	pthread_create(&tn[key+2],NULL,LSA_Gen,NULL);
	pthread_create(&tn[key+3],NULL,increaseAge,NULL);
	
	pthread_create(&tn[key+4],NULL,Routing_Table,NULL);
	pthread_create(&tn[key+5],NULL,Print,NULL);	

	for(int j = 0; j<(key+6); j++)
		pthread_join(tn[j],NULL);

	
} 

int main(int argc, char *argv[])
{
	//cout<<"created socket5";
	//unsigned short port = (unsigned short) strtoul(argv[1], NULL, 0);
//	cout<<"created socket6";
	//printf("%hu",port);
	//getting the IP Address of the machine
	GlobalAddress = IPlookup();
	//GlobalAddress = "127.0.0.1";
	//string daddress = argv[1];
	//unsigned short dport = (unsigned short) strtoul(argv[2], NULL, 0);
	//NSAddress = daddress;
	//NSPort = dport;
	//Read from the configuration File
	readConfig();
	readNameConfig();
	configure(NSAddress, NSPort);
	return 0;
}


/*
void* alive_call(void* args)
{
	cout <<"\nentered alive:\n";
	parameters* dim = (parameters*) args; 

   	string IP = dim->IP; 
	unsigned short port = dim->port;
	cout<<"IP:"<<dim->IP<<",port: "<<dim->port<<"\n";
	
	// Don't leak the memory
	//free(dim);
	while(1)
	{
		//pthread_mutex_lock(&alive_mutex);
		cout<<"alive";
		sleep(20);
		pthread_mutex_lock(&alive_mutex);
		alive(IP,port);
		pthread_mutex_lock(&alive_mutex);
		
		//pthread_mutex_unlock(&alive_mutex);
	}
	
	pthread_exit(NULL);
}
*/
/*if(it ->second.Cost == 0 && strcmp(it->second.destinationIP.c_str, GlobalAddress.c_str()) == 0 && it->second.destinationPort == GlobalPort)
								{
									//send reached 						
			
								}
								else if(it ->second.Cost != 0 && strcmp(it->second.destinationIP.c_str, GlobalAddress.c_str()) != 0 && it->second.destinationPort != GlobalPort)
								{
									//send 
									
								}
								else
								{

								}*/
