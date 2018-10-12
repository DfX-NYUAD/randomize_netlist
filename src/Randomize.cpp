
#include "IO.hpp"
#include "Randomize.hpp"
#include "Data.hpp"

const std::string Data::STRINGS_GLOBAL_SOURCE = "GLOBAL_SOURCE";
const std::string Data::STRINGS_GLOBAL_SINK = "GLOBAL_SINK";

int main (int argc, char** argv) {
	Data data;
	double HD;
	unsigned iter;

	std::cout << std::endl;
	std::cout << "Randomize netlist" << std::endl;
	std::cout << "-----------------" << std::endl << std::endl;

	// track start time
	auto start_time = std::chrono::system_clock::now();

	// init random generator with high-resolution timing seed
	auto now = std::chrono::high_resolution_clock::now();
	auto nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
	std::srand(nanos);

	// set locale for output; for using thousand separators
	std::cout.imbue(std::locale(""));

	// parse program parameters and test files
	IO::parseParametersFiles(data, argc, argv);

	// parse cells
	IO::parseCells(data);

	// parse netlist
	IO::parseNetlist(data);

	// init the original graph
	//
	// It's important to understand that the original graph CANNOT be copied for each iteration -- the pointers in the graph nodes would
	// be based on the initial data and, hence, any modifications on the graph would impose those edits only on the underlying initial
	// data.  Besides, the alternative of initializing the graph once and then rewriting all the pointers for a graph copy would
	// probably not help much for performance.
	//
	Randomize::initGraph(data.netlist.nodes, data.netlist);

	// sanity check for cycles on original graph
	if (Randomize::checkGraphForCycles( &(data.netlist.nodes[Data::STRINGS_GLOBAL_SOURCE])) ) {
		std::cout << "Randomize> Error: original netlist contains a cycle; exiting ..." << std::endl;
		exit(1);
	}

	// for original graph, determine the order of all graph nodes, from global source to global sink;
	// required for subsequent HD evaluation, which is based on propagation of Boolean values from inputs to outputs
	Randomize::determGraphOrder(data.netlist.nodes);

	// now, iteratively run random modifications on graph copies, until HD approaches desired value
	//
	iter = 0;
	HD = 0.0;
	do {
		iter++;
		Randomize::iteration(data, HD);

		std::cout << "Randomize> Iteration: " << iter << std::endl;
		std::cout << "Randomize>  Current HD: " << HD << std::endl;

		// TODO drop once randomization is done
		break;
	}
	while (HD < data.HD_target);

	// output randomized netlist
	IO::writeNetlist(data);

	// also log runtime
	std::chrono::duration<double> runtime = std::chrono::system_clock::now() - start_time;
	std::cout << "Randomize> Overall runtime: " << runtime.count() << " seconds" << std::endl;
	std::cout << "Randomize>" << std::endl;
};

void Randomize::iteration(Data const& data, double& HD) {
	// threads
	std::vector<std::thread> threads;
	std::mutex m;
	// local graph data
	std::unordered_map<std::string, Data::Node> nodes;
	// local netlist data
	Data::Netlist netlist;

	// init the local netlist
	netlist = data.netlist;

	// TODO random operation on local netlist
	//
	// init the local graph 
	Randomize::initGraph(nodes, netlist);

	// TODO if a new gate has been inserted, check for loop

	// determine the order of all graph nodes, from global source to global sink;
	// required for subsequent HD evaluation, which is based on propagation of Boolean values from inputs to outputs
	Randomize::determGraphOrder(nodes);

	//// evaluate HD in parallel
	//threads.reserve(data.threads);
	//double const iterations_per_thread = data.HD_sampling_iterations / data.threads;
	//for (unsigned t = 1; t <= data.threads; t++) {
	//	// pass original netlist and local graph as copies, since the threads have to work on separate data sets
	//	threads.emplace_back( std::thread(Randomize::evaluateHD, data.netlist, nodes, std::ref(netlist), std::ref(iterations_per_thread), std::ref(HD), std::ref(m)) );
	//}
	//// join threads; the main thread execution will pause until all threads are done
	//for (std::thread& t : threads) {
	//	t.join();
	//}
	//// clean up all threads
	//threads.clear();

	// TODO if HD improved, make local netlist the new global netlist
}

