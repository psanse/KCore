/**
 * @file kcore_imp.h

 */

//
//#include "utils/logger.h"
//#include "graph/graph_unweighted.h"
//#include "graph_fast_sort.h"
//#include <vector>
//#include <iostream>
//#include <algorithm>
//#include <map>
//
//#include <cassert>		 //undef NDEBUG to avoid assertions


//useful aliases

using vint = std::vector<int>;
using vint_it = std::vector<int>::iterator;
using vint_cit = std::vector<int>::const_iterator;

using map_t = std::map<int, int>;
using map_it = std::map<int, int>::iterator;


template<class GraphT>
inline int KCore<GraphT>::find_kcore(graph_type& g)
{
	//degenerate minimum degree ordering
	GraphFastRootSort<GraphT> gfs(g);
	vint degen_order = gfs.new_order(GraphFastRootSort<GraphT>::strategy_t::min_degeneracy, GraphFastRootSort<GraphT>::placement_t::last_to_first, GraphFastRootSort<GraphT>::sort_order_t::new_to_old);		//MUST BE LAST-TO-FIRST!

	//I. count maximum of the neighbors of each vertex v that precedes v in the ordering
	//II.find the maximum count
	int max_width = 0;
	for (auto j = 1; j < degen_order.size(); ++j) {

		auto width = 0;
		for (auto i = j - 1; i >= 0; --i) {
			if (g.is_edge(degen_order[j], degen_order[i])) {
				++width;
			}
		}

		//update max_width
		if (max_width < width) {
			max_width = width;
		}

		//TO NOTE: no early exit if width decreases in two subsequent vertices - 
		//the values are not guaranteed to increase monotonically and then
		//decrease monotonically after the maximum core (as seen in tests with brock200_1.clq 13/01/25)

		//ON THE OTHER HAND, the maximum width value is the maximum core number 
	}

	return max_width;
}

template<class GraphT>
inline KCore<GraphT>::KCore(graph_type& g) : g_(g), NV_(g.num_vertices()), deg_(NV_), pos_(NV_) {

	try {
		ver_.assign(NV_, bitgraph::EMPTY_ELEM);
	}
	catch (std::bad_alloc& ba) {
		LOGG_ERROR("bad_alloc exception - KCore<T>::reset_subgraph", ba.what());
		LOG_ERROR("exiting...");
		std::exit(EXIT_FAILURE);
	}
}

template<class GraphT>
inline KCore<GraphT>::KCore(graph_type& g, vertex_bitset_t subg) :
	g_(g),
	NV_(g.num_vertices()),
	deg_(NV_),
	pos_(NV_),
	subg_(std::move(subg))
{
	try {
		ver_.assign(subg_.count(), EMPTY_ELEM);
	}
	catch (std::bad_alloc& ba) {
		LOGG_ERROR("bad_alloc exception - KCore<T>::reset_subgraph", ba.what());
		LOG_ERROR("Exiting...");
		std::exit(EXIT_FAILURE);
	}
}

template<class GraphT>
inline KCore<GraphT>::KCore(graph_type& g, const vertices& subg) :
	g_(g),
	NV_(g.num_vertices()),
	deg_(NV_),
	pos_(NV_)
{
	try {
		subg_ = typename graph_type::vertex_bitset_t{ static_cast<std::size_t>(NV_), subg };
		ver_.assign(subg_.count(), EMPTY_ELEM);
	}
	catch (std::bad_alloc& ba) {
		LOGG_ERROR("bad_alloc exception - KCore<T>::reset_subgraph", ba.what());
		LOG_ERROR("Exiting...");
		std::exit(EXIT_FAILURE);
	}
}


template<class GraphT>
inline void KCore<GraphT>::reset_subgraph(vertex_bitset_t subg) {

	subg_ = std::move(subg);

	try {
		ver_.assign(subg_.count(), EMPTY_ELEM);
	}
	catch (const std::bad_alloc& ba) {
		LOGG_ERROR("bad_alloc exception - KCore<T>::reset_subgraph", ba.what());
		std::terminate();
	}
}


