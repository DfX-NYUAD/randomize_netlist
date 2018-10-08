
#include "IO.hpp"
#include "Randomize.hpp"
#include "Data.hpp"

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
	Randomize::initGraph(data.netlist.nodes, data);

	// sanity check for cycles on original graph
	if (Randomize::checkGraphForCycles(&(data.netlist.nodes[data.globalNodeNames.source]))) {
		std::cout << "Randomize> Error: original netlist contains a cycle; exiting ..." << std::endl;
		exit(1);
	}

	// iteratively run modifications, until HD approaches 50%
	//
	iter = 0;
	HD = 0.0;
	do {
		iter++;
		HD = Randomize::iteration(data);

		std::cout << "Randomize> Iteration: " << iter << std::endl;
		std::cout << "Randomize>  Current HD: " << HD << std::endl;
	}
	while (HD < 0.45);

	// TODO output randomized netlist
};

//void Randomize::rewriteConnectivity(std::pair<std::string, std::string> const& a, Data::Gate& gate, Data& data) {
//
//	//if (Randomize::DBG) {
//	//	std::cout << "DBG> Mapping: " << a.first << " -> " << a. second << std::endl;
//	//	std::cout << "DBG> Gate: " << gate.name << std::endl;
//	//}
//
//	// first is pin name, second is net/pin name
//	for (auto& output : gate.outputs) {
//
//		// the gate's output matches with the assignment driver or the assignment sink
//		if (output.second == a.first || output.second == a.second) {
//
//			if (Randomize::DBG) {
//				std::cout << "DBG>  Affected gate: " << gate.name;
//				std::cout << "; mapping: " << a.first << " -> " << a. second;
//				std::cout << "; affected output: " << output.first << "(" << output.second << ")" << std::endl;
//			}
//
//			// in case the assignment driver is a PO, rename the gate output towards that PO
//			if (data.netlist.outputs.find(a.first) != data.netlist.outputs.end()) {
//				output.second = a.first;
//			}
//			// regular case, no PO; rename gate output towards new wire
//			else {
//				output.second = a.first + "_" + a.second;
//			}
//
//			// also memorize that new wire; by using set, no redundant wires arise
//			data.netlist.wires.insert(output.second);
//
//			if (Randomize::DBG) {
//				std::cout << "DBG>   New output: " << output.second << std::endl;
//			}
//		}
//	}
//
//	// first is pin name, second is net/pin name
//	for (auto& input : gate.inputs) {
//
//		// the gate's input matches with the assignment driver or the assignment sink
//		if (input.second == a.first || input.second == a.second) {
//
//			if (Randomize::DBG) {
//				std::cout << "DBG>  Affected gate: " << gate.name;
//				std::cout << "; mapping: " << a.first << " -> " << a. second;
//				std::cout << "; affected input: " << input.first << "(" << input.second << ")" << std::endl;
//			}
//
//			// in case the assignment driver is a PO, rename the gate input towards that PO
//			if (data.netlist.outputs.find(a.first) != data.netlist.outputs.end()) {
//				input.second = a.first;
//			}
//			// regular case, no PO; rename gate input towards new wire
//			else {
//				input.second = a.first + "_" + a.second;
//			}
//
//			// also memorize that new wire; by using set, no redundant wires arise
//			data.netlist.wires.insert(input.second);
//
//			if (Randomize::DBG) {
//				std::cout << "DBG>   New input: " << input.second << std::endl;
//			}
//		}
//	}
//}

