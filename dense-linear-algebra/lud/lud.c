#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include "../../include/rdtsc.h"
#include "../../include/common_args.h"
#include "common.h"

//#define USEGPU 1
int BLOCK_SIZE = 16;
static int do_verify = 0;
static int do_write = 0;


static struct option long_options[] = {
	/* name, has_arg, flag, val */
	{"input", 1, NULL, 'i'},
	{"platform", 1, NULL, 'p'},
	{"device", 1, NULL, 'd'},
	{"size", 1, NULL, 's'},
	{"verify", 0, NULL, 'v'},
	{0,0,0,0}
};

int is2powerRecursive(int value){
	int isPower=1;
	
	if (value == 1)
		return 0;
	else if(value%2 == 0 && value > 1)
		isPower = is2powerRecursive(value/2);
	else 
		return 1;

	return isPower;
}
void printHelp(){
			fprintf(stderr, "Usage for dwarf lud : \n\
		OpenCL Options:\n\
			-p : Id platfom OpenCL.\n\
			-d : Id device OpenCL.\n\
			-t : device type to use.\n\
		Lud Specific options:\n\
			[-h]: Print this help message.\n\
			[-v]: Verify the dwarfs results.\n\
			[-w]: Same as verify but also print matrix.\n\
			[-s matrix_size]: Set matrix size to use. Currently WIP.\n\
			[-i input_file]: Path to matrix file to test.\n\
			[-l BLOCK_SIZE]: Allows user to set an initital BLOCK_SIZE. it affects both local and global work size\n\
		Execution example:\n\
			./lud -p 0 -d 0 -- -v -i <path_to_file>\n");
}

	int