template<class GraphT>
inline void KCore<GraphT>::find_kcore(bool is_subg) {

	// Initialize data structures (fail-fast on allocation failure).
	init_kcore(is_subg);
	bin_sort(is_subg);

	if (!is_subg) {

		// K-core computation for the full graph.
		for (auto v : ver_) {

			vertex_bitset_t& neigh = g_.neighbors(v);
			neigh.init_scan(bitgraph::bbo::NON_DESTRUCTIVE);

			int u = bitgraph::bbo::noBit;
			while ((u = neigh.next_bit()) != bitgraph::bbo::noBit) {

				if (deg_[u] > deg_[v]) {
					SWAP_BIN(u);
					--deg_[u];
				}
			}
		}
	}
	else {

		// K-core computation for the induced subgraph.
		vertex_bitset_t neigh(NV_);

		for (auto v : ver_) {

			AND(g_.neighbors(v), subg_, neigh);
			neigh.init_scan(bitgraph::bbo::NON_DESTRUCTIVE);

			int u = bitgraph::bbo::noBit;
			while ((u = neigh.next_bit()) != bitgraph::bbo::noBit) {

				if (deg_[u] > deg_[v]) {
					SWAP_BIN(u);
					--deg_[u];
				}
			}
		}
	}
}

template<class GraphT>
inline int KCore<GraphT>::find_kcore_UB(int UB_out) {


	//inits data structures
	init_kcore(false);
	bin_sort(false);


	//check that UB_out is not the maximum graph degree
	//i.e. deg = 1 , bin has size 2 (0 and 1)
	if (bin_.size() <= UB_out + 1) {
		LOG_INFO("UB is not worse than maximum graph degree: vertices left as is - KCore<GraphT>::kcore_UB");
		return UB_out;
	}

	//variables / update UB to the nearest existing degree
	int u = BBObject::noBit;
	int v = BBObject::noBit;
	int UB = UB_out;
	int w = ver_[bin_[UB]];
	if (deg_[w] != UB) { UB = deg_[w]; }

	//main loop
	int deg = UB;
	int p_iter;
	int p_newUB;
	bool first_time_newLB = true;
	while (deg >= 1) {
		p_iter = bin_[deg];
		do {
			//classical kcore loop for vertices with degree deg
			v = ver_[p_iter];
			first_time_newLB = true;
			p_newUB = -1;

			//loop over neighbors of v with degree greater than UB
			g_.neighbors(v).init_scan(bbo::NON_DESTRUCTIVE);
			//if (g_.neighbors(v).init_scan(bbo::NON_DESTRUCTIVE) != -1) {
			while ((u = g_.neighbors(v).next_bit()) != BBObject::noBit) {

				if (deg_[u] > UB) {

					/////////////////
					SWAP_BIN(u);
					/////////////////

					//update degree: if vertex u has deg LB+1, instead of updating to LB it takes the degree of v
					if (deg_[u] == UB + 1) {
						deg_[u] = deg_[v];
						if (first_time_newLB && deg_[v] != UB) {
							first_time_newLB = false;
							p_newUB = pos_[u];		//new position of u
						}
					}
					else deg_[u]--;
				}
			}
			//}

			///////////////////////////////////////////////////////
			//extra loop for new vertices with degree LB caused by v

			if (p_newUB != BBObject::noBit) {
				while (p_newUB != bin_[UB + 1]) {

					//classical kcore loop for new vertices with degree LB
					v = ver_[p_newUB];

					//loop over neighbors of v with degree greater than UB
					g_.neighbors(v).init_scan(bbo::NON_DESTRUCTIVE);
					//if (g_.neighbors(v).init_scan(bbo::NON_DESTRUCTIVE) != -1) {

					while ((u = g_.neighbors(v).next_bit()) != BBObject::noBit) {
						if (deg_[u] > UB) {

							/////////////////
							SWAP_BIN(u);										//swaps u to the last position of vertices with one less degree	(does not update degree)
							/////////////////

							(deg_[u] == UB + 1) ? deg_[u] = deg_[v] : deg_[u]--;		//updates degree
						}
					}
					//}//endif

					p_newUB++;	//next vertex in UB
				}
			}// end of extra loop 			

			p_iter++;	//next vertex v

		} while (p_iter != bin_[deg + 1]);

		//find next legal deg below current deg
		const int DEG = deg;
		do {
			deg--;
		} while (bin_[DEG] == bin_[deg] && deg > 0);
	}//end of main loop


	//new bin sort over ver_ in reverse order for final sorting
	init_bin();
	bitgraph::vertices lv(ver_);			//requires a copy
	bin_sort(lv, true);			//reverse order

	return UB;
}

