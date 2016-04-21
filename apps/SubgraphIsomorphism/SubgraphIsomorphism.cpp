#include <fstream>
#include <iostream>
#include <string>
#include <ctime>
#include <graphlab.hpp>

#include "DataGraphVertex.h"
//#include "Pattern.h"

#define EDGE_FILE
#define LABELED_EDGE_FILE
#define INITIAL_OPTIMIZATION
#define DEGREE_OPTIMIZATION
//#define UNDIRECTED_GRAPH
#define PDSI

using namespace std;

double spread_factor;
Pattern pattern;

typedef graphlab::distributed_graph<DataGraphVertex, graphlab::empty> graph_type;

bool SemanticCompare(const DataGraphVertex& gv, const PatternVertex& pv)
{
	return (gv.label == pv.label);
}

//+= overload
class GatheredMessages
{
public:
	GatheredMessages& operator+=(GatheredMessages other)
	{
		bool exist = false;
		for(MessagesInVertex &v: gm)
		{
			if(v.source == other.gm[0].source)
			{
				v.tr = trBidirection;
				exist = true;
				break;
			}
		}
		
		if(!exist)
			gm.insert( gm.end(), other.gm.begin(), other.gm.end() );
		
		return *this;
	}
	
	void save(graphlab::oarchive& oarc) const
	{
		oarc << gm;
	}
	
	void load(graphlab::iarchive& iarc)
	{
		iarc >> gm;
	}
	
public:
	std::vector<MessagesInVertex> gm;
};

