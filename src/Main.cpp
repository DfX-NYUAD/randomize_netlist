
#include "IO.hpp"
#include "Main.hpp"
#include "Data.hpp"

// TODO use regex for various parsing/matching tasks instead of find, find_first_of, find_last_of
//

// string identifiers for key nodes in graph
const std::string Data::STRINGS_GLOBAL_SOURCE = "GLOBAL_SOURCE";
const std::string Data::STRINGS_GLOBAL_SINK = "GLOBAL_SINK";
const std::string Data::STRINGS_GLOBAL_DUMMY_PI = "GLOBAL_DUMMY_PI";
// string identifiers for netlist files 
const std::string Data::STRINGS_DEFAULT_NETLIST = "netlist.v";
// US locale for outputs; mainly for "," separator for multiples of thousand
const std::string Data::LOCALE = "en_US.utf8";

// TODO could be different for other libaries; implement more generic pattern
//
// regexp for cell type along with its driving strength
//
// regexp explained: "(.)+" one or more arbitrary character(s); "(_)?" zero or one occurrence of '_'; "(X)" one occurrence of 'X'; "[0-9]+"
// one or more numerical character(s)
const std::regex IO::REGEX_CELL_DRIVING_STRENGTH = std::regex("(.)+(_)?(X)[0-9]+");

int main (int argc, char** argv) {
	Data data;
	double HD = 0.0;
	// threads
	std::vector<std::thread> threads;
	std::mutex m;

	std::cout << std::endl;
	std::cout << "HD evaluation" << std::endl;
	std::cout << "-------------" << std::endl << std::endl;

	// track start time
	auto start_time = std::chrono::system_clock::now();

	// try to set locale
	try {
		std::cout.imbue(std::locale(std::string(Data::LOCALE).c_str()));
	}
	catch (std::runtime_error) {
		std::cout << "IO>" << std::endl;
		std::cout << "IO> Warning: failed to set locale \"" << Data::LOCALE << "\" ..." << std::endl;
		std::cout << "IO>" << std::endl;
	}

	// parse program parameters and test files
	IO::parseParametersFiles(data, argc, argv);

	// parse cells
	IO::parseCells(data);

	// parse input netlist
	IO::parseNetlist(data.cells, data.netlist, data.files.in_netlist);

	// parse golden netlist
	//
	IO::parseNetlist(data.cells, data.golden_netlist, data.files.golden_netlist);

	// sanity checks on the ports of the two netlists
	if ((data.golden_netlist.inputs.size() != data.netlist.inputs.size()) || (data.golden_netlist.outputs.size() != data.netlist.outputs.size())) {
		std::cout << "IO> Error: the golden netlist and the input netlist have different number of ports; check your files" << std::endl;
		exit(1);
	}
	// compare actual ports
	else {
		for (unsigned i = 0; i < data.golden_netlist.inputs.size(); i++) {

			if (data.golden_netlist.inputs[i] != data.netlist.inputs[i]) {
				std::cout << "IO> Error: the golden netlist and the input netlist have some different input ports; check your files" << std::endl;
				exit(1);
			}
		}
		for (unsigned i = 0; i < data.golden_netlist.outputs.size(); i++) {

			if (data.golden_netlist.outputs[i] != data.netlist.outputs[i]) {
				std::cout << "IO> Error: the golden netlist and the input netlist have some different output ports; check your files" << std::endl;
				exit(1);
			}
		}
	}

	// init the original graphs
	//
	Main::initGraph(data.netlist);
	Main::initGraph(data.golden_netlist);

	// sanity check for cycles on original graphs
	if (Main::checkGraphForCycles( &(data.netlist.nodes[Data::STRINGS_GLOBAL_SOURCE])) ) {
		std::cout << "IO> Error: input netlist \"" << data.files.in_netlist << "\" contains a cycle; exiting ..." << std::endl;
		exit(1);
	}
	if (Main::checkGraphForCycles( &(data.golden_netlist.nodes[Data::STRINGS_GLOBAL_SOURCE])) ) {
		std::cout << "IO> Error: golden netlist \"" << data.files.golden_netlist << "\" contains a cycle; exiting ..." << std::endl;
		exit(1);
	}

	// for original graphs, determine the order of all graph nodes, from global source to global sink;
	// required for subsequent HD evaluation, which is based on propagation of Boolean values from inputs to outputs
	//
	Main::determGraphOrder(data.netlist);
	Main::determGraphOrder(data.golden_netlist);

	std::cout << "HD> Evaluating HD of netlist against golden netlist ..." << std::endl;

	// evaluate HD in parallel
	threads.reserve(data.parameters.threads);
	unsigned const iterations_per_thread = std::ceil(static_cast<double>(data.parameters.HD_sampling_iterations) / data.parameters.threads);
	double HD_threads = 0.0;

	for (unsigned t = 0; t < data.parameters.threads; t++) {

		// notes
		// 1) pass nodes for original netlist and for local graph as copies, since the threads have to work on separate graph data 
		// 2) HD will be summed up in the threads (using the mutex)
		threads.emplace_back( std::thread(Main::evaluateHD,
					data.golden_netlist.nodes,
					data.netlist.nodes,
					std::ref(data.golden_netlist.topology),
					std::ref(data.netlist.topology),
					std::ref(data.golden_netlist.inputs),
					std::ref(data.golden_netlist.outputs),
					std::ref(iterations_per_thread),
					std::ref(HD_threads),
					std::ref(m),
					std::ref(data.parameters.lazy_Boolean_evaluation))
				);
	}

	// join threads; the main thread execution will pause until all threads are done
	for (std::thread& t : threads) {
		t.join();
	}
	// once done, clean up all threads
	threads.clear();

	// normalize HD summed up across all threads
	HD = (HD_threads / data.parameters.threads);
	std::cout << "HD>  HD: " << HD << std::endl;

	// also log runtime
	std::chrono::duration<double> runtime = std::chrono::system_clock::now() - start_time;
	std::cout << "Main> Overall runtime: " << runtime.count() << " seconds" << std::endl;
	std::cout << "Main>" << std::endl;
};

