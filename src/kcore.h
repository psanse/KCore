/**
 * @file kcore.h
 * @brief Declaration and implementation of the KCore class, which computes the
 *        coreness of the vertices of an undirected graph.
 *
 * @details The decomposition runs in O(|E|) time using the bin-sort algorithm of
 *          Batagelj and Zaversnik. It can be applied to the whole graph or to the
 *          subgraph induced by a set of vertices. The class also provides a degeneracy
 *          ordering of the vertices and greedy clique heuristics based on it.
 *
 * @reference V. Batagelj, M. Zaversnik, "An O(m) Algorithm for Cores Decomposition of Networks", 2003.
 *            https://arxiv.org/abs/cs/0310049
 *
 * @author pss
 * @date 2014-06-06
 * @date last modified 2026-10-18
 * @since 2014-06-06
 * @version 2025-01-27
 * 
 * @note Requires the BITGRAPH library.
 *
 * @todo Store an owned bitset or a vector of vertices for the induced subgraph, instead of
 *       relying on an externally supplied set (13/01/25).
 * @todo Add other types to define the set of vertices that induces the subgraph (13/01/25).
 */

#ifndef KCORE_H
#define KCORE_H

#include "utils/logger.h"
#include "graph/graph_unweighted.h"
#include "graph_fast_sort.h"
#include <vector>
#include <iostream>
#include <algorithm>
#include <map>

#include <cassert>		 //undef NDEBUG to avoid assertions

/**
 * @def SWAP_BIN(u)
 * @brief Moves vertex @p u to the last position of the bin with one degree less.
 *
 * @details Swaps @p u with the first vertex of its degree bin in @c ver_, updates @c pos_
 *          and advances the start of the bin in @c bin_. The degree of @p u in @c deg_
 *          is NOT updated: the caller must decrement it afterwards.
 *          Meant to be used only inside KCore member functions.
 *
 * @param u vertex to move
 */
#define SWAP_BIN(u)	{ int du = deg_[(u)]; int pu = pos_[(u)]; int pw = bin_[du]; int w = ver_[pw];		\
						if( (u) != w) { pos_[(u)] = pw; pos_[w] = pu; ver_[pu] = w; ver_[pw] = (u);}	\
						bin_[du]++;																			}
namespace kcore {

	/**
	 * @class KCore
	 * @brief Computes and stores the coreness of the vertices of an undirected graph.
	 *
	 * @details The k-core of a graph is its maximal subgraph in which every vertex has
	 *          degree at least k. The coreness (core number) of a vertex is the largest k
	 *          such that the vertex belongs to the k-core. The maximum core number of
	 *          a graph is its degeneracy.
	 *
	 *          Typical use: construct the object, call find_kcore(), and then query the
	 *          results with the getters. The getters are only meaningful after a
	 *          decomposition has been computed.
	 *
	 * @tparam GraphT undirected graph type (for example bitgraph::ugraph or bitgraph::sparse_ugraph)
	 *
	 * @note The graph is stored by reference and must outlive the KCore object.
	 *       It must not be modified while the object is in use.
	 *
	 * @todo Verify that the graph has not been changed (08/09/2025).
	 */
	template<class GraphT>
	class KCore {
	public:

		/** @brief Streams the k-core ordering of the whole graph (see print_kcore()). */
		friend std::ostream& operator<< (std::ostream& o, const KCore& kc) { kc.print_kcore(false, false, o); return o; }

		/** @brief Data structure to print in the private print() method. */
		enum class print_mode : int {
			degree = 0,		///< coreness of the vertices
			bin,			///< bin starts of the bin sort
			vertex,			///< vertices in k-core order
			position		///< position of the vertices in the ordering
		};

		//alias types
		using graph_type = GraphT;
		using vertex_bitset_t = typename graph_type::vertex_bitset_t;
		using core_numbers_t = std::vector<int>;
		using core_t = int;
		using vertices = bitgraph::vertices;
		using vertex_ordering_t = bitgraph::vertices;
		using map_t = std::map<int, int>;

		//////////////
		//globals