template<class GraphT>
inline void KCore<GraphT>::init_kcore(bool is_subg) {

	int max_deg = 0;

	if (!is_subg) {

		// Compute degrees and maximum degree of the full graph.
		for (int v = 0; v < NV_; ++v) {
			deg_[v] = g_.degree(v);
			if (max_deg < deg_[v]) {
				max_deg = deg_[v];
			}
		}
	}
	else {

		// Compute degrees in the induced subgraph.
		subg_.init_scan(bitgraph::bbo::NON_DESTRUCTIVE);

		int v = bitgraph::bbo::noBit;
		while ((v = subg_.next_bit()) != bitgraph::bbo::noBit) {
			deg_[v] = g_.degree(v, subg_);
			if (max_deg < deg_[v]) {
				max_deg = deg_[v];
			}
		}
	}

	// Allocate and initialize degree bins (fail-fast).
	try {
		bin_.assign(static_cast<std::size_t>(max_deg) + 1, 0);
	}
	catch (const std::bad_alloc& ba) {
		LOGG_ERROR("bad_alloc exception - KCore<GraphT>::init_kcore",
			ba.what());
		std::terminate();
	}

	// Count vertices in each degree bin.
	if (!is_subg) {

		for (bitgraph::vertex_t v = 0; v < NV_; ++v) {
			++bin_[deg_[v]];
		}
	}
	else {

		subg_.init_scan(bitgraph::bbo::NON_DESTRUCTIVE);

		bitgraph::vertex_t v = bitgraph::bbo::noBit;
		while ((v = subg_.next_bit()) != bitgraph::bbo::noBit) {
			++bin_[deg_[v]];
		}
	}
}

template<class GraphT>
inline void KCore<GraphT>::init_bin(bool is_subg) {

	bitgraph::degree_t max_deg = 0;

	// Find the maximum degree.
	if (!is_subg) {
		if (!deg_.empty()) {
			max_deg = *std::max_element(deg_.begin(), deg_.end());
		}
	}
	else {
		subg_.init_scan(bbo::NON_DESTRUCTIVE);

		int v = BBObject::noBit;
		while ((v = subg_.next_bit()) != BBObject::noBit) {
			max_deg = std::max(max_deg, deg_[v]);
		}
	}

	// Allocate and initialize degree bins (fail-fast).
	try {
		bin_.assign(static_cast<std::size_t>(max_deg) + 1, 0);
	}
	catch (const std::bad_alloc& ba) {
		LOGG_ERROR("bad_alloc exception - KCore<GraphT>::init_bin",
			ba.what());
		std::terminate();
	}

	// Count vertices in each degree bin.
	if (!is_subg) {

		for (bitgraph::vertex_t v = 0; v < NV_; ++v) {
			++bin_[deg_[v]];
		}
	}
	else {

		subg_.init_scan(bitgraph::bbo::NON_DESTRUCTIVE);

		bitgraph::vertex_t v = BBObject::noBit;
		while ((v = subg_.next_bit()) != BBObject::noBit) {
			++bin_[deg_[v]];
		}
	}
}