//void Randomize::evaluateAndOutput(Data::AssignmentF2F const& assignment, Data& data) {
//	std::ofstream out;
//	unsigned total_connections;
//	unsigned correct_connections;
//
//	std::cout << "Randomize>" << std::endl;
//	std::cout << "Randomize> Rewriting netlist for F2F mappings ..." << std::endl;
//
//	out.open(data.files.out_netlist.c_str());
//
//	// 1) rewrite the gates' connectivity, based on F2F mappings
//	//
//	// also add new wires to model those F2F mappings
//	//
//	if (Randomize::DBG) {
//		std::cout << "DBG> Rewriting gates' connectivity, adding wires to model F2F mappings ... " << std::endl;
//		std::cout << "DBG>" << std::endl;
//	}
//	for (auto& gate : data.netlist.gates) {
//
//		// check all the mappings, revise gate connectivity if it's covered by one assignment
//		//
//		// note that some inputs as well as some outputs can be affected by that mapping, inputs namely when the output is reused in
//		// the bottom tier for another gate's input
//		//
//		for (auto const& a : assignment.bottom_to_top) {
//
//			Randomize::rewriteConnectivity(a, gate, data);
//		}
//		for (auto const& a : assignment.top_to_bottom) {
//
//			Randomize::rewriteConnectivity(a, gate, data);
//		}
//	}
//	std::cout << "Randomize> Done" << std::endl;
//
//	// 2) write out netlist
//	//
//	std::cout << "Randomize> Writing out netlist ..." << std::endl;
//
//	out << "// Netlist recovered by proximity attack on obfuscated F2F mappings" << std::endl;
//	out << "//" << std::endl;
//	out << std::endl;
//	out << "module recovered(";
//
//	// output all inputs for module ports
//	for (auto const& input : data.netlist.inputs) {
//		out << input << ", ";
//	}
//
//	// output all outputs for module ports
//	unsigned count = 1;
//	for (auto const& output : data.netlist.outputs) {
//
//		// the last output has no comma following, and it also closes the port list
//		if (count == data.netlist.outputs.size()) {
//			out << output << ");" << std::endl;
//		}
//		else {
//			out << output << ", ";
//			count++;
//		}
//	}
//	out << std::endl;
//
//	// output all inputs
//	for (auto const& input : data.netlist.inputs) {
//		out << "input " << input << ";" << std::endl;
//	}
//	out << std::endl;
//
//	// output all outputs
//	for (auto const& output : data.netlist.outputs) {
//		out << "output " << output << ";" << std::endl;
//	}
//	out << std::endl;
//
//	// output all wires
//	for (auto const& wire : data.netlist.wires) {
//		out << "wire " << wire << ";" << std::endl;
//	}
//	out << std::endl;
//
//	// output all gates 
//	for (auto const& gate: data.netlist.gates) {
//		unsigned outputs_remaining = gate.outputs.size();
//		unsigned inputs_remaining = gate.inputs.size();
//
//		out << gate.type << " ";
//		out << gate.name << " ";
//
//		out << "(";
//
//		for (auto const& input : gate.inputs) {
//
//			// corner case: no outputs are present and this is the last input, so end port mapping already here
//			if (outputs_remaining == 0 && inputs_remaining == 1) {
//				out << "." << input.first << "(" << input.second << ")";
//			}
//			else {
//				out << "." << input.first << "(" << input.second << "), ";
//				inputs_remaining--;
//			}
//		}
//
//		// only one output
//		if (outputs_remaining == 1) {
//			out << "." << gate.outputs.begin()->first << "(" << gate.outputs.begin()->second << ")";
//		}
//		// multiple outputs (or zero outputs)
//		else {
//			for (auto const& output : gate.outputs) {
//
//				if (outputs_remaining == 1) {
//					out << "." << output.first << "(" << output.second << ")";
//				}
//				else {
//					out << "." << output.first << "(" << output.second << "), ";
//					outputs_remaining--;
//				}
//			}
//		}
//
//		out << ");" << std::endl;
//	}
//
//	out << std::endl;
//	out << "endmodule" << std::endl;
//	out << std::endl;
//
//	std::cout << "Randomize> Done" << std::endl;
//	std::cout << "Randomize>" << std::endl;
//	std::cout << "Randomize> Evaluating connections:" << std::endl;
//
//	// 3) evaluate the CCR: correct_connections over total_connections
//	//
//	total_connections = assignment.bottom_to_top.size() + assignment.top_to_bottom.size();
//	correct_connections = 0;
//
//	out << "//" << std::endl;
//	out << "// F2F assignmets:" << std::endl;
//	out << "//" << std::endl;
//	out << "//  Bottom to top:" << std::endl;
//	for (auto const& a : assignment.bottom_to_top) {
//
//		out << "//   " << a.first << " -> " << a.second;
//
//		// evaluate correct connections by name
//		//
//		// corner case: if only one assignment is possible, this is consider as correct; this is mimicked by using the same
//		// string
//		if (data.F2F.bottom_to_top.count(a.first) == 1) {
//			out << " (correct)" << std::endl;
//			correct_connections++;
//		}
//		// regular case: names match
//		else if (a.second.find(a.first) != std::string::npos) {
//			out << " (correct)" << std::endl;
//			correct_connections++;
//		}
//		else {
//			out << " (not correct)" << std::endl;
//		}
//	}
//	out << "//" << std::endl;
//
//	out << "//  Top to bottom:" << std::endl;
//	out << "//" << std::endl;
//	for (auto const& a : assignment.top_to_bottom) {
//
//		out << "//   " << a.first << " -> " << a.second;
//
//		// evaluate correct connections by name
//		//
//		// corner case: if only one assignment is possible, this is consider as correct; this is mimicked by using the same
//		// string
//		if (data.F2F.top_to_bottom.count(a.first) == 1) {
//			out << " (correct)" << std::endl;
//			correct_connections++;
//		}
//		// regular case: names match
//		else if (a.first.find(a.second) != std::string::npos) {
//			out << " (correct)" << std::endl;
//
//			correct_connections++;
//		}
//		else {
//			out << " (not correct)" << std::endl;
//		}
//	}
//	out << "//" << std::endl;
//
//	std::cout << "Randomize>  Correct connections: " << correct_connections << std::endl;
//	out << "// Correct connections: " << correct_connections << std::endl;
//
//	std::cout << "Randomize>  Total connections: " << total_connections << std::endl;
//	out << "// Total connections: " << total_connections << std::endl;
//
//	std::cout << "Randomize>  Correct connections ratio: " << static_cast<double>(correct_connections) / total_connections << std::endl;
//	out << "// Correct connections ratio: " << static_cast<double>(correct_connections) / total_connections << std::endl;
//
//	out.close();
//}

