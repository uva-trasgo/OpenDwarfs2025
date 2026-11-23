#define UNROLL_FACTOR 4  // 2 or 4 gives the best balance on FPGA DDR channels

typedef struct
{
	int starting;
	int no_of_edges;
}Node;

__kernel void kernel1(__global const Node* restrict g_graph_nodes,
		__global const int* restrict g_graph_edges,
		__global int* restrict g_graph_mask,
		__global int* restrict g_updating_graph_mask,
		__global const int* restrict g_graph_visited,
		__global int* restrict g_cost,
		int no_of_nodes) 
{
	unsigned int tid = get_global_id(0);

	if(tid < no_of_nodes && g_graph_mask[tid] != 0)
	{
		g_graph_mask[tid] = 0;
		const unsigned int start = g_graph_nodes[tid].starting;
		const unsigned int end = start + g_graph_nodes[tid].no_of_edges;
		const int cost = g_cost[tid] + 1; // Kept: loop-invariant precomputation

		#pragma unroll UNROLL_FACTOR
		for(int i = start; i < end; i++)
		{
			int id = g_graph_edges[i];
			if(!g_graph_visited[id])
			{
				g_cost[id] = cost;
				g_updating_graph_mask[id] = 1;
			}
		}
	}
}

__kernel void kernel2(__global int* restrict g_graph_mask,
		__global int* restrict g_updating_graph_mask,
		__global int* restrict g_graph_visited,
		__global int* restrict g_over,
		int no_of_nodes)
{
	unsigned int tid = get_global_id(0);
	if(tid < no_of_nodes && g_updating_graph_mask[tid] == 1)
	{
		g_graph_mask[tid] = 1;
		g_graph_visited[tid] = 1;
		*g_over = 1;
		g_updating_graph_mask[tid] = 0;
	}
}
