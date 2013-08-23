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

using namespace std;

const char* FILE_NAME="/home/andrey/data/alj_logs/logs_topics_countries.txt";

// 3 hours cut-off for the page-to-page transition
const int TIME_LIMIT =  15*60 * 1000;
const int TIME_MIN =  1000;

// min size of the user trajectory
const unsigned int MIN_PATH_SIZE = 2;

const unsigned int MAX_TOPICS = 20;

struct visit{
	long int id;
	unsigned int page;
	int time;
	set<int> topics;
};

typedef vector<visit> trajectory;

typedef pair<unsigned, unsigned> topic_pair;

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

static int getNumberOfTopics(const trajectory& traj){
	set<unsigned> topics;
	for (auto v: traj){
		for (auto t: v.topics)
			topics.insert(t);
	}
	return topics.size();
}

static double getAverageNumberOfTopics(const vector<trajectory>& sequences){
	double res=0;
	for (auto t: sequences)
		res+=getNumberOfTopics(t);
	res/=sequences.size();
	return res;
}

static void genRandomSequence(vector<trajectory>& baseline, vector<trajectory>& random){
	// collect all visits
	vector<visit> allVisits;
	for (unsigned i = 0; i < baseline.size(); i++){
		for (unsigned j = 0; j < baseline[i].size(); j++){
			allVisits.push_back(baseline[i][j]);
		}
	}

	cerr<<allVisits.size()<<endl;
	for (int i=allVisits.size()-1; i > 0; i--){
		unsigned j = rand() % (i+1);
		visit tmp = allVisits[i];
		allVisits[i] = allVisits[j];
		allVisits[j] = tmp;
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


int main() {
	ifstream  in(FILE_NAME);

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

//	map<unsigned, double> topic_distr;
//	getTopicDistribution(sequences, topic_distr);
	cout<<"number of sequences: "<<sequences.size()<<endl;

	unsigned i=0;
	for (auto traj: sequences){
		cout<<traj[0].id<<" ";
		for (auto v: traj){
			cout<<v.page<<" (";
			for (auto topic: v.topics)
				cout<<topic<<" ";
			cout<<")";
		}
		cout<<getNumberOfTopics(traj);
		cout<<endl;
		if (i++ > 30)
			break;
	}

	cout<<"avg number of topics: "<<getAverageNumberOfTopics(sequences)<<endl;

	vector<trajectory> generated;
	genRandomSequence(sequences, generated);
	cout<<"avg number of topics: "<<getAverageNumberOfTopics(generated)<<endl;
	 i=0;
	for (auto traj: generated){
		cout<<traj[0].id<<" ";
		for (auto v: traj){
			cout<<v.page<<" (";
			for (auto topic: v.topics)
				cout<<topic<<" ";
			cout<<")";
		}
		cout<<getNumberOfTopics(traj);
		cout<<endl;
		if (i++ > 30)
			break;
	}
	for (unsigned i=0; i < 10; i++){
		generated.clear();
		genRandomSequence(sequences, generated);
		cout<<"avg number of topics: "<<getAverageNumberOfTopics(generated)<<endl;
	}


	return 0;
}
