#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/stat.h>//Cabecera añadida para crear directorios
#include<getopt.h>
#include<string.h>
#include<unistd.h>
#include<time.h>

static struct option long_options[] = {
	/* name, has_arg, flag, val */
	{"sizes", 1, NULL, 'n'},
	{"help", 0, NULL, 'h'},
	{0,0,0,0}
};

/**makes array more uniform by upping every position, while reducing the last, by 1*/
void uniform_array(int* array,int size,int sum_array){
	int uniform=-1;
	float var = 0.0;
	do{
		for (int i=0;i<size;i++){
			array[i]++;
			array[size-1] --;
		}

		//checks if array in last is 1/3 or less of total of array
		var = (float)(array[size-1]);
		if ( (var/sum_array) <= 0.15)
			uniform=0;

	}while(uniform==-1);

}

/** Selects and writes edges for a graph with N depth **/
void write_edges_graph_v2(FILE* fp,int nNodes, long nEdges, int* edgesPerNode, int depth){

	long nEdgesDepth=0;
	int edgesWritten=0,nodeId=0,prevNodeId=0,counter;

	if(fp!=NULL){
		for (int d = 0; d < depth; ++d)
		{

			//set numbre of edges to write on iteration
			if((d+1) != depth)
				nEdgesDepth = rand() % (nEdges/depth) + (nEdges/depth)/3;
			else
				nEdgesDepth = nEdges - edgesWritten;
			

			fprintf(fp,"%ld\n",nEdgesDepth);
			counter = 0;

			//writting edge graph process
			while(counter < nEdgesDepth){
				nodeId = rand()%nNodes;
		
				//are there still edges to write and not equal to previous one
				if( edgesPerNode[nodeId]>0){
				//if( edgesPerNode[nodeId]>0 && nodeId != prevNodeId){
					fprintf(fp,"%d %ld\n",nodeId,rand()%nEdges);
					
					edgesPerNode[nodeId]--;
					prevNodeId = nodeId;
					counter++;
				}
			}

			edgesWritten += nEdgesDepth; 

			if((d+1)!=depth)
				fprintf(fp,"0\n");
		}
	}else
	{
		printf("File closed unexpectedly");
		exit (-1);
	}
}
/** Writes node graph of bfs file**/
void write_node_graph_v1_1(char* file, int nNodes, long totalEdges, int depth){
	

	FILE* fp=NULL;
	int* nEdgesNode = calloc(sizeof(int*),nNodes);	
	int allocatedEdges=0, nodeEdges=0, maxEdgesPerNode=totalEdges/nNodes;
	float check=0.0;//neded for check previous to write edge graph
	fp = fopen(file,"w");

	if(fp!=NULL){//If file and dir are corrects we create start the process
		fprintf(fp,"%d\n",nNodes);

		for(int i=0; (i<nNodes) && (allocatedEdges < totalEdges); i++){
			
			//A way to distribute the edges as evenly as posible on big graphs			
			nodeEdges = (rand() % maxEdgesPerNode)+ 1;

			//Check to make sure totalEdges == current edges at the end
			if( (allocatedEdges + nodeEdges ) > totalEdges ){
				nodeEdges = totalEdges - allocatedEdges;
			}
			else if((i + 1) == nNodes && (allocatedEdges + nodeEdges ) < totalEdges ){
				nodeEdges = totalEdges - allocatedEdges;
			}

			nEdgesNode[i] = nodeEdges;
			allocatedEdges +=nodeEdges;

		}

		check = (float)(nEdgesNode[nNodes-1]);//cast variable to make operation 
		//checks if last node has 1/3 of total edges to distribute the graph
		if( (check/totalEdges)>= 0.2 )
			uniform_array(nEdgesNode,nNodes,totalEdges);

		//Writes the node graph
		for (int i=0;i<nNodes;i++)
			fprintf(fp,"%d %d\n",i,nEdgesNode[i]);

		//acts as an indicator where edgesgraph starts
		fprintf(fp,"0\n");
		write_edges_graph_v2(fp,nNodes,totalEdges,nEdgesNode,depth);
		//write_edges_graph_v1(fp,nNodes,totalEdges,nEdgesNode);
		//write_edges_graph_sec(fp,nNodes,totalEdges,nEdgesNode);
	
	}else{
		printf("could nor open file");
		exit (-1);

	}	
	fprintf(fp,"\n");
	fclose(fp);
	free(nEdgesNode);
}