void Main::evaluateHD(
	std::unordered_map<std::string, Data::Node> orig_nodes_copy,
	std::unordered_map<std::string, Data::Node> nodes_copy,
	std::vector< std::vector<Data::Node const*> > const& orig_nodes_topology,
	std::vector< std::vector<Data::Node const*> > const& nodes_copy_topology,
	std::vector<std::string> const& inputs,
	std::vector<std::string> const& outputs,
	unsigned const& iterations,
	double& HD_threads,
	std::mutex& m,
	bool const& lazy_evaluation
) {
	// HD for this call/thread
	double HD_local;
	// HD for each iteration within this call/thread
	double HD_curr_iter;

	if (Main::DBG) {
		std::cout << "DBG> Evaluate HD ..." << std::endl;
	}

	// run multiple iterations
	//
	HD_local = 0.0;
	for (unsigned it = 1; it <= iterations; it++) {

		if (Main::DBG) {
			std::cout << "DBG> Iteration: " << it << "/" << iterations << std::endl;
		}

		// first, randomly assign the same bits to the input nodes in the original and the current graph
		//
		for (auto const& input : inputs) {
			orig_nodes_copy[input].bit = nodes_copy[input].bit = static_cast<bool>(Main::rand(0, 2));

			if (Main::DBG_VERBOSE) {
				std::cout << "DBG>  Assign the following random bit to PI \"" << input << "\": " << orig_nodes_copy[input].bit << std::endl;
			}
		}

		// second, evaluate all bit values for both graphs
		//
		if (Main::DBG) {
			std::cout << "DBG> Evaluate original graph/netlist" << std::endl;
		}

		Main::evaluateHDHelper(orig_nodes_copy, orig_nodes_topology, lazy_evaluation);

		if (Main::DBG) {
			std::cout << "DBG> Done" << std::endl;
			std::cout << "DBG> Evaluate current graph/netlist" << std::endl;
		}

		Main::evaluateHDHelper(nodes_copy, nodes_copy_topology, lazy_evaluation);

		if (Main::DBG) {
			std::cout << "DBG> Done" << std::endl;
		}

		// third, evaluate HD
		//
		HD_curr_iter = 0.0;
		for (auto const& output : outputs) {

			if (orig_nodes_copy[output].bit != nodes_copy[output].bit) {
				HD_curr_iter++;
			}

			// dbg log of output bits
			if (Main::DBG_VERBOSE) {

				std::cout << "DBG>  Original graph, PO \"" << output << "\": " << orig_nodes_copy[output].bit << std::endl;
				std::cout << "DBG>  Current graph, PO \"" << output << "\": " << nodes_copy[output].bit << std::endl;
			}
		}
		// HD of this current iteration
		HD_curr_iter /= outputs.size();

		if (Main::DBG) {
			std::cout << "DBG> HD for current iteration: " << HD_curr_iter << std::endl;
		}

		// update HD of this thread 
		HD_local += HD_curr_iter;
	}

	// after running all iterations
	//
	// normalize HD
	HD_local /= iterations;

	// sum up HD across all threads, using the mutex
	m.lock();

	HD_threads += HD_local;

	//// also log the current, local HD
	//std::cout << "Main>  HD for current thread: " << HD_local << std::endl;

	m.unlock();

	if (Main::DBG) {
		std::cout << "DBG> HD for this thread: " << HD_local << std::endl;
		std::cout << "DBG> Evaluate HD done" << std::endl;
	}
}

