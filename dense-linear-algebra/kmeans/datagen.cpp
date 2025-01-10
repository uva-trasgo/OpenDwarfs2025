/*
 * datagen.cpp
 * by Sam Kauffman - University of Virginia
 *
 * This program generates datasets for Rodinia's kmeans
 *
 * Usage:
 * datagen <numObjects> [ <numFeatures> ] [-f]
 *
 * numFeatures defaults to 34
 * Features are integers from 0 to 255 by default
 * With -f, features are floats from 0.0 to 1.0
 * Output file is "<numObjects>_<numFeatures>.txt"
 * One object per line. The first number in each line is an object ID and is
 * ignored by kmeans.
 *
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <cstring>
#include <ctime>

using namespace std;

void usage(){
	cerr << "Error: Invalid inputs. Usage: \n \
	./createkmeans [options]:\n\n \
		<-o num o objects>: number of objets\n \
		<-n num of features>: number of features of each object.\n \
		<-f>: make features to be of float value.\n\n \
	Example: ./createkmeans -o 256 -n 15 -f"<<endl;
		exit(-1 );
}


int main( int argc, char * argv[] )
{
	bool floats = false;
	stringstream ss;
	string PATH = "../test/dense-linear-algebra/kmeans/";
	int nObj = 0;
	int nFeat = 34;
	
	if ( argc < 2 )
		usage();
	else
	
		for(int i=1; i<argc; i+=2)
		
			if(strcmp(argv[i],"-o")==0)
				nObj=atoi(argv[i+1]);

			else if(strcmp(argv[i],"-n")==0)
				nFeat = atoi(argv[i+1]);

			else if(strcmp(argv[i],"-f")==0)
				floats = true;

			else
				usage();
	//One of the inputs has invalid value
	if(nObj<0||nFeat<0)
		usage();

	string f = floats ? "f" : "";
	
	ss << PATH << nObj << "_" << nFeat << f << ".txt";
	ofstream outf( ss.str().c_str(), ios::out | ios::trunc );
	srand( time( NULL ) );

	if ( !floats )
		for ( int i = 0; i < nObj; i++ )
		{
			outf << i << " ";
			for ( int j = 0; j < nFeat; j++ )
				outf << ( rand() % 256 ) << " ";
			outf << endl;
	}
	else
	{
		outf.precision( 4 );
		outf.setf( ios::fixed );
		for ( int i = 0; i < nObj; i++ )
		{
			outf << i << " ";
			for ( int j = 0; j < nFeat; j++ )
				outf << ( (float)rand() / (float)RAND_MAX ) << " ";
			outf << endl;
		}
	}

	outf.close();
	string type = floats ? " \"float\"" : " \"int\"";
	cout << "Generated " << nObj << " objects with " << nFeat << type << " features in " << ss.str() << ".\n";
}