bool Randomize::checkGraphForCycles(Data::Node const* node) {

	if (Randomize::DBG) {
		std::cout << "DBG> Check graph for cycles; consider node: " << node->name << std::endl;
	}

	// node not visited/checked yet
	//
	//
	if (!node->visited) {

		if (Randomize::DBG) {
			std::cout << "DBG>  Proceed with node \"" << node->name <<"\";";
			std::cout << " not visited yet; mark as visited and as part of recursion" << std::endl;
		}

		// mark as visited/checked, and also mark as part of this recursion
		node->visited = node->recursion = true;

		// now, check all children in depth-first manner
		//
		for (unsigned c = 0; c < node->children.size(); c++) {
			auto const* child = node->children[c];

			if (Randomize::DBG) {
				std::cout << "DBG>   Consider node \"" << node->name << "\"'s child: \"" << child->name << "\"";
				std::cout << "; child " << c + 1 << "/" << node->children.size() << std::endl;
			}

			// child not visited yet; check recursively whether some cycle can be found
			//
			if (!child->visited && Randomize::checkGraphForCycles(child)) {

				if (Randomize::DBG) {
					std::cout << "DBG> Return from recursive check of node \"" << node->name << "\"'s child: \"" << child->name << "\"";
					std::cout << "; a cycle was found ..." << std::endl;
				}

				return true;
			}
			// child already visited; in case it has been visited during the current recursive call, then we found a cycle/backedge
			// http://www.geeksforgeeks.org/detect-cycle-in-a-graph/
			//
			else if (child->recursion) {

				if (Randomize::DBG) {
					std::cout << "DBG>   Cycle found; passed this node \"" << child->name << "\" already during recursion" << std::endl;
				}

				return true;
			}
			// child already visited, but not part of the recursion anymore; represents a transitive edge from one parent node
			// to some child node, which is fine
			// 
			else {
				if (Randomize::DBG) {
					std::cout << "DBG>   Cleared node \"" << node->name << "\"'s child: " << child->name;
					std::cout << "; child " << c + 1 << "/" << node->children.size() << std::endl;
				}
			}
		}
	}

	if (Randomize::DBG) {
		std::cout << "DBG> Check graph for cycles; DONE consider node: \"" << node->name << "\"" << std::endl;
	}

	// after return from recursion; mark as "not anymore part of a recursion"
	node->recursion = false;

	return false;
}

void Randomize::determGraphOrder(std::unordered_map<std::string, Data::Node> const& nodes) {

	if (Randomize::DBG) {
		std::cout << "DBG> Determining the graph order, in ascending order from global source to global sink ..." << std::endl;
	}

	// global source has index 0 by definition
	nodes.find(Data::STRINGS_GLOBAL_SOURCE)->second.index = 0;

	// recursively handle all node, starting from global source
	Randomize::determGraphOrderRec(&(nodes.find(Data::STRINGS_GLOBAL_SOURCE)->second));

	// dbg log
	//
	if (Randomize::DBG) {

		std::cout << "DBG> Print netlist graph: " << std::endl;

		for (auto const& node_iter : nodes) {
			auto const& node = node_iter.second;

			if (Randomize::DBG) {
				std::cout << "DBG>  \"" << node.name << "\":" << std::endl;

				std::cout << "DBG>   Index: " << node.index << std::endl;

				switch (node.type) {
					case Data::Node::Type::Dummy :
						std::cout << "DBG>   Type: Dummy" << std::endl;
						break;
					case Data::Node::Type::Wire :
						std::cout << "DBG>   Type: Wire" << std::endl;
						break;
					case Data::Node::Type::Gate :
						std::cout << "DBG>   Type: Gate" << std::endl;
						break;
					case Data::Node::Type::PI :
						std::cout << "DBG>   Type: PI" << std::endl;
						break;
					case Data::Node::Type::PO :
						std::cout << "DBG>   Type: PO" << std::endl;
						break;
				}

				std::cout << "DBG>   Children [" << node.children.size() << "]:";
				for (auto const* child : node.children) {
					std::cout << " \"" << child->name << "\"";
					std::cout << "; Index: " << child->index;
				}
				std::cout << std::endl;

				std::cout << "DBG>   Parents [" << node.parents.size() << "]:";
				for (auto const* parent : node.parents) {
					std::cout << " \"" << parent->name << "\"";
					std::cout << "; Index: " << parent->index;
				}
				std::cout << std::endl;
			}
		}

		std::cout << "DBG> Done" << std::endl;
		std::cout << "DBG> " << std::endl;
	}
}

