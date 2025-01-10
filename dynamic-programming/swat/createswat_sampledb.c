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
	{"Length multiplier", 1, NULL, 'l'},
	{"seed", 1, NULL, 's'},
	{"help", 0, NULL, 'h'},
	{0,0,0,0}
};

/** Determines what element is next in a sequence and returns it the caller
 * Uses Human DNA prob for each element
/** Source for DNA probabilities:
 *  https://flexbooks.ck12.org/cbook/ck-12-advanced-biology/section/8.3/primary/lesson/chargaffs-base-pairing-rules-advanced-bio-adv/ **/
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
			return baseElem>=4 ? 'a' : 't';
		else
			return baseElem>=4 ? 'g' : 'c';
	}
}



/**Sets a path for the sampledbNK1 and sampledbNK1.data files**/
void sampledb_path(char* var,char* var2,int len,int num_seq){

 	char* sz=calloc(sizeof(char),64);
	int value;


 	//Calculate the length in KB for the file name
	value=len/DNA_BASE;
    sprintf(sz, "%dK%d",value,num_seq);

    //adds file names to path
	strcat(var,"sampledb");
	strcat(var,sz);
	strcpy(var2,var);
	strcat(var2,".data");

	free(sz);
}

/**Writes a .loc file, given the dir of sampledb file. Used only in write_swat_sampledb**/
void write_loc_file(char* file,int data_len, int num_seq){

	char* loc_path=calloc(sizeof(char),64);
	FILE* fp=NULL, *fpl=NULL;

	strcat(loc_path,file);
	strcat(loc_path,".loc");
	fpl = fopen(loc_path,"wb");

	//We write the file here, in groups of 4 bytes aka 8 hexdecimal digits
	if (fpl)
	{
		//we get the number of KiB and claculates dataKB
		int num_KiBs=(data_len * num_seq)/DNA_BASE;
		int data_KB[1] = {(data_len * num_seq)/(1000 * num_KiBs)};//this works for our purose, somehow.

		fwrite(data_KB,4,1,fpl);

		//multiply data_KB by 4 times num_KiB
		data_KB[0] = data_KB[0] * ( 4 * num_KiBs);
		fwrite(data_KB,4,1,fpl);

	}else{
		printf("Error writing file %s\n",loc_path);
	}

	fclose(fpl);
	free(loc_path);
}



/**Same as  write_swat_query but for sampledb. It also creates .data and .loc files for the program**/
void write_sampledb_file(char* file_path,char* file_path2, int len,int num_seq){	

	FILE* fp=NULL;
	FILE* fpb=NULL;
	int i,j,baseElem;
	char prevE = ' ';
	char elem = ' ';

	fp = fopen(file_path,"w");
	fpb = fopen(file_path2,"wb");
	if(fp!=NULL && fpb!=NULL){//If file and dir are corrects we create and fill the file
		for (i=0;i<num_seq;i++){
			fprintf(fp,">lcl|%d\n",i);
			for (j=0;j<len;j++){
			

				elem = getDNAElem(prevE);
				//fprintf(fp,&elem);
				fwrite(&elem, sizeof( char ),1, fp);
				prevE = elem;

				//For printng the element in file .data
				if(elem == 'A')//base A 30%
					fwrite("^@",sizeof("^@"),1,fpb);

				else if(elem == 'T')//base T 30%
					fwrite("^Q",sizeof("^Q"),1,fpb);
				
				else if(elem == 'C')//base C 20%
					fwrite("^B",sizeof("^B"),1,fpb);
				
				else//base G 20%
					fwrite("^F",sizeof("^F"),1,fpb);
				



				// if(baseElem<=2){//base A 30%
				// 	fprintf(fp,"a");
				// 	fwrite("^@",sizeof("^@"),1,fpb);
				// 	//fwrite("^@",sizeof(char),3,fpb);//Also write the binary correct with this option
				
				// }else if(baseElem<=5){//base T 30%
				// 	fprintf(fp,"t");
				// 	fwrite("^Q",sizeof("^Q"),1,fpb);
				// 	prev = 'T';
				
				// }else if(baseElem<=7){//base C 20%

				// 	fprintf(fp,"c");
				// 	fwrite("^B",sizeof("^B"),1,fpb);
				// 	prev = 'C';
				
				// }else{//base G 20%
				// 	fprintf(fp,"g");
				// 	fwrite("^F",sizeof("^F"),1,fpb);
				// 	prev = 'G';
				// }
			}
			fprintf(fp,"\n");
			fprintf(fpb,"\n");
		}
	}else{
		printf("could nor open file");
		fclose(fp);
		fclose(fpb);

		exit(-1);
	}	

	fclose(fp);
	fclose(fpb);
	write_loc_file(file_path,len,num_seq);
}




int main(int argc, char** argv){

	const char* usage = "Usage: %s [-h] [-n multiplier] [-s seed] [-f dirpath]\n\n \
			-h: print help message\n\
			-l: Set multiplier for length of dna sequence. Default 1 with base 1024. \n\
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
					dir_path_data = calloc(sizeof(char),128);
					strcat(dir_path,optarg);
					strcat(dir_path_data,optarg);
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
		dir_path_data = calloc(sizeof(char),128);

	    strcat(dir_path,"../test/dynamic-programming/swat/");//Default path to write files test
	    strcat(dir_path_data,"../test/dynamic-programming/swat/");//Default path to write files test

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

	printf("\n 	creating sampledbs files acording to specs\n\n");
	sampledb_path(dir_path,dir_path_data,dna_len,num_seq);
	write_sampledb_file(dir_path,dir_path_data,dna_len,num_seq);

	free(dir_path);
	free(dir_path_data);

	printf("Process done. Exiting now\n\n");
	exit(0);
	
}
