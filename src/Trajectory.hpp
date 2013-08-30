#ifndef H_Trajectory
#define H_Trajectory

#include <set>

// basic unit of all user trajectories
struct UserVisit {
	// user ID
	long int id;
	// page ID
	unsigned int page;
	//  timestamp
	long int time;
	// set of attributes of the page
	std::set<int> attributes;

};


// represent the sequence of pages visited by one user (i.e., the sequence of UserVisits)
class Trajectory{
private:
	// sequence of visits
	vector<UserVisit> trajectory;

public:
	// TODO
};

#endif