#include <vector>
#include <string>

#include <graphlab.hpp>

using namespace std;

typedef std::vector<graphlab::vertex_id_type> IDVector;

enum TopologicalRelation {trUnknown = 0, trUnconnected, trTarget, trSource, trBidirection};

class Message
{
public:
	Message();
	Message(int size_of_pattern);
	
	void save(graphlab::oarchive& oarc) const;
	void load(graphlab::iarchive& iarc);
	
	int size() const;
public:
	IDVector matched_id;
	int source_pos;
	IDVector forwarding_trace;
};

class MessagesInVertex
{
public:
	MessagesInVertex();
	
	void save(graphlab::oarchive& oarc) const;
	void load(graphlab::iarchive& iarc);
	
	int size() const;
public:	
	vector<Message> messages;
	graphlab::vertex_id_type source;	
	TopologicalRelation tr;
};