void Main::evaluateHDHelper(std::unordered_map<std::string, Data::Node>& nodes, std::vector< std::vector<Data::Node const*> > const& nodes_topology, bool const& lazy_evaluation) {
	bool output_found;
	std::string function;
	std::string::size_type input_pin_pos;

	// index by index, propagate these random inputs through the whole netlist/graph; note that walking over linked graph could not
	// guarantee that the Boolean value of the parents is already computed, that can only be achieved when considering the indices
	//
	// index 0: global source; index 1: PIs; index 2: first set of gates after the PIs; index 3: first set of wires after the first
	// gates; and so on
	//
	// consider only wire/PO nodes; walk in steps of 2 starting from the index 3
	for (int index = 3; index < nodes[Data::STRINGS_GLOBAL_SINK].index; index += 2) {

		if (Main::DBG_VERBOSE) {
			std::cout << "DBG>  Evaluate Boolean assignments -- current graph index: " << index << std::endl;
		}

		for (auto const* node_ptr : nodes_topology[index]) {

			// work directly on the node reference, not the node pointer from the topology
			//
			auto& node = nodes[node_ptr->name];

			// sanity check for nodes with current index
			if (node.index != index) {
				std::cout << "Main> Error -- the following node has an unexpected index: \"" << node.name << "\"" << std::endl;
			}

			// sanity check to consider only wire/PO nodes
			if (!(node.type == Data::Node::Type::Wire || node.type == Data::Node::Type::PO)) {
				std::cout << "Main> Error -- the following node is neither a wire node nor a PO node: \"" << node.name << "\"" << std::endl;
			}

			if (Main::DBG_VERBOSE) {
				if (node.type == Data::Node::Type::Wire) {
					std::cout << "DBG>   Current wire node: " << node.name << std::endl;
				} else {
					std::cout << "DBG>   Current PO node: " << node.name << std::endl;
				}
			}

			// evaluate Boolean value for wire/PO from parent (driver)
			//
			// sanity check for single driver
			if (node.parents.size() != 1) {
				std::cout << "Main> Error -- the following wire/PO node has more than one parent: \"" << node.name << "\"" << std::endl;
			}

			// parent is gate
			if (node.parents[0]->type == Data::Node::Type::Gate) {

				auto const* gate = node.parents[0]->gate;

				if (Main::DBG_VERBOSE) {
					std::cout << "DBG>    Matching driver: " << gate->name << std::endl;
				}

				// check among all outputs of the gate
				output_found = false;
				for (auto const& output : gate->outputs) {

					if (output.second == node.name) {

						if (Main::DBG_VERBOSE) {
							std::cout << "DBG>     Driver output: " << output.first << std::endl;
						}

						// sanity check for multiple driver
						if (output_found) {
							std::cout << "Main> Error -- the following wire/PO node has more than one driver: \"" << node.name << "\"" << std::endl;
						}
						output_found = true;

						// copy Boolean function string
						function = gate->cell->functions.find(output.first)->second;

						if (Main::DBG_VERBOSE) {
							std::cout << "DBG>     Output function: " << function << std::endl;
						}

						// revise function string with bit values of all the wires/PIs connected to the gate's
						// inputs
						for (auto const& input : gate->inputs) {

							// input pin may occur multiple times, replace all occurrence
							//
							input_pin_pos = 0;
							while ((input_pin_pos = function.find(input.first, input_pin_pos)) != std::string::npos) {

								//function.replace(
								//		// replace input pin word
								//		input_pin_pos, input.first.length(),
								//		// Boolean value of related node; the node name is captured in the
								//		// input of the gate
								//		std::to_string(nodes[input.second].bit)
								//	);
								//
								// avoid repetitive use of std::to_string, just use hard-coded strings
								if (nodes[input.second].bit) {
									function.replace(
											// replace input pin word
											input_pin_pos, input.first.length(),
											// Boolean value of related node
											"1"
										);
								}
								else {
									function.replace(
											// replace input pin word
											input_pin_pos, input.first.length(),
											// Boolean value of related node
											"0"
										);
								}

								// update start position -- after replacing the input pin with its one-bit
								// assignment, the search should continue right after that bit
								input_pin_pos++;
							}

							if (Main::DBG_VERBOSE) {
								std::cout << "DBG>      Current input: " << input.first << std::endl;
								std::cout << "DBG>       Driven by the following node: " << input.second << std::endl;
								std::cout << "DBG>       Revised output function: " << function << std::endl;
							}
						}

						// evaluate Boolean value and assign to node
						node.bit = Main::evaluateString(function, lazy_evaluation);

						if (Main::DBG_VERBOSE) {
							std::cout << "DBG>      Final function value: " << node.bit << std::endl;
						}
					}
				}
				// log in case no driver was found
				if (!output_found) {
					std::cout << "Main> Error -- the following wire/PO node has no driver: \"" << node.name << "\"" << std::endl;
				}
			}
			// parent is PI -- just copy Boolean value
			else if (node.parents[0]->type == Data::Node::Type::PI) {
				node.bit = node.parents[0]->bit;
			}
			// parent is neither PI nor gate; error
			else {
				std::cout << "Main> Error -- the following node has an incompatible parent: \"" << node.name << "\"" << std::endl;
			}
		}
	}
}

