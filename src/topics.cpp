//============================================================================
// Name        : alj_topicanalysis.cpp
// Author      : Andrey Gubichev
// Version     :
// Copyright   : 
//============================================================================

#include <iostream>
#include <fstream>
#include <map>
#include <set>
#include <vector>
#include <math.h>
#include <stdlib.h>
#include <algorithm>
using namespace std;

const char* FILE_NAME="/home/andrey/data/alj_logs/logs_countries_corr.txt";

// 3 hours cut-off for the page-to-page transition
const int TIME_LIMIT =  30*60 * 1000;
const int TIME_MIN =  1000;

// min size of the user trajectory
const unsigned int MIN_PATH_SIZE = 4;

const unsigned int MAX_TOPICS = 20;


struct visit{
	long int id;
	unsigned int page;
	int time;
	set<int> topics;
};

typedef vector<visit> trajectory;

typedef pair<unsigned, unsigned> topic_pair;

typedef map<unsigned,double> distribution;

// construct sequences of visits from the visit data
static void constructTrajectories(map<long int, trajectory >& time_visits, vector<trajectory>& sequences){
	unsigned lessthanone=0;
	for (map<long int, trajectory>::iterator it = time_visits.begin(); it != time_visits.end(); it++){
		if (it->second.size() < 2)
			continue;

		trajectory &s = it->second;
		trajectory tr;

		unsigned i = 0;
		tr.push_back(s[i]);
		bool skip= false;

		while (i+1 < s.size()){
			// check for the time limit on transition
			if ((s[i+1].time - s[i].time < TIME_LIMIT) && (s[i+1].time-s[i].time> TIME_MIN)){
				// no duplicate nodes
				if (s[i+1].page != s[i].page)
				//i++;
					tr.push_back(s[i+1]);
				i++;
			}
			else if (s[i+1].time - s[i].time > TIME_MIN){
				//time interval is too large; start new trajectory
				if (tr.size() >= MIN_PATH_SIZE){
					sequences.push_back(tr);
				}
				tr.clear();
				tr.push_back(s[++i]);
			} else if (s[i+1].time - s[i].time <= TIME_MIN){
				tr.clear();
				skip=true;
				break;
			}
		}

		if (tr.size() >= MIN_PATH_SIZE && !skip)
			sequences.push_back(tr);
	}
	cerr<<"less than 100 ms: "<<lessthanone<<endl;
}

static double getNumberOfTopics(const trajectory& traj){
	set<unsigned> topics;
	unsigned max_topics=0;
	unsigned total_topics=0;
	for (auto v: traj){
		if (v.topics.size()>max_topics)
			max_topics=v.topics.size();
		total_topics+=v.topics.size();
		for (auto t: v.topics)
			topics.insert(t);
	}
	return 1.*traj.size()*topics.size()/total_topics;
}


// get average topic-to-topic transition number
static float getAverageTransitionNumber(vector<trajectory>& sequence){
	float avgTransit = 0;
	for (unsigned i = 0; i < sequence.size(); i++){
		vector<visit> &v = sequence[i];
		for (unsigned j = 0; j < v.size() - 1; j++){
			set<unsigned> intersection;
			set_intersection(v[j].topics.begin(), v[j].topics.end(), 
								  v[j+1].topics.begin(), v[j+1].topics.end(),  
								  inserter(intersection, intersection.begin()));

			if (intersection.size()==0)
				avgTransit += 1;
		}
	}
	return avgTransit/sequence.size();
}

static void genShuffledSequence(const vector<trajectory>& baseline, vector<trajectory>& shuffle){
	shuffle = baseline;

	for (unsigned i=0; i < shuffle.size(); i++){
		trajectory &tr = shuffle[i];
		for (int j = tr.size()-1; j > 0; j--){
			unsigned k = rand()%(j+1);
			visit tmp = tr[j];
			tr[j] = tr[k];
			tr[k] = tmp;
		}
	}
}


static double getJaccard(const set<int>& a, const set<int>& b){
	set<unsigned> u, i;
	set_union(a.begin(), a.end(), b.begin(), b.end(), inserter(u, u.begin()));
	set_intersection(a.begin(), a.end(), b.begin(), b.end(),  inserter(i, i.begin()));
	return 1.0*(i.size())/(u.size());
}

static double getJaccardMetric(const trajectory& traj){
	double res=0;
	for (unsigned i=0; i < traj.size()-1; i++){
		res += getJaccard(traj[i].topics, traj[i+1].topics);
	}

	return res/(traj.size()-1);
}

static double getAverageNumberOfTopics(const vector<trajectory>& sequences){
	double res=0;
	for (auto t: sequences)
		res+=getNumberOfTopics(t);
	res/=sequences.size();
	return res;
}

static double getAverageJaccard(const vector<trajectory>& sequences){
	double res=0;
	for (auto t:sequences)
		res+=getJaccardMetric(t);
	return res/sequences.size();
}

static void genRandomSequence(vector<trajectory>& baseline, vector<trajectory>& random){
	// collect all visits
	vector<visit> allVisits;
	for (unsigned i = 0; i < baseline.size(); i++){
		for (unsigned j = 0; j < baseline[i].size(); j++){
			allVisits.push_back(baseline[i][j]);
		}
	}

	for (int i=allVisits.size()-1; i > 0; i--){
		unsigned j = rand() % (i+1);
		visit tmp = allVisits[i];
		allVisits[i] = allVisits[j];
		allVisits[j] = tmp;
	}

	// create a random sequence of collected visits. We keep the sequence length the same
	unsigned ind = 0;
	for (unsigned i=0; i < baseline.size(); i++){
		unsigned size = baseline[i].size();

		trajectory tr;
		for (unsigned j=0; j < size; j++){
			tr.push_back(allVisits[ind++]);
		}
		random.push_back(tr);
	}

}


