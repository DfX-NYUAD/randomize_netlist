#ifndef _RANDOMIZE_IO
#define _RANDOMIZE_IO

// library includes
#include "Generic.hpp"
#include "Data.hpp"

/// IO handler
class IO {
	private:
		/// debugging code switch (private)
		static constexpr bool DBG = false;

	// private data, functions
	private:
		static void testFile(std::string const& file);
		static void printHelp(Data const& data);

	// constructors, destructors, if any non-implicit
	private:
		/// empty default constructor; private in order to avoid instances of ``static'' class
		IO() {
		}

	// public data, functions
	public:
		// regexp for cells along with their driving strength
		static const std::regex REGEX_CELL_DRIVING_STRENGTH;


		static void parseParametersFiles(Data& data, int const& argc, char** argv);
		static void parseCells(Data& data);

		static void parseNetlist(
				std::unordered_map<std::string, Data::Cell> const& cells,
				Data::Netlist& netlist,
				std::string const& file
			);

		static void writeNetlist(Data& data, double const& HD, unsigned const& iterations, bool scramble = false);

};

#endif