double Randomize::iteration(Data& data) {
	std::unordered_map<std::string, Data::Node> nodes;
	std::vector<std::thread> threads;
	std::mutex m;

	//// TODO
	//// init threads for HD evaluation
	//threads.reserve(data.threads);
	//for (unsigned t = 1; t <= data.threads; t++) {
	//	//threads.emplace_back( std::thread(Randomize::iteration, std::ref(data), std::ref(success), std::ref(trials), std::ref(m), std::ref(start_time)) );
	//}
	//// join threads; the main thread execution will pause until all threads are done
	//for (std::thread& t : threads) {
	//	t.join();
	//}
	//// clean up all threads
	//threads.clear();

	// init the graph copy
	//
	Randomize::initGraph(nodes, data);

//	// graph could be build up
//	if (success_trial) {
//
//		// now check for cycles, start from global source
//		//
//		// returns true if cycle found; hence negate to indicate success (no cycle)
//		success_trial = !Randomize::checkGraphForCycles(
//				&(nodes[data.globalNodeNames.source])
//			);
//
//		// in case the run was the first successful one, evaluate that run and generate obtained netlist (using the mutex)
//		if (success_trial & !success) {
//			m.lock();
//
//			success = true;
//
//			std::cout << "Randomize>" << std::endl;
//			std::cout << "Randomize> Success! Found F2F assignment without cycles" << std::endl;
//
//			// also report attack time
//			std::chrono::duration<double> runtime = std::chrono::system_clock::now() - start_time;
//
//			std::cout << "Randomize>" << std::endl;
//			std::cout << "Randomize> Randomize runtime: " << runtime.count() << " seconds" << std::endl;
//
//			// evaluate assignment and output netlist
//			Randomize::evaluateAndOutput(assignment, data);
//
//			m.unlock();
//		}
//	}
//	// handling of the graph may fail in case not all drivers can be mapped to some sink
//	else {
//		std::cout << "Randomize> Failed to assign all F2F mappings; check the mappings.file ..." << std::endl;
//	}

	if (Randomize::DBG) {
		std::cout << "DBG> Randomize trial SUCCESS" << std::endl;
	}

	// TODO return HD
	return 0.0;
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
			std::cout << "DBG>  Proceed with node " << node->name <<";";
			std::cout << " not visited yet; mark as visited and as part of recursion" << std::endl;
		}

		// mark as visited/checked, and also mark as part of this recursion
		node->visited = node->recursion = true;

		// now, check all children in depth-first manner
		//
		for (unsigned c = 0; c < node->children.size(); c++) {
			auto const* child = node->children[c];

			if (Randomize::DBG) {
				std::cout << "DBG>   Consider node " << node->name << "'s child: " << child->name;
				std::cout << "; child " << c + 1 << "/" << node->children.size() << std::endl;
			}

			// child not visited yet; check recursively whether some cycle can be found
			//
			if (!child->visited && Randomize::checkGraphForCycles(child)) {

				if (Randomize::DBG) {
					std::cout << "DBG> Return from recursive check of node " << node->name << "'s child: " << child->name;
					std::cout << "; a cycle was found ..." << std::endl;
				}

				return true;
			}
			// child already visited; in case it has been visited during the current recursive call, then we found a cycle/backedge
			// http://www.geeksforgeeks.org/detect-cycle-in-a-graph/
			//
			else if (child->recursion) {

				if (Randomize::DBG) {
					std::cout << "DBG>   Cycle found; passed this node " << child->name << " already during recursion" << std::endl;
				}

				return true;
			}
			// child already visited, but not part of the recursion anymore; represents a transitive edge from one parent node
			// to some child node, which is fine
			// 
			else {
				if (Randomize::DBG) {
					std::cout << "DBG>   Cleared node " << node->name << "'s child: " << child->name;
					std::cout << "; child " << c + 1 << "/" << node->children.size() << std::endl;
				}
			}
		}
	}

	if (Randomize::DBG) {
		std::cout << "DBG> Check graph for cycles; DONE consider node: " << node->name << std::endl;
	}

	// after return from recursion; mark as "not anymore part of a recursion"
	node->recursion = false;

	return false;
}