template<class GraphT>
inline void KCore<GraphT>::bin_sort(bool is_subg) {


	//sets bin_ with the position in the new ordering (I): 
	//based on the number of vertices with that degree previously stored with init_bin()
	//Example  bin_ = {3, 4, 2, 1} -> bin_ = {0, 3, 7, 9}

	auto start = 0;
	for (auto& bval : bin_) {
		auto num = bval;
		bval = start;
		start += num;
	}

	if (!is_subg) {

		//bin_sort full graph
		for (auto v = 0u; v < ver_.size(); ++v) {
			pos_[v] = bin_[deg_[v]];					//stores the new position of v (pos_ is not strictly needed for bin_sort, but it is for kcore)
			ver_[pos_[v]] = v;							//places v in the new position in ver_ (mapping -  new-to-old format)			
			bin_[deg_[v]]++;
		}
	}
	else {

		//bin_sort subgraph induced by subg_			
		subg_.init_scan(bitgraph::bbo::NON_DESTRUCTIVE);
		bitgraph::vertex_t v = bitgraph::bbo::noBit;
		while ((v = subg_.next_bit()) != bitgraph::bbo::noBit) {
			pos_[v] = bin_[deg_[v]];
			ver_[pos_[v]] = v;
			bin_[deg_[v]]++;
		}
	}

	//restores the bin indexes with the position in the new ordering (I)
	//after fixing: bin[0] = 0 always, bin[1] = 0 if there are no isolated vertices etc.
	//bin_ = {1, 4, 8, 10} -> bin_ = {0, 1, 4, 8}

	for (auto d = bin_.size() - 1; d >= 1; --d) {
		bin_[d] = bin_[d - 1];
	}
	bin_[0] = 0;

}

template<class GraphT>
inline void KCore<GraphT>::bin_sort(
	bitgraph::vertex_ordering_t& ordering,
	bool reverse)
{
	
		//init bin_ to point at the correct position in vertex array according to size
	int start = 0;
	int num = 0;
	for (auto d = 0; d < bin_.size(); ++d) {
		num = bin_[d];
		bin_[d] = start;
		start += num;
	}

	if (reverse) {
		for (vint::reverse_iterator it = ordering.rbegin(); it != ordering.rend(); ++it) {
			int v = *it;
			pos_[v] = bin_[deg_[v]];
			ver_[pos_[v]] = v;
			bin_[deg_[v]]++;
		}
	}
	else {
		for (vint::iterator it = ordering.begin(); it != ordering.end(); ++it) {
			int v = *it;
			pos_[v] = bin_[deg_[v]];
			ver_[pos_[v]] = v;
			bin_[deg_[v]]++;
		}

	}

	//corrects bin indexes after ordering (after update will point to the first element of their type)
	//after correction: bin[0]=0 always, bin[1]=0 if there are no isolated vertices etc.
	for (auto d = bin_.size() - 1; d >= 1; --d) {
		bin_[d] = bin_[d - 1];
	}
	bin_[0] = 0;
}

template<class GraphT>
inline int KCore<GraphT>::minimum_width(bool rev) {

	int maxNumNeigh = EMPTY_ELEM;
	int	numNeigh = EMPTY_ELEM;
	vertex_bitset_t bb_unsel(NV_);
	vertex_bitset_t bb_sel(NV_);

	if (rev) {

		for (auto it = ver_.rbegin(); it != ver_.rend(); ++it) {

			//computes subgraph population (TODO: optimize)
			bb_unsel = g_.neighbors(*it);
			bb_unsel.erase_bit(bb_sel);

			///////////////////////////////
			numNeigh = bb_unsel.count();
			if (maxNumNeigh < numNeigh) { maxNumNeigh = numNeigh; }
			////////////////////////////////

			bb_sel.set_bit(*it);
		}

	}
	else {

		for (auto it = ver_.begin(); it != ver_.end(); ++it) {

			//computes subgraph population (TODO: optimize)
			bb_unsel = g_.neighbors(*it);
			bb_unsel.erase_bit(bb_sel);

			///////////////////////////////
			numNeigh = bb_unsel.count();
			if (maxNumNeigh < numNeigh) { maxNumNeigh = numNeigh; }
			///////////////////////////////

			bb_sel.set_bit(*it);
		}
	}

	return maxNumNeigh;
}



template<class GraphT>
inline auto KCore<GraphT>::max_core_number() const noexcept -> core_t
{

	//assert
	if (ver_.empty()) {
		return -1;
	}

	return deg_[ver_.back()];
}

