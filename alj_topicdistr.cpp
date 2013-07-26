//============================================================================
// compute the distances between two topic distributions
// Name        : alj_topicdistr.cpp
//============================================================================

#include <iostream>
#include <map>
#include <fstream>
#include <math.h>
using namespace std;

// file with first distr
const char* FILE_DISTR1="/tmp/distr_main.txt";
// file with second distr
const char* FILE_DISTR2="/tmp/distr_second.txt";

static void readDistr(const char* file_name, map<unsigned, double>& distr){
	ifstream in(file_name);
	unsigned topic;
	float freq;
	while (in >> topic >> freq){
		distr[topic] = freq;
	}
	cerr<<"done reading"<<endl;
}

// compute the earth mover's distance between two distributions
static double getEMD( map<unsigned, double>& distr1,  map<unsigned, double>& distr2){
	double res = 0;
	double prev = 0;

	for (auto it = distr1.begin(); it != distr1.end(); it++){
		double emd = prev + distr1[it->first] - distr2[it->first];
		res += fabs(emd);
		prev = emd;
	}
	return res;
}

int main() {
	map<unsigned, double> distr1;
	map<unsigned, double> distr2;
	readDistr(FILE_DISTR1, distr1);
	readDistr(FILE_DISTR2, distr2);

	cerr<<"EMD distance: "<<getEMD(distr1, distr2)<<endl;

	return 0;
}
