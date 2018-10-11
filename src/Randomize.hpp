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
				std::unordered_map<std::string, Data::Node> const& nodes,
				double& HD,
				std::mutex& m
			);

	// constructors, destructors, if any non-implicit
	private:

	// public data, functions
	public:
		static void iteration(
				Data& data,
				double& HD
			);
		static void initGraph(
				std::unordered_map<std::string, Data::Node>& nodes,
				Data const& data
			);

		static bool checkGraphForCycles(Data::Node const* node);

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
};

#endif