bool Main::evaluateString(std::string function, bool const& lazy_evaluation) {
	std::string::size_type pos_begin, pos_end;
	bool substring_result;
	int inverted_bits;

	if (Main::DBG_VERBOSE) {
		std::cout << "DBG> Boolean (sub-)string to evaluate: \"" << function << "\"" << std::endl;
	}

	// recursively evaluate substrings within parentheses, if any
	//
	// init the pos_begin with the last character's position
	pos_begin = function.length() - 1;
	// the last '(' should not come later than two characters before the last character, e.g., "!(0)"
	while ((pos_begin = function.find_last_of('(', pos_begin - 2)) != std::string::npos) {

		// the first ')' should not come before two characters after the '('
		pos_end = function.find_first_of(')', pos_begin + 2);

		if (Main::DBG_VERBOSE) {
			std::cout << "DBG>  pos_begin: " << pos_begin << std::endl;
			std::cout << "DBG>  pos_end: " << pos_end << std::endl;
			//std::cout << "DBG>   Substring before recursion: " << function.substr(pos_begin + 1, pos_end - pos_begin - 1) << std::endl;
		}

		substring_result = Main::evaluateString(function.substr(pos_begin + 1, pos_end - pos_begin - 1), lazy_evaluation);

		if (Main::DBG_VERBOSE) {
			std::cout << "DBG>  Boolean result after recursion: " << substring_result << std::endl;
		}

		// after returning from recursion, replace the substring with its Boolean value
		//
		//function.replace(pos_begin, pos_end - pos_begin + 1, std::to_string(substring_result));
		//
		// avoid repetitive use of std::to_string, just use hard-coded strings
		if (substring_result) {
			function.replace(pos_begin, pos_end - pos_begin + 1, "1");
		}
		else {
			function.replace(pos_begin, pos_end - pos_begin + 1, "0");
		}

		if (Main::DBG_VERBOSE) {
			std::cout << "DBG>  Modified Boolean (sub-)string after evaluation: \"" << function << "\"" << std::endl;
		}

		// after replacing the substring with its Boolean value, reset pos_begin to the now last character's position
		pos_begin = function.length() - 1;
	}

	// evaluate the substring
	//
	if (function.length() <= 2) {

		if (function == "1") {
			return true;
		}
		else if (function == "0") {
			return false;
		}
		else if (function == "!1") {
			return false;
		}
		else if (function == "!0") {
			return true;
		}
		// "~" notation not expected as of now; also not considered further below
		//
		//else if (function == "~1") {
		//	return false;
		//}
		//else if (function == "~0") {
		//	return true;
		//}
		else {
			std::cout << "Main> Error -- the following Boolean (sub-)string could not be parsed correctly: \"" << function << "\"" << std::endl;
			return false;
		}
	}
	// longer, more complex substring; should be "A OPERATOR B"
	//
	// implemented in look-up table manner, more efficient than again decomposing string and handling sub-strings separately
	else {
		// "|", OR 
		if (function.find('|') != std::string::npos) {

			// determine the number of inverted bits
			inverted_bits = std::count(function.begin(), function.end(), '!');

			switch (inverted_bits) {

				// regular bits, none inverted
				case 0:
					if (function == "0 | 0") {
						return false;
					}
					else if (lazy_evaluation) {
						return true;
					}
					else {
						if (function == "0 | 1") {
							return true;
						}
						else if (function == "1 | 0") {
							return true;
						}
						else if (function == "1 | 1") {
							return true;
						}
						// OR operation with some error
						else {
							std::cout << "Main> Error -- the following Boolean (sub-)string could not be parsed correctly: \"" << function << "\"" << std::endl;
							return false;
						}
					}

				// one bit inverted
				case 1:
					if (function == "!1 | 0") {
						return false;
					}
					else if (function == "0 | !1") {
						return false;
					}
					else if (lazy_evaluation) {
						return true;
					}
					else {
						// A inverted
						if (function == "!0 | 0") {
							return true;
						}
						else if (function == "!0 | 1") {
							return true;
						}
						else if (function == "!1 | 1") {
							return true;
						}
						// B inverted
						else if (function == "0 | !0") {
							return true;
						}
						else if (function == "1 | !0") {
							return true;
						}
						else if (function == "1 | !1") {
							return true;
						}
						// OR operation with some error
						else {
							std::cout << "Main> Error -- the following Boolean (sub-)string could not be parsed correctly: \"" << function << "\"" << std::endl;
							return false;
						}
					}

				// both bits inverted
				case 2:
					if (function == "!1 | !1") {
						return false;
					}
					else if (lazy_evaluation) {
						return true;
					}
					else {
						if (function == "!0 | !0") {
							return true;
						}
						else if (function == "!0 | !1") {
							return true;
						}
						else if (function == "!1 | !0") {
							return true;
						}
						// OR operation with some error
						else {
							std::cout << "Main> Error -- the following Boolean (sub-)string could not be parsed correctly: \"" << function << "\"" << std::endl;
							return false;
						}
					}

				// OR operation with some bit inversion error
				default:
					std::cout << "Main> Error -- the following Boolean (sub-)string could not be parsed correctly: \"" << function << "\"" << std::endl;
					return false;
			}
		}
		// "&", AND
		else if (function.find('&') != std::string::npos) {

			// determine the number of inverted bits
			inverted_bits = std::count(function.begin(), function.end(), '!');

			switch (inverted_bits) {

				// regular bits, none inverted
				case 0:
					if (function == "1 & 1") {
						return true;
					}
					else if (lazy_evaluation) {
						return false;
					}
					else {
						if (function == "0 & 0") {
							return false;
						}
						else if (function == "0 & 1") {
							return false;
						}
						else if (function == "1 & 0") {
							return false;
						}
						// AND operation with some error
						else {
							std::cout << "Main> Error -- the following Boolean (sub-)string could not be parsed correctly: \"" << function << "\"" << std::endl;
							return false;
						}
					}

				// one bit inverted
				case 1:
					if (function == "!0 & 1") {
						return true;
					}
					else if (function == "1 & !0") {
						return true;
					}
					else if (lazy_evaluation) {
						return false;
					}
					else {
						// A inverted
						if (function == "!0 & 0") {
							return false;
						}
						else if (function == "!1 & 0") {
							return false;
						}
						else if (function == "!1 & 1") {
							return false;
						}
						// B inverted
						else if (function == "0 & !0") {
							return false;
						}
						else if (function == "0 & !1") {
							return false;
						}
						else if (function == "1 & !1") {
							return false;
						}
						// AND operation with some error
						else {
							std::cout << "Main> Error -- the following Boolean (sub-)string could not be parsed correctly: \"" << function << "\"" << std::endl;
							return false;
						}
					}

				// both bits inverted
				case 2:
					// A and B inverted
					if (function == "!0 & !0") {
						return true;
					}
					else if (lazy_evaluation) {
						return false;
					}
					else {
						if (function == "!0 & !1") {
							return false;
						}
						else if (function == "!1 & !0") {
							return false;
						}
						else if (function == "!1 & !1") {
							return false;
						}
						// AND operation with some error
						else {
							std::cout << "Main> Error -- the following Boolean (sub-)string could not be parsed correctly: \"" << function << "\"" << std::endl;
							return false;
						}
					}

				// AND operation with some bit inversion error
				default:
					std::cout << "Main> Error -- the following Boolean (sub-)string could not be parsed correctly: \"" << function << "\"" << std::endl;
					return false;
			}
		}
		// "^", XOR
		else if (function.find('^') != std::string::npos) {

			// determine the number of inverted bits
			inverted_bits = std::count(function.begin(), function.end(), '!');

			switch (inverted_bits) {

				// regular bits, none inverted
				case 0:
					if (function == "0 ^ 0") {
						return false;
					}
					else if (function == "0 ^ 1") {
						return true;
					}
					else if (function == "1 ^ 0") {
						return true;
					}
					else if (function == "1 ^ 1") {
						return false;
					}
					// XOR operation with some error
					else {
						std::cout << "Main> Error -- the following Boolean (sub-)string could not be parsed correctly: \"" << function << "\"" << std::endl;
						return false;
					}

				// one bit inverted
				case 1:
					// A inverted
					if (function == "!0 ^ 0") {
						return true;
					}
					else if (function == "!0 ^ 1") {
						return false;
					}
					else if (function == "!1 ^ 0") {
						return false;
					}
					else if (function == "!1 ^ 1") {
						return true;
					}
					// B inverted
					else if (function == "0 ^ !0") {
						return true;
					}
					else if (function == "0 ^ !1") {
						return false;
					}
					else if (function == "1 ^ !0") {
						return false;
					}
					else if (function == "1 ^ !1") {
						return true;
					}
					// XOR operation with some error
					else {
						std::cout << "Main> Error -- the following Boolean (sub-)string could not be parsed correctly: \"" << function << "\"" << std::endl;
						return false;
					}

				// both bits inverted
				case 2:
					// A and B inverted
					if (function == "!0 ^ !0") {
						return false;
					}
					else if (function == "!0 ^ !1") {
						return true;
					}
					else if (function == "!1 ^ !0") {
						return true;
					}
					else if (function == "!1 ^ !1") {
						return false;
					}
					// XOR operation with some error
					else {
						std::cout << "Main> Error -- the following Boolean (sub-)string could not be parsed correctly: \"" << function << "\"" << std::endl;
						return false;
					}

				// XOR operation with some bit inversion error
				default:
					std::cout << "Main> Error -- the following Boolean (sub-)string could not be parsed correctly: \"" << function << "\"" << std::endl;
					return false;
			}
		}
		// unknown operator or other error
		else {
			std::cout << "Main> Error -- the following Boolean (sub-)string could not be parsed correctly: \"" << function << "\"" << std::endl;
			return false;
		}
	}
}