template<class GraphT>
inline auto KCore<GraphT>::core_set(std::size_t k) const -> vertices
{

	vint res;

	for (auto v = 0; v < deg_.size(); ++v) {
		if (deg_[v] >= k) {
			res.emplace_back(v);
		}
	}

	return res;
}

template<class GraphT>
inline std::size_t KCore<GraphT>::core_size(std::size_t k) const noexcept {

	auto count = 0;

	for (auto v = 0; v < deg_.size(); ++v) {
		if (deg_[v] == k) {
			count++;
		}
	}

	return count;
}

template<class GraphT>
inline int KCore<GraphT>::make_kcore_filter(map_t& filter, bool reverse) {
	///////////////////////////////
	// maps kcore number to the starting vertex of the next kcore partition (filter[kc(v)]->first vertex w,  kc(w)=kc(v)+1)
	// Example [v,kc(v)], reverse=TRUE, kcore dec:[1,1][2,1][3,2][4,1] then filter[2]=1 the single element in filter
	// Example [v,kc(v)], reverse=FALSE, kcore dec:[1,1][2,1][3,2][4,1] then filter[1]=4 the single element in filter
	//
	// PARAMS:
	// 1-filter: the mapping
	// 2-reverse: if TRUE assumes kcore partitions are in increasing order from first to last
	//            if FALSE assumes kcore partitions are in decreasing order from first to last
	//
	// RETURNS: size of filter
	// REMARKS: kcore-decomposition must exist (prior call to k-core())

		//***assert existence of kcore decomposition

	filter.clear();
	int next_kcore_pos = 0;
	int current_k = EMPTY_ELEM;

	if (reverse) {
		current_k = deg_[ver_.back()];
		for (auto i = static_cast<int>(ver_.size()) - 2; i >= 0; --i) {
			++next_kcore_pos;
			if (deg_[ver_[i]] != current_k) {
				//adds to map kcore values and intermediate values
				int diff = current_k - deg_[ver_[i]];
				int ck = current_k;

				do {
					filter.insert(std::pair<int, int>(ck--, next_kcore_pos));
					--diff;
				} while (diff >= 1);

				//filter.insert(std::pair<int, int>(current_k,next_kcore_pos));
				current_k = deg_[ver_[i]];
			}
		}
	}
	else {

		current_k = deg_[ver_.front()];
		for (auto i = 1; i < ver_.size(); ++i) {
			++next_kcore_pos;
			if (deg_[ver_[i]] != current_k) {
				//adds to map kcore values and intermediate values
				int diff = deg_[ver_[i]] - current_k;
				int ck = current_k;

				do {
					filter.insert(std::pair<int, int>(ck--, next_kcore_pos));
					--diff;
				} while (diff >= 1);

				//filter.insert(std::pair<int, int>(current_k,next_kcore_pos));
				current_k = deg_[ver_[i]];
			}
		}
	}

	return filter.size();
}

template<class GraphT>
inline auto KCore<GraphT>::find_heur_clique(int num_iter) -> vertices
{

	int max_size = 1;
	int iter = 1;
	vertices curr_clique;
	vertices largest_clique;
	vertices neighbors;

	//main loop
	for (auto i = static_cast<int>(ver_.size()) - 1; i >= 0; --i) {

		//CUT at root level
		int v = ver_[i];
		if (deg_[v] < max_size) {
			break;
		}

		//determines neighbor set in degeneracy order
		neighbors.clear();
		for (int j = i - 1; j >= 0; --j) {
			if (deg_[ver_[j]] >= max_size && g_.neighbors(v).is_bit(ver_[j])) {
				neighbors.push_back(ver_[j]);									//vertices are placed in neighbor in degeneracy order (I)
			}
		}

		//determines largest greedy clique in neighbor set
		curr_clique.clear();
		for (int n = 0; n < neighbors.size(); ++n) {	//vertices selected in degeneracy order (I)							
			bool good_vertex = true;
			for (auto l = 0; l < curr_clique.size(); ++l) {
				if (!g_.neighbors(curr_clique[l]).is_bit(neighbors[n])) {
					good_vertex = false;
					break;
				}
			}
			if (good_vertex) {
				curr_clique.push_back(neighbors[n]);
			}
		}

		//adds initial vertex
		curr_clique.push_back(v);

		//update size
		if (max_size < curr_clique.size()) {
			largest_clique = curr_clique;
			max_size = curr_clique.size();

			//I/O
			LOGG_DEBUG(iter, ".", "lb:", max_size, " seed:", ver_[i]);
		}


		//evaluation of number of iterations (exit condition received parameter)
		if (++iter >= num_iter && num_iter != EMPTY_ELEM)	break;
	}

	if (num_iter != EMPTY_ELEM) {
		LOGG_INFO("kc[", "it:", iter, ",", " lb:", max_size, "]");
	}
	else LOGG_INFO("kc[lb:", max_size, "]");

	return largest_clique;
}

