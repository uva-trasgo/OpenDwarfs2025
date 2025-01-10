#include "include/file_io.h"
#include "include/structures.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
int main(){
	char fname[] = "testFile.xyzr";
	residue *residues = calloc(sizeof(residues),5);
	
	srand(time(NULL));	
   for (int i = 0; i < 5; i++)
   {
   	  residues[i].natoms = 5;
   	  residues[i].atoms = calloc(sizeof(atom),5);
      for (int j = 0; j < residues[i].natoms; j++)
      {
         residues[i].atoms[j].x =  (float)rand() / (float)(RAND_MAX) * (rand() % 60) * (rand() % 2 ==0 ? 1: -1);
         residues[i].atoms[j].y =  (float)rand() / (float)(RAND_MAX) * (rand() % 60) * (rand() % 2 ==0 ? 1: -1);
         residues[i].atoms[j].z =  (float)rand() / (float)(RAND_MAX) * (rand() % 60) * (rand() % 2 ==0 ? 1: -1);
         residues[i].atoms[j].radius = (float)rand() / (float)(RAND_MAX) * (rand() % 60) * (rand() % 2 ==0 ? 1: -1);
      }
   }

	write_xyzr(fname, residues,5);

   for (int i = 0; i < 5; i++)
   {
   	  free(residues[i].atoms);
   }
   free(residues);
}