void printSequences(const vector<trajectory>& sequences, unsigned offset, unsigned amount){
	unsigned count=offset;
	for (auto traj: sequences){
		cout<<"["<<count<<"] ";
		for (auto v: traj){
			cout<<v.page<<" (";
			for (auto topic: v.topics)
				cout<<topic<<" ";
			cout<<")";
		}
		cout<<" --- "<<getNumberOfTopics(traj)<<", "<<getJaccardMetric(traj);
		cout<<endl;
		if (++count > offset+amount)
			break;
	}
}


// compute the topic distribution of documents that _follow_ any document with the specified topic
static void getFollowingTopicDistribution(int topic, vector<trajectory>& sequences, map<unsigned, double>& freq, unsigned max_topics){
	for (unsigned i =0; i <= max_topics; i++){
		freq[i] = 0;
	}

	unsigned total = 0;
	for (unsigned i = 0; i < sequences.size(); i++){
		trajectory& t = sequences[i];
		for (unsigned k = 0; k < t.size()-1; k++){
			if (t[k].topics.count(topic)){
				total+=t[k+1].topics.size();
				for (auto topic: t[k+1].topics)
				freq[topic] += 1;
			}
		}
	}

	if (total)
		for (map<unsigned, double>::iterator it = freq.begin(); it != freq.end(); it++){
			freq[it->first] /= total;
		}
}

// compute the earth mover's distance between two distributions
static double getEMD( map<unsigned, double>& distr1,  map<unsigned, double>& distr2){
	double res = 0;
	double prev = 0;

	for (map<unsigned, double>::iterator it = distr1.begin(); it != distr1.end(); it++){
		double emd = prev + distr1[it->first] - distr2[it->first];
		res += fabs(emd);
		prev = emd;
	}
	return res;
}


int main(int argc, char** argv) {

	if (argc < 2){
		cerr <<"usage: "<<argv[0] <<" <trajectory file> "<<endl;
		return 1;
	}


	ifstream  in(argv[1]);

	map<long int, trajectory > time_visits;

	vector<trajectory> sequences;

	long int id, timestamp;
	unsigned page, topic_count, topic;
	while (in>>id>>timestamp>>page>>topic_count){
		if (!time_visits.count(id)){
			vector<visit> t;
			time_visits[id]=t;
		}
		visit tt;
		tt.time=timestamp;
		for (unsigned i=0; i < topic_count; i++){
			in>>topic;
			tt.topics.insert(topic);
		}
		if (!topic_count){
			in>>topic;
			continue;
		}
		tt.page=page;
		tt.id=id;
		time_visits[id].push_back(tt);
	}

	constructTrajectories(time_visits, sequences);

//	map<nusigned, double> topic_distr;
//	getTopicDistribution(sequences, topic_distr);
	cout<<"number of sequences: "<<sequences.size()<<endl;
	cout<<"Baseline: avg number of topics: "<<getAverageNumberOfTopics(sequences)<<endl;
	cout<<"Baseline: avg Jaccard coefficient: "<<getAverageJaccard(sequences)<<endl;
	cout<<"Baseline: avg topic transition: "<<getAverageTransitionNumber(sequences)<<endl;
	printSequences(sequences,0,20);
	set<unsigned> allTopics;

	for (auto t: sequences){
		for (auto v: t){
			for (auto topic: v.topics)
				allTopics.insert(topic);
		}
	}

//	printSequences(sequences,0,20);
	cout<<"number of topics: "<<allTopics.size()<<endl;
	cout<<"max topic: "<<*allTopics.rbegin()<<endl;
	map<unsigned, distribution> baseline_freq;
	for (auto topic: allTopics){
		distribution freq;
		getFollowingTopicDistribution(topic, sequences, freq, *allTopics.rbegin());
		baseline_freq[topic]=freq;
	}

	vector<trajectory> generated;
//	printSequences(generated,0,20);



	unsigned exper_count=2;
	map<unsigned, double> EMD;
	// init EMD vector
	for (auto t: allTopics)
		EMD[t]=0;

	for (unsigned i=0; i < exper_count; i++){
		generated.clear();
		genRandomSequence(sequences, generated);
		cout<<"Random: avg number of topics: "<<getAverageNumberOfTopics(generated)<<endl;
		cout<<"Random: avg Jaccard coefficient: "<<getAverageJaccard(generated)<<endl;
		cout<<"Random: avg topic transition: "<<getAverageTransitionNumber(generated)<<endl;
		for (auto topic: allTopics){
			distribution freq;
			getFollowingTopicDistribution(topic, generated, freq, *allTopics.rbegin());
			double curEMD=getEMD(baseline_freq[topic], freq);
			cerr<<"topic, EMD: "<<topic<<" "<<curEMD<<endl;
			EMD[topic]+=getEMD(baseline_freq[topic], freq)/exper_count;
		}
	}

	for (auto t: allTopics){
		cerr<<t<<" "<<EMD[t]<<endl;
	}

	for (unsigned i=0; i < 2; i++){
		generated.clear();
		genShuffledSequence(sequences, generated);
		cout<<"Shuffled: avg number of topics: "<<getAverageNumberOfTopics(generated)<<endl;
		cout<<"Shuffled: avg Jaccard coefficient: "<<getAverageJaccard(generated)<<endl;
		cout<<"Shuffled: avg topic transition: "<<getAverageTransitionNumber(generated)<<endl;
		//printSequences(generated,0,20);
	}

	return 0;
}
