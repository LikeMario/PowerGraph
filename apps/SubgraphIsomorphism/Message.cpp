#include "Message.h"


Message::Message()
{
	source_pos = -1;
}

Message::Message(int size_of_pattern)
{
	matched_id = IDVector(size_of_pattern, -1);
	source_pos = -1;
}

void Message::save(graphlab::oarchive& oarc) const 
{
	oarc << matched_id << source_pos << forwarding_trace;
}

void Message::load(graphlab::iarchive& iarc)
{
	iarc >> matched_id >> source_pos >> forwarding_trace;
}

int Message::size() const
{
	return sizeof(*this) + sizeof(graphlab::vertex_id_type) * (matched_id.capacity() + forwarding_trace.capacity());
};

MessagesInVertex::MessagesInVertex()
{
	source = -1;
	tr = trUnknown;
}

void MessagesInVertex::save(graphlab::oarchive& oarc) const 
{
	oarc << messages << source << tr;
}

void MessagesInVertex::load(graphlab::iarchive& iarc)
{
	iarc >> messages >> source >> tr;
}

int MessagesInVertex::size() const
{
	//int total = sizeof(*this);
	int total = 0;
	for(const Message &m : messages)
	{
		total += m.size();
	}
	total += sizeof(Message)*(messages.capacity()-messages.size());
	return total;
}