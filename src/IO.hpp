#ifndef _RANDOMIZE_IO
#define _RANDOMIZE_IO

// library includes
#include "Generic.hpp"
// forward declarations
class Data;

/// IO handler
class IO {
	private:
		/// debugging code switch (private)
		static constexpr bool DBG = false;

	// private data, functions
	private:

	// constructors, destructors, if any non-implicit
	private:
		/// empty default constructor; private in order to avoid instances of ``static'' class
		IO() {
		}

	// public data, functions
	public:
		static void testFile(std::string const& file);
		static void parseParametersFiles(Data& data, int const& argc, char** argv);
		static void parseCells(Data& data);
		static void parseNetlist(Data& data);
		static void writeNetlist(Data& data, double const& HD, unsigned const& iterations, bool scramble = false);
};

#endif
