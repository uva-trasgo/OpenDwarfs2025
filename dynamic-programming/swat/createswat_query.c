#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/stat.h>//Cabecera añadida para crear directorios
#include<getopt.h>
#include<string.h>
#include<unistd.h>
#include<time.h>

#define DNA_BASE 1000

static struct option long_options[] = {
	/* name, has_arg, flag, val */
	{"Number of sequences", 1, NULL, 'n'},
	{"Length multiplier", 1, NULL, 'l'},
	{"seed", 1, NULL, 's'},
	{"help", 0, NULL, 'h'},
	{0,0,0,0}
};

/** Determines what element is next in a sequence and returns it the caller
 *  Uses Human DNA prob for each element 
 *  Source for DNA probabilities:
 *  https://flexbooks.ck12.org/cbook/ck-12-advanced-biology/section/8.3/primary/lesson/chargaffs-base-pairing-rules-advanced-bio-adv/ 
 **/
char getDNAElem(char prevE){
	int baseElem = rand()%50;	

	//Generic, if prev is empty of C or D
	if (prevE == ' ' || prevE == 'c' || prevE == 't')
	{
		if(baseElem<15)			//base A 30%
			return 'a';
		else if(baseElem<30)	//base T 30%
			return 't';
		else if(baseElem<40)	//base C 20%
			return 'c';
		else					//base G 20%
			return 'g';
		
	//Specific, if prevE is A or G
	}else{
		//recalculates the rand number
		//not really needed
		baseElem = rand()%10;
		if(prevE == 'a')
			return baseElem>=4 ? 'a' : 't'; // 50/50 chance
		else
			return baseElem>=4 ? 'g' : 'c'; // 50/50 chance
	}
}

/**Sets a path for the query files for swat**/
void query_path(char* var,int length,int number){

  	char* sz=calloc(sizeof(char),64);
 	int value;

 	//Calculate the length in KiB for the file name
	value=length/DNA_BASE;
    sprintf(sz, "%dK%d",value,number);

    //adds file name to path
	strcat(var,"query");
	strcat(var,sz);
}


/**Writes a file of a genetic sequence ramdomly generates, on the path given **/
/** Source for probabilities: https://flexbooks.ck12.org/cbook/ck-12-advanced-biology/section/8.3/primary/lesson/chargaffs-base-pairing-rules-advanced-bio-adv/*/
void write_query_file(char* file_path,int len,int num){	
	FILE* fp=NULL;
	int i,j,baseElem;
	char prevE = ' ';
	char elem = ' ';
	
	fp = fopen(file_path,"w");

	if(fp!=NULL){//If file and dir are corrects we create and fill the file
		
		for (i=0;i<num;i++){
			fprintf(fp,">lcl|%d\n",i);
			for (j=0;j<len;j++){
			
				elem = getDNAElem(prevE);
				//fprintf(fp,&elem);
				fwrite(&elem, sizeof( char ),1, fp);
				prevE = elem;
			}
			fprintf(fp,"\n");
		}
	}else{
		printf("Could nor open file");
		fclose(fp);
		exit(-1);
	}	
	fprintf(fp,"\n");
	fclose(fp);

}


int main(int argc, char** argv){

	const char* usage = "Usage: %s [-h] [-n multiplier] [-s seed] [-f dirpath]\n\n \
			-h: print help message\n\
			-l: Set multiplier for length of dna sequence. Default 1 with base 1024. \n\
			-n: Set number of sequences for query. Default 1 . \n\
			-s: Set a predetermines seed for the generation of files\n\
			-f: Tell the program a existent directory path to save the file generated\n\n";
	
	int opt,option_index=0;
	int dna_len=1024;
	int num_seq=1;
	long seed = -1;
	char* dir_path=NULL,*dir_path_data=NULL;

	while ((opt = getopt_long(argc, argv, "n:l:s:f:h", long_options, &option_index)) != -1 )
	{
		switch(opt)
		{
			case 'l':
				if(atoi(optarg) > 0)
					dna_len = atoi(optarg)*DNA_BASE;
				else{
					printf("Multiplier must be integer and greater than 0\n");
					exit(-1);
					}
				break;
			case 'n':
				if(atoi(optarg) > 0)
					num_seq = atoi(optarg);
				else{
					printf("Invalid num of sequences\n");
					exit(-1);
					}
				break;
			case 'h':
				fprintf(stderr, usage,argv[0]);
				exit(-1);
				break;
			case 's':
				if((seed = atol(optarg))<=-1){
					printf("Seed wrongly set. Using random one");
					seed = -1;
				}

			break;
			case 'f':
				if(optarg !=NULL){
					dir_path = calloc(sizeof(char),128);
					strcat(dir_path,optarg);
					printf("%s\n  %s\n",dir_path,optarg);
				}
			break;	
			default:
				fprintf(stderr, usage,argv[0]);
				exit(-1);
		}
	}

	if(seed <= -1)
		srand(time(NULL));
	else
		srand(seed);

	if(!dir_path){//if default dir path is non existant and no path was selected
		
		dir_path = calloc(sizeof(char),128);

	    strcat(dir_path,"../test/dynamic-programming/swat/");//Default path to write files test

	    //We cehck first for existent directory
		struct stat sb;//instanciación de sb para usar stat ()

	    if ( stat(dir_path, &sb) != 0 || !S_ISDIR(sb.st_mode) )
	    {
	        printf("Directory no found. Creating swat/ in test/dynamic-programming/\n");

	        mkdir("../test/", 0700)	;
	        mkdir("../test/dynamic-programming/", 0700)	;
	        mkdir("../test/dynamic-programming/swat/", 0700)	;
	    }

	}

	printf("\n 	creating query file acording to specs\n");

	query_path(dir_path,dna_len,num_seq);
	write_query_file(dir_path,dna_len,num_seq);

	free(dir_path);

	printf("Process done. Exiting now\n\n");
	exit(0);
	
}
