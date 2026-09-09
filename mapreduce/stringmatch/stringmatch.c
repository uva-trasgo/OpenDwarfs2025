#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "../../include/rdtsc.h"
#include "../../include/common_args.h"

#define DEFAULT_WARP_SIZE 64
#define DEFAULT_TASK_NUM 1
#define STRINGMATCH_NAME_MAX_LENGTH 256

typedef enum _FUNC_RETURN_CODE
{
    RET_SUCCESS,
    RET_FAILURE
} func_ret_t;

typedef struct __stopwatch_t
{
    struct timeval begin;
    struct timeval end;
} stopwatch;

static struct option long_options[] = {
    /* name, has_arg, flag, val */
    {"platform", 1, NULL, 'p'},
    {"device", 1, NULL, 'd'},
    {"device-type", 1, NULL, 't'},
    {"input-file", 1, NULL, 'i'},
    {"help", 0, NULL, 'h'},
    {"result-file", 0, NULL, 'o'},
    {0, 0, 0, 0}};

// RNG for targets selection and text generation
#define RNG_SEED 42u

static unsigned int rng_state;

static void rng_seed(unsigned int seed)
{
    rng_state = seed ? seed : 1u;
}

static unsigned int rng_next(void)
{
    unsigned int x = rng_state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    rng_state = x;
    return x;
}

void stopwatch_start(stopwatch *sw)
{
    if (sw == NULL)
        return;

    bzero(&sw->begin, sizeof(struct timeval));
    bzero(&sw->end, sizeof(struct timeval));

    gettimeofday(&sw->begin, NULL);
}

void stopwatch_stop(stopwatch *sw)
{
    if (sw == NULL)
        return;

    gettimeofday(&sw->end, NULL);
}

void printHelp()
{
    fprintf(stderr, "Usage for dwarf stringmatch : \n\
		OpenCL Options:\n\
			-p                  : Id platfom OpenCL.\n\
			-d                  : Id device OpenCL.\n\
			-t                  : Device type to use.\n\
		Stringmatch Specific options:\n\
			-h                  : Print this help message.\n\
			-i input_file       : Path to keys-text file to evaluate.\n\
            -o                  : Save the results in a file.\n\
		Execution example:\n\
			./stringmatch -p 0 -d 0 -- -i <path_to_file>\n");
}

void write_stringmatch(FILE *out, int maxKeyLen, int num_targets,
                       cl_int4 *h_outputOffsetSizes, char *h_outputKeys,
                       char *h_outputVals, cl_uint outputKeysBufferSize)
{
    fprintf(out, "%-*s  %s\n", maxKeyLen, "Key", "Occurrences");
    for (int i = 0; i < num_targets; i++)
    {
        if (h_outputOffsetSizes[i].w > 0 && h_outputOffsetSizes[i].x < (int)outputKeysBufferSize)
        {
            int keySize = h_outputOffsetSizes[i].y;
            char *key = h_outputKeys + h_outputOffsetSizes[i].x;
            int *vals = (int *)(h_outputVals + h_outputOffsetSizes[i].z);
            fprintf(out, "%-*.*s  %d\n", maxKeyLen, keySize, key, vals[0]);
        }
    }
}

