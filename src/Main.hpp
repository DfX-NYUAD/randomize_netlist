#ifndef _MAIN
#define _MAIN

// library includes
#include "Generic.hpp"
#include "Data.hpp"

class Main {
	private:
		/// debugging code switch (private)
		static constexpr bool DBG = false;
		static constexpr bool DBG_VERBOSE = false;

	// private data, functions
	private:
		static void evaluateHDHelper(std::unordered_map<std::string, Data::Node>& nodes, std::vector< std::vector<Data::Node const*> > const& nodes_topology, bool const& lazy_evaluation);
		static void determGraphOrderRec(Data::Node const* node);
		static bool evaluateString(std::string function, bool const& lazy_evaluation);

		// random-number function
		//
		// note: range is [min, max)
		inline static int rand(int const& min, int const& max) {
			if (max == min) {
				return min;
			}
			else {
				return min + (std::rand() % (max - min));
			}
		};


	// constructors, destructors, if any non-implicit
	private:

	// public data, functions
	public:
		static void initGraph(Data::Netlist& netlist);
		static void determGraphOrder(Data::Netlist const& netlist);
		static bool checkGraphForCycles(Data::Node const* node);

		static void evaluateHD(
				std::unordered_map<std::string, Data::Node> orig_nodes_copy,
				std::unordered_map<std::string, Data::Node> nodes_copy,
				std::vector< std::vector<Data::Node const*> > const& orig_nodes_topology,
				std::vector< std::vector<Data::Node const*> > const& nodes_copy_topology,
				std::vector<std::string> const& inputs,
				std::vector<std::string> const& outputs,
				std::vector< std::vector<bool> > const& input_patterns,
				bool const& lazy_evaluation,
				unsigned const& iterations,
				unsigned thread_ID,
				double& HD,
				std::mutex& m
			);
};

#endif