bool Main::checkGraphForCycles(Data::Node const* node) {

	if (Main::DBG_VERBOSE) {
		std::cout << "DBG> Check graph for cycles; consider node: " << node->name << std::endl;
	}

	// node not visited/checked yet
	//
	//
	if (!node->visited) {

		if (Main::DBG_VERBOSE) {
			std::cout << "DBG>  Proceed with node \"" << node->name <<"\";";
			std::cout << " not visited yet; mark as visited and as part of recursion" << std::endl;
		}

		// mark as visited/checked, and also mark as part of this recursion
		node->visited = node->recursion = true;

		// now, check all children in depth-first manner
		//
		for (unsigned c = 0; c < node->children.size(); c++) {
			auto const* child = node->children[c];

			if (Main::DBG_VERBOSE) {
				std::cout << "DBG>   Consider node \"" << node->name << "\"'s child: \"" << child->name << "\"";
				std::cout << "; child " << c + 1 << "/" << node->children.size() << std::endl;
			}

			// child not visited yet; check recursively whether some cycle can be found
			//
			if (!child->visited && Main::checkGraphForCycles(child)) {

				if (Main::DBG_VERBOSE) {
					std::cout << "DBG> Return from recursive check of node \"" << node->name << "\"'s child: \"" << child->name << "\"";
					std::cout << "; a cycle was found ..." << std::endl;
				}

				return true;
			}
			// child already visited; in case it has been visited during the current recursive call, then we found a cycle/backedge
			// http://www.geeksforgeeks.org/detect-cycle-in-a-graph/
			//
			else if (child->recursion) {

				if (Main::DBG_VERBOSE) {
					std::cout << "DBG>   Cycle found; passed this node \"" << child->name << "\" already during recursion" << std::endl;
				}

				return true;
			}
			// child already visited, but not part of the recursion anymore; represents a transitive edge from one parent node
			// to some child node, which is fine
			// 
			else {
				if (Main::DBG_VERBOSE) {
					std::cout << "DBG>   Cleared node \"" << node->name << "\"'s child: " << child->name;
					std::cout << "; child " << c + 1 << "/" << node->children.size() << std::endl;
				}
			}
		}
	}

	if (Main::DBG_VERBOSE) {
		std::cout << "DBG> Check graph for cycles; DONE consider node: \"" << node->name << "\"" << std::endl;
	}

	// after return from recursion; mark as "not anymore part of a recursion"
	node->recursion = false;

	return false;
}