void Randomize::determGraphOrderRec(Data::Node const* node) {

	// derive index for current node from maximum among parents
	//
	for (auto const* parent : node->parents) {
		node->index = std::max(node->index, parent->index + 1);
	}
	
	if (Randomize::DBG_VERBOSE) {

		std::cout << "DBG>  Depth-first traversal of graph -- current node: " << node->name << std::endl;
		std::cout << "DBG>   Topological index: " << node->index << std::endl;

		if (!node->children.empty()) {
			std::cout << "DBG>   Children: " << node->children.size() << std::endl;
			for (auto const* child : node->children) {
				std::cout << "DBG>    Child: " << child->name << std::endl;
				std::cout << "DBG>     Current topological index of child: " << child->index << std::endl;
			}
		}
	}

	// traverse all children in depth-first manner
	//
	for (auto *child : node->children) {

		// only traverse when useful; in case the child's index is already larger than the index of the current node, no updates
		// will be possible
		//
		if (child->index <= node->index) {

			if (Randomize::DBG_VERBOSE) {
				std::cout << "DBG>  Depth-first traversal of graph; continue with child of node: " << node->name << std::endl;
			}

			Randomize::determGraphOrderRec(child);
		}
	}
	
	if (Randomize::DBG_VERBOSE) {

		std::cout << "DBG>  Depth-first traversal of graph; done for now with node: " << node->name << std::endl;
	}
}