int main(int argc, char *argv[])
{
    int opt, option_index = 0;
    cl_int errcode;

    /* Benchmark-specific parameters with defaults */
    const char *input_file = NULL;
    int num_targets;
    int print_result = 0;
    char outputFile[STRINGMATCH_NAME_MAX_LENGTH];

    func_ret_t ret;
    stopwatch sw;
    cl_uint warp_size = 64;

    /* OpenCL objects */
    cl_program clProgram;
    cl_kernel clKernel_mapper;
    cl_kernel clKernel_reducer;
    cl_mem d_inputDataset, d_inputKeys, d_inputVals, d_inputOffsetSizes;
    cl_mem d_targetOffsetSizes, d_targetBucketOffsets, d_targetBucketCapacities, d_targetMatchCounts;
    cl_mem d_interVals, d_interOffsetSizes;
    cl_mem d_psKeySizes, d_psValSizes, d_psCounts;
    cl_mem d_outputKeys, d_outputVals, d_outputOffsetSizes;

    /* Host-side data pointers */
    char *h_inputDataset = NULL;
    char *h_inputKeys = NULL;
    char *h_inputVals = NULL;
    cl_int4 *h_inputOffsetSizes = NULL;
    cl_int2 *h_targetOffsetSizes = NULL;
    cl_uint *h_targetBucketOffsets = NULL;
    cl_uint *h_targetBucketCapacities = NULL;
    cl_uint *h_targetMatchCounts = NULL;
    char *h_outputKeys = NULL;
    char *h_outputVals = NULL;
    cl_int4 *h_outputOffsetSizes = NULL;

    size_t fileSize = 0;
    size_t targetsSize = 0;
    int recordNum, taskNum, recordsPerTask;
    cl_uint warpSize = DEFAULT_WARP_SIZE;

    FILE *kernelFile;
    char *kernelSource;
    size_t kernelLength;

    cl_mem d_m;

    ocd_init(&argc, &argv, NULL);
    ocd_initCL();

    while ((opt = getopt_long(argc, argv, ":i:h:o", long_options, &option_index)) != -1)
    {
        printf("opt=%c (%d), optarg=%s\n", opt, opt, optarg ? optarg : "NULL");
        switch (opt)
        {
        case 'i':
            input_file = optarg;
            break;
        case 'o':
            print_result = 1;
            break;
        case 'h':
            printHelp();
            exit(EXIT_FAILURE);
        case '?':
            fprintf(stderr, "\nError.invalid option. Exiting dwarf.\n\n");
            exit(EXIT_FAILURE);
        case ':':
            fprintf(stderr, "\nMissing argument\n");
        default:
            printHelp();
            exit(EXIT_FAILURE);
        }
    }

    if (!input_file || (optind < argc) || (optind == 1))
    {
        printHelp();
        exit(EXIT_FAILURE);
    }

    /* --- Load input data from --input-file --- */
    int file_targets_count;

    /* Read targets and text from file */
    printf("Reading input from file %s\n", input_file);
    FILE *fp = fopen(input_file, "r");
    if (!fp)
    {
        fprintf(stderr, "Cannot open input file\n");
        exit(EXIT_FAILURE);
    }

    /* Get total file size */
    fseek(fp, 0, SEEK_END);
    size_t totalSize = ftell(fp);
    rewind(fp);

    /* Read the entire file into a buffer */
    char *fileBuffer = (char *)malloc(totalSize + 1);
    fread(fileBuffer, 1, totalSize, fp);
    fileBuffer[totalSize] = '\0';
    fclose(fp);

    /* Find the first newline to split targets from text */
    char *newline = strchr(fileBuffer, '\n');
    if (!newline)
    {
        fprintf(stderr, "Input file must have targets on the first line\n");
        exit(EXIT_FAILURE);
    }

    /* Extract targets (first line) */
    targetsSize = newline - fileBuffer;
    h_inputKeys = (char *)malloc(targetsSize + 1);
    memcpy(h_inputKeys, fileBuffer, targetsSize);
    h_inputKeys[targetsSize] = '\0';

    /* Extract text (everything after the first line) */
    fileSize = totalSize - (targetsSize + 1);
    printf("Using text file (%zu bytes)\n", fileSize);

    h_inputDataset = (char *)malloc(fileSize + 1);
    memcpy(h_inputDataset, newline + 1, fileSize);
    h_inputDataset[fileSize] = '\0';

    /* Count targets from the file */
    file_targets_count = 1;
    for (size_t i = 0; i < targetsSize; i++)
    {
        if (h_inputKeys[i] == ',')
            file_targets_count++;
    }
    num_targets = file_targets_count;

    free(fileBuffer);

    printf("Loaded %d targets, text size: %zu bytes\n", num_targets, fileSize);

    /* --- Query device limits --- */
    size_t max_worksize[3];
    errcode = clGetDeviceInfo(device_id, CL_DEVICE_MAX_WORK_ITEM_SIZES, sizeof(size_t) * 3, &max_worksize, NULL);
    CHKERR(errcode, "Failed to get device info!");

    while (warpSize > max_worksize[0])
    {
        warpSize = warpSize / 2;
    }
    if (warpSize < 1)
        warpSize = 1;

    /* --- Build OpenCL kernels --- */
    clProgram = ocdBuildProgramFromFile(context, device_id, "stringmatch_kernel", NULL);
    clKernel_mapper = clCreateKernel(clProgram, "mapper", &errcode);
    CHKERR(errcode, "Failed to create mapper kernel!");
    clKernel_reducer = clCreateKernel(clProgram, "reducer", &errcode);
    CHKERR(errcode, "Failed to create reducer kernel!");

    /* --- Split the text into lines. These become the mapper's records, so
       the amount of available parallelism scales with the size of the text
       being searched instead of staying pinned at num_targets. --- */
    int numLinesEstimate = 0;
    for (size_t i = 0; i < fileSize; i++)
    {
        if (h_inputDataset[i] == '\n')
            numLinesEstimate++;
    }
    if (fileSize > 0 && h_inputDataset[fileSize - 1] != '\n')
        numLinesEstimate++;
    if (numLinesEstimate < 1)
        numLinesEstimate = 1;

    int *h_lineOffsets = (int *)malloc(sizeof(int) * numLinesEstimate);
    int *h_lineSizes = (int *)malloc(sizeof(int) * numLinesEstimate);
    int numLines = 0;
    {
        size_t lineStart = 0;
        for (size_t i = 0; i <= fileSize; i++)
        {
            if (i == fileSize || h_inputDataset[i] == '\n')
            {
                int lineLen = (int)(i - lineStart);
                if (lineLen > 0)
                {
                    h_lineOffsets[numLines] = (int)lineStart;
                    h_lineSizes[numLines] = lineLen;
                    numLines++;
                }
                lineStart = i + 1;
            }
        }
    }
    printf("Split text into %d line record(s)\n", numLines);
    if (numLines < 1)
        numLines = 1; /*empty text*/

    /* --- Prepare MapReduce metadata for the MAPPER (records = lines) --- */
    recordNum = numLines;
    taskNum = (recordNum + (int)warpSize - 1) / (int)warpSize; /* ~1 line per thread */
    if (taskNum < 1)
        taskNum = 1;
    recordsPerTask = (recordNum + taskNum - 1) / taskNum;

    /* --- Populate input records: one {lineOffset,lineSize} pair per line.
       .x/.y (the old "key" fields) are unused now -- a line has no single
       key of its own, it gets checked against every target inside map(). --- */
    h_inputVals = (char *)calloc(numLines, sizeof(int) * 2);
    h_inputOffsetSizes = (cl_int4 *)calloc(numLines, sizeof(cl_int4));
    for (int i = 0; i < numLines; i++)
    {
        cl_int *val_buf = (cl_int *)(h_inputVals + i * sizeof(int) * 2);
        val_buf[0] = (i < numLinesEstimate) ? h_lineOffsets[i] : 0;
        val_buf[1] = (i < numLinesEstimate) ? h_lineSizes[i] : 0;

        h_inputOffsetSizes[i].x = 0;
        h_inputOffsetSizes[i].y = 0;
        h_inputOffsetSizes[i].z = i * (cl_int)sizeof(int) * 2;
        h_inputOffsetSizes[i].w = sizeof(int) * 2;
    }
    free(h_lineOffsets);
    free(h_lineSizes);

    /* --- Target table: one {offset, size} pair per search target, shared
       read-only by every mapper thread (same parsing as before, now writing
       into a small dedicated array sized by num_targets instead of by
       recordNum, since recordNum means something different now). --- */
    h_targetOffsetSizes = (cl_int2 *)calloc(num_targets, sizeof(cl_int2));

    char *target_ptr = h_inputKeys;
    for (int i = 0; i < num_targets; i++)
    {
        int keyOffset = target_ptr - h_inputKeys;

        char *comma = strchr(target_ptr, ',');
        int keySize;
        if (comma)
        {
            keySize = comma - target_ptr;
        }
        else
        {
            keySize = strlen(target_ptr);
        }

        char *singleTarget = (char *)malloc(keySize + 1);
        memcpy(singleTarget, target_ptr, keySize);
        singleTarget[keySize] = '\0';
        memcpy(h_inputKeys + keyOffset, singleTarget, keySize + 1);

        h_targetOffsetSizes[i].x = keyOffset;
        h_targetOffsetSizes[i].y = keySize;

        if (comma)
        {
            target_ptr = comma + 1;
        }
        else
        {
            target_ptr += keySize;
        }

        free(singleTarget);
    }

    cl_uint targetBucketCapacity = (cl_uint)(numLines * 4);
    if (targetBucketCapacity < 256)
        targetBucketCapacity = 256;

    h_targetBucketOffsets = (cl_uint *)calloc(num_targets, sizeof(cl_uint));
    h_targetBucketCapacities = (cl_uint *)calloc(num_targets, sizeof(cl_uint));
    h_targetMatchCounts = (cl_uint *)calloc(num_targets, sizeof(cl_uint));
    for (int t = 0; t < num_targets; t++)
    {
        h_targetBucketOffsets[t] = (cl_uint)t * targetBucketCapacity;
        h_targetBucketCapacities[t] = targetBucketCapacity;
    }
    cl_uint totalBucketSlots = (cl_uint)num_targets * targetBucketCapacity;

    cl_uint outputKeysBufferSize = (cl_uint)num_targets * 1024; /* generous: concatenated target text */
    h_outputKeys = (char *)calloc(outputKeysBufferSize, 1);
    h_outputVals = (char *)calloc(num_targets * sizeof(int), 1);
    h_outputOffsetSizes = (cl_int4 *)calloc(num_targets, sizeof(cl_int4));

    /* --- Create device buffers --- */
    d_inputDataset = clCreateBuffer(context, CL_MEM_READ_ONLY, fileSize + 1, NULL, &errcode);
    d_inputKeys = clCreateBuffer(context, CL_MEM_READ_ONLY, targetsSize + 1, NULL, &errcode);
    d_inputVals = clCreateBuffer(context, CL_MEM_READ_ONLY, numLines * sizeof(int) * 2, NULL, &errcode);
    d_inputOffsetSizes = clCreateBuffer(context, CL_MEM_READ_ONLY, numLines * sizeof(cl_int4), NULL, &errcode);

    d_targetOffsetSizes = clCreateBuffer(context, CL_MEM_READ_ONLY, num_targets * sizeof(cl_int2), NULL, &errcode);
    d_targetBucketOffsets = clCreateBuffer(context, CL_MEM_READ_ONLY, num_targets * sizeof(cl_uint), NULL, &errcode);
    d_targetBucketCapacities = clCreateBuffer(context, CL_MEM_READ_ONLY, num_targets * sizeof(cl_uint), NULL, &errcode);
    d_targetMatchCounts = clCreateBuffer(context, CL_MEM_READ_WRITE, num_targets * sizeof(cl_uint), NULL, &errcode);

    d_interVals = clCreateBuffer(context, CL_MEM_READ_WRITE, totalBucketSlots * sizeof(int), NULL, &errcode);
    d_interOffsetSizes = clCreateBuffer(context, CL_MEM_READ_WRITE, totalBucketSlots * sizeof(cl_int4), NULL, &errcode);

    d_psKeySizes = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(cl_uint), NULL, &errcode);
    d_psValSizes = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(cl_uint), NULL, &errcode);
    d_psCounts = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(cl_uint), NULL, &errcode);

    d_outputKeys = clCreateBuffer(context, CL_MEM_WRITE_ONLY, outputKeysBufferSize, NULL, &errcode);
    d_outputVals = clCreateBuffer(context, CL_MEM_WRITE_ONLY, num_targets * sizeof(int), NULL, &errcode);
    d_outputOffsetSizes = clCreateBuffer(context, CL_MEM_WRITE_ONLY, num_targets * sizeof(cl_int4), NULL, &errcode);

    /* --- Transfer input data to device --- */
    stopwatch_start(&sw);

    errcode = clEnqueueWriteBuffer(commands, d_inputDataset, CL_TRUE, 0, fileSize + 1, h_inputDataset, 0, NULL, &ocdTempEvent);
    errcode |= clEnqueueWriteBuffer(commands, d_inputKeys, CL_TRUE, 0, targetsSize + 1, h_inputKeys, 0, NULL, &ocdTempEvent);
    errcode |= clEnqueueWriteBuffer(commands, d_inputVals, CL_TRUE, 0, numLines * sizeof(int) * 2, h_inputVals, 0, NULL, &ocdTempEvent);
    errcode |= clEnqueueWriteBuffer(commands, d_inputOffsetSizes, CL_TRUE, 0, numLines * sizeof(cl_int4), h_inputOffsetSizes, 0, NULL, &ocdTempEvent);
    errcode |= clEnqueueWriteBuffer(commands, d_targetOffsetSizes, CL_TRUE, 0, num_targets * sizeof(cl_int2), h_targetOffsetSizes, 0, NULL, &ocdTempEvent);
    errcode |= clEnqueueWriteBuffer(commands, d_targetBucketOffsets, CL_TRUE, 0, num_targets * sizeof(cl_uint), h_targetBucketOffsets, 0, NULL, &ocdTempEvent);
    errcode |= clEnqueueWriteBuffer(commands, d_targetBucketCapacities, CL_TRUE, 0, num_targets * sizeof(cl_uint), h_targetBucketCapacities, 0, NULL, &ocdTempEvent);
    errcode |= clEnqueueWriteBuffer(commands, d_targetMatchCounts, CL_TRUE, 0, num_targets * sizeof(cl_uint), h_targetMatchCounts, 0, NULL, &ocdTempEvent);
    clFinish(commands);
    CHKERR(errcode, "Failed to enqueue write buffers!");

    START_TIMER(ocdTempEvent, OCD_TIMER_H2D, "Input Copy", ocdTempTimer)
    END_TIMER(ocdTempTimer)

    /* --- Execute Mapper Kernel --- */
    {
        int argIdx = 0;
        errcode = clSetKernelArg(clKernel_mapper, argIdx++, sizeof(cl_mem), (void *)&d_inputDataset);
        errcode |= clSetKernelArg(clKernel_mapper, argIdx++, sizeof(cl_mem), (void *)&d_inputOffsetSizes);
        errcode |= clSetKernelArg(clKernel_mapper, argIdx++, sizeof(cl_mem), (void *)&d_inputVals);
        errcode |= clSetKernelArg(clKernel_mapper, argIdx++, sizeof(cl_mem), (void *)&d_inputKeys);
        errcode |= clSetKernelArg(clKernel_mapper, argIdx++, sizeof(cl_mem), (void *)&d_targetOffsetSizes);
        errcode |= clSetKernelArg(clKernel_mapper, argIdx++, sizeof(int), (void *)&num_targets);
        errcode |= clSetKernelArg(clKernel_mapper, argIdx++, sizeof(cl_mem), (void *)&d_targetBucketOffsets);
        errcode |= clSetKernelArg(clKernel_mapper, argIdx++, sizeof(cl_mem), (void *)&d_targetBucketCapacities);
        errcode |= clSetKernelArg(clKernel_mapper, argIdx++, sizeof(cl_mem), (void *)&d_targetMatchCounts);
        errcode |= clSetKernelArg(clKernel_mapper, argIdx++, sizeof(cl_mem), (void *)&d_interVals);
        errcode |= clSetKernelArg(clKernel_mapper, argIdx++, sizeof(cl_mem), (void *)&d_interOffsetSizes);
        errcode |= clSetKernelArg(clKernel_mapper, argIdx++, sizeof(int), (void *)&recordNum);
        errcode |= clSetKernelArg(clKernel_mapper, argIdx++, sizeof(int), (void *)&recordsPerTask);
        errcode |= clSetKernelArg(clKernel_mapper, argIdx++, sizeof(int), (void *)&taskNum);
        CHKERR(errcode, "Failed to set mapper kernel arguments!");

        size_t localWorkSize_map = warpSize;
        size_t globalWorkSize_map = (size_t)taskNum * warpSize;

        printf("Launching mapper: %d task(s) x %zu threads for %d line record(s)\n",
               taskNum, localWorkSize_map, recordNum);

        errcode = clEnqueueNDRangeKernel(commands, clKernel_mapper, 1, NULL,
                                         &globalWorkSize_map, &localWorkSize_map,
                                         0, NULL, &ocdTempEvent);
        clFinish(commands);
        START_TIMER(ocdTempEvent, OCD_TIMER_KERNEL, "Mapper Kernel", ocdTempTimer)
        END_TIMER(ocdTempTimer)
        CHKERR(errcode, "Failed to enqueue mapper kernel!");
    }

    {
        clEnqueueReadBuffer(commands, d_targetMatchCounts, CL_TRUE, 0, num_targets * sizeof(cl_uint), h_targetMatchCounts, 0, NULL, NULL);
        for (int t = 0; t < num_targets; t++)
        {
            if (h_targetMatchCounts[t] > h_targetBucketCapacities[t])
            {
                fprintf(stderr, "Warning: target %d exceeded its match-bucket capacity (%u); reported count is a lower bound\n",
                        t, h_targetBucketCapacities[t]);
            }
        }
    }

    /* --- Execute Reducer Kernel --- */
    {
        int reduceRecordNum = num_targets;
        int reduceTaskNum = DEFAULT_TASK_NUM;
        int reduceRecordsPerTask = (reduceRecordNum + reduceTaskNum - 1) / reduceTaskNum;

        int argIdx = 0;
        errcode = clSetKernelArg(clKernel_reducer, argIdx++, sizeof(cl_mem), (void *)&d_inputKeys);
        errcode |= clSetKernelArg(clKernel_reducer, argIdx++, sizeof(cl_mem), (void *)&d_targetOffsetSizes);
        errcode |= clSetKernelArg(clKernel_reducer, argIdx++, sizeof(cl_mem), (void *)&d_targetBucketOffsets);
        errcode |= clSetKernelArg(clKernel_reducer, argIdx++, sizeof(cl_mem), (void *)&d_targetBucketCapacities);
        errcode |= clSetKernelArg(clKernel_reducer, argIdx++, sizeof(cl_mem), (void *)&d_targetMatchCounts);
        errcode |= clSetKernelArg(clKernel_reducer, argIdx++, sizeof(cl_mem), (void *)&d_interOffsetSizes);
        errcode |= clSetKernelArg(clKernel_reducer, argIdx++, sizeof(cl_mem), (void *)&d_interVals);
        errcode |= clSetKernelArg(clKernel_reducer, argIdx++, sizeof(cl_mem), (void *)&d_psKeySizes);
        errcode |= clSetKernelArg(clKernel_reducer, argIdx++, sizeof(cl_mem), (void *)&d_psValSizes);
        errcode |= clSetKernelArg(clKernel_reducer, argIdx++, sizeof(cl_mem), (void *)&d_psCounts);
        errcode |= clSetKernelArg(clKernel_reducer, argIdx++, sizeof(cl_mem), (void *)&d_outputKeys);
        errcode |= clSetKernelArg(clKernel_reducer, argIdx++, sizeof(cl_mem), (void *)&d_outputVals);
        errcode |= clSetKernelArg(clKernel_reducer, argIdx++, sizeof(cl_mem), (void *)&d_outputOffsetSizes);
        errcode |= clSetKernelArg(clKernel_reducer, argIdx++, sizeof(int), (void *)&reduceRecordNum);
        errcode |= clSetKernelArg(clKernel_reducer, argIdx++, sizeof(int), (void *)&reduceRecordsPerTask);
        errcode |= clSetKernelArg(clKernel_reducer, argIdx++, sizeof(int), (void *)&reduceTaskNum);
        CHKERR(errcode, "Failed to set reducer kernel arguments!");

        size_t localWorkSize_reduce = warpSize;
        size_t globalWorkSize_reduce = (size_t)reduceTaskNum * warpSize;

        errcode = clEnqueueNDRangeKernel(commands, clKernel_reducer, 1, NULL,
                                         &globalWorkSize_reduce, &localWorkSize_reduce,
                                         0, NULL, &ocdTempEvent);
        clFinish(commands);
        START_TIMER(ocdTempEvent, OCD_TIMER_KERNEL, "Reducer Kernel", ocdTempTimer)
        END_TIMER(ocdTempTimer)
        CHKERR(errcode, "Failed to enqueue reducer kernel!");
    }

    /* --- Read results back --- */
    errcode = clEnqueueReadBuffer(commands, d_outputKeys, CL_TRUE, 0, outputKeysBufferSize, h_outputKeys, 0, NULL, &ocdTempEvent);
    errcode |= clEnqueueReadBuffer(commands, d_outputVals, CL_TRUE, 0, num_targets * sizeof(int), h_outputVals, 0, NULL, &ocdTempEvent);
    errcode |= clEnqueueReadBuffer(commands, d_outputOffsetSizes, CL_TRUE, 0, num_targets * sizeof(cl_int4), h_outputOffsetSizes, 0, NULL, &ocdTempEvent);
    clFinish(commands);
    START_TIMER(ocdTempEvent, OCD_TIMER_D2H, "Output Copy", ocdTempTimer)
    END_TIMER(ocdTempTimer)
    stopwatch_stop(&sw);
    CHKERR(errcode, "Failed to read output buffers!");

    /* --- Print results --- */
    int maxKeyLen = 3; /* at least as wide as the "Key" header */
    for (int i = 0; i < num_targets; i++)
    {
        if (h_outputOffsetSizes[i].w > 0 && h_outputOffsetSizes[i].x < (int)outputKeysBufferSize)
        {
            if (h_outputOffsetSizes[i].y > maxKeyLen)
                maxKeyLen = h_outputOffsetSizes[i].y;
        }
    }
    printf("MapReduce completed.\n");
    write_stringmatch(stdout, maxKeyLen, num_targets, h_outputOffsetSizes,
                      h_outputKeys, h_outputVals, outputKeysBufferSize);

    if (print_result)
    {
        snprintf(outputFile, (sizeof(char) * STRINGMATCH_NAME_MAX_LENGTH),
                 "stringmatch_result_N%u_S%lu", num_targets, fileSize);
        FILE *outFp = fopen(outputFile, "w");
        if (outFp)
        {
            write_stringmatch(outFp, maxKeyLen, num_targets, h_outputOffsetSizes,
                              h_outputKeys, h_outputVals, outputKeysBufferSize);
            fclose(outFp);
            printf("Results saved to %s\n", outputFile);
        }
        else
        {
            fprintf(stderr, "Warning: could not open '%s' for writing results\n", outputFile);
        }
    }

    /* --- Cleanup --- */
    clReleaseMemObject(d_inputDataset);
    clReleaseMemObject(d_inputKeys);
    clReleaseMemObject(d_inputVals);
    clReleaseMemObject(d_inputOffsetSizes);
    clReleaseMemObject(d_targetOffsetSizes);
    clReleaseMemObject(d_targetBucketOffsets);
    clReleaseMemObject(d_targetBucketCapacities);
    clReleaseMemObject(d_targetMatchCounts);
    clReleaseMemObject(d_interVals);
    clReleaseMemObject(d_interOffsetSizes);
    clReleaseMemObject(d_psKeySizes);
    clReleaseMemObject(d_psValSizes);
    clReleaseMemObject(d_psCounts);
    clReleaseMemObject(d_outputKeys);
    clReleaseMemObject(d_outputVals);
    clReleaseMemObject(d_outputOffsetSizes);

    clReleaseKernel(clKernel_mapper);
    clReleaseKernel(clKernel_reducer);
    clReleaseProgram(clProgram);
    clReleaseCommandQueue(commands);
    clReleaseContext(context);

    free(h_inputDataset);
    free(h_inputKeys);
    free(h_inputVals);
    free(h_inputOffsetSizes);
    free(h_targetOffsetSizes);
    free(h_targetBucketOffsets);
    free(h_targetBucketCapacities);
    free(h_targetMatchCounts);
    free(h_outputKeys);
    free(h_outputVals);
    free(h_outputOffsetSizes);

    ocd_finalize();
    return EXIT_SUCCESS;
} /* ----------  end of function main  ---------- */