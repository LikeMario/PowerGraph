#include <string>
#include <vector>

#include <graphlab.hpp>

using namespace std;

class PatternVertex
{
public:
	PatternVertex(): id(-1), label(""), in_degree(0), out_degree(0) {};
	PatternVertex(int id, string label) : id(id), label(label), in_degree(0), out_degree(0) {};
	~PatternVertex();
	
	void save(graphlab::oarchive& oarc) const;
	void load(graphlab::iarchive& iarc);
	
	bool AddTarget(int t);
	bool AddSource(int t);
	void ProcessNeighbors();
	
	bool IsTarget(int id) const;
	bool IsSource(int id) const;
	bool IsNeighbor(int id) const;
	bool IsBi(int id) const;
	
	int size() const;
public:
	int id;
	string label;
	unsigned int in_degree;
	unsigned int out_degree;
	
	vector<int>	targets;
	vector<int> sources;
	vector<int> neighbors;
	vector<int>	bidirected_neighbors;
};
