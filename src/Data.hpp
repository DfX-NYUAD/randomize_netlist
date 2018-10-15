#ifndef _RANDOMIZE_DATA
#define _RANDOMIZE_DATA

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
		// names/identifier for global source/sink nodes
		static const std::string STRINGS_GLOBAL_SOURCE;
		static const std::string STRINGS_GLOBAL_SINK;

		// threads
		unsigned threads = 1;
		// HD sampling iterations
		unsigned HD_sampling_iterations = 1e3;
		// desired HD value
		double HD_target = 0.5;

		// POD for files names
		struct files {
			std::string in_netlist;
			std::string cells_inputs;
			std::string cells_outputs;
			std::string cells_functions;
			std::string out_netlist;
		} files;

		// PODs for cells
		struct Cell {
			std::string type;
			std::string type_wo_strength;
			std::string strength;
			std::set<std::string> inputs;
			std::set<std::string> outputs;
			// output pin name, Boolean function of input pin names
			std::unordered_map<std::string, std::string> functions;
		};
		// actual container
		// mapping: name, cell
		std::unordered_map<std::string, Cell> cells;

		// PODs for gates
		struct Gate {
			std::string name;
			Cell const* cell = nullptr;

			// cell pin name, net/pin name
			std::unordered_map<std::string, std::string> inputs;
			std::unordered_map<std::string, std::string> outputs;
		};

		// PODs for graph data
		struct Node {
			// name of gate or pin
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

			// current Boolean value; -1 means undefined; required for HD evaluation
			mutable int bit = -1;

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

		// PODs for netlists
		struct Netlist {
			// module name
			std::string module_name;
			// I/Os ports
			std::set<std::string> inputs;
			std::set<std::string> outputs;
			// wires
			std::set<std::string> wires;
			// gates
			std::vector<Gate> gates;

			// graph container
			// mapping: name, node
			std::unordered_map<std::string, Data::Node> nodes;
		} netlist, netlist_original;
};

#endif
