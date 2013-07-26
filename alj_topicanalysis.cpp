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

const char* FILE_NAME="/home/andrey/alj/chato/alj.txt";

// 3 hours cut-off for the page-to-page transition
const int TIME_LIMIT = 0.5*60 * 60;

// min size of the user trajectory
const unsigned int MIN_PATH_SIZE = 2;

const unsigned int MAX_TOPICS = 17;

struct visit{
	long int id;
	unsigned int page;
	int time;
	int topic;
};

typedef vector<visit> trajectory;

typedef pair<unsigned, unsigned> topic_pair;

// compute the average number of topics per sequence
static float getAverageTopicNumber(vector<trajectory>& sequence){
	float avgTopic = 0;
	for (unsigned i =0; i < sequence.size(); i++){
		vector<visit> &v = sequence[i];
		set<unsigned> topics;
		for (vector<visit>::iterator it = v.begin(); it != v.end(); it++)
			topics.insert(it->topic);
		avgTopic += topics.size();
	}
	return avgTopic/sequence.size();
}

// compute the average number of distinct documents per sequence
static float getAverageDocNumber(vector<trajectory>& sequence){
	float avgDoc = 0;
	for (unsigned i =0; i < sequence.size(); i++){
		vector<visit> &v = sequence[i];
		set<unsigned> docs;
		for (vector<visit>::iterator it = v.begin(); it != v.end(); it++)
			docs.insert(it->page);
		avgDoc += docs.size();
	}
	return avgDoc/sequence.size();
}

static bool shareTopic(const visit& v1, const visit& v2){
	if (v1.topic == v2.topic)
		return true;
//	if (v1.topic2 == v2.topic )
//		return true;
//	if (v2.topic2 == v1.topic)
//		return true;
//	if (v1.topic2 == v2.topic2 && v1.topic2 != 0)
//		return true;
	return false;
}

static void findFrequentTopics(vector<trajectory>& sequence, map<unsigned, double>& freq){
	for (unsigned i =0; i < MAX_TOPICS; i++){
		freq[i] = 0;
	}

	for (unsigned i = 0; i < sequence.size(); i++){
		vector<visit> &v = sequence[i];
		set<unsigned> topics;
		for (vector<visit>::iterator it = v.begin(); it != v.end(); it++)
			topics.insert(it->topic);

		for (set<unsigned>::iterator it = topics.begin(); it != topics.end(); it++){
			freq[*it]++;
		}
	}

	for (map<unsigned, double>::iterator it = freq.begin(); it != freq.end(); it++){
		freq[it->first] /= sequence.size();
	}
}

static void findFrequentTopicPairs(vector<trajectory>& sequence, map<topic_pair, double>& freq){
	for (unsigned i =0; i < MAX_TOPICS; i++){
		for (unsigned j = 0; j < MAX_TOPICS; j++)
			freq[pair<unsigned,unsigned>(i,j)] = 0;
	}

	for (unsigned i = 0; i < sequence.size(); i++){
		vector<visit> &v = sequence[i];
		set<unsigned> topics;
		for (vector<visit>::iterator it = v.begin(); it != v.end(); it++)
			topics.insert(it->topic);

		for (set<unsigned>::iterator it = topics.begin(); it != topics.end(); it++){
			for (set<unsigned>::iterator it1 = topics.begin(); it1 != topics.end(); it1++){
				if (*it1 < *it){
					freq[pair<unsigned,unsigned>(*it, *it1)] += 1;
				}
			}
		}
	}

	for (map<topic_pair, double>::iterator it = freq.begin(); it != freq.end(); it++){
		freq[it->first] /= sequence.size();
	}
}


static void findFrequenceAdjTopicPairs(vector<trajectory>& sequence, map<topic_pair, double>& freq){
	for (unsigned i =0; i < MAX_TOPICS; i++){
		for (unsigned j = 0; j < MAX_TOPICS; j++)
			freq[pair<unsigned,unsigned>(i,j)] = 0;
	}

	for (unsigned i = 0; i < sequence.size(); i++){
		vector<visit> &v = sequence[i];
		set<pair<unsigned, unsigned> > topics;
		for (unsigned k = 0; k < v.size()-1; k++)
			topics.insert(pair<unsigned,unsigned>(v[k].topic, v[k+1].topic));

		for (set<pair<unsigned,unsigned> >::iterator it = topics.begin(); it != topics.end(); it++){
				freq[*it]+=1;
		}
	}
	for (map<topic_pair, double>::iterator it = freq.begin(); it != freq.end(); it++){
		freq[it->first] /= sequence.size();
	}
}


// get average topic-to-topic transition number
static float getAverageTransitionNumber(vector<trajectory>& sequence){
	float avgTransit = 0;
	for (unsigned i = 0; i < sequence.size(); i++){
		vector<visit> &v = sequence[i];
		for (unsigned j = 0; j < v.size() - 1; j++){
			if (!shareTopic(v[j], v[j+1]))
				avgTransit += 1;
		}
	}
	return avgTransit/sequence.size();
}