main ( int argc, char *argv[] )
{
	int matrix_dim = 32; /* default matrix_dim */
	int opt, option_index=0;
	func_ret_t ret;
	const char *input_file = NULL;
	float *m, *mm;
	char* output_file = NULL;
	stopwatch sw;


	cl_program clProgram;
	cl_kernel clKernel_diagonal;
	cl_kernel clKernel_perimeter;
	cl_kernel clKernel_internal;
	cl_int dev_type;

	cl_int errcode;

	FILE *kernelFile;
	char *kernelSource;
	size_t kernelLength;

	cl_mem d_m;

	ocd_init(&argc, &argv, NULL);
	ocd_initCL();

	while ((opt = getopt_long(argc, argv, "::hvwi:s:l:", 
					long_options, &option_index)) != -1 ) {
		switch(opt){
			case 'h':
			printHelp();
			exit(EXIT_FAILURE);
				break;
			case 'i':
				input_file = optarg;
				break;
			case 'v':
				if(do_write)
					do_verify = 0;
				else
					do_verify = 1;
				break;
			case 'w':
				do_write = 1;
				do_verify = 0;
				break;
			case 's':
				matrix_dim = atoi(optarg);
				//printf("Matrix dimension == %d\n",matrix_dim);
			fprintf(stderr, "Currently not supported, use -i instead\n");
			printHelp();
				exit(EXIT_FAILURE);
			break;
			case 'l':
				if(is2powerRecursive(atoi(optarg)) == 0 )
					BLOCK_SIZE = atoi(optarg);
				else{
					printf("Invalid BLOCK_SIZE. Exiting\n");
					exit(EXIT_FAILURE);
				}
			break;
			case '?':
				fprintf(stderr, "\n\
		Error.invalid option. Exiting dwarf.\n\n");
				exit(EXIT_FAILURE);
				break;

			case ':':
				fprintf(stderr, "missing argument\n");
				//break;
			default:
			printHelp();
				exit(EXIT_FAILURE);
		}
	}

	if ( (optind < argc) || (optind == 1)) {
		printHelp();
		exit(EXIT_FAILURE);
	}

	if (input_file) {
		printf("Reading matrix from file %s\n", input_file);
		ret = create_matrix_from_file(&m, input_file, &matrix_dim);
		if (ret != RET_SUCCESS) {
			m = NULL;
			fprintf(stderr, "error create matrix from file %s\n", input_file);
			exit(EXIT_FAILURE);
		}
	} else {
		printf("No input file specified!\n");
		exit(EXIT_FAILURE);
	}

	if (do_write){
		printf("Before LUD\n");
		print_matrix(m, matrix_dim);
	}
	if (do_verify || do_write){
		matrix_duplicate(m, &mm, matrix_dim);
	}


	size_t max_worksize[3];
	errcode = clGetDeviceInfo(device_id, CL_DEVICE_MAX_WORK_ITEM_SIZES,sizeof(size_t)*3, &max_worksize, NULL);
	CHKERR(errcode, "Failed to get device info!");

	//Start by 16*16, but if not allowed divide by two until MAX_WORK_ITEM_SIZES is less or equal than what we are going to ask for.
	//Changed to 64*64
	 while(BLOCK_SIZE*BLOCK_SIZE>max_worksize[0])
	 	BLOCK_SIZE = BLOCK_SIZE/2;

	char arg[100];
 	char* kernel_files;
	int num_kernels = 1;
	sprintf(arg,"-D BLOCK_SIZE=%d", (int)BLOCK_SIZE);
	kernel_files = (char*) malloc(sizeof(char*)*num_kernels);
	strcpy(kernel_files,"lud_kernel");

	clProgram = ocdBuildProgramFromFile(context,device_id,kernel_files,arg);

	clKernel_diagonal = clCreateKernel(clProgram, "lud_diagonal", &errcode);
	CHKERR(errcode, "Failed to create kernel!");
	clKernel_perimeter = clCreateKernel(clProgram, "lud_perimeter", &errcode);
	CHKERR(errcode, "Failed to create kernel!");
	clKernel_internal = clCreateKernel(clProgram, "lud_internal", &errcode);
	CHKERR(errcode, "Failed to create kernel!");

    d_m = clCreateBuffer(context, CL_MEM_READ_WRITE, matrix_dim*matrix_dim*sizeof(float), NULL, &errcode);
	CHKERR(errcode, "Failed to create buffer!");

	/* beginning of timing point */
	stopwatch_start(&sw);

	errcode = clEnqueueWriteBuffer(commands, d_m, CL_TRUE, 0, matrix_dim*matrix_dim*sizeof(float), (void *) m, 0, NULL, &ocdTempEvent);

	clFinish(commands);
	START_TIMER(ocdTempEvent, OCD_TIMER_H2D, "Matrix Copy", ocdTempTimer)
		END_TIMER(ocdTempTimer)
		CHKERR(errcode, "Failed to enqueue write buffer!");

	int i=0;
	size_t localWorkSize[2];
	size_t globalWorkSize[2];

#ifdef START_POWER
	for( int iter = 0; iter < 1000; iter++)
#endif
		for (i=0; i < matrix_dim-BLOCK_SIZE; i += BLOCK_SIZE) {
			errcode = clSetKernelArg(clKernel_diagonal, 0, sizeof(cl_mem), (void *) &d_m);
			errcode |= clSetKernelArg(clKernel_diagonal, 1, sizeof(int), (void *) &matrix_dim);
			errcode |= clSetKernelArg(clKernel_diagonal, 2, sizeof(int), (void *) &i);
			CHKERR(errcode, "Failed to set kernel arguments!");

			localWorkSize[0] = BLOCK_SIZE;
			globalWorkSize[0] = BLOCK_SIZE;

			errcode = clEnqueueNDRangeKernel(commands, clKernel_diagonal, 1, NULL, globalWorkSize, localWorkSize, 0, NULL, &ocdTempEvent);
			clFinish(commands);
		    //printf("max Work-item Size2: %d\n",(int)max_worksize[0]);	
			START_TIMER(ocdTempEvent, OCD_TIMER_KERNEL, "Diagonal Kernels", ocdTempTimer)
				END_TIMER(ocdTempTimer)
				CHKERR(errcode, "Failed to enqueue kernel!");
			errcode = clSetKernelArg(clKernel_perimeter, 0, sizeof(cl_mem), (void *) &d_m);
			errcode |= clSetKernelArg(clKernel_perimeter, 1, sizeof(int), (void *) &matrix_dim);
			errcode |= clSetKernelArg(clKernel_perimeter, 2, sizeof(int), (void *) &i);
			CHKERR(errcode, "Failed to set kernel arguments!");

			localWorkSize[0] = BLOCK_SIZE*2;
			globalWorkSize[0] = ((matrix_dim-i)/BLOCK_SIZE-1)*localWorkSize[0];

			errcode = clEnqueueNDRangeKernel(commands, clKernel_perimeter, 1, NULL, globalWorkSize, localWorkSize, 0, NULL, &ocdTempEvent);
			clFinish(commands);
			START_TIMER(ocdTempEvent, OCD_TIMER_KERNEL, "Perimeter Kernel", ocdTempTimer)
				CHKERR(errcode, "Failed to enqueue kernel!");
			END_TIMER(ocdTempTimer)
				errcode = clSetKernelArg(clKernel_internal, 0, sizeof(cl_mem), (void *) &d_m);
			errcode |= clSetKernelArg(clKernel_internal, 1, sizeof(int), (void *) &matrix_dim);
			errcode |= clSetKernelArg(clKernel_internal, 2, sizeof(int), (void *) &i);
			CHKERR(errcode, "Failed to set kernel arguments!");
			
			localWorkSize[0] = BLOCK_SIZE;
			localWorkSize[1] = BLOCK_SIZE;
			globalWorkSize[0] = ((matrix_dim-i)/BLOCK_SIZE-1)*localWorkSize[0];
			globalWorkSize[1] = ((matrix_dim-i)/BLOCK_SIZE-1)*localWorkSize[1];

			errcode = clEnqueueNDRangeKernel(commands, clKernel_internal, 2, NULL, globalWorkSize, localWorkSize, 0, NULL, &ocdTempEvent);
			clFinish(commands);
			START_TIMER(ocdTempEvent, OCD_TIMER_KERNEL, "Internal Kernel", ocdTempTimer)
				END_TIMER(ocdTempTimer)
				CHKERR(errcode, "Failed to enqueue kernel!");
		}
	errcode = clSetKernelArg(clKernel_diagonal, 0, sizeof(cl_mem), (void *) &d_m);
	errcode |= clSetKernelArg(clKernel_diagonal, 1, sizeof(int), (void *) &matrix_dim);
	errcode |= clSetKernelArg(clKernel_diagonal, 2, sizeof(int), (void *) &i);
	CHKERR(errcode, "Failed to set kernel arguments!");
	localWorkSize[0] = BLOCK_SIZE;
	globalWorkSize[0] = BLOCK_SIZE;

	errcode = clEnqueueNDRangeKernel(commands, clKernel_diagonal, 1, NULL, globalWorkSize, localWorkSize, 0, NULL, &ocdTempEvent);
	clFinish(commands);
	START_TIMER(ocdTempEvent, OCD_TIMER_KERNEL, "Diagonal Kernels", ocdTempTimer)
		CHKERR(errcode, "Failed to enqueue kernel!");
	END_TIMER(ocdTempTimer)

		errcode = clEnqueueReadBuffer(commands, d_m, CL_TRUE, 0, matrix_dim*matrix_dim*sizeof(float), (void *) m, 0, NULL, &ocdTempEvent);
	clFinish(commands);
	START_TIMER(ocdTempEvent, OCD_TIMER_D2H, "Matrix copy", ocdTempTimer)
		END_TIMER(ocdTempTimer)
		/* end of timing point */
	stopwatch_stop(&sw);

	clReleaseMemObject(d_m);
	//Saves output to a file to compare


	if (do_verify){
		printf("Generating output to files\n");
		
		printf(">>>Verify<<<<\n");
		printf("matrix_dim: %d\n",matrix_dim);
		write_matrix(m, matrix_dim,"matrix_LU.dat");
		lud_verify(mm, m, matrix_dim);

		free(mm);
		printf("Generated output as matrix_LU.dat and res.dat \n");

	}else if (do_write){
		printf("Generating output to files\n");
		
		printf("After LUD\n");
		print_matrix(m, matrix_dim);
		printf(">>>Verify<<<<\n");
		printf("matrix_dim: %d\n",matrix_dim);
		write_matrix(m, matrix_dim,"matrix_LU.dat");
		lud_verify(mm, m, matrix_dim); 

		free(mm);
		printf("Generated output as matrix_LU.dat and res.dat \n");
	}





	clReleaseKernel(clKernel_diagonal);
	clReleaseKernel(clKernel_perimeter);
	clReleaseKernel(clKernel_internal);
	clReleaseProgram(clProgram);
	clReleaseCommandQueue(commands);
	clReleaseContext(context);

	free(m);
	ocd_finalize();
	return EXIT_SUCCESS;
}				/* ----------  end of function main  ---------- */
