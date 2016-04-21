#include <string>
#include <vector>

#include <graphlab.hpp>

#include "Message.h"
#include "Pattern.h"

using namespace std;

enum StepIndicator {GetReady = 0, FirstStep, SecondStep};

class DataGraphVertex
{
public:	
	DataGraphVertex();
	explicit DataGraphVertex(string vertexlabel);
	explicit DataGraphVertex(string vertexlabel, Pattern p):label(vertexlabel), local_pattern(p), si(GetReady), replicas_cost(0), message_amount(0), subgraph_repetition(0) {};
	
	void save(graphlab::oarchive& oarc) const;
	void load(graphlab::iarchive& iarc);
	
	bool IsDuplicated(IDVector& nm);

	bool IsTarget(graphlab::vertex_id_type id) const;
	bool IsSource(graphlab::vertex_id_type id) const;
	bool IsNeighbor(graphlab::vertex_id_type id) const;
	bool IsBi(graphlab::vertex_id_type id) const;
	
	int size() const;
public:
	string label;
	Pattern local_pattern;
	StepIndicator si;
	MessagesInVertex messages_in_vertex;
	vector<IDVector> matched_subgraph;
	
	IDVector	targets;
	IDVector 	sources;
	IDVector 	neighbors;
	IDVector	bidirected_neighbors;
	
	long long replicas_cost;
	long message_amount;
	long subgraph_repetition;
};
