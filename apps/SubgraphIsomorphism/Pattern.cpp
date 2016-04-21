#include "Pattern.h"

Pattern::Pattern(): patternvector_size(0)
{
	
}

Pattern::~Pattern()
{
	
}

void Pattern::save(graphlab::oarchive& oarc) const 
{
	oarc << m_pattern << patternvector_size;
}

void Pattern::load(graphlab::iarchive& iarc)
{
	iarc >> m_pattern >> patternvector_size;
}

bool Pattern::CreatePattern(string file, bool attributed)
{
	std::ifstream graphfile;
	std::string line;
	graphfile.open(file);

	int id;	
	for (int i = 0; std::getline(graphfile, line); i++)
	{
		id = i;
		string label = "";
		std::stringstream strm(line);
		
		if(attributed && !line.empty())
			strm >> label;
		
		PatternVertex v(id, label);
		string other;
		while (1)
		{
			strm >> other;
			if (strm.fail())
			{
				break;
			}
			v.AddTarget(stoi(other));
		}
		m_pattern.push_back(v);			
	}
	graphfile.close();
	return true;
}

bool Pattern::IntializePattern()
{	
	for (PatternVertex &v : m_pattern)
	{
		for (int t : v.targets)
		{
			m_pattern[t].AddSource(v.id);
		}
	}

	for (PatternVertex &v : m_pattern)
	{
		v.ProcessNeighbors();
	}

	//patternvector_size = patternvector_sizeof(*this);
	for (PatternVertex &v : m_pattern)
	{
		patternvector_size += v.size();
	}
	patternvector_size += sizeof(PatternVertex)*(m_pattern.capacity()-m_pattern.size());

	return true;
}

void Pattern::Reset()
{
	m_pattern.clear();
	patternvector_size = 0;
}