void Main::determGraphOrder(Data::Netlist const& netlist) {

	if (Main::DBG_VERBOSE) {
		std::cout << "DBG> Determining the graph order, in ascending order from global source to global sink ..." << std::endl;
	}

	// global source has index 0 by definition
	netlist.nodes[Data::STRINGS_GLOBAL_SOURCE].index = 0;

	// recursively handle all node, starting from global source
	Main::determGraphOrderRec(&(netlist.nodes[Data::STRINGS_GLOBAL_SOURCE]));

	// also memorize topology/indices separately
	//
	// reset and allocate topology vector
	netlist.topology.clear();
	for (int i = 0; i <= netlist.nodes[Data::STRINGS_GLOBAL_SINK].index; i++) {
		netlist.topology.emplace_back( std::vector<Data::Node const*>() );
	}
	// now memorize all nodes encoded by their index
	for (auto const& node_iter : netlist.nodes) {
		auto const* node = &(node_iter.second);

		// sanity check for bounds
		if ((node->index >= 0) && (node->index < static_cast<int>(netlist.topology.size()))) {
			netlist.topology[node->index].emplace_back(node);
		}
	}

	// dbg log
	//
	if (Main::DBG_VERBOSE) {

		std::cout << "DBG> Print topology of netlist graph: " << std::endl;

		for (unsigned i = 0; i < netlist.topology.size(); i++) {
			for (auto const* node : netlist.topology[i]) {

				if (Main::DBG_VERBOSE) {
					std::cout << "DBG>  \"" << node->name << "\":" << std::endl;

					std::cout << "DBG>   Index: " << node->index << std::endl;

					switch (node->type) {
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

					std::cout << "DBG>   Children [" << node->children.size() << "]:";
					for (auto const* child : node->children) {
						std::cout << " \"" << child->name << "\"";
						std::cout << "; Index: " << child->index;
					}
					std::cout << std::endl;

					std::cout << "DBG>   Parents [" << node->parents.size() << "]:";
					for (auto const* parent : node->parents) {
						std::cout << " \"" << parent->name << "\"";
						std::cout << "; Index: " << parent->index;
					}
					std::cout << std::endl;
				}
			}
		}

		std::cout << "DBG> Done" << std::endl;
		std::cout << "DBG> " << std::endl;
	}
}

void Main::determGraphOrderRec(Data::Node const* node) {

	// derive index for current node from maximum among parents
	//
	for (auto const* parent : node->parents) {
		node->index = std::max(node->index, parent->index + 1);
	}
	
	if (Main::DBG_VERBOSE) {

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

			if (Main::DBG_VERBOSE) {
				std::cout << "DBG>  Depth-first traversal of graph; continue with child of node: " << node->name << std::endl;
			}

			Main::determGraphOrderRec(child);
		}
	}
	
	if (Main::DBG_VERBOSE) {

		std::cout << "DBG>  Depth-first traversal of graph; done for now with node: " << node->name << std::endl;
	}
}