void Randomize::initGraph(std::unordered_map<std::string, Data::Node>& nodes, Data::Netlist const& netlist) {

	if (Randomize::DBG) {
		std::cout << "DBG> Initializing the graph ..." << std::endl;
	}

	// add global sink/source as nodes
	nodes.insert(std::make_pair(
				Data::STRINGS_GLOBAL_SOURCE,
				Data::Node(Data::STRINGS_GLOBAL_SOURCE, Data::Node::Type::Dummy)
			));
	nodes.insert(std::make_pair(
				Data::STRINGS_GLOBAL_SINK,
				Data::Node(Data::STRINGS_GLOBAL_SINK, Data::Node::Type::Dummy)
			));

	// sanity check whether nodes had been inserted / can be found
	if (Randomize::DBG) {

		if (nodes.find(Data::STRINGS_GLOBAL_SOURCE) == nodes.end()) {
			std::cout << "DBG>  Error: the following node was not inserted/found: \"" << Data::STRINGS_GLOBAL_SOURCE << "\"" << std::endl;
		}
		if (nodes.find(Data::STRINGS_GLOBAL_SINK) == nodes.end()) {
			std::cout << "DBG>  Error: the following node was not inserted/found: \"" << Data::STRINGS_GLOBAL_SINK << "\"" << std::endl;
		}
	}

	// add inputs as nodes
	// 
	for (auto const& input : netlist.inputs) {

		nodes.insert(std::make_pair(
					input,
					Data::Node(input, Data::Node::Type::PI)
				));

		// sanity check whether nodes had been inserted / can be found
		if (Randomize::DBG) {
			if (nodes.find(input) == nodes.end()) {
				std::cout << "DBG>  Error: the following node was not inserted/found: \"" << input << "\"" << std::endl;
			}
		}

		// also add new node for primary inputs as child to global source
		nodes[Data::STRINGS_GLOBAL_SOURCE].children.emplace_back( &(nodes[input]) );
		// also add global source as parent for new node
		nodes[input].parents.emplace_back( &nodes[Data::STRINGS_GLOBAL_SOURCE] );
	}

	// add outputs as nodes
	for (auto const& output : netlist.outputs) {

		nodes.insert(std::make_pair(
					output,
					Data::Node(output, Data::Node::Type::PO)
				));

		// sanity check whether nodes had been inserted / can be found
		if (Randomize::DBG) {
			if (nodes.find(output) == nodes.end()) {
				std::cout << "DBG>  Error: the following node was not inserted/found: \"" << output << "\"" << std::endl;
			}
		}

		// also add global sink as child for new node
		nodes[output].children.emplace_back( &(nodes[Data::STRINGS_GLOBAL_SINK]) );
		// also add new node as parent for global sink
		nodes[Data::STRINGS_GLOBAL_SINK].parents.emplace_back( &(nodes[output]) );
	}

	// add gates as nodes
	for (auto const& gate : netlist.gates) {

		nodes.insert(std::make_pair(
					gate.name,
					Data::Node(gate.name, Data::Node::Type::Gate)
				));

		// sanity check whether nodes had been inserted / can be found
		if (Randomize::DBG) {
			if (nodes.find(gate.name) == nodes.end()) {
				std::cout << "DBG>  Error: the following node was not inserted/found: \"" << gate.name << "\"" << std::endl;
			}
		}

		// also memorize pointer to gate in node
		nodes[gate.name].gate = &gate;
	}

	// add wires as nodes
	for (auto const& wire : netlist.wires) {

		nodes.insert(std::make_pair(
					wire,
					Data::Node(wire, Data::Node::Type::Wire)
				));

		// sanity check whether nodes had been inserted / can be found
		if (Randomize::DBG) {
			if (nodes.find(wire) == nodes.end()) {
				std::cout << "DBG>  Error: the following node was not inserted/found: \"" << wire << "\"" << std::endl;
			}
		}
	}

	// connect graph based on connectivity of gates
	for (auto const& gate : netlist.gates) {
		auto const& gate_iter = nodes.find(gate.name);

		if (Randomize::DBG_VERBOSE) {
			std::cout << "DBG>  Gate: \"" << gate.name << "\"" << std::endl;
		}

		// check all the inputs of the gate
		//
		// gate.inputs: cell pin is key, pin/net name is value
		for (auto const& input_iter : gate.inputs) {
			auto const& node_iter = nodes.find(input_iter.second);

			if (Randomize::DBG_VERBOSE) {
				std::cout << "DBG>   Input pin: \"" << input_iter.first << "\"" << std::endl;
			}

			// there's a node matching the input of the gate
			if (node_iter != nodes.end()) {

				if (Randomize::DBG_VERBOSE) {
					std::cout << "DBG>    Connected node: \"" << node_iter->first << "\"" << std::endl;
				}

				// memorize the gate's node as child of the node
				node_iter->second.children.emplace_back(
						&(gate_iter->second)
					);

				// memorize the node as parent for the gate's node
				gate_iter->second.parents.emplace_back(
						&(node_iter->second)
					);
			}
			// there's no node matching; that means the pin is dangling
			else {
				std::cout << "IO> Warning: the input pin \"" << input_iter.first << "\" of gate \"" << gate.name << "\" is dangling;" << std::endl;
				std::cout << "IO>  the following wire/pin should have been connected, but was not found in the graph: \"" << input_iter.second << "\"" << std::endl;
			}
		}

		// check all the outputs of the gate
		//
		// gate.outputs: cell pin is key, pin/net name is value
		for (auto const& output_iter : gate.outputs) {
			auto const& node_iter = nodes.find(output_iter.second);

			if (Randomize::DBG_VERBOSE) {
				std::cout << "DBG>   Output pin: \"" << output_iter.first << "\"" << std::endl;
			}

			// there's a node matching the output of the gate
			if (node_iter != nodes.end()) {

				if (Randomize::DBG_VERBOSE) {
					std::cout << "DBG>    Connected node: \"" << node_iter->first << "\"" << std::endl;
				}

				// memorize the node as child for the gate's node
				gate_iter->second.children.emplace_back(
						&(node_iter->second)
					);

				// memorize the gate's node as parent of the node
				node_iter->second.parents.emplace_back(
						&(gate_iter->second)
					);
			}
			// there's no node matching; that means the pin is dangling
			else {
				std::cout << "IO> Warning: the output pin \"" << output_iter.first << "\" of gate \"" << gate.name << "\" is dangling;" << std::endl;
				std::cout << "IO>  the following wire/pin should have been connected, but was not found in the graph: \"" << output_iter.second << "\"" << std::endl;
			}
		}
	}

	// dbg log
	//
	if (Randomize::DBG) {

		std::cout << "DBG> Print netlist graph: " << std::endl;

		// count all edges
		int edges = 0;
		for (auto const& node_iter : nodes) {
			auto const& node = node_iter.second;

			edges += node.children.size();
			edges += node.parents.size();

			if (Randomize::DBG) {
				std::cout << "DBG>  \"" << node.name << "\":" << std::endl;

				std::cout << "DBG>   Children [" << node.children.size() << "]:";
				for (auto const* child : node.children) {
					std::cout << " \"" << child->name << "\"";
				}
				std::cout << std::endl;

				std::cout << "DBG>   Parents [" << node.parents.size() << "]:";
				for (auto const* parent : node.parents) {
					std::cout << " \"" << parent->name << "\"";
				}
				std::cout << std::endl;
			}
		}

		std::cout << "DBG> Done" << std::endl;
		std::cout << "DBG>  Nodes: " << nodes.size() << std::endl;
		std::cout << "DBG>  Edges: " << edges << std::endl;
		std::cout << "DBG> " << std::endl;
	}
}
