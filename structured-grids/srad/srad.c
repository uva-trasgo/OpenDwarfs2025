// includes, system
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
//#include "srad.h"
#include <string.h>
#include <malloc.h>
//#include "/home/pallavid/altera/14.0/hld/host/include/CL/cl_ext.h"


//#define OUTPUT
//defines form header srad.h tha are being used
//The header was removed due to only being used the next 2 defines
#define STR_SIZE 256
#define TIMER

#define AOCL_ALIGNMENT 64
int  BLOCK_SIZE = 16;
#include "../../include/rdtsc.h"
#include "../../include/common_args.h"

//If GPU is defined in srad.h the C implementation runs, otherwise the OpenCL one.

void random_matrix(float *I, int rows, int cols);
void runTest( int argc, char** argv);
void usage(int argc, char **argv)
{
	fprintf(stderr, "Usage: %s <rows> <cols> <y1> <y2> <x1> <x2> <lamda> <no. of iter> [platform & device]\n", argv[0]);
	fprintf(stderr, "\t<rows>   - number of rows\n");
	fprintf(stderr, "\t<cols>    - number of cols\n");
	fprintf(stderr, "\t<y1> 	 - y1 value of the speckle\n");
	fprintf(stderr, "\t<y2>      - y2 value of the speckle\n");
	fprintf(stderr, "\t<x1>       - x1 value of the speckle\n");
	fprintf(stderr, "\t<x2>       - x2 value of the speckle\n");
	fprintf(stderr, "\t<lamda>   - lambda (0,1)\n");
	fprintf(stderr, "\t<no. of iter>   - number of iterations\n");
	fprintf(stderr,"The following are optional to input.\n");
	fprintf(stderr,"\t[-B blocksize]: Set a specific blocksize for calculations.\n");
	fprintf(stderr,"\t[-o]: Make so output is written to a file.\n");
	exit(EXIT_FAILURE);
}


	int
main( int argc, char** argv) 
{
	ocd_init(&argc, &argv, NULL);	
	runTest( argc, argv);
	ocd_finalize();
	return EXIT_SUCCESS;
}


	void