		/**
		 * @brief Maximum core number of a graph in O(|V|*|V|).
		 *
		 * @details Computes the width of the graph according to a minimum degeneracy
		 *          ordering. Meant as a reference implementation for small and
		 *          medium dense graphs; not suitable for large sparse graphs.
		 *          No early exit is possible: the width is not monotonic along the ordering.
		 *
		 * @param g graph
		 * @returns maximum core number of @p g
		 */
		static int find_kcore(graph_type& g);

		//////////////////////////////
		//construction / destruction
	public:

		/**
		 * @brief Constructor for the whole graph.
		 * @param g graph to decompose
		 */
		explicit KCore(graph_type& g);

		/**
		 * @brief Constructor for the subgraph induced by a bitset of vertices.
		 * @param graph - input graph
		 * @param vertices bitset with the vertices of the induced subgraph
		 */
		KCore(graph_type& graph,
			vertex_bitset_t vertices);

		/**
		 * @brief Constructor for the subgraph induced by a list of vertices.
		 * @param graph - input graph
		 * @param vertices vertices of the induced subgraph
		 */
		KCore(graph_type& graph,
			const vertices& vertices);

		// copy and move semantics forbidden: the graph is stored by reference and must outlive the KCore object
		KCore(const KCore&) = delete;
		KCore& operator=(const KCore&) = delete;
		KCore(KCore&&) = delete;
		KCore& operator=(KCore&&) = delete;

		virtual ~KCore() = default;

		////////////////
		//setters and getters

		/**
		 * @brief Maximum core number of the graph.
		 * @pre find_kcore() has been called.
		 * @returns maximum core number, or -1 if there is no decomposition
		 */
		core_t max_core_number() const noexcept;

		/**
		 * @brief Core number of a vertex.
		 * @pre find_kcore() has been called.
		 * @param v vertex
		 * @returns core number of @p v
		 */
		core_t coreness(std::size_t v) const noexcept
		{
			return deg_[v];
		}

		/**
		 * @brief Number of vertices whose core number is exactly @p k.
		 * @pre find_kcore() has been called.
		 * @param k core number, k >= 0
		 */
		std::size_t core_size(std::size_t k) const noexcept;

		/**
		 * @brief Vertices with core number at least @p k, that is, the vertices of the k-core.
		 *
		 * @details For k = 0 all vertices are returned, for k = 1 the vertices of the
		 *          1-core, and so on.
		 *
		 * @pre find_kcore() has been called.
		 * @param k core number, k >= 0
		 */
		vertices core_set(std::size_t k) const;

		/**
		 * @brief Core numbers of all the vertices.
		 * @pre find_kcore() has been called.
		 */
		const core_numbers_t& coreness_numbers() const noexcept
		{
			return deg_;
		}

		/**
		 * @brief Vertices arranged by non-decreasing core number (new-to-old format).
		 * @pre find_kcore() has been called.
		 */
		const vertex_ordering_t& kcore_ordering() const noexcept
		{
			return ver_;
		}

		/** @brief Graph being decomposed. */
		const graph_type& graph() const noexcept
		{
			return g_;
		}

		/** @brief Bitset of vertices that induces the subgraph (empty for the whole graph). */
		const vertex_bitset_t& subgraph() const noexcept
		{
			return subg_;
		}

		/**
		 * @brief Sets a new induced subgraph.
		 *
		 * @details Only reallocates the ordering @c ver_. A new call to find_kcore(true)
		 *          is required to obtain the coreness of the new subgraph.
		 *          Terminates the program if memory allocation fails.
		 *
		 * @param vertices bitset with the vertices of the new induced subgraph
		 */
		void reset_subgraph(vertex_bitset_t vertices);

		//////////////
		// Main operations

		/**
		 * @brief Computes the coreness of the vertices and a degeneracy ordering (core algorithm, O(|E|)).
		 *
		 * @details After its execution:
		 *          - kcore_ordering() holds the vertices in non-decreasing core number (new-to-old format);
		 *          - coreness_numbers() holds the core number of every vertex;
		 *          - max_core_number() is the core number of the last vertex in the ordering.
		 *
		 *          Terminates the program if memory allocation fails.
		 *
		 * @param is_subgraph if TRUE computes the decomposition of the subgraph induced by
		 *					  the vertices in get_subgraph(), otherwise of the whole graph
		 */
		void find_kcore(bool is_subgraph = false);

