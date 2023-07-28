/*
this code is original from http://www.geeksforgeeks.org/greedy-algorithms-set-6-dijkstras-shortest-path-algorithm/ , but we made modification to it
*/
 
#include <stdio.h>
#include <limits.h>
 
// Number of vertices in the graph

#define Max 999999999


// A utility function to find the vertex with minimum distance value, from
// the set of vertices not yet included in shortest path tree
int minDistance(int dist[], bool sptSet[], int count)
{
   // Initialize min value
   int min = INT_MAX, min_index;
 
   for (int v = 0; v < count; v++)
     if (sptSet[v] == false && dist[v] <= min){
		 //nextHop[v] = v;
         min = dist[v], min_index = v;
	 	
	 }
 
   return min_index;
}
 
// A utility function to print the constructed distance array
/*int printSolution(int dist[], int n)
{
   printf("Destination \t next hop\t Distance from Source\n ");
   for (int i = 0; i < V; i++)
      printf("%d \t\t %d\t\t\t %d\n", i, nextHop[i], dist[i]);
   return 0;
}
 */

// Funtion that implements Dijkstra's single source shortest path algorithm
// for a graph represented using adjacency matrix representation
void dijkstra(int **graph, int src, int count)
{
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
     for (int i = 0; i < count; i++){
        dist[i] = INT_MAX, sptSet[i] = false,nextHop[i] = Max;
		for(int j= 0; j < count; j++){
			index[i][j] = 0;
		}
	}

for(int i=0;i<count;i++){
	printf("dist should be large numbers:%d",dist[i]);
}


     // Distance of source vertex from itself is always 0
     dist[src] = 0;
	 nextHop[src] = 0;
	 index[src][src] = 1;
	 index1[src][src] = 1;
     // Find shortest path for all vertices
     for (int c = 0; c < count-1; c++)
     {
       // Pick the minimum distance vertex from the set of vertices not
       // yet processed. u is always equal to src in first iteration.
       int u = minDistance(dist, sptSet,count);
       // Mark the picked vertex as processed
       sptSet[u] = true;
       // Update dist value of the adjacent vertices of the picked vertex.
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
	
	 for(int i=0;i<count;i++)
	 {
		 int m;
		
		 if(index2[i]==src)
		 {
			 nextHop[i]=i;
		 }
		 else
		 {
			 m=index2[i];
			 if(index2[m]==src)
			 {
				nextHop[i]=m;
			 }			
			 while(index2[m]!=src)
			 {
				 m=index2[m];
			 }
			 nextHop[i]=m;
		 }
	 }
	
 
     // print the constructed distance array
   //  printSolution(dist, V);

	printf("Destination \t next hop\t Distance from Source\n ");
   for (int i = 0; i < count; i++)
      printf("%d \t\t %d\t\t\t %d\n", i, nextHop[i], dist[i]);
}
 
// driver program to test above function

int main()
{
   //Let us create the example graph discussed above 
	int **graph = new int*[9];
	for(int i = 0; i<9; i++)
		graph[i] = new int[9];

	for(int i = 0; i< 9; i++)
		for(int j = 0; j<9;j++)
		{
			graph[i][j] =0;
		}

	graph[0][1]=4;
	graph[0][7] = 8;
	
	graph[1][0] = 6;
	graph[1][2] = 8;
	graph[1][7] = 11;

	graph[2][1] = 8;
	graph[2][3] = 7;
	graph[2][5] = 4;
	graph[2][8] = 2;

	graph[3][2] = 7;
	graph[3][4] = 9;
	graph[3][5] = 14;

	graph[4][3] = 9;
	graph[4][5] = 10;
	
	graph[5][2] = 4;
	graph[5][4] = 10;
	graph[5][6] = 2;

	graph[6][3] = 14;
	graph[6][5] = 2;
	graph[6][7] = 1;
	graph[6][8] = 6;

	graph[7][0] =8;
	graph[7][1] = 11;
	graph[7][6] = 1;
	graph[7][8] = 7;

	graph[8][2] = 2;
	graph[8][6] = 6;
	graph[8][7] = 7;

   /*graph = 	     {{0, 4, 0, 0, 0, 0, 0, 8, 0},
                      {4, 0, 8, 0, 0, 0, 0, 11, 0},
                      {0, 8, 0, 7, 0, 4, 0, 0, 2},
                      {0, 0, 7, 0, 9, 14, 0, 0, 0},
                      {0, 0, 0, 9, 0, 10, 0, 0, 0},
                      {0, 0, 4, 0, 10, 0, 2, 0, 0},
                      {0, 0, 0, 14, 0, 2, 0, 1, 6},
                      {8, 11, 0, 0, 0, 0, 1, 0, 7},
                      {0, 0, 2, 0, 0, 0, 6, 7, 0}
		     };*/
 
	//int graph[V][V] = {{0,3,10},{3,0,2},{10,2,0}};
    dijkstra(graph, 0,9);
	
	
	
 
    return 0;
}