void Randomize::initGraph(std::unordered_map<std::string, Data::Node>& nodes, Data const& data) {

	if (Randomize::DBG) {
		std::cout << "DBG> Initializing the graph ..." << std::endl;
	}

	// add global sink/source as nodes
	nodes.insert(std::make_pair(
				data.globalNodeNames.source,
				Data::Node(data.globalNodeNames.source)
			));
	nodes.insert(std::make_pair(
				data.globalNodeNames.sink,
				Data::Node(data.globalNodeNames.sink)
			));

	// sanity check whether nodes had been inserted / can be found
	if (Randomize::DBG) {

		if (nodes.find(data.globalNodeNames.source) == nodes.end()) {
			std::cout << "DBG>  Error: the following node was not inserted/found: \"" << data.globalNodeNames.source << "\"" << std::endl;
		}
		if (nodes.find(data.globalNodeNames.sink) == nodes.end()) {
			std::cout << "DBG>  Error: the following node was not inserted/found: \"" << data.globalNodeNames.sink << "\"" << std::endl;
		}
	}

	// add inputs as nodes
	// 
	for (auto const& input : data.netlist.inputs) {

		nodes.insert(std::make_pair(
					input,
					Data::Node(input)
				));

		// sanity check whether nodes had been inserted / can be found
		if (Randomize::DBG) {
			if (nodes.find(input) == nodes.end()) {
				std::cout << "DBG>  Error: the following node was not inserted/found: \"" << input << "\"" << std::endl;
			}
		}

		// also add new node for primary inputs as child to global source
		nodes[data.globalNodeNames.source].children.emplace_back( &(nodes[input]) );
	}

	// add outputs as nodes
	for (auto const& output : data.netlist.outputs) {

		nodes.insert(std::make_pair(
					output,
					Data::Node(output)
				));

		// sanity check whether nodes had been inserted / can be found
		if (Randomize::DBG) {
			if (nodes.find(output) == nodes.end()) {
				std::cout << "DBG>  Error: the following node was not inserted/found: \"" << output << "\"" << std::endl;
			}
		}

		// also add global sink as child for new node
		nodes[output].children.emplace_back( &(nodes[data.globalNodeNames.sink]) );
	}

	// add gates as nodes
	for (auto const& gate : data.netlist.gates) {

		nodes.insert(std::make_pair(
					gate.name,
					Data::Node(gate.name)
				));

		// sanity check whether nodes had been inserted / can be found
		if (Randomize::DBG) {
			if (nodes.find(gate.name) == nodes.end()) {
				std::cout << "DBG>  Error: the following node was not inserted/found: \"" << gate.name << "\"" << std::endl;
			}
		}
	}

	// add wires as nodes
	for (auto const& wire : data.netlist.wires) {

		nodes.insert(std::make_pair(
					wire,
					Data::Node(wire)
				));

		// sanity check whether nodes had been inserted / can be found
		if (Randomize::DBG) {
			if (nodes.find(wire) == nodes.end()) {
				std::cout << "DBG>  Error: the following node was not inserted/found: \"" << wire << "\"" << std::endl;
			}
		}
	}

	// connect graph based on connectivity of gates
	for (auto const& gate : data.netlist.gates) {
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

				//// memorize the node as parent for the gate's node
				//nodes.find(gate.name)->second.parents.emplace_back(
				//		&(node_iter->second)
				//	);
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

				//// memorize the gate's node as parent of the node
				//node_iter->second.parents.emplace_back(
				//		&(nodes.find(gate.name)->second)
				//	);
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
			//edges += node.parents.size();

			if (Randomize::DBG) {
				std::cout << "DBG>  " << node.name << ":" << std::endl;

				std::cout << "DBG>   Children [" << node.children.size() << "]:";
				for (auto const* child : node.children) {
					std::cout << " " << child->name;
				}
				std::cout << std::endl;

				//std::cout << "DBG>   Parents [" << node.parents.size() << "]:";
				//for (auto const* parent : node.parents) {
				//	std::cout << " " << parent->name;
				//}
				//std::cout << std::endl;
			}
		}

		std::cout << "DBG> Done" << std::endl;
		std::cout << "DBG>  Nodes: " << nodes.size() << std::endl;
		std::cout << "DBG>  Edges: " << edges << std::endl;
		std::cout << "DBG> " << std::endl;
	}
}
	
