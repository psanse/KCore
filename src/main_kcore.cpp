/**
 * @file main_kcore.cpp
 * @brief Main executable for the k-core decomposition of undirected graphs.
 *
 * @details Computes the coreness of every vertex of a graph. With no arguments it runs
 * on a small demo graph; otherwise it reads the graph from the DIMACS file given as the first argument.
 *
 * @author pss
 */

#include <iostream>
#include "kcore.h"

/**
 * @brief Prints the maximum core number and the coreness of every vertex
 * @param ug graph to decompose
 */
static void print_decomposition(bitgraph::ugraph& ug) {

	kcore::KCore<bitgraph::ugraph> kc{ ug };
	kc.find_kcore();

	std::cout << "|V|=" << ug.num_vertices() << " max core number: " << kc.max_core_number() << std::endl;
	std::cout << "[vertex:coreness] in degeneracy order: " << kc << std::endl;
}

/**
 * @brief Demo: k-core decomposition of a graph read from a file, or of a small demo graph
 * @param argc number of command line arguments
 * @param argv command line arguments; optional first argument: path to a DIMACS graph file
 * @returns 0 on success
 */
int main(int argc, char** argv) {

	if (argc > 1) {
		bitgraph::ugraph ug{ argv[1] };
		print_decomposition(ug);
	}
	else {
		//triangle {0, 1, 2} with a pendant vertex 3 and an isolated vertex 4
		bitgraph::ugraph ug{ 5 };
		ug.add_edge(0, 1);
		ug.add_edge(1, 2);
		ug.add_edge(0, 2);
		ug.add_edge(2, 3);
		print_decomposition(ug);			//max core number 2
	}

	return 0;
}
