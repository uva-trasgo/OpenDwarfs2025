/******************************************************************************************************
* (c) Virginia Polytechnic Insitute and State University, 2011.
* Base MapReduce framework: StreamMR, by Marwa K. Elteir (City of Scientific
* Researches and Technology Applications, Egypt).
*******************************************************************************************************/

#pragma OPENCL EXTENSION cl_khr_byte_addressable_store : enable
#pragma OPENCL EXTENSION cl_khr_global_int32_base_atomics : enable
#pragma OPENCL EXTENSION cl_khr_local_int32_base_atomics : enable

typedef struct
{
        int lineOffset;
        int lineSize;
} VAL_T;

/****************************************************************************/
//Map phase kernels
/****************************************************************************/
void emitMatch(int targetIndex,
               int targetKeySize,
               __global uint* targetBucketOffsets,
               __global uint* targetBucketCapacities,
               __global uint* targetMatchCounts,
               __global char* interVals,
               __global int4* interOffsetSizes)
{
    uint idx = atom_inc(&targetMatchCounts[targetIndex]);

    if (idx >= targetBucketCapacities[targetIndex]) {
        return;
    }

    uint slot = targetBucketOffsets[targetIndex] + idx;
    __global int* valSlot = (__global int*)(interVals + slot * sizeof(int));
    *valSlot = 1;

    interOffsetSizes[slot].y = targetKeySize;
    interOffsetSizes[slot].z = (int)(slot * sizeof(int));
    interOffsetSizes[slot].w = sizeof(int);
}

void map(__global char* inputDataset,
         __global void* val,
         __global char* targetsText,
         __global int2* targetOffsetSizes,
         int numTargets,
         __global uint* targetBucketOffsets,
         __global uint* targetBucketCapacities,
         __global uint* targetMatchCounts,
         __global char* interVals,
         __global int4* interOffsetSizes)
{
    __global VAL_T* pVal = (__global VAL_T*)val;
    int lineOffset = pVal->lineOffset;
    int lineSize = pVal->lineSize;
    __global char* buf = inputDataset + lineOffset;
    __global char* lineEnd = buf + lineSize;

    __global char* wordStart = buf;
    __global char* c = buf;

    while (c <= lineEnd) {
        if (c == lineEnd || *c == ' ' || *c == '\n' || *c == '\0' || *c == '_') {
            int wordSize = (int)(c - wordStart);

            if (wordSize > 0) {
                for (int t = 0; t < numTargets; t++) {
                    int targetSize = targetOffsetSizes[t].y;
                    if (wordSize == targetSize) {
                        __global char* targetWord = targetsText + targetOffsetSizes[t].x;
                        __global char* s = wordStart;
                        __global char* k = targetWord;
                        int i;
                        for (i = 0; i < wordSize; i++, s++, k++) {
                            if (*s != *k) break;
                        }
                        if (i == wordSize) {
                            emitMatch(t, targetSize, targetBucketOffsets,
                                      targetBucketCapacities, targetMatchCounts,
                                      interVals, interOffsetSizes);
                        }
                    }
                }
            }
            wordStart = c + 1;
        }
        c++;
    }
}

__kernel void mapper(__global char* inputDataset,
                      __global int4* inputOffsetSizes,  /* one per line record; .z/.w -> inputVals */
                      __global char* inputVals,         /* {lineOffset,lineSize} pairs, one per line */
                      __global char* targetsText,
                      __global int2* targetOffsetSizes, /* one per target: {offset,size} into targetsText */
                      int numTargets,
                      __global uint* targetBucketOffsets,
                      __global uint* targetBucketCapacities,
                      __global uint* targetMatchCounts,
                      __global char* interVals,
                      __global int4* interOffsetSizes,
                      int recordNum,        /* number of line records (scales with file size) */
                      int recordsPerTask,
                      int taskNum)
{
    int bid = get_group_id(0);
    int tid = get_local_id(0);
    int blockDimx = get_local_size(0);

    if (bid * recordsPerTask >= recordNum) return;
    int recordBase = bid * recordsPerTask * blockDimx;
    int terminate = (bid + 1) * (recordsPerTask * blockDimx);
    if (terminate > recordNum) terminate = recordNum;

    for (int i = recordBase + tid; i < terminate; i += blockDimx) {
        int4 offsetSize = inputOffsetSizes[i];
        __global void* val = inputVals + offsetSize.z;

        map(inputDataset, val, targetsText, targetOffsetSizes, numTargets,
            targetBucketOffsets, targetBucketCapacities, targetMatchCounts,
            interVals, interOffsetSizes);
    }
}