template<>
inline auto KCore<bitgraph::sparse_ugraph>::find_heur_clique_opt(int num_iter) -> vertices
{
	///////////////////////
	// Optimized version of find_clique for speed
	// date:30/12/2014
	//
	// COMMENTS: At the moment, only available for sparse_graph type

	std::size_t max_size = 1;
	auto iter = 1;
	//auto from = EMPTY_ELEM;

	vertices curr_clique;
	vertices largest_clique;
	bitgraph::sparse_bitarray bbneigh(NV_);

	//main loop
	for (auto i = static_cast<int>(ver_.size()) - 1; i > 0; --i) {

		//CUT at root level
		if (deg_[ver_[i]] < max_size) {
			break;
		}

		//determines neighbor set
		curr_clique.clear();
		bbneigh = g_.neighbors(ver_[i]);

		//iterates over all vertices to pick them in degeneracy ordering
		for (auto j = i - 1; j >= 0; --j) {

			if (deg_[ver_[j]] < max_size) { break; }

			if (bbneigh.is_bit(ver_[j])) {				//adjacent
				curr_clique.push_back(ver_[j]);
				bbneigh &= g_.neighbors(ver_[j]);
			}
		}

		//adds initial vertex
		curr_clique.push_back(ver_[i]);

		//update size
		if (max_size < curr_clique.size()) {
			largest_clique = curr_clique;
			max_size = largest_clique.size();

			//I/O
			LOGG_DEBUG(iter, ".", "lb:", max_size, " seed: [", i, ":", ver_[i], "]");
		}

		//evaluation of number of iterations 
		if (++iter >= num_iter && num_iter != bitgraph::EMPTY_ELEM) { break; }
	}

	if (num_iter != bitgraph::EMPTY_ELEM) {
		LOGG_INFO("kc[", "it:", iter, ",", " lb:", max_size, "]");
	}
	else { LOGG_INFO("kc[lb:", max_size, "]"); }

	return largest_clique;
}


