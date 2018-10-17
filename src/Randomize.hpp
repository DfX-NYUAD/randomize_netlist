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
		static unsigned constexpr TRIALS_LIMIT_FACTOR = 3;
		static void randomizeHelperReplaceCell(Data const& data, Data::Netlist& netlist);
		static void randomizeHelperSwapOutputs(bool& consider_fanout, Data::Netlist& netlist);
		static void randomizeHelperSwapInputs(Data::Netlist& netlist);
		static void randomizeHelperDeleteGate(Data::Netlist& netlist);
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
		enum class RandomOperation : unsigned {ReplaceCell = 1, SwapOutputs = 2, SwapInputs = 3, DeleteGate = 4, InsertGate = 5};

		static void iteration(Data& data, double& HD);
		static void initGraph(Data::Netlist& netlist);
		static void determGraphOrder(std::unordered_map<std::string, Data::Node> const& nodes);
		static bool checkGraphForCycles(Data::Node const* node);
};

#endif
