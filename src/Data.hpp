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
		// locale for outputs
		static const std::string LOCALE;

		// names/identifier for global source/sink nodes
		static const std::string STRINGS_GLOBAL_SOURCE;
		static const std::string STRINGS_GLOBAL_SINK;

		struct parameters {
			// threads
			unsigned threads = 1;
			// step size for HD to output/generate intermediate netlist result
			double intermediate_output_HD_step = 0.05;
			// desired HD value
			double HD_target = 0.5;
			// HD sampling iterations
			unsigned HD_sampling_iterations = 1e3;
			// acceptance ratio for modifications with inferior HD
			double acceptance_ratio = 1e-2;

			// try to consider and keep the fan-outs when swapping outputs for a pair of gates
			bool consider_fanout = false;
			// try to keep the same driving strength when replacing the cell type;
			// parsed in IO::parseParametersFiles
			// can be overwritten in Randomize::randomizeHelperReplaceCell if needed
			mutable bool consider_driving_strength = true;

			// default is -1; pick operation randomly
			static constexpr int DEFAULT_RANDOM_OP = -1;
			int random_op = DEFAULT_RANDOM_OP;

		} parameters;

		// POD for files names
		struct files {
			std::string in_netlist;
			std::string cells_inputs;
			std::string cells_outputs;
			std::string cells_functions;
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
			std::vector<std::string> inputs;
			std::vector<std::string> outputs;
			// wires
			std::vector<std::string> wires;
			// gates
			std::vector<Gate> gates;

			// graph container
			// mapping: name, node
			std::unordered_map<std::string, Data::Node> nodes;
		} netlist, netlist_original;

		// PODs for tracking modifications statistic
		struct NetlistModifications {
			unsigned replacedCells = 0;
			unsigned swappedInputs = 0;
			unsigned swappedOutputs = 0;
			unsigned deletedGates = 0;
			unsigned insertedGates = 0;
		} netlist_modifications;
};

#endif