// compute the average number of articles per sequence
static float getAverageNodeNumber(vector<trajectory>& sequence){
	float avgNode = 0;
	for (unsigned i =0; i < sequence.size(); i++){
		avgNode += sequence[i].size();
	}
	return avgNode/sequence.size();
}


// construct sequences of visits from the visit data
static void constructTrajectories(map<long int, trajectory >& time_visits, vector<trajectory>& sequences){
	for (map<long int, trajectory>::iterator it = time_visits.begin(); it != time_visits.end(); it++){
		if (it->second.size() < 2)
			continue;

		trajectory &s = it->second;
		trajectory tr;

		unsigned i = 0;
		tr.push_back(s[i]);

		while (i+1 < s.size()){
			// check for the time limit on transition
			if (s[i+1].time - s[i].time < TIME_LIMIT){
				// no duplicate nodes
				if (s[i+1].page != s[i].page)
				//i++;
					tr.push_back(s[i+1]);
				i++;
			}
			else{
				//time interval is too large; start new trajectory
				if (tr.size() >= MIN_PATH_SIZE){
					sequences.push_back(tr);
				}
				tr.clear();
				tr.push_back(s[++i]);
			}
		}

		if (tr.size() >= MIN_PATH_SIZE)
			sequences.push_back(tr);
	}
}

// compute the topic distribution in the dataset
static void getTopicDistribution(vector<trajectory>& sequences, map<unsigned, double>& freq){
	for (unsigned i =0; i <= MAX_TOPICS; i++){
		freq[i] = 0;
	}
	unsigned total = 0;
	for (unsigned i = 0; i < sequences.size(); i++){
		trajectory& t = sequences[i];
		for (vector<visit>::iterator it = t.begin(); it != t.end(); it++){
			freq[it->topic] += 1;
			total++;
		}
	}

	for (map<unsigned, double>::iterator it = freq.begin(); it != freq.end(); it++){
		freq[it->first] /= total;
	}
}

// compute the topic distribution of documents that _follow_ any document with the specified topic
static void getFollowingTopicDistribution(int topic, vector<trajectory>& sequences, map<unsigned, double>& freq){
	for (unsigned i =0; i <= MAX_TOPICS; i++){
		freq[i] = 0;
	}

	unsigned total = 0;
	for (unsigned i = 0; i < sequences.size(); i++){
		trajectory& t = sequences[i];
		for (unsigned k = 0; k < t.size()-1; k++){
			if (t[k].topic == topic){
				total++;
				freq[t[k+1].topic] += 1;
			}
		}
	}

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

	long int id, time;
	unsigned page, topic;
	while (in>>id>>time>>page>>topic){
		if (!time_visits.count(id)){
			vector<visit> t;
			time_visits[id]=t;
		}
		visit tt;
		tt.time=time;
		tt.topic=topic;
		tt.page=page;
		tt.id=id;
		time_visits[id].push_back(tt);
	}

	constructTrajectories(time_visits, sequences);

	map<unsigned, double> topic_distr;
	getTopicDistribution(sequences, topic_distr);


//	for (map<unsigned, double>::iterator it = topic_distr.begin(); it != topic_distr.end(); it++)
//		cerr<< it->first<<" "<<it->second<<endl;
//
//	map<unsigned, double> topic_following_distr;
//
//	for (unsigned i = 1; i < MAX_TOPICS; i++){
//		getFollowingTopicDistribution(i, sequences,topic_following_distr);
////		for (map<unsigned, double>::iterator it = topic_following_distr.begin(); it != topic_following_distr.end(); it++)
////			cerr<< it->first<<" "<<it->second<<endl;
//		cerr<<topic_distr[i]<<" "<<getEMD(topic_distr, topic_following_distr)<<endl;
//	}

	cerr<<"avg node number: "<<getAverageNodeNumber(sequences)<<
			", avg doc number: "<<getAverageDocNumber(sequences)<<
			", avg topic number: "<<getAverageTopicNumber(sequences)<<
			", avg transitions: "<<getAverageTransitionNumber(sequences)<<endl;

	map<unsigned, double> freq;
	findFrequentTopics(sequences, freq);

	for (map<unsigned, double>::iterator it = freq.begin(); it != freq.end(); it++){
		cerr<<it->first<<" "<<it->second<<endl;
	}

	map<topic_pair, double> freqpair;
	findFrequentTopicPairs(sequences, freqpair);

	for (map<topic_pair, double>::iterator it = freqpair.begin(); it != freqpair.end(); it++){
		if (it->second != 0)
			cerr<<it->first.first<<" "<<it->first.second<<": "<<it->second<<"\t"<<freqpair[topic_pair(it->first.second,it->first.first)]<<"\t"<<freq[it->first.first]* freq[it->first.second]<<endl;
	}
	return 0;
}