class PatternMatch: public graphlab::ivertex_program<graph_type, GatheredMessages>, 
						public graphlab::IS_POD_TYPE
{
private:
	// a variable local to this program
	bool perform_scatter;
public:
	// 
	edge_dir_type gather_edges(icontext_type& context, const vertex_type& vertex) const 
	{
		return graphlab::ALL_EDGES;
	}
	// 
	gather_type gather(icontext_type& context, const vertex_type& vertex, edge_type& edge) const 
	{
		gather_type gathered_messages;
		MessagesInVertex m_in_v;
		if(edge.source().id()==vertex.id())
		{
			m_in_v = edge.target().data().messages_in_vertex;
			m_in_v.tr = trTarget;
		}
		else
		{
			m_in_v = edge.source().data().messages_in_vertex;
			m_in_v.tr = trSource;
		}
		
#ifdef PDSI
		//random control
		srand(time(NULL));
		int part = ceil(spread_factor*m_in_v.messages.size());
		random_shuffle(m_in_v.messages.begin(), m_in_v.messages.end());
		m_in_v.messages.erase (m_in_v.messages.begin()+part,m_in_v.messages.end());
#endif
		
		gathered_messages.gm.push_back(m_in_v);
		return gathered_messages;
	}
	// 
	void apply(icontext_type& context, vertex_type& vertex, const gather_type& total)
	{
//cout << "Vertex " << vertex.id() << " is running." << endl;
		switch(vertex.data().si)
		{	
			case GetReady:
			{
				vertex.data().messages_in_vertex.source = vertex.id();
				
				perform_scatter = true;
				vertex.data().si = FirstStep;
				vertex.data().replicas_cost +=  vertex.data().size();

#if defined(LABELED_EDGE_FILE)&& defined(EDGE_FILE) 
				if(vertex.id()%7==0)
					vertex.data().label = "seven";
				else if(vertex.id()%5==0)
					vertex.data().label = "five";
				else if(vertex.id()%3==0)
					vertex.data().label = "three";
				else if(vertex.id()%2==0)
					vertex.data().label = "two";
				else
					vertex.data().label = "rest";
#endif
				break;
			}
			case FirstStep:
			{	
				for(const MessagesInVertex &m_in_v : total.gm)
				{
					vertex.data().replicas_cost += sizeof(MessagesInVertex);
					switch(m_in_v.tr)
					{
						case trTarget:
						{
							vertex.data().targets.push_back(m_in_v.source);
							vertex.data().neighbors.push_back(m_in_v.source);
							break;
						}						
						case trSource:
						{
							vertex.data().sources.push_back(m_in_v.source);
							vertex.data().neighbors.push_back(m_in_v.source);
							break;
						}						
						case trBidirection: 
						{
							vertex.data().bidirected_neighbors.push_back(m_in_v.source);
							vertex.data().targets.push_back(m_in_v.source);
							vertex.data().sources.push_back(m_in_v.source);
							vertex.data().neighbors.push_back(m_in_v.source);
							break;
						}
						case trUnknown: 
						case trUnconnected: 
						default:
							break;
					}
				}

#if defined(LABELED_EDGE_FILE)&& defined(EDGE_FILE) 
				/*if(vertex.data().sources.size() > 10)
				{
					vertex.data().label = "Celebrity";
				}
				else if(vertex.data().sources.size() > 5)
				{
					vertex.data().label = "NormalUser";
				}
				else
				{
					vertex.data().label = "InactiveUser";
				}*/
				//cout << vertex.id() << " is " << vertex.data().label << endl;
#endif
				
				
				if(vertex.data().local_pattern.m_pattern.size()==1)
				{
					const PatternVertex &pv = vertex.data().local_pattern.m_pattern[0];
					if (vertex.data().label == pv.label)
					{
						Message m(vertex.data().local_pattern.m_pattern.size());
						m.matched_id[pv.id] = vertex.id();
						m.source_pos = pv.id;
						
						vertex.data().matched_subgraph.push_back(m.matched_id);
					}		
				}
				else
				{
				#ifdef INITIAL_OPTIMIZATION
					const PatternVertex &pv = vertex.data().local_pattern.m_pattern[0];
				#else 
					for (const PatternVertex &pv : vertex.data().local_pattern.m_pattern)
				#endif
					{
						if (vertex.data().label == pv.label 
					#ifdef DEGREE_OPTIMIZATION
						&& vertex.num_in_edges()>= pv.in_degree && vertex.num_out_edges()>= pv.out_degree)
					#else 
						)
					#endif						
						{
							Message m(vertex.data().local_pattern.m_pattern.size());
							m.matched_id[pv.id] = vertex.id();
							m.source_pos = pv.id;
							m.forwarding_trace.push_back(vertex.id());
							vertex.data().messages_in_vertex.messages.push_back(m);
						}
					}
				}
				vertex.data().replicas_cost +=  vertex.data().size();
				perform_scatter = (vertex.data().messages_in_vertex.messages.size()!= 0);
				vertex.data().si = SecondStep;
				break;
			}
			case SecondStep:
			{
				vertex.data().messages_in_vertex.messages.clear();
				for(const MessagesInVertex &m_in_v : total.gm)
				{
					vertex.data().replicas_cost += sizeof(MessagesInVertex);
					vertex.data().message_amount += m_in_v.messages.size();
//cout << "Message from vertex " << m_in_v.source << endl;	
					for (const Message &m : m_in_v.messages)
					{
						vertex.data().replicas_cost += m.size();
/*cout << "Message: ";
for(int i : m.matched_id)
{
	cout<< i << " ";
}
cout << endl;*/						
						//If the current gv is already in the message
						if (find(m.matched_id.begin(), m.matched_id.end(), vertex.id()) != m.matched_id.end())
						{
							if(find(m.forwarding_trace.begin(), m.forwarding_trace.end(), vertex.id()) == m.forwarding_trace.end())
							{
								//Forward the message to gv's neighours except the source of the message.
								Message new_m = m;
								int found_pos = find(m.matched_id.begin(), m.matched_id.end(), vertex.id()) - m.matched_id.begin();
								new_m.source_pos = found_pos;
								new_m.forwarding_trace.push_back(vertex.id());
								vertex.data().messages_in_vertex.messages.push_back(new_m);
							}
							
						}
						else
						{
							//If there is any possible vertex in the pattern, which current gv can match.
							//If found, then update the message and send to all neighbors.
							for (int pv_id : vertex.data().local_pattern.m_pattern[m.source_pos].neighbors)
							{
								if ((vertex.data().local_pattern.m_pattern[pv_id].label == vertex.data().label) && (m.matched_id[pv_id] == -1) //same label and still available//-1
							#ifdef DEGREE_OPTIMIZATION
								&& vertex.num_in_edges()>= vertex.data().local_pattern.m_pattern[pv_id].in_degree && vertex.num_out_edges()>= vertex.data().local_pattern.m_pattern[pv_id].out_degree)//comparison of in/out degree
							#else 
								)
							#endif
									
								{
									bool satisify_pre_matches = true;
									for (int matched_pos = 0; matched_pos < m.matched_id.size(); matched_pos++)
									{
										if (m.matched_id[matched_pos] != -1)
										{
											if (
												(vertex.data().local_pattern.m_pattern[pv_id].IsBi(matched_pos) && vertex.data().IsBi(m.matched_id[matched_pos])) ||
												(vertex.data().local_pattern.m_pattern[pv_id].IsTarget(matched_pos) && !vertex.data().local_pattern.m_pattern[matched_pos].IsBi(pv_id) && vertex.data().IsTarget(m.matched_id[matched_pos])) ||
												(vertex.data().local_pattern.m_pattern[pv_id].IsSource(matched_pos) && !vertex.data().local_pattern.m_pattern[matched_pos].IsBi(pv_id) && vertex.data().IsSource(m.matched_id[matched_pos])) ||
												!vertex.data().local_pattern.m_pattern[pv_id].IsNeighbor(matched_pos) 
												)//compare topological relation
											{
												//Do nothing
											}
											else
											{
												satisify_pre_matches = false;
												break;
											}
										}
									}
									
									
									if (satisify_pre_matches)
									{
										Message new_m = m;
										new_m.matched_id[pv_id] = vertex.id();
										new_m.source_pos = pv_id;
										new_m.forwarding_trace.clear();
										new_m.forwarding_trace.push_back(vertex.id());

										if (find(new_m.matched_id.begin(), new_m.matched_id.end(), -1) != new_m.matched_id.end())
										{
											vertex.data().messages_in_vertex.messages.push_back(new_m);
										}
										else//A subgraph is found!
										{
											vertex.data().subgraph_repetition++;
											if (!vertex.data().IsDuplicated(new_m.matched_id))
											{
												vertex.data().matched_subgraph.push_back(new_m.matched_id);
												/*cout << "A subgraph is found in " << vertex.id() << ": ";
												for(int i : new_m.matched_id)
												{
													cout<< i << " ";
												}
												cout << endl;*/
											}
																		
										}
									}

								}
							}
						}
					}
				}
/*for(Message &m: vertex.data().messages_in_vertex.messages)
{
	cout << "New Message: ";
	for(int i : m.matched_id)
	{
		cout<< i << " ";
	}
cout << endl;
}*/

				perform_scatter = (vertex.data().messages_in_vertex.messages.size()!= 0);
				
				vertex.data().replicas_cost +=  vertex.data().size();
				break;
			}
		}
		
		if(perform_scatter)
			context.signal(vertex);		
	}
	// 
	edge_dir_type scatter_edges(icontext_type& context, const vertex_type& vertex) const
	{
		if (perform_scatter) return graphlab::ALL_EDGES;
		else return graphlab::NO_EDGES;
	}
	//
	void scatter(icontext_type& context, const vertex_type& vertex, edge_type& edge) const
	{
		if(edge.source().id()==vertex.id())
		{
			context.signal(edge.target());
		}
		else
		{
			context.signal(edge.source());
		}
		
	}
};