		/**
		 * @brief Degeneracy ordering assuming a known upper bound for the core number.
		 *
		 * @details Vertices with degree @p UB (or the closest existing degree above @p UB) are
		 *          placed last in the ordering. It is a degeneracy ordering provided that
		 *          @p UB is at least the core number of the graph. Compared with find_kcore(),
		 *          vertices are processed as follows:
		 *          1. start with the vertices of degree UB;
		 *          2. for each degree set, remove vertices iteratively;
		 *          3. select any vertex that reached degree UB as a consequence of 2 and remove it iteratively;
		 *          4. go to 1.
		 *
		 *          The result is an ordering of width d (the core number) that places vertices
		 *          by non-decreasing degree starting at UB. This differs from find_kcore(), which
		 *          places vertices in non-increasing core number. It may be useful for
		 *          enumerating maximal cliques (idea not proved).
		 *
		 *          The core numbers in coreness_numbers() are NOT valid after this call; only
		 *          the ordering is. The last vertex in the ordering has degree equal to the returned bound.
		 *
		 * @note Implemented only for the whole graph, not for induced subgraphs.
		 *
		 * @param UB upper bound of the core number
		 * @returns the corrected bound: the nearest existing degree not smaller than @p UB
		 *
		 * @warning experimental: the algorithm is not fully tested and may not be correct in all cases.
		 * @date 2016-03-05
		 */
		int find_kcore_UB(int UB);

		/**
		 * @brief Width of the graph for the ordering stored in the object.
		 *
		 * @details The width of a graph for an ordering is the maximum number of neighbors
		 *          of a vertex that come AFTER it in the ordering (or BEFORE it, for @p rev = TRUE).
		 *          A non-increasing core number ordering is a minimum width ordering, and the
		 *          minimum width of a graph equals its maximum core number.
		 *          Computationally heavy: meant for tests.
		 *
		 * @param rev if FALSE computes the minimum width, if TRUE a width that is not necessarily minimum
		 * @returns the width of the graph for the ordering
		 */
		int minimum_width(bool rev = false);

		//////////////////////
		//clique heuristics - to refactor

		/**
		 * @brief Greedy clique heuristic based on the degeneracy ordering.
		 * @pre find_kcore() has been called.
		 * @param num_iter maximum number of seed vertices to try; EMPTY_ELEM tries all of them
		 * @returns the largest clique found
		 */
		vertices find_heur_clique(int num_iter = bitgraph::EMPTY_ELEM);

		/**
		 * @brief Optimized version of find_heur_clique(). Only available for sparse graphs.
		 * @pre find_kcore() has been called.
		 * @param num_iter maximum number of seed vertices to try; EMPTY_ELEM tries all of them
		 * @returns the largest clique found
		 */
		vertices find_heur_clique_opt(int num_iter = bitgraph::EMPTY_ELEM);

		/**
		 * @brief Version of find_heur_clique() for large sparse graphs. Only available for sparse graphs.
		 * @pre find_kcore() has been called.
		 * @param num_iter maximum number of seed vertices to try; EMPTY_ELEM tries all of them
		 * @returns the largest clique found
		 */
		vertices find_heur_clique_sparse(int num_iter = bitgraph::EMPTY_ELEM);

		////////////////
		//Experimental - to refactor

		/**
		 * @brief Maps core numbers to the start of the next core partition (experimental).
		 *
		 * @details Computes a mapping from each core number to the starting position
		 *          of the next core partition in the vertex ordering.
		 *
		 *          The interpretation depends on the ordering of core partitions:
		 *          - @p reverse = true: partitions are ordered by increasing core number.
		 *          - @p reverse = false: partitions are ordered by decreasing core number.
		 *
		 *          For example, given the pairs [vertex, core number]:
		 *          [1,1] [2,1] [3,2] [4,1]
		 *          - @p reverse = true:  filter[2] = 1.
		 *          - @p reverse = false: filter[1] = 4.
		 *
		 *          Originally used in clique algorithms. This functionality is
		 *          experimental and may be removed in the future.
		 *
		 * @pre find_kcore() must have been called to compute the core decomposition.
		 *
		 * @param filter Output mapping from core numbers to partition boundaries.
		 * @param reverse Whether core partitions are ordered by increasing core number.
		 * @return Number of entries in @p filter.
		 */
		int make_kcore_filter(map_t& filter,
			bool reverse = true);

