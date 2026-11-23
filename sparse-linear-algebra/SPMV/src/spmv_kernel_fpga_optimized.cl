#define UNROLL_FACTOR 4

void __kernel csr(const unsigned int num_rows,
		__global const unsigned int * restrict Ap, 
		__global const unsigned int * restrict Aj, 
		__global const float * restrict Ax, 
		__global float * restrict x, 
		__global float * restrict y)
{
	unsigned int row = get_global_id(0);

	if(row < num_rows)
	{     
		float sum = y[row];

		const unsigned int row_start = Ap[row];      	
		const unsigned int row_end = Ap[row+1];

		unsigned int jj = 0;
		#pragma unroll UNROLL_FACTOR
		for (jj = row_start; jj < row_end; jj++)
			sum += Ax[jj] * x[Aj[jj]];      

		y[row] = sum;
	}
}