class graph_writer
{
public:
	std::string save_vertex(graph_type::vertex_type v)
	{
		std::stringstream strm;		
		strm << v.id() << "\t" << v.data().replicas_cost << "\t" << v.data().message_amount << "\t" << v.data().subgraph_repetition << "\t" << v.data().matched_subgraph.size() << "\t";
		for(IDVector idv : v.data().matched_subgraph)
		{
			//strm <<"subgraph: \t(";
			for(graphlab::vertex_id_type i: idv)
			{
				strm << i << ",";
			}
			strm <<";";
			//strm <<")\t";
		}
		strm << "\n";
		return strm.str();
	}
	std::string save_edge(graph_type::edge_type e) 
	{
		return "";
	}
};

bool graph_line_parser(graph_type& graph, 
				const std::string& filename,
				const std::string& textline)
#ifdef EDGE_FILE
{
	if (textline.empty()) return true;
	unsigned int source;
	unsigned int target;
	if (sscanf(textline.c_str(), "%u %u", &source, &target) < 2) {
		// parsed less than 2 objects, failure.
		return false;
	}
	else {
		graph.add_edge(source, target);
		#ifdef UNDIRECTED_GRAPH
		graph.add_edge(target, source);//if undirected edge
		#endif
		return true;
	}
}
#else 
{
	std::stringstream strm(textline);
	graphlab::vertex_id_type vid;
	std::string label;
	// first entry in the line is a vertex ID
	strm >> vid;
	strm >> label;
	// insert this web page
	graph.add_vertex(vid, DataGraphVertex(label, pattern));
	//graph.add_vertex(vid, DataGraphVertex(label));
	// while there are elements in the line, continue to read until we fail
	while(1)
	{
		graphlab::vertex_id_type other_vid;
		strm >> other_vid;
		if(strm.fail()) return true;
		graph.add_edge(vid, other_vid);
	}
	return true;
}
#endif

int main(int argc, char** argv)
{
	double start_s=clock();
	
	graphlab::mpi_tools::init(argc, argv);
	graphlab::distributed_control dc;

	 // Parse command line options -----------------------------------------------
	graphlab::command_line_options clopts("Subgraph Isomorphism.");
	string pattern_file = "pattern.txt";
	string graph_file = "graph.txt";
	string proportion = "1.0"; 
	clopts.attach_option("pattern", pattern_file,
   	                    "The pattern file must be provided.");
	clopts.add_positional("pattern");
	clopts.attach_option("graph", graph_file,
   	                    "The graph file must be provided.");
	clopts.add_positional("graph");
#ifdef PDSI
	clopts.attach_option("proportion", proportion,
   	                    "The parameter proportion must be provided.");
	clopts.add_positional("proportion");
#endif
	if(!clopts.parse(argc, argv)) {
   		dc.cout() << "Error in parsing command line arguments." << std::endl;
   		return EXIT_FAILURE;
		}


	// Initialize pattern
	//stringstream sstm;
#ifdef EDGE_FILE
	#ifdef LABELED_EDGE_FILE
		//sstm << "labeled_pattern0";
		bool labeled = true;
	#else
		//sstm << "unlabeled_pattern0";
		bool labeled = false;
	#endif
#else 
	//sstm << "labeled_pattern0";
	bool labeled = true;
#endif
	
	//string pattern_file = sstm.str();
	cout << "Searching for " << pattern_file << endl;
	pattern.Reset();
	pattern.CreatePattern(pattern_file, labeled);
	pattern.IntializePattern();

#ifdef PDSI
	spread_factor = stod(proportion);
	if(spread_factor<0 || spread_factor>1)
	{
		cout<< "The spread_factor must between 0 and 1, default value 1.0 is used." <<endl;
		spread_factor = 1.0;
	}
	else
		cout<< "The spread_factor is " << spread_factor <<endl;
#endif	



	// Build the graph ----------------------------------------------------------
	graph_type graph(dc, clopts);
	graph.load(graph_file, graph_line_parser);
		
	graphlab::mpi_tools::finalize();
	
	double finalized_s=clock();
	cout << "Ingress time: " << (finalized_s-start_s)/double(CLOCKS_PER_SEC) << endl;
	
	graphlab::omni_engine<PatternMatch> engine(dc, graph, "synchronous", clopts);
	engine.signal_all();
	engine.start();

	const float runtime = engine.elapsed_seconds();
   	dc.cout() << "Finished Running engine in " << runtime << " seconds." << std::endl;
	
	graph.save("output",
				graph_writer(), 
				false, // set to true if each output file is to be gzipped
				true, // whether vertices are saved
				false); // whether edges are saved
		

	double stop_s=clock();
	cout << "Execution time: " << (stop_s-finalized_s)/double(CLOCKS_PER_SEC) << endl;
}