int main(int argc, char** argv){
	
	const char* usage = "Usage: %s [-h] [-n nodes] [-e edges] [-f dirpath]\n\n \
				-h: print hel message\n\
				-n: Set num of nodes of node graph. Default 100\n\
				-e: Set num of edges of edge graph. Default 300\n\
				-d: Sets depth of the graph (Edges per Depths). Default 1. Currently W.I.P. \n\
				-f: Tell the program a existent directory path to save the file generated\n\n";

	
	int opt,option_index=0;
	int num_nodes=100,depth=1,seed=-1;
	long num_edges=300;
	char* dir_path=NULL;

	while ((opt = getopt_long(argc, argv, "n:e:d:f:h", long_options, &option_index)) != -1 )
	{
		switch(opt)
		{

			case 'h':
				fprintf(stderr, usage,argv[0]);
				exit(-1);
			break;

			case 'n':
				if(atoi(optarg) >= 10)
					num_nodes = atoi(optarg);
				else{
					printf("Values can't be float or smaller than 10\n");
					exit(-1);
				}
			break;

			case 'e':	
				if(atol(optarg) >= (num_nodes*3))
					num_edges = atol(optarg);
				else{
					printf("Values can't be float.\nIt must also be 3 times bigger than number of nodes\n");
					exit(-1);
				}
			break;
			case 'd':	
				if(atoi(optarg) >0)
					depth = atoi(optarg);
				else{
					printf("Values can't be float, or negatives\n");
					exit(-1);
				}
			break;

			case 'f':
				if(optarg !=NULL){
					dir_path = calloc(sizeof(char),128);
					strcat(dir_path,optarg);
				}
			break;	
			default:
				fprintf(stderr, usage,argv[0]);
				exit(-1);
		}
	}

	//Simple checks before generation
	if( num_edges <(num_nodes*3)){
		printf ("Edges is too small.\nMust be at leatst 3 times bigger than number of Nodes used.\n");
		printf("Number of Nodes == %d\nNumber of Edges == %ld\n",num_nodes,num_edges);
		printf("Exiting program\n");
		exit(-1);
	}else if( depth < 0){
		printf ("Depth set for the system are smaller to 0.\n");
		printf("Exiting program\n");
		exit(-1);
	}

	if(seed > 0)
		srand (seed);
	else
		srand (time (NULL));

	if(!dir_path){//if default dir path is non existant and no`path was selected
   		dir_path=calloc(sizeof(char),128);
   		strcat(dir_path,"../test/graph-traversal/bfs/");

	    //We cehck first for existent directory
		struct stat sb;//instanciación de sb para usar stat ()

	    if ( stat(dir_path, &sb) != 0 || !S_ISDIR(sb.st_mode) )
	    {
	        printf("Directory no found. Creating bfs/ at test/graph-traversal/\n");

	        mkdir("../test/", 0700)	;
	        mkdir("../test/graph-traversal/", 0700)	;
	        mkdir("../test/graph-traversal/bfs/", 0700)	;
	    }

	}

	printf("\n	Creating file for dwarf BFS\n");

  	char* sz=calloc(sizeof(char),32);
  	
  	//making the file name and adds it to path
    sprintf(sz, "%d.txt", num_nodes);
	strcat(dir_path,"graph_input");
	strcat(dir_path,sz);

	write_node_graph_v1_1(dir_path,num_nodes,num_edges,depth);
	free(sz);
	free(dir_path);

	printf("\nProcess done. Exiting now\n\n");
	exit(0);
	
}