		///////////////
		//I/O

		/**
		 * @brief Streams the k-core information as a list of [vertex:coreness].
		 *
		 * @param include_degree if TRUE also prints the degree of the vertices in the graph
		 * @param is_subgraph if TRUE prints the vertices of the induced subgraph, otherwise
		 *                the vertices of the whole graph in k-core order
		 * @param o output stream
		 * @returns the output stream
		 */
		std::ostream& print_kcore(
			bool include_degree = false,
			bool is_subgraph = false,
			std::ostream& o = std::cout) const;

		//////////////
		// private interface
	private:

		/**
		 * @brief Initializes the degrees @c deg_ and the degree bins @c bin_.
		 * @details @c bin_[d] is set to the number of vertices of degree d.
		 *          Terminates the program if memory allocation fails.
		 * @param is_subg if TRUE considers the subgraph induced by @c subg_, otherwise the whole graph
		 */
		void init_kcore(bool is_subg = false);

		/**
		 * @brief Initializes the degree bins @c bin_ from the current degrees.
		 * @details @c bin_[d] is set to the number of vertices of degree d.
		 *          Terminates the program if memory allocation fails.
		 * @pre @c deg_ is already set
		 * @param is_subg if TRUE considers the subgraph induced by @c subg_, otherwise the whole graph
		 *
		 * @date 2016-03-13
		 * @version 2025-01-12
		 */
		void init_bin(bool is_subg = false);

		/**
		 * @brief Bin sorts the vertices by non-decreasing degree in @c ver_ (new-to-old format).
		 * @details Also sets @c pos_ and turns @c bin_ into the start position of each bin,
		 *          as required by find_kcore().
		 * @pre init_kcore() has been called
		 * @param is_subg if TRUE considers the subgraph induced by @c subg_, otherwise the whole graph
		 */
		void bin_sort(bool is_subg = false);

		/**
		 * @brief Sorts vertices in @p lv by non-decreasing degree using bin sort (experimental).
		 *
		 * @details Vertices are processed in the order specified by @p lv, or in
		 *          reverse order when @p reverse is true. Sorting takes linear time.
		 *
		 *          Currently used by find_kcore_UB(), which requires @p lv to
		 *          contain all vertices of the graph. Support for induced subgraphs
		 *          is expected to work but has not been tested.
		 *
		 * @pre Vertex degrees must be initialized in @c deg_ and bins in @c bin_.
		 *
		 * @param ordering Vertex ordering to be sorted.
		 * @param reverse If true, processes vertices in reverse order.
		 *
		 * @date 2016-03-07
		 * @version 2016-03-13
		 */
		void bin_sort(bitgraph::vertex_ordering_t& ordering, bool reverse);

		/**
		 * @brief Streams one of the internal data structures (debugging).
		 * @param mode data structure to print
		 * @param o output stream
		 */
		std::ostream& print(
			print_mode mode = print_mode::vertex,
			std::ostream& o = std::cout);

		///////////
		// data members
	private:

		graph_type& g_;				///< the one and only graph G=(V, E)
		int NV_;					///< number of vertices |V|, for convenience
		vertex_bitset_t subg_;		///< vertices of the induced subgraph to study

		core_numbers_t deg_;		///< coreness of the vertices (the degree during the computation)
		vertices bin_;				///< bins for the bin sort: position of the first vertex of each degree in @c ver_
		vertices ver_;				///< vertices in non-decreasing k-core order (new-to-old format)
		vertices pos_;				///< position of each vertex in @c ver_ (old-to-new format)
	};

} // end namespace kcore


//////////////////////////////////////////////////////
// IMPLEMENTATION - in header for generic code

#include "kcore_imp.h"


#endif // KCORE_H
