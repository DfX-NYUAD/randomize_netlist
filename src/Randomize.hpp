#ifndef _RANDOMIZE_MAIN
#define _RANDOMIZE_MAIN

// library includes
#include "Generic.hpp"
#include "Data.hpp"

class Randomize {
	private:
		/// debugging code switch (private)
		static constexpr bool DBG = false;
		static constexpr bool DBG_VERBOSE = false;

	// private data, functions
	private:
		static void evaluateHD(
				Data::Netlist orig_netlist_copy,
				std::unordered_map<std::string, Data::Node> nodes_copy,
				unsigned const& iterations,
				double& HD_threads,
				std::mutex& m
			);
		static void evaluateHDHelper(std::unordered_map<std::string, Data::Node>& nodes);
		static void determGraphOrderRec(Data::Node const* node);
		static bool evaluateString(std::string function);

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
		static void iteration(
				Data const& data,
				double& HD
			);
		static void initGraph(
				std::unordered_map<std::string, Data::Node>& nodes,
				Data::Netlist const& netlist
			);
		static void determGraphOrder(std::unordered_map<std::string, Data::Node> const& nodes);
		static bool checkGraphForCycles(Data::Node const* node);
};

#endif