//bool Randomize::tackleF2F(std::unordered_set<std::string>& output_set, std::unordered_multimap<std::string, std::string>& input_map, std::unordered_multimap<std::string, std::string> const& inverted_input_map, std::unordered_map<std::string, Data::Node>& nodes, Data::AssignmentF2F& assignment, bool const& top_to_bottom) {
//
//	// tackle F2F outputs to inputs for the other tier until all are resolved
//	//
//	while (!output_set.empty()) {
//
//		if (Randomize::DBG) {
//			std::cout << "DBG> Remaining output_set [" << output_set.size() << "]:" << std::endl;
//
//			for (auto const& output : output_set) {
//				std::cout << "DBG>  " << output << std::endl;
//			}
//		}
//
//		// pick key/output randomly
//		auto const& output = std::next(output_set.begin(),
//				Randomize::rand(0, output_set.size())
//			);
//
//		if (Randomize::DBG) {
//			std::cout << "DBG> output: " << *output << std::endl;
//			std::cout << "DBG>  Remaining inputs -- in other tier! -- for that output [" << input_map.count(*output) << "]:" << std::endl;
//
//			auto iter = input_map.equal_range(*output);
//			for (auto input = iter.first; input != iter.second; ++input) {
//				std::cout << "DBG>   " << (*input).second << std::endl;
//			}
//		}
//
//		// pick assignment for that key/output randomly
//		//
//		// sanity check whether any assignment remains; may arise due to unlucky selection of assignments such that no input
//		// remains, or due to inappropriate definition for sinks in the mappings file
//		if (input_map.count(*output) == 0) {
//			return false;
//		}
//
//		// CANNOT USE REFERENCE HERE; otherwise the erasing procedure below, iterating over input_map, will mess up the value of
//		// input
//		auto input = std::next(input_map.equal_range(*output).first,
//				Randomize::rand(0, input_map.count(*output))
//			)->second;
//
//		if (Randomize::DBG) {
//			std::cout << "DBG>   Picking the following input port from the other tier as being driven by \"" << *output << "\": \"" << input << "\"" << std::endl;
//		}
//
//		// memorize the node related to the picked assignment as child for the output node
//		nodes.find(*output)->second.children.emplace_back(
//			&(nodes.find(input)->second)
//		);
//
//		if (Randomize::DBG) {
//			std::cout << "DBG>   Memorize the assignment in the graph..." << std::endl;
//			std::cout << "DBG>    Node " << nodes.find(*output)->second.name << "'s new set of children:" << std::endl;
//			for (auto const* child : nodes.find(*output)->second.children) {
//				std::cout << "DBG>     " << child->name << std::endl;
//			}
//		}
//
//
//		// also memorize the picked assignment
//		if (top_to_bottom) {
//			assignment.top_to_bottom.insert(std::make_pair(
//						*output,
//						input
//					));
//			}
//		else {
//			assignment.bottom_to_top.insert(std::make_pair(
//						*output,
//						input
//					));
//		}
//
//		// erase all other assignments from any output to this same input, to avoid multi-driver assignments
//		// 
//		// also, considering there are only 1-to-1 F2F mappings, erase all other possible mappings for the output/driver just
//		// assigned
//		//
//		auto outputs = inverted_input_map.equal_range(input);
//
//		for (auto output_iter = outputs.first; output_iter != outputs.second; ++output_iter) {
//
//			auto inputs = input_map.equal_range(output_iter->second);
//
//			for (auto input_iter = inputs.first; input_iter != inputs.second; ) {
//
//				if (input_iter->second == input) {
//
//					if (Randomize::DBG) {
//						std::cout << "DBG>   Removing the following other, previously possible driver/assignment for \"" << input << "\": ";
//						std::cout << "\"" << input_iter->first << "\" -> \"" << input_iter->second << "\"";
//						std::cout << std::endl;
//					}
//
//					input_iter = input_map.erase(input_iter);
//				}
//				else if (input_iter->first == *output) {
//
//					if (Randomize::DBG) {
//						std::cout << "DBG>   Removing the following other, previously possible driver/assignment for \"" << *output << "\": ";
//						std::cout << "\"" << input_iter->first << "\" -> \"" << input_iter->second << "\"";
//						std::cout << std::endl;
//					}
//
//					input_iter = input_map.erase(input_iter);
//				}
//				else {
//					if (Randomize::DBG) {
//						std::cout << "DBG>   NOT removing the following assignment: ";
//						std::cout << "\"" << input_iter->first << "\" -> \"" << input_iter->second << "\"";
//						std::cout << std::endl;
//					}
//
//					++input_iter;
//				}
//			}
//		}
//
//		// erase key/output from set of yet unassigned outputs
//		output_set.erase(output);
//	}
//
//	return true;
//}