/****************************************************************************/
//Reduce phase kernels
/****************************************************************************/
void reduce(__global void *key,
            __global void *val,
            int keySize,
            int valCount,
		    __global uint*	psKeySizes,
		    __global uint*	psValSizes,
		    __global uint*	psCounts,
		    __global int4*  interOffsetSizes,
		    __global char*  interVals,
		    __global char*	outputKeys,
		    __global char*	outputVals,
		    __global int4*	outputOffsetSizes,
		    int valStartIndex)
{

    int matchCount = 0;

    for (int i = 0; i < valCount; i++) {
        int offsetIndex = valStartIndex + i;
        __global int *valData = (__global int*)(interVals + interOffsetSizes[offsetIndex].z);
        matchCount += valData[0];
    }

    int outIndex = atom_inc(psCounts);

    __global char *outKey = outputKeys + atom_add(psKeySizes, keySize);
    __global char *inKey = (__global char*)key;
    for (int i = 0; i < keySize; i++) {
        outKey[i] = inKey[i];
    }

    __global int *outVal = (__global int*)(outputVals + atom_add(psValSizes, sizeof(int)));
    *outVal = matchCount;

    int keyOffset = outKey - outputKeys;
    int valOffset = (__global char*)outVal - outputVals;

    outputOffsetSizes[outIndex].x = keyOffset;
    outputOffsetSizes[outIndex].y = keySize;
    outputOffsetSizes[outIndex].z = valOffset;
    outputOffsetSizes[outIndex].w = sizeof(int);
}

__kernel void  reducer( __global char*       targetsText,
                        __global int2*      targetOffsetSizes,
                        __global uint*      targetBucketOffsets,
                        __global uint*      targetBucketCapacities,
                        __global uint*      targetMatchCounts,
                        __global int4*      interOffsetSizes,
                        __global char*      interVals,
                        __global uint*		psKeySizes,
                        __global uint*		psValSizes,
                        __global uint*		psCounts,
                        __global char*		outputKeys,
                        __global char*		outputVals,
                        __global int4*		outputOffsetSizes,
                        int		recordNum,        /* = numTargets */
                        int		recordsPerTask,
                        int		taskNum)
{
	int index = get_global_id(0);
	int bid = get_group_id(0);
	int tid = get_local_id(0);
	int blockDimx=get_local_size(0);
	
	if (bid*recordsPerTask >= recordNum) return;
	int recordBase = bid * recordsPerTask * blockDimx;
	int terminate = (bid + 1) * (recordsPerTask * blockDimx);
	if (terminate > recordNum) terminate = recordNum;

        // Initialize global variables carrying keysize, valsizes, keyvaloffests to zero
        if (index == 0)
        {
                *psKeySizes = 0;
                *psValSizes=0;
                *psCounts=0;
        }
        barrier(CLK_GLOBAL_MEM_FENCE);

	for (int i = recordBase + tid; i < terminate; i+=blockDimx)
	{
		int cindex = i;  /* target index */

		int keySize = targetOffsetSizes[cindex].y;
		__global char *key = targetsText + targetOffsetSizes[cindex].x;

		uint valStartIndex = targetBucketOffsets[cindex];
		uint valCount = targetMatchCounts[cindex];
		uint capacity = targetBucketCapacities[cindex];
		if (valCount > capacity) valCount = capacity;

		reduce(key,
			   interVals + valStartIndex * sizeof(int),
			   keySize,
			   (int)valCount,
			   psKeySizes,
			   psValSizes,
			   psCounts,
			   interOffsetSizes,
			   interVals,
			   outputKeys,
			   outputVals,
			   outputOffsetSizes,
			   (int)valStartIndex);
	}
}