void Main::initGraph(Data::Netlist& netlist) {

	if (Main::DBG) {
		std::cout << "DBG> Initializing the graph ..." << std::endl;
	}

	// it is important to clear any previous graph data
	netlist.nodes.clear();

	// add global sink/source as nodes
	netlist.nodes.insert(std::make_pair(
				Data::STRINGS_GLOBAL_SOURCE,
				Data::Node(Data::STRINGS_GLOBAL_SOURCE, Data::Node::Type::Dummy)
			));
	netlist.nodes.insert(std::make_pair(
				Data::STRINGS_GLOBAL_SINK,
				Data::Node(Data::STRINGS_GLOBAL_SINK, Data::Node::Type::Dummy)
			));

	// also add dummy PI into the graph -- this PI is used for gates without any input, e.g., TIE cells, to be able to handle them in
	// the graph structure as well
	netlist.nodes.insert(std::make_pair(
				Data::STRINGS_GLOBAL_DUMMY_PI,
				Data::Node(Data::STRINGS_GLOBAL_DUMMY_PI, Data::Node::Type::PI)
			));

	// also add dummy PI as child to global source
	netlist.nodes[Data::STRINGS_GLOBAL_SOURCE].children.emplace_back( &(netlist.nodes[Data::STRINGS_GLOBAL_DUMMY_PI]) );
	// also add global source as parent for new node
	netlist.nodes[Data::STRINGS_GLOBAL_DUMMY_PI].parents.emplace_back( &netlist.nodes[Data::STRINGS_GLOBAL_SOURCE] );

	// add inputs as nodes
	for (auto const& input : netlist.inputs) {

		// sanity check whether some node w/ the same name has been inserted before
		if (netlist.nodes.find(input) != netlist.nodes.end()) {
			std::cout << "Main>  Error: the following node, to be inserted as PI, has been previously inserted: \"" << input << "\"" << std::endl;
		}

		netlist.nodes.insert(std::make_pair(
					input,
					Data::Node(input, Data::Node::Type::PI)
				));

		// sanity check whether nodes had been inserted / can be found
		if (Main::DBG) {
			if (netlist.nodes.find(input) == netlist.nodes.end()) {
				std::cout << "DBG>  Error: the following node was not inserted/found: \"" << input << "\"" << std::endl;
			}
		}

		// also add new node for primary inputs as child to global source
		netlist.nodes[Data::STRINGS_GLOBAL_SOURCE].children.emplace_back( &(netlist.nodes[input]) );
		// also add global source as parent for new node
		netlist.nodes[input].parents.emplace_back( &netlist.nodes[Data::STRINGS_GLOBAL_SOURCE] );
	}

	// add outputs as nodes
	for (auto const& output : netlist.outputs) {

		// sanity check whether some node w/ the same name has been inserted before
		if (netlist.nodes.find(output) != netlist.nodes.end()) {
			std::cout << "Main>  Error: the following node, to be inserted as PO, has been previously inserted: \"" << output << "\"" << std::endl;
		}

		netlist.nodes.insert(std::make_pair(
					output,
					Data::Node(output, Data::Node::Type::PO)
				));

		// sanity check whether nodes had been inserted / can be found
		if (Main::DBG) {
			if (netlist.nodes.find(output) == netlist.nodes.end()) {
				std::cout << "DBG>  Error: the following node was not inserted/found: \"" << output << "\"" << std::endl;
			}
		}

		// also add global sink as child for new node
		netlist.nodes[output].children.emplace_back( &(netlist.nodes[Data::STRINGS_GLOBAL_SINK]) );
		// also add new node as parent for global sink
		netlist.nodes[Data::STRINGS_GLOBAL_SINK].parents.emplace_back( &(netlist.nodes[output]) );
	}

	// add gates as nodes
	for (auto const& gate : netlist.gates) {

		// sanity check whether some node w/ the same name has been inserted before
		if (netlist.nodes.find(gate.name) != netlist.nodes.end()) {
			std::cout << "Main>  Error: the following node, to be inserted as gate, has been previously inserted: \"" << gate.name << "\"" << std::endl;
		}

		netlist.nodes.insert(std::make_pair(
					gate.name,
					Data::Node(gate.name, Data::Node::Type::Gate)
				));

		// sanity check whether nodes had been inserted / can be found
		if (Main::DBG) {
			if (netlist.nodes.find(gate.name) == netlist.nodes.end()) {
				std::cout << "DBG>  Error: the following node was not inserted/found: \"" << gate.name << "\"" << std::endl;
			}
		}

		// also memorize pointer to gate in node
		netlist.nodes[gate.name].gate = &gate;
	}

	// add wires as nodes
	for (auto const& wire : netlist.wires) {

		// sanity check whether some node w/ the same name has been inserted before
		if (netlist.nodes.find(wire) != netlist.nodes.end()) {
			std::cout << "Main>  Error: the following node, to be inserted as wire, has been previously inserted: \"" << wire << "\"" << std::endl;
		}

		netlist.nodes.insert(std::make_pair(
					wire,
					Data::Node(wire, Data::Node::Type::Wire)
				));

		// sanity check whether nodes had been inserted / can be found
		if (Main::DBG) {
			if (netlist.nodes.find(wire) == netlist.nodes.end()) {
				std::cout << "DBG>  Error: the following node was not inserted/found: \"" << wire << "\"" << std::endl;
			}
		}
	}

	// connect graph based on connectivity of gates
	for (auto const& gate : netlist.gates) {
		auto& gate_node = netlist.nodes[gate.name];

		if (Main::DBG_VERBOSE) {
			std::cout << "DBG>  Gate: \"" << gate.name << "\"" << std::endl;
		}

		// check all the inputs of the gate
		//
		// gate.inputs: cell pin is key, pin/net name is value
		for (auto const& input_iter : gate.inputs) {
			auto node_iter = netlist.nodes.find(input_iter.second);

			if (Main::DBG_VERBOSE) {
				std::cout << "DBG>   Input pin: \"" << input_iter.first << "\"" << std::endl;
			}

			// there's a node matching the input of the gate
			if (node_iter != netlist.nodes.end()) {

				if (Main::DBG_VERBOSE) {
					std::cout << "DBG>    Connected node: \"" << node_iter->first << "\"" << std::endl;
				}

				// memorize the gate's node as child of the node
				node_iter->second.children.emplace_back(
						&(gate_node)
					);

				// memorize the node as parent for the gate's node
				gate_node.parents.emplace_back(
						&(node_iter->second)
					);
			}
			// there's no node matching; that means the pin is dangling
			else {
				std::cout << "IO> Warning: the input pin \"" << input_iter.first << "\" of gate \"" << gate.name << "\" is dangling;" << std::endl;
				std::cout << "IO>  the following wire/pin should have been connected, but was not found in the graph: \"" << input_iter.second << "\"" << std::endl;
			}
		}
		// some gates, e.g., TIE cells, don't have inputs
		if (gate.inputs.empty()) {

			// to enable handling them in the graph, connect them to the dummy PI
			//
			netlist.nodes[Data::STRINGS_GLOBAL_DUMMY_PI].children.emplace_back( &(gate_node) );
			gate_node.parents.emplace_back( &netlist.nodes[Data::STRINGS_GLOBAL_DUMMY_PI] );
		}

		// check all the outputs of the gate
		//
		// gate.outputs: cell pin is key, pin/net name is value
		for (auto const& output_iter : gate.outputs) {
			auto const& node_iter = netlist.nodes.find(output_iter.second);

			if (Main::DBG_VERBOSE) {
				std::cout << "DBG>   Output pin: \"" << output_iter.first << "\"" << std::endl;
			}

			// there's a node matching the output of the gate
			if (node_iter != netlist.nodes.end()) {

				if (Main::DBG_VERBOSE) {
					std::cout << "DBG>    Connected node: \"" << node_iter->first << "\"" << std::endl;
				}

				// memorize the node as child for the gate's node
				gate_node.children.emplace_back(
						&(node_iter->second)
					);

				// memorize the gate's node as parent of the node
				node_iter->second.parents.emplace_back(
						&(gate_node)
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
	if (Main::DBG) {

		std::cout << "DBG> Print netlist graph: " << std::endl;

		// count all edges
		int edges = 0;
		for (auto const& node_iter : netlist.nodes) {
			auto const& node = node_iter.second;

			edges += node.children.size();
			edges += node.parents.size();

			if (Main::DBG) {
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
		std::cout << "DBG>  Nodes: " << netlist.nodes.size() << std::endl;
		std::cout << "DBG>  Edges: " << edges << std::endl;
		std::cout << "DBG> " << std::endl;
	}
}
