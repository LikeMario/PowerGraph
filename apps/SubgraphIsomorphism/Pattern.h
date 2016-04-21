#include "PatternVertex.h"

using namespace std;

class Pattern
{
public:
	Pattern();
	~Pattern();	
	
	void save(graphlab::oarchive& oarc) const;
	void load(graphlab::iarchive& iarc);
	
	bool CreatePattern(string file, bool attributed);
	bool IntializePattern();
	void Reset();
	
public:
	vector<PatternVertex> m_pattern;
	int patternvector_size;
};
