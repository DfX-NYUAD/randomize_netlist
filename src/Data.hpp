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
		// threads
		unsigned threads = 1;

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
			std::vector<std::string> inputs;
			std::vector<std::string> outputs;
			std::vector<std::string> functions;
		};
		// actual container
		// mapping: name, cell
		std::unordered_map<std::string, Cell> cells;

		// PODs for graph data
		struct Node {
			// name of gate or pin
			std::string name;

			//enum class type_enum : unsigned {gate, input, output};
			//type_enum type;

			//std::vector<Node const*> parents;
			std::vector<Node const*> children;

			mutable bool visited = false;
			mutable bool recursion = false;

			Node(
				std::string n = "",
				//type_enum type,
				//std::vector<Node const*> p = std::vector<Node const*>(),
				std::vector<Node const*> c = std::vector<Node const*>()
			) :
				name(n),
				//type(t),
				//parents(p),
				children(c) {}
		};
		// names/identifier for global source/sink
		struct globalNodeNames {
			std::string source = "GLOBAL_SOURCE";
			std::string sink = "GLOBAL_SINK";
		} globalNodeNames;

		// PODs for gates
		struct Gate {
			std::string type;
			std::string name;
			// cell pin name, net/pin name
			std::unordered_map<std::string, std::string> inputs;
			std::unordered_map<std::string, std::string> outputs;
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
		} netlist;
};

#endif