runTest( int argc, char** argv) 
{
	int rows, cols, size_I, size_R, niter = 10, iter;
	float *I, *J, lambda, q0sqr, sum, sum2, tmp, meanROI,varROI ;
	int i, j, k, out=0;


	cl_program clProgram;
	cl_kernel clKernel_srad1;
	cl_kernel clKernel_srad2;

	cl_int errcode;

	cl_mem J_cuda;
	cl_mem C_cuda;
	cl_mem E_C, W_C, N_C, S_C;

	FILE *kernelFile;
	char *kernelSource;
	size_t kernelLength;

	ocd_initCL();



   
     char* kernel_files;
     int num_kernels = 1;
     kernel_files = (char*) malloc(sizeof(char*)*num_kernels);
	 strcpy(kernel_files,"srad_kernel");
        
     clProgram=ocdBuildProgramFromFile(context,device_id,kernel_files, NULL);

	clKernel_srad1 = clCreateKernel(clProgram, "srad_cuda_1", &errcode);
	CHKERR(errcode, "Failed to create kernel!");
	clKernel_srad2 = clCreateKernel(clProgram, "srad_cuda_2", &errcode);
	CHKERR(errcode, "Failed to create kernel!");

	unsigned int r1, r2, c1, c2;
	int *c;



	if (argc >= 9)
	{
		rows = atoi(argv[1]);  //number of rows in the domain
		cols = atoi(argv[2]);  //number of cols in the domain
		if ((rows%16!=0) || (cols%16!=0)){
			fprintf(stderr, "rows and cols must be multiples of 16\n");
			exit(1);
		}
		r1   = atoi(argv[3]);  //y1 position of the speckle
		r2   = atoi(argv[4]);  //y2 position of the speckle
		c1   = atoi(argv[5]);  //x1 position of the speckle
		c2   = atoi(argv[6]);  //x2 position of the speckle
		lambda = atof(argv[7]); //Lambda value
		niter = atoi(argv[8]); //number of iterations

		if (argc>9){
			if(strcmp(argv[9],"-B")==0){
				BLOCK_SIZE = atoi(argv[10]) >=16 ? atoi(argv[10]) : 16;
				out = argc==12 ? 1 : 0;
			
			}else if(strcmp(argv[9],"-o")==0){
				out++;
				if(strcmp(argv[10],"-B")==0)
					BLOCK_SIZE = atoi(argv[11]) >=16 ? atoi(argv[11]) : 16;
			}
			else
				usage(argc, argv);
		}

		printf("Blocksize set to %d\n",BLOCK_SIZE);

	}
	else{
		usage(argc, argv);
	}



	size_I = cols * rows;
	size_R = (r2-r1+1)*(c2-c1+1);   

	I = (float*) memalign(AOCL_ALIGNMENT, sizeof(float)*size_I );
	J = (float*) memalign(AOCL_ALIGNMENT, sizeof(float)*size_I );
	c  = (int*) memalign(AOCL_ALIGNMENT, sizeof(int)*size_I) ;


	int prev_BLOCKSIZE = BLOCK_SIZE;
	size_t max_worksize[3];
	errcode = clGetDeviceInfo(device_id, CL_DEVICE_MAX_WORK_ITEM_SIZES,sizeof(size_t)*3, &max_worksize, NULL);
	CHKERR(errcode, "Failed to get device info!");
	while(BLOCK_SIZE*BLOCK_SIZE>max_worksize[0])
		BLOCK_SIZE = BLOCK_SIZE/2;

	printf("Blocksize Reescaled to %d size\n",BLOCK_SIZE);




	//Allocate device memory
	J_cuda = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(float)*size_I, NULL, &errcode);
	CHKERR(errcode, "Failed to create buffer!");
	C_cuda = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(float)*size_I, NULL, &errcode);
	CHKERR(errcode, "Failed to create buffer!");
	E_C = clCreateBuffer(context, CL_MEM_READ_WRITE , sizeof(float)*size_I, NULL, &errcode);
	CHKERR(errcode, "Failed to create buffer!");
	W_C = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(float)*size_I, NULL, &errcode);
	CHKERR(errcode, "Failed to create buffer!");
	S_C = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(float)*size_I, NULL, &errcode);
	CHKERR(errcode, "Failed to create buffer!");
	N_C = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(float)*size_I, NULL, &errcode);
	CHKERR(errcode, "Failed to create buffer!");


	printf("Randomizing the input matrix\n");
	//Generate a random matrix
	random_matrix(I, rows, cols);

	for (k = 0;  k < size_I; k++ ) {
		J[k] = (float)exp(I[k]) ;
	}
	printf("Start the SRAD main loop\n");
	for (iter=0; iter< niter; iter++){     
		sum=0; sum2=0;
		for (i=r1; i<=r2; i++) {
			for (j=c1; j<=c2; j++) {
				tmp   = J[i * cols + j];
				sum  += tmp ;
				sum2 += tmp*tmp;
			}
		}
		meanROI = sum / size_R;
		varROI  = (sum2 / size_R) - meanROI*meanROI;
		q0sqr   = varROI / (meanROI*meanROI);

		//Currently the input size must be divided by 16 - the block size
		int block_x = cols/BLOCK_SIZE ;
		int block_y = rows/BLOCK_SIZE ;

		size_t localWorkSize[2] = {BLOCK_SIZE, BLOCK_SIZE};
		size_t globalWorkSize[2] = {block_x*localWorkSize[0], block_y*localWorkSize[1]};


		//Copy data from main memory to device memory
		errcode = clEnqueueWriteBuffer(commands, J_cuda, CL_TRUE, 0, sizeof(float)*size_I, (void *) J, 0, NULL, &ocdTempEvent);

		clFinish(commands);
		START_TIMER(ocdTempEvent, OCD_TIMER_H2D, "SRAD Data Copy", ocdTempTimer)
		END_TIMER(ocdTempTimer)
		CHKERR(errcode, "Failed to enqueue write buffer!");

		//Run kernels
		errcode = clSetKernelArg(clKernel_srad1, 0, sizeof(cl_mem), (void *) &E_C);
		errcode |= clSetKernelArg(clKernel_srad1, 1, sizeof(cl_mem), (void *) &W_C);
		errcode |= clSetKernelArg(clKernel_srad1, 2, sizeof(cl_mem), (void *) &N_C);
		errcode |= clSetKernelArg(clKernel_srad1, 3, sizeof(cl_mem), (void *) &S_C);
		errcode |= clSetKernelArg(clKernel_srad1, 4, sizeof(cl_mem), (void *) &J_cuda);
		errcode |= clSetKernelArg(clKernel_srad1, 5, sizeof(cl_mem), (void *) &C_cuda);
		errcode |= clSetKernelArg(clKernel_srad1, 6, sizeof(int), (void *) &cols);
		errcode |= clSetKernelArg(clKernel_srad1, 7, sizeof(int), (void *) &rows);
		errcode |= clSetKernelArg(clKernel_srad1, 8, sizeof(float), (void *) &q0sqr);
		CHKERR(errcode, "Failed to set kernel arguments!");
		errcode = clEnqueueNDRangeKernel(commands, clKernel_srad1, 2, NULL, globalWorkSize, localWorkSize, 0, NULL, &ocdTempEvent);
		clFinish(commands);
		START_TIMER(ocdTempEvent, OCD_TIMER_KERNEL, "SRAD Kernel srad1", ocdTempTimer)
		END_TIMER(ocdTempTimer)
		CHKERR(errcode, "Failed to enqueue kernel!");
		errcode = clSetKernelArg(clKernel_srad2, 0, sizeof(cl_mem), (void *) &E_C);
		errcode |= clSetKernelArg(clKernel_srad2, 1, sizeof(cl_mem), (void *) &W_C);
		errcode |= clSetKernelArg(clKernel_srad2, 2, sizeof(cl_mem), (void *) &N_C);
		errcode |= clSetKernelArg(clKernel_srad2, 3, sizeof(cl_mem), (void *) &S_C);
		errcode |= clSetKernelArg(clKernel_srad2, 4, sizeof(cl_mem), (void *) &J_cuda);
		errcode |= clSetKernelArg(clKernel_srad2, 5, sizeof(cl_mem), (void *) &C_cuda);
		errcode |= clSetKernelArg(clKernel_srad2, 6, sizeof(int), (void *) &cols);
		errcode |= clSetKernelArg(clKernel_srad2, 7, sizeof(int), (void *) &rows);
		errcode |= clSetKernelArg(clKernel_srad2, 8, sizeof(float), (void *) &lambda);
		errcode |= clSetKernelArg(clKernel_srad2, 9, sizeof(float), (void *) &q0sqr);
		CHKERR(errcode, "Failed to set kernel arguments!");
		errcode = clEnqueueNDRangeKernel(commands, clKernel_srad2, 2, NULL, globalWorkSize, localWorkSize, 0, NULL, &ocdTempEvent);
		clFinish(commands);
		START_TIMER(ocdTempEvent, OCD_TIMER_KERNEL, "SRAD Kernel srad2", ocdTempTimer)
		END_TIMER(ocdTempTimer)
		CHKERR(errcode, "Failed to enqueue kernel!");

		//Copy data from device memory to main memory
		errcode = clEnqueueReadBuffer(commands, J_cuda, CL_TRUE, 0, sizeof(float)*size_I, (void *) J, 0, NULL, &ocdTempEvent);
		clFinish(commands);
		START_TIMER(ocdTempEvent, OCD_TIMER_D2H, "SRAD Data Copy", ocdTempTimer)
		END_TIMER(ocdTempTimer)
		CHKERR(errcode, "Failed to enqueue read buffer!");


	}

	printf("Computation Done\n");

	//saving output to file	
	if (out){

		printf("Writting Output to srad_out.txt:\n"); 
		FILE* outfile= fopen("srad_out.txt","w");
		check(outfile != NULL,"srad_writting_output - Cannot Open File");	
		if(rows==cols)
			fprintf(outfile,"%d\n",rows);
		else
			fprintf(outfile,"%d\n%d\n",rows,cols);

		for( i = 0 ; i < rows ; i++){
			for ( j = 0 ; j < cols ; j++)
				fprintf(outfile,"%.5f ", J[i * cols + j]); 
			fprintf(outfile,"\n"); 
		}

		printf("Dwarfs SRAD has finished generating output file.\n");
		fclose (outfile);
	}
	free(I);
	free(J);

	clReleaseMemObject(C_cuda);
	clReleaseMemObject(J_cuda);
	clReleaseMemObject(E_C);
	clReleaseMemObject(W_C);
	clReleaseMemObject(N_C);
	clReleaseMemObject(S_C);
	clReleaseKernel(clKernel_srad1);
	clReleaseKernel(clKernel_srad2);
	clReleaseProgram(clProgram);
	clReleaseCommandQueue(commands);
	clReleaseContext(context);

	free(c);

}


void random_matrix(float *I, int rows, int cols){

	int i, j;
	srand(7);

	for( i = 0 ; i < rows ; i++){
		for ( j = 0 ; j < cols ; j++){
			I[i * cols + j] = rand()/(float)RAND_MAX ;
		}
	}

}