template<>
inline auto KCore<bitgraph::sparse_ugraph>::find_heur_clique_sparse(int num_iter) -> vertices
{
	///////////////////////
	// Optimized version of find_clique for speed in large sparse graphs
	// date:30/12/2014
	//
	// COMMENTS: (only available for sparse_graph type)

	struct less_kcore {
		less_kcore(std::vector<int>& pos, std::vector<int>& ver, std::vector<int>& deg) :pos(pos), ver(ver), deg(deg) {}
		bool operator()(int lhs, int rhs) const {
			return(deg[ver[pos[rhs]]] > deg[ver[pos[lhs]]]);	//CHECK rhs / lhs!!!
		}
		std::vector<int>& pos;
		std::vector<int>& ver;
		std::vector<int>& deg;
	};

	struct remove_kcore {

		//constructor
		remove_kcore(std::vector<int>& pos, std::vector<int>& ver, std::vector<int>& deg, int max_clique) :
			pos(pos), ver(ver), deg(deg), max_core_number(max_clique)
		{}

		//functor operator
		bool operator()(int data) const {
			return(deg[ver[pos[data]]] < max_core_number);
		}

		//data members
		std::vector<int>& pos;
		std::vector<int>& ver;
		std::vector<int>& deg;
		int max_core_number;
	};

	std::size_t max_size = 1;
	auto iter = 1;
	//auto from = EMPTY_ELEM;

	vertices curr_clique;
	vertices largest_clique;
	vertices candidates;
	bitgraph::sparse_bitarray bbneigh(NV_);

	//main loop
	for (auto i = ver_.size() - 1; i > 0; --i) {

		//CUT at root level
		if (deg_[ver_[i]] < max_size) { break; }

		//determines neighbor set
		curr_clique.clear();
		bbneigh = g_.neighbors(ver_[i]);
		bbneigh.extract(candidates);

		//sort degeneracy
		std::sort(candidates.begin(), candidates.end(), less_kcore(pos_, ver_, deg_));

		candidates.erase(remove_if(candidates.begin(), candidates.end(), remove_kcore(pos_, ver_, deg_, static_cast<int>(max_size))), candidates.end());


		//iterates over all vertices to pick them in degeneracy ordering
		for (auto n = 0u; n < candidates.size(); ++n) {

			if (bbneigh.is_bit(candidates[n])) {				//adjacent

				curr_clique.push_back(candidates[n]);
				bbneigh &= g_.neighbors(candidates[n]);
			}
		}

		//adds initial vertex
		curr_clique.push_back(ver_[i]);

		//update size
		if (max_size < curr_clique.size()) {
			largest_clique = curr_clique;
			max_size = largest_clique.size();

			//I/O
			LOGG_DEBUG(iter, ".", "lb:", max_size, " seed: [", i, ":", ver_[i], "]");
		}

		//evaluation of number of iterations 
		if (++iter >= num_iter && num_iter != bitgraph::EMPTY_ELEM) { break; }
	}

	//log
	if (num_iter != bitgraph::EMPTY_ELEM) {
		LOGG_INFO("kc[", "it:", iter, ",", " lb:", max_size, "]");
	}
	else { LOGG_INFO("kc[lb:", max_size, "]"); }

	return largest_clique;
}

///////////////////////////
//
// INPUT/OUTPUT
//
///////////////////////

template<class GraphT>
inline std::ostream& KCore<GraphT>::print(
	print_mode type, 
	std::ostream& os) {
	switch (type) {
	case print_mode::degree:
		for (auto i = 0; i < deg_.size(); ++i) {
			os << deg_[i] << " ";
		}
		os << std::endl;
		break;
	case print_mode::bin:
		for (auto i = 0; i < bin_.size(); ++i) {
			o << bin_[i] << " ";
		}
		os << std::endl;
		break;
	case print_mode::vertex:
		for (auto i = 0; i < ver_.size(); ++i) {
			o << ver_[i] << " ";
		}
		os << std::endl;
		break;
	case print_mode::position:
		for (auto i = 0; i < pos_.size(); ++i) {
			os << pos_[i] << " ";
		}
		os << std::endl;
		break;

	default:
		;
	}

	return o;
}

template<class GraphT>
inline std::ostream& KCore<GraphT>::print_kcore(
	bool include_degree, 
	bool is_subgraph,
	std::ostream& os) const
{

	if (! is_subgraph) {

		//whole graph
		for (auto it = ver_.begin(); it != ver_.end(); ++it) {
			os << "[" << *it << ":" << deg_[*it];
			if (include_degree) {
				os << ":" << g_.degree(*it) << "] ";
			}
			else {
				os << "] ";
			}
		}
	}
	else {

		//subgraph
		auto v = bitgraph::bbo::noBit;

		//scans a copy: this method is const and scanning modifies the bitset state
		vertex_bitset_t sg(subg_);
		sg.init_scan(bitgraph::bbo::NON_DESTRUCTIVE);

		while ((v = sg.next_bit()) != bitgraph::bbo::noBit) {
			os << "[" << v << "," << deg_[v];
			if (include_degree) {
				os << ":" << g_.degree(v) << "] ";
			}
			else {
				os << "] ";
			}
		}
	}

	return os;
}


