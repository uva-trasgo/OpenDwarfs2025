#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/stat.h>//Cabecera añadida para crear directorios
#include<getopt.h>
#include<string.h>
#include<unistd.h>
#include<math.h>
#include<time.h>
#include<omp.h>



static struct option long_options[] = {
	/* name, has_arg, flag, val */
	{"sizes", 1, NULL, 'n'},
	{"help", 0, NULL, 'h'},
	{0,0,0,0}
};


/** Generates a matrix that should a LU Decomposition.
 * It multplies 2 Random Matrixes, a Lowr triangle like and Upper triangle like matrix.
 * 
 * It's quite slow on bigger matrixes due to matrix product being so slow in computers CPU
 * TODO: try and make an implementation with OpenCL in the future.
**/
void write_lud_OMP(char* file_path, int sz)
{	
	FILE* fp=NULL;
	float value;
	float* ML = malloc(sizeof(float)*sz*sz);
	float* MU = malloc(sizeof(float)*sz*sz);
	float* MR = malloc(sizeof(float)*sz*sz);

	int ex=1;
	fp = fopen(file_path,"w");

	if(fp!=NULL){//If file and dir are corrects we create and fill the file

		fprintf(fp,"%d\n",sz);
		//seems to cause less errors making matrix this way
		// for(int i=0; i<sz; i++){
		// 	for(int j=0; j<sz; j++){
		// 		MU [i*sz+j] = j>=i ? (float)rand()/(float)(RAND_MAX) : 0.0;
		// 		ML [i*sz+j] = i==j ? 1 : (i<j ? (float)rand()/(float)(RAND_MAX):0.0);//Saved trasposed for better use of PC memory
		// 	}
		// }
		
		//We create an Upper and lower matrix
		//for efficiency we save also the lower as an upper for later in the product
		#pragma omp parallel for
		for(int i=0; i<sz; i++){

		 	for(int j=i; j<sz; j++)
		 		MU [i*sz+j] = (float)rand()/(float)(RAND_MAX);
			
		 	for(int j=i; j<sz; j++)
		 		ML [i*sz+j] =(float)rand()/(float)(RAND_MAX);//Saved trasposed for better use of PC memory
		}
		#pragma omp parallel for
		for(int i=0; i<sz; i++)
			ML [i*sz+i]=1;//Saved trasposed for better use of PC memory
		

		printf("Matrix MU and ML are generated\n");

		//We used OMP to try and improve the product time
		//Since all are saved like an upper matrix we can reduce the iterations on loop K to start from I		
		#pragma omp parallel for
		for(int i=0; i<sz; i++)	
			for(int j=0; j<sz; j++)
				for(int k=i; k<sz; k++)
					MR[i*sz+j] += MU [i*sz+k] * ML [j*sz+k]; 
		

		printf("Matrix MR is generated\n");
		printf("Writting it to a file\n");
	
		for(int i=0; i<sz; i++){	
			for(int j=0; j<sz; j++)
				fprintf(fp,"%.5f ",MR[i*sz+j]);
			fprintf(fp,"\n");
		}

	}else{
		printf("could nor open file");
		exit(-1);
	}	
	fprintf(fp,"\n");

	fclose(fp);
	free(MR);
	free(ML);
	free(MU);

}



int main(int argc, char** argv){
	
	const char* usage = "Usage: %s [-h] [-n sizes] [-f dirpath]\n\n \
				-h print hel message\n\
				-n: Set matrix sized and number of matrix. Value must have a square root\n\
				-f: Tell the program a existent directory path to save the file generated\n\n";

	
	srand(time(NULL));
	int opt,option_index=0;
	int size=64;
	char* dir_path=NULL;
  	char* sz=calloc(sizeof(char),32);

	while ((opt = getopt_long(argc, argv, "n:f:h", long_options, &option_index)) != -1 )
	{
		switch(opt)
		{
			case 'n':
				if(atoi(optarg) > 0)
					size = atoi(optarg);
				else
					printf("Values can't be float or smaller than 0");
				break;
			case 'h':
				fprintf(stderr, usage,argv[0]);
				exit(-1);
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


	if(!dir_path){//if default dir path is non existant and no`path was selected
   		dir_path=calloc(sizeof(char),128);
   		strcat(dir_path,"../test/dense-linear-algebra/lud/");

	    //We cehck first for existent directory
		struct stat sb;//instanciación de sb para usar stat ()

	    if ( stat(dir_path, &sb) != 0 || !S_ISDIR(sb.st_mode) )
	    {
	        printf("Directory no found. Creating lud/ in test/dense-linear-algebra/\n");

	        mkdir("../test/", 0700)	;
	        mkdir("../test/dense-linear-algebra/", 0700)	;
	        mkdir("../test/dense-linear-algebra/lud/", 0700)	;
	    }

	}

  	
    sprintf(sz, "%d.dat", size);
	strcat(dir_path,sz);
	
	printf("\n\n 	Creating matrix acording to specs\n\n");

 	write_lud_OMP(dir_path,size);



	free(sz);
	free(dir_path);

	printf("Process done. Exiting now\n\n");
	exit(0);
}
