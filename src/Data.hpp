#ifndef _DATA
#define _DATA

// library includes
#include "Generic.hpp"

class Data {
	private:
		/// debugging code switch (private)
		static constexpr bool DBG = false;

	// private data, functions
	private:

	// constructors, destructors, if any non-implicit
	private:

	// public data
	public:
		// the name of the binary; defined in IO::parseParametersFiles
		std::string binary;

		// locale for outputs
		static const std::string LOCALE;

		// names/identifier for global nodes
		static const std::string STRINGS_GLOBAL_SOURCE;
		static const std::string STRINGS_GLOBAL_SINK;
		static const std::string STRINGS_GLOBAL_DUMMY_PI;

		struct Parameters {
			// threads
			unsigned threads = 1;
			// HD sampling iterations
			unsigned HD_sampling_iterations = 1e3;
			// if active, Boolean evaluation is short-cut, but this can miss errors in Boolean strings
			bool lazy_Boolean_evaluation = true;

		} parameters;

		// POD for files names
		struct Files {
			std::string netlist;
			std::string golden_netlist;
			std::string cells_inputs;
			std::string cells_outputs;
			std::string cells_functions;
			std::string input_patterns;
		} files;

		// POD for cells
		struct Cell {
			std::string type;
			std::string type_wo_strength;
			int strength;
			std::set<std::string> inputs;
			std::set<std::string> outputs;
			// output pin name, Boolean function of input pin names
			std::unordered_map<std::string, std::string> functions;
		};
		// actual container
		// mapping: name, cell
		std::unordered_map<std::string, Cell> cells;

		// POD for gates
		struct Gate {
			std::string name;
			Cell const* cell = nullptr;

			// cell pin name, net/pin name
			std::unordered_map<std::string, std::string> inputs;
			std::unordered_map<std::string, std::string> outputs;
		};

		// POD for graph data
		struct Node {
			// name of gate, wire, or pin
			std::string name;

			enum class Type : unsigned {Dummy, Gate, Wire, PI, PO};
			Type type;

			std::vector<Node const*> parents;
			std::vector<Node const*> children;

			// index for topological order, from global source to global sink in ascending order
			// -1 means undefined, global source will be assigned 0
			mutable int index = -1;

			// flags for checking for combinatorial loops/cycles
			mutable bool visited = false;
			mutable bool recursion = false;

			// current Boolean value; for HD evaluation
			mutable bool bit = false;

			// pointer to gate, if any
			Gate const* gate = nullptr;

			Node(
				std::string n = "",
				Type t = Type::Dummy,
				std::vector<Node const*> p = std::vector<Node const*>(),
				std::vector<Node const*> c = std::vector<Node const*>()
			) :
				name(n),
				type(t),
				parents(p),
				children(c) {}
		};

		// POD for netlists
		struct Netlist {
			// module name
			std::string module_name;
			// I/Os ports
			std::vector<std::string> inputs;
			std::vector<std::string> outputs;
			// wires
			std::vector<std::string> wires;
			// gates
			std::vector<Gate> gates;
			// assignments; mapping: PO sink, PI source
			std::unordered_map<std::string, std::string> assignments;

			// graph container
			// mapping: name, node
			mutable std::unordered_map<std::string, Data::Node> nodes;

			// graph topology
			// first dimension: topological index; second dimension: all nodes belonging to that index
			mutable std::vector< std::vector<Data::Node const*> > topology;
		} netlist, golden_netlist;

		// input patterns, from optional parameter/argument
		// first dimension: patterns; second dimension: bits of one pattern, expected to be in same order as
		// Netlist.inputs
		std::vector< std::vector<bool> > input_patterns;
};

#endif
