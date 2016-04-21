#include "DataGraphVertex.h"

extern Pattern pattern;

DataGraphVertex::DataGraphVertex() :si(GetReady), replicas_cost(0), message_amount(0), subgraph_repetition(0)
{
	local_pattern = pattern;
	//cout << "pattern copied, no. of vertices: " << local_pattern.m_pattern.size() << endl;
}

DataGraphVertex::DataGraphVertex(string vertexlabel) :label(vertexlabel), si(GetReady), replicas_cost(0), message_amount(0), subgraph_repetition(0)
{
	local_pattern = pattern;
	//cout << "pattern copied, no. of vertices: " << local_pattern.m_pattern.size() << endl;
}

void DataGraphVertex::save(graphlab::oarchive& oarc) const 
{
	oarc << label << local_pattern << si << messages_in_vertex << matched_subgraph << targets << sources << neighbors << bidirected_neighbors << replicas_cost << message_amount << subgraph_repetition;
}

void DataGraphVertex::load(graphlab::iarchive& iarc)
{
	iarc >> label >> local_pattern >> si >> messages_in_vertex >> matched_subgraph >> targets >> sources >> neighbors >> bidirected_neighbors >> replicas_cost >> message_amount >> subgraph_repetition;
}

bool DataGraphVertex::IsDuplicated(IDVector& new_matched)
{
	int message_length = new_matched.size();
	for (const IDVector& matched : matched_subgraph)
	{
		bool duplicated = true;
		for (int i = 0; i < message_length; i++)
		{
			if (matched[i] != new_matched[i])
			{
				duplicated = false;
				break;
			}
		}
		if (duplicated)
		{
			return	true;
		}
	}
	return false;	
}

bool DataGraphVertex::IsTarget(graphlab::vertex_id_type id) const
{
	return (find(targets.begin(), targets.end(), id) != targets.end());	
}

bool DataGraphVertex::IsSource(graphlab::vertex_id_type id) const
{
	return (find(sources.begin(), sources.end(), id) != sources.end());
}

bool DataGraphVertex::IsNeighbor(graphlab::vertex_id_type id) const
{
	return (find(neighbors.begin(), neighbors.end(), id) != neighbors.end());
}

bool DataGraphVertex::IsBi(graphlab::vertex_id_type id) const
{
	return (find(bidirected_neighbors.begin(), bidirected_neighbors.end(), id) != bidirected_neighbors.end());
}

int DataGraphVertex::size() const
{
	int total = sizeof(*this) + label.size() + local_pattern.patternvector_size + messages_in_vertex.size();
	for(const IDVector& idv : matched_subgraph)
	{
		total += sizeof(graphlab::vertex_id_type)*idv.capacity();
	}
	total += sizeof(graphlab::vertex_id_type)*(targets.capacity() + sources.capacity() + neighbors.capacity() + bidirected_neighbors.capacity());
	return total;
}