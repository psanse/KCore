/**
 * @file test_kcore.cpp
 * @brief Unit tests for the KCore class, which computes the coreness of the vertices of undirected graphs.
 *
 * @details Covers construction (whole graph and induced subgraph), the O(|E|) decomposition, the
 *          quadratic reference implementation, the degeneracy ordering with a known upper bound
 *          (find_kcore_UB), the greedy clique heuristic and the stream output.
 *
 * @author pss
 * @version 2025-01-13
 *
 * @todo Check DISABLED_kcore_decomp_subgraph: not working in release mode (03/03/25).
 */


#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include "gtest/gtest.h"
#include "graph/algorithms/graph_gen.h"
#include "kcore.h"
#include "graph/graph_types.h"
#include "utils/logger.h"

using namespace bitgraph;

namespace {
	
	/**
	 * @brief Test fixture: star graph with 4 vertices and edges 0-1, 0-2, 0-3.
	 */
	class KcoreWTest : public ::testing::Test {
	protected:
		void SetUp() override {
			ug.reset(NV);
			ug.add_edge(0, 1);
			ug.add_edge(0, 2);
			ug.add_edge(0, 3);
			ug.set_name("star");
		}
		void TearDown() override {}

		//undirected graph instance	
		const int NV = 4;
		ugraph ug;												//undirected graph with integer weights
	};

}

TEST_F(KcoreWTest, constructor) {

	KCore<ugraph> kc(ug);

	EXPECT_EQ(4, kc.graph().num_vertices());
	EXPECT_TRUE(kc.subgraph().is_empty());


}

TEST_F(KcoreWTest, set_subgraph) {

	//bitset that induces a subgraph in G
	decltype(ug)::bitset_type bbset(4);
	bbset.set_bit(0);
	bbset.set_bit(2);
	
	//KCore with subgraph
	KCore<ugraph> kc(ug, bbset);
	auto psg = kc.subgraph();

	EXPECT_TRUE	(kc.subgraph().is_bit(0));
	EXPECT_TRUE	(kc.subgraph().is_bit(2));
	EXPECT_FALSE(kc.subgraph().is_bit(1));
	EXPECT_FALSE(kc.subgraph().is_bit(3));

}

TEST_F(KcoreWTest, set_subgraph_from_vector) {

	//bitset that induces a subgraph in G
	vertices vList = { 0, 2, 3 };

	//KCore with subgraph
	KCore<ugraph> kc(ug, vList);
	auto psg = kc.subgraph();

	EXPECT_TRUE(kc.subgraph().is_bit(0));
	EXPECT_TRUE(kc.subgraph().is_bit(2));
	EXPECT_FALSE(kc.subgraph().is_bit(1));
	
}

TEST_F(KcoreWTest, kcore_decomp_full_graph) {

	KCore<ugraph> kc(ug);

	/////////////////
	kc.find_kcore();
	/////////////////

	EXPECT_EQ(1, kc.max_core_number());						//1-core is the maximum core number in a star graph

	EXPECT_EQ(1, kc.coreness(0));
	EXPECT_EQ(1, kc.coreness(1));
	EXPECT_EQ(1, kc.coreness(2));
	EXPECT_EQ(1, kc.coreness(3));

	vint kcore_num = kc.coreness_numbers();					//coreness of all vertices
	EXPECT_EQ(1, kcore_num[0]);
	EXPECT_EQ(1, kcore_num[1]);
	EXPECT_EQ(1, kcore_num[2]);
	EXPECT_EQ(1, kcore_num[3]);

	//size of k-cores
	EXPECT_EQ(0, kc.core_size(0));							//number of vertices in the 0-core (isolani in disconnected graphs)
	EXPECT_EQ(4, kc.core_size(1));							//number of vertices in the 1-core (and not in the 2-core)
	EXPECT_EQ(0, kc.core_size(2));							//number of vertices in the 2-core (and not in the 3-core)

	//set of vertices in k-cores
	vertices kcore_set = kc.core_set(1);					//vertices in the 1-core
	EXPECT_EQ(4, kcore_set.size());

	kcore_set = kc.core_set(0);								//vertices in the 0-core: all vertices
	EXPECT_EQ(4, kcore_set.size());

	//k-core sorting (new-to-old format)
	vertex_ordering kcore_ord = kc.kcore_ordering();		//arrangement of vertices in non-decreasing kcore order

	vertex_ordering kcore_ord_exp = { 1, 2, 3, 0 };			//0 is the last because it was placed in the last bin	
	EXPECT_EQ(kcore_ord_exp, kcore_ord);
}


TEST_F(KcoreWTest, minimum_width) {

	KCore<ugraph> kc(ug);

	//////////////////
	kc.find_kcore();
	//////////////////
	
	auto min_width	= kc.minimum_width	(false);			//1 - minimum width of the graph 
	auto width		= kc.minimum_width	(true);				//3 - a width but not minimum
	
	EXPECT_EQ(min_width, kc.max_core_number());				//minimum width = k in the max k-core 
	EXPECT_NE(width, kc.max_core_number());
	
}


TEST_F(KcoreWTest, kcore_decomp_static) {

	KCore<ugraph> kc(ug);

	//////////////////
	kc.find_kcore();
	int max_kcore_quad = KCore<ugraph>::find_kcore(ug);
	//////////////////

	EXPECT_EQ(max_kcore_quad, kc.max_core_number());			//minimum width = k in the max k-core 

}

TEST(KCore, kcore_decomp_static_brock) {

	ugraph ug(BITGRAPH_GRAPH_TEST_DATA_DIR "brock200_1.clq");

	//core number in O(|V|*|V|)
	int max_core_quad = KCore<ugraph>::find_kcore(ug);								
	///////////////////////////////////////////////////

	//core number in O(|E|)
	KCore<ugraph> kc(ug);									
	kc.find_kcore();
	

	///////////////////////////////////////////////////
	EXPECT_EQ(max_core_quad, kc.max_core_number());			 
	///////////////////////////////////////////////////

}


TEST_F(KcoreWTest, find_heur_clique) {

	KCore<ugraph> kc(ug);
	kc.find_kcore();

	///////////////////////////////
	vint clique = kc.find_heur_clique();
	///////////////////////////////

	//the largest clique in a star graph is an edge
	ASSERT_EQ(2, clique.size());
	EXPECT_TRUE(ug.is_edge(clique[0], clique[1]));
}

TEST(KCore, find_heur_clique_brock) {

	ugraph ug(BITGRAPH_GRAPH_TEST_DATA_DIR "brock200_1.clq");

	KCore<ugraph> kc(ug);
	kc.find_kcore();

	//must terminate and return a valid clique
	vint clique = kc.find_heur_clique();

	ASSERT_GE(clique.size(), 2u);
	for (std::size_t i = 0; i < clique.size() - 1; ++i) {
		for (std::size_t j = i + 1; j < clique.size(); ++j) {
			EXPECT_TRUE(ug.is_edge(clique[i], clique[j]));
		}
	}
}

TEST_F(KcoreWTest, make_kcore_filter) {

	KCore<ugraph> kc(ug);
	kc.find_kcore();

	//all vertices have core number 1: no partition boundaries
	map_t filter;
	EXPECT_EQ(0, kc.make_kcore_filter(filter, true));
	EXPECT_EQ(0, kc.make_kcore_filter(filter, false));
}

TEST_F(KcoreWTest, stream_operator) {

	KCore<ugraph> kc(ug);
	kc.find_kcore();

	std::stringstream sstr;
	sstr << kc;

	EXPECT_EQ("[1:1] [2:1] [3:1] [0:1] ", sstr.str());
}
TEST(KCoreSparse, kcore_decomp_full_graph){
	
	//star graph with 11 vertices and a clique {1, 2, 7}
	sparse_ugraph sug(PATH_GRAPH_TESTS_CMAKE_SRC_CODE "star.clq");		
	
	KCore<sparse_ugraph> kc(sug);

	EXPECT_EQ	(11, kc.graph().num_vertices());
	EXPECT_TRUE	( kc.subgraph().is_empty());

	////////////////////
	kc.find_kcore();
	///////////////////

	//1-core = V \ {0, 1, 6}
	EXPECT_EQ(8, kc.core_size(1));

	//2-core = {0, 1, 6}
	vint core2	= kc.core_set(2);
	EXPECT_EQ(1, count(core2.begin(), core2.end(), 0));
	EXPECT_EQ(1, count(core2.begin(), core2.end(), 1));
	EXPECT_EQ(1, count(core2.begin(), core2.end(), 6));
	EXPECT_EQ(0, count(core2.begin(), core2.end(), 7));
	
}

TEST(KCoreSparse, DISABLED_kcore_decomp_subgraph) {

	//star graph with 11 vertices and a clique {0, 1, 6}
	sparse_ugraph sug(PATH_GRAPH_TESTS_CMAKE_SRC_CODE "star.clq");

	//bitset that induces a 3-clique in G
	decltype(sug)::bitset_type bbset(sug.num_vertices());
	bbset.set_bit(0);
	bbset.set_bit(1);
	bbset.set_bit(6);

	KCore<sparse_ugraph> kc(sug, bbset);

	EXPECT_EQ(11, kc.graph().num_vertices());
	EXPECT_TRUE(kc.subgraph().is_bit(0));
	EXPECT_TRUE(kc.subgraph().is_bit(1));
	EXPECT_TRUE(kc.subgraph().is_bit(6));
	EXPECT_EQ(3, kc.subgraph().count());

	//////////////////////
	kc.find_kcore(true);											//on G[{0, 1, 6}]			
	/////////////////////

	//1-core = V \ {0, 1, 6}
	EXPECT_EQ(0, kc.core_size(1));


	//2-core = {0, 1, 6}
	vint core2 = kc.core_set(2);
	EXPECT_EQ(3, kc.core_size(2));
	EXPECT_EQ(1, count(core2.begin(), core2.end(), 0));
	EXPECT_EQ(1, count(core2.begin(), core2.end(), 1));
	EXPECT_EQ(1, count(core2.begin(), core2.end(), 6));
	EXPECT_EQ(0, count(core2.begin(), core2.end(), 7));

}

/////////////////
//
// find_kcore_UB: degeneracy ordering with a known upper bound of the core number
// (checked on real and random graphs, with exact and looser bounds)
//
/////////////////

TEST(KCoreUB, kcore_example){

	ugraph ug(PATH_GRAPH_TESTS_CMAKE_SRC_CODE "brock200_1.clq");
		
	//test with real kcore number
	KCore<ugraph> kc(ug);	
	kc.find_kcore();
	int kcn=kc.max_core_number();
	int UB_corr=kc.find_kcore_UB(kcn);
	int kcUBn=kc.max_core_number();
	EXPECT_EQ(UB_corr, kcUBn);
	EXPECT_EQ(kcUBn, kc.minimum_width(true));   //checks width (real degrees)
	

	//test with UB above the real kcore number (corrected to the nearest existing degree)
	KCore<ugraph> kc1(ug);
	kc1.find_kcore();
	kcn = kc1.max_core_number();
	UB_corr=kc1.find_kcore_UB(kcn + 1);
	kcUBn=kc1.max_core_number();

	EXPECT_EQ(UB_corr, kcUBn);	
	EXPECT_EQ(kcUBn, kc1.minimum_width(true)); //checks width (real degrees)

	
	//test with a looser UB above the real kcore number
	KCore<ugraph> kc2(ug);
	kc2.find_kcore();
	kcn=kc2.max_core_number();
	UB_corr=kc2.find_kcore_UB(kcn + 5);
	kcUBn=kc2.max_core_number();

	EXPECT_EQ(UB_corr, kcUBn);	
	EXPECT_EQ(kcUBn, kc2.minimum_width(true)); //checks width (real degrees)
}


TEST(KCoreUB, kcore_example_II){

	ugraph ug(PATH_GRAPH_TESTS_CMAKE_SRC_CODE "r10_0.2_23.txt");
	
	//test with real kcore number
	KCore<ugraph> kc(ug);
	kc.find_kcore();
	int kcn=kc.max_core_number();
	int UB_corr=kc.find_kcore_UB(kcn);
	int kcUBn=kc.max_core_number();

	EXPECT_EQ(UB_corr, kcUBn);
	EXPECT_EQ(kcUBn, kc.minimum_width(true)); //checks width (real degrees)
}

///////////////
//
// DISABLED TESTS
//
////////////////


TEST(KCoreUB, DISABLED_kcore_example_I) {

	ugraph ug(PATH_GRAPH_TESTS_CMAKE_SRC_CODE "ia-southernwomen.edges");

	//test with real kcore number
	KCore<ugraph> kc(ug);
	kc.find_kcore();
	int kcn = kc.max_core_number();
	int UB_corr = kc.find_kcore_UB(kcn);
	int kcUBn = kc.max_core_number();

	EXPECT_EQ(UB_corr, kcUBn);
	EXPECT_EQ(kcUBn, kc.minimum_width(true)); //checks width (real degrees)

}

TEST(KCoreUB, DISABLED_random){
	random_attr_t rd(10, 100, .05, .20, 50, 10, .05);
	const bool store_graph=true;
	char PATH[250]="C:\\Users\\pablo\\Desktop\\random_tests\\";
	stringstream sstr("");
	string name("");
	
	for(int tam=rd.nLB ; tam<=rd.nUB; tam+=rd.incN){
		for(double den=rd.pLB ; den<=rd.pUB; den+=rd.incP){
			for(int rep=0; rep<rd.nRep; rep++){
				ugraph ug;
				RandomGen<ugraph>::create_graph(ug, tam,den);

				//write the graph
				name.clear();
				name="r";
				sstr.str(""); sstr.clear();
				sstr<<tam<<"_"<<den<<"_"<<rep;
				name+=sstr.str();
				ug.set_name(name+".txt");		//change name appropiately
				
				//name stamp
				sstr.str(""); sstr.clear();	
				sstr << ug.name();
				
				//store instance if desired
				//if(store_graph){
				//	if(comclq::graph_to_dimacs(ug, PATH+ug.name())==-1){
				//		LOG_WARNING("random_test: could not store generated instance");
				//	}
				//}
			
				KCore<ugraph> kc(ug);
				kc.find_kcore();
				int kcn=kc.max_core_number();

				KCore<ugraph> kcUB(ug);
				int UB_corr= kcUB.find_kcore_UB(kcn);
				int kcUBn=kcUB.max_core_number();
				EXPECT_EQ(UB_corr, kcUBn);
				EXPECT_EQ(kcUBn, kcUB.minimum_width(true));		//checks width (real degrees)
				
				
				if(kcn!=kcUBn){
					sstr<<":"<<"kcore: "<<kc.max_core_number()<<" kcore_UB: "<<kcUB.max_core_number();
					LOGG_INFO(sstr.str());
				}
	
			}
		}
	}
}

