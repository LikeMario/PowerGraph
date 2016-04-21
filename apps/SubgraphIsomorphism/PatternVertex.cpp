#include "PatternVertex.h"

PatternVertex::~PatternVertex()
{
}

void PatternVertex::save(graphlab::oarchive& oarc) const 
{
	oarc << id << label << in_degree << out_degree << targets << sources << neighbors << bidirected_neighbors;
}

void PatternVertex::load(graphlab::iarchive& iarc)
{
	iarc >> id >> label >> in_degree >> out_degree >> targets >> sources >> neighbors >> bidirected_neighbors;
}

bool PatternVertex::AddTarget(int t)
{
	for (int i : targets)
	{
		if (t == i)
		{
			return false;
		}
	}
	targets.push_back(t);
	out_degree++;
	return true;
}

bool PatternVertex::AddSource(int s)
{
	for (int i : sources)
	{
		if (s == i)
		{
			return false;
		}
	}
	sources.push_back(s);
	in_degree++;
	return true;
}

void PatternVertex::ProcessNeighbors()
{
	neighbors = targets;
	for (int sid : sources)
	{
		if (find(targets.begin(), targets.end(), sid) != targets.end())
		{
			bidirected_neighbors.push_back(sid);
		}
		else
		{
			neighbors.push_back(sid);
		}
	}
}

bool PatternVertex::IsTarget(int id) const
{
	return (find(targets.begin(), targets.end(), id) != targets.end());	
}

bool PatternVertex::IsSource(int id) const
{
	return (find(sources.begin(), sources.end(), id) != sources.end());
}

bool PatternVertex::IsNeighbor(int id) const
{
	return (find(neighbors.begin(), neighbors.end(), id) != neighbors.end());
}

bool PatternVertex::IsBi(int id) const
{
	return (find(bidirected_neighbors.begin(), bidirected_neighbors.end(), id) != bidirected_neighbors.end());
}

int PatternVertex::size() const
{
	return sizeof(*this) + label.size() + sizeof(int)*(targets.capacity() + sources.capacity() + neighbors.capacity() + bidirected_neighbors.capacity());
}
