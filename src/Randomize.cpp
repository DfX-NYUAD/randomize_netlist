
#include "IO.hpp"
#include "Randomize.hpp"
#include "Data.hpp"

// string identifiers for key nodes in graph
const std::string Data::STRINGS_GLOBAL_SOURCE = "GLOBAL_SOURCE";
const std::string Data::STRINGS_GLOBAL_SINK = "GLOBAL_SINK";
// string identifiers for netlist files 
const std::string Data::STRINGS_DEFAULT_NETLIST = "netlist.v";
// US locale for outputs; mainly for "," separator for multiples of thousand
const std::string Data::LOCALE = "en_US.utf8";

// signal handler
static int s_interrupted = 0;
static void s_signal_handler (int signal_value) {

	std::cout << "Randomize>" << std::endl;
	std::cout << "Randomize> Caught signal \"" << signal_value << "\"; aborting after current iteration ..." << std::endl;
	std::cout << "Randomize>" << std::endl;

	s_interrupted = 1;
}
static void s_catch_signals (void) {
	struct sigaction action;
	action.sa_handler = s_signal_handler;
	action.sa_flags = 0;
	sigemptyset (&action.sa_mask);
	sigaction (SIGINT, &action, NULL);
	sigaction (SIGTERM, &action, NULL);
}

int main (int argc, char** argv) {
	Data data;
	double HD;
	unsigned iter;
	unsigned current_intermediate_output_HD_step;

	std::cout << std::endl;
	std::cout << "Randomize netlist" << std::endl;
	std::cout << "-----------------" << std::endl << std::endl;

	// track start time
	auto start_time = std::chrono::system_clock::now();

	// init random generator with high-resolution timing seed
	auto now = std::chrono::high_resolution_clock::now();
	auto nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
	std::srand(nanos);

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

	// also parse the golden netlist, if different than the input netlist
	//
	if (data.files.in_netlist != data.files.golden_netlist) {

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
	}
	// otherwise initialize the golden netlist as copy from the input netlist
	else {
		data.golden_netlist = data.netlist;
	}

	// init the original graphs
	//
	Randomize::initGraph(data.netlist);
	Randomize::initGraph(data.golden_netlist);

	// sanity check for cycles on original graphs
	if (Randomize::checkGraphForCycles( &(data.netlist.nodes[Data::STRINGS_GLOBAL_SOURCE])) ) {
		std::cout << "Randomize> Error: input netlist contains a cycle; exiting ..." << std::endl;
		exit(1);
	}
	if (Randomize::checkGraphForCycles( &(data.golden_netlist.nodes[Data::STRINGS_GLOBAL_SOURCE])) ) {
		std::cout << "Randomize> Error: golden netlist contains a cycle; exiting ..." << std::endl;
		exit(1);
	}

	// for original graphs, determine the order of all graph nodes, from global source to global sink;
	// required for subsequent HD evaluation, which is based on propagation of Boolean values from inputs to outputs
	//
	Randomize::determGraphOrder(data.netlist);
	Randomize::determGraphOrder(data.golden_netlist);

	// now, iteratively run random modifications on graph copies, until HD approaches desired value
	//
	iter = 1;
	HD = 0.0;
	current_intermediate_output_HD_step = 0;
	// also register signal handler
	s_catch_signals();
	do {

		std::cout << "Randomize> Iteration: " << iter << std::endl;
		std::cout << "Randomize>  Current HD: " << HD << std::endl;

		Randomize::iteration(data, HD);

		std::cout << "Randomize>" << std::endl;

		// also track intermediate results; write out netlist for every ``intermediate_output_HD_step'' step
		//
		// init current_intermediate_output_HD_step, could be larger than 1 for resuming randomization and comparing to golden,
		// original netlist
		if (current_intermediate_output_HD_step == 0) {
			current_intermediate_output_HD_step = std::ceil(HD / data.parameters.intermediate_output_HD_step);
		}
		if (HD >= (current_intermediate_output_HD_step * data.parameters.intermediate_output_HD_step)) {

			std::cout << "Randomize> Generate intermediate result #" << current_intermediate_output_HD_step << std::endl;
			std::cout << "Randomize>" << std::endl;
			IO::writeNetlist(data, HD, iter);
			// if desired, also generate a scrambled version
			if (data.parameters.also_output_scrambled_netlists) {
				IO::writeNetlist(data, HD, iter, true);
			}

			current_intermediate_output_HD_step++;
		}

		iter++;
	}
	// continue until HD reaches target, or until signal is caught
	while ((HD < data.parameters.HD_target) && (s_interrupted != 1));

	std::cout << "Randomize> Done" << std::endl;

	// log modification statistics
	std::cout << "Randomize>  Replaced cell type for that many gates: " << data.netlist_modifications.replacedCells << std::endl;
	std::cout << "Randomize>  Swapped outputs for that many pairs of gates: " << data.netlist_modifications.swappedOutputs << std::endl;
	std::cout << "Randomize>  Swapped inputs for that many pairs of gates: " << data.netlist_modifications.swappedInputs << std::endl;
	std::cout << "Randomize>  Deleted that many gates: " << data.netlist_modifications.deletedGates << std::endl;
	std::cout << "Randomize>  Inserted that many gates: " << data.netlist_modifications.insertedGates << std::endl;
	std::cout << "Randomize>" << std::endl;

	// output final randomized netlist
	IO::writeNetlist(data, HD, iter - 1);
	// if desired, also generate a scrambled version
	if (data.parameters.also_output_scrambled_netlists) {
		IO::writeNetlist(data, HD, iter - 1, true);
	}

	// also log runtime
	std::chrono::duration<double> runtime = std::chrono::system_clock::now() - start_time;
	std::cout << "Randomize> Overall runtime: " << runtime.count() << " seconds" << std::endl;
	std::cout << "Randomize>" << std::endl;
};

void Randomize::iteration(Data& data, double& HD) {
	bool check_for_loops;
	Randomize::RandomOperation op;
	// threads
	std::vector<std::thread> threads;
	std::mutex m;

	// init a local netlist from the current global one (not the original one)
	Data::Netlist netlist = data.netlist;

	// random operation on local netlist
	//
	std::cout << "Randomize>  Performing random operation on current netlist ..." << std::endl;

	// if not provided as parameter, pick operation randomly
	// see also Randomize::RandomOperation for definition of opcodes
	//
	if (data.parameters.random_op == Data::Parameters::DEFAULT_RANDOM_OP) {
		op = static_cast<Randomize::RandomOperation>(Randomize::rand(1, 6));
	}
	else {
		op = static_cast<Randomize::RandomOperation>(data.parameters.random_op);
	}
	switch (op) {

		case Randomize::RandomOperation::ReplaceCell:
			std::cout << "Randomize>   1) Replace underlying cell" << std::endl;
			Randomize::randomizeHelperReplaceCell(data.cells, data.parameters.consider_driving_strength, netlist);
			check_for_loops = false;
			break;

		case Randomize::RandomOperation::SwapOutputs:
			std::cout << "Randomize>   2) Swap outputs" << std::endl;
			Randomize::randomizeHelperSwapOutputs(data.parameters.consider_fanout, netlist);
			// connectivity is modified, so check for loops
			check_for_loops = true;
			break;

		case Randomize::RandomOperation::SwapInputs:
			std::cout << "Randomize>   3) Swap inputs" << std::endl;
			Randomize::randomizeHelperSwapInputs(netlist);
			// connectivity is modified, so check for loops
			check_for_loops = true;
			break;

		case Randomize::RandomOperation::DeleteGate:
			std::cout << "Randomize>   4) Delete gate" << std::endl;
			Randomize::randomizeHelperDeleteGate(netlist);
			check_for_loops = false;
			break;

		case Randomize::RandomOperation::InsertGate:
			std::cout << "Randomize>   5) Insert gate" << std::endl;
			Randomize::randomizeHelperInsertGate(data.cells, netlist);
			// connectivity is modified, by picking some input nets among the netlist, so check for loops
			check_for_loops = true;
			break;

		default:
			std::cout << "Randomize>   Warning: unsupported operation code: \"" << data.parameters.random_op << "\" -- skipping operation" << std::endl;
			check_for_loops = false;
	}

	// init the local graph 
	//
	// It's important to understand that the original graph CANNOT be simply copied here -- the pointers in the graph nodes would be
	// based on the initial data and, hence, any modifications on the graph would impose those edits only on the underlying initial
	// data.  Besides, the alternative of initializing the graph once and then rewriting all the pointers for a graph copy would
	// probably not help much for performance.
	Randomize::initGraph(netlist);

	// if required, check for loops
	if (check_for_loops) {
		std::cout << "Randomize>    Operation changed connectivity -- check for combinatorial loops ..." << std::endl;
		if (Randomize::checkGraphForCycles( &(netlist.nodes[Data::STRINGS_GLOBAL_SOURCE])) ) {
			std::cout << "Randomize>     Found some loop; rejecting this netlist modification" << std::endl;
			return;
		}
		else {
			std::cout << "Randomize>     No loop found" << std::endl;
		}
	}

	// determine the order of all graph nodes, from global source to global sink;
	// required for subsequent HD evaluation, which is based on propagation of Boolean values from inputs to outputs
	//
	Randomize::determGraphOrder(netlist);

	std::cout << "Randomize>  Evaluating HD of modified netlist ..." << std::endl;

	// evaluate HD in parallel
	threads.reserve(data.parameters.threads);
	unsigned const iterations_per_thread = std::ceil(static_cast<double>(data.parameters.HD_sampling_iterations) / data.parameters.threads);
	double HD_threads = 0.0;

	for (unsigned t = 0; t < data.parameters.threads; t++) {

		// notes
		// 1) pass nodes for original netlist and for local graph as copies, since the threads have to work on separate graph data 
		// 2) HD will be summed up in the threads (using the mutex)
		threads.emplace_back( std::thread(Randomize::evaluateHD,
					data.golden_netlist.nodes,
					netlist.nodes,
					std::ref(data.golden_netlist.topology),
					std::ref(netlist.topology),
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
	HD_threads /= data.parameters.threads;

	// if HD improved, make local netlist the new global netlist
	//
	// also allow for some random chance to accept an inferior solution
	if ((HD_threads > HD) || ((100 * data.parameters.acceptance_ratio) > Randomize::rand(0, 100))) {
		// different logging
		if (HD_threads > HD) {
			std::cout << "Randomize>   New HD (" << HD_threads << ") improves current HD (" << HD << "); memorizing netlist modification" << std::endl;
		}
		else {
			std::cout << "Randomize>   New HD (" << HD_threads << ") does not improve current HD (" << HD << "); still memorizing netlist modification (by random chance considering ``acceptance_ratio'' of " << data.parameters.acceptance_ratio << ")" << std::endl;
		}

		data.netlist = std::move(netlist);
		// also track new HD
		HD = HD_threads;

		// also track modification on netlist
		switch (op) {

			case Randomize::RandomOperation::ReplaceCell:
				data.netlist_modifications.replacedCells++;
				break;

			case Randomize::RandomOperation::SwapOutputs:
				data.netlist_modifications.swappedOutputs++;
				break;

			case Randomize::RandomOperation::SwapInputs:
				data.netlist_modifications.swappedInputs++;
				break;

			case Randomize::RandomOperation::DeleteGate:
				data.netlist_modifications.deletedGates++;
				break;

			case Randomize::RandomOperation::InsertGate:
				data.netlist_modifications.insertedGates++;
				break;
		}
	}
	else {
		std::cout << "Randomize>   New HD (" << HD_threads << ") does not improve current HD (" << HD << "); drop netlist modification" << std::endl;
	}
}

// 1) swap gate type / underlying cell
//
// note that consider_driving_strength is passed by value / as copy; hence, data.parameters.consider_driving_strength is not affected/overwritten
void Randomize::randomizeHelperReplaceCell(std::unordered_map<std::string, Data::Cell> const& cells, bool consider_driving_strength, Data::Netlist& netlist) {
	bool found;
	unsigned trials, trials_stop;
	Data::Cell const* cell;

	// pick gate randomly
	//
	Data::Gate& gate = netlist.gates[
		Randomize::rand(0, netlist.gates.size())
			];

	std::cout << "Randomize>    Randomly picked gate: \"" << gate.name << "\"" << std::endl;

	// search for other cell type;
	// search until a suitable cell was found, or until sufficient number of trials have been conducted
	//
	found = false;
	trials = 0;
	trials_stop = cells.size() * Randomize::TRIALS_LIMIT_FACTOR;

	while (!found) {
		trials++;

		// break handler
		if (trials == trials_stop) {
			// so far the driving strength might have had to match;
			// but now, since many trials had already been done, drop that constraint, and reset trials counter
			if (consider_driving_strength) {
				consider_driving_strength = false;
				trials = 0;
			}
			// driving strength has been already ignored (for 2nd part of trials); still no suitable cell found; abort
			else {
				break;
			}
		}

		// pick cell randomly
		auto it = cells.begin();
		std::advance(it, Randomize::rand(0, cells.size()));
		cell = &((it)->second);

		// possibly consider driving strength, see also above
		//
		if (!consider_driving_strength || (consider_driving_strength && (cell->strength == gate.cell->strength))) {

			// should be a cell with same number of inputs/outputs
			//
			if (cell->inputs.size() == gate.cell->inputs.size() && cell->outputs.size() == gate.cell->outputs.size()) {

				// but, of course, shouldn't be a cell with the very same functions
				//
				bool same_fct = false;
				for (auto const& new_fct : cell->functions) {
					auto const old_fct = gate.cell->functions.find(new_fct.first);

					// in the old cell, there's an output pin with the same name as for the new cell
					if (old_fct != gate.cell->functions.end()) {
						// the related function should be different; if not, break
						if (new_fct.second == old_fct->second) {
							same_fct = true;
							break;
						}
					}
				}

				if (!same_fct) {
					found = true;
				}
			}
		}
	}

	if (!found) {
		std::cout << "Randomize>    Warning: could not find any other suitable cell to replace this gate's cell (\"" <<
			gate.cell->type << "\" with; skipping operation ..." << std::endl;
	}
	// cell to replace with found;
	// randomly re-assign all nets/pins connected to input/output pins of gate to some pins of the new cell
	else {
		std::cout << "Randomize>    Current cell type: \"" << gate.cell->type << "\"; new cell type: \"" << cell->type << "\"" << std::endl;

		std::set<std::string> pins_not_connected_yet;
		// cell pin name, net/pin name
		std::unordered_map<std::string, std::string> inputs, outputs;

		// for each input of the gate, replace the pin name with one of the new cell
		//
		pins_not_connected_yet = cell->inputs;
		for (auto& input : gate.inputs) {

			// randomly pick one input
			auto it = pins_not_connected_yet.begin();
			std::advance(it, Randomize::rand(0, pins_not_connected_yet.size()));

			// create the following pair: new pin name, old net/pin name assignment
			inputs.insert(std::make_pair(
						*it,
						input.second
					));

			// also remove this pin from the set of yet unconnected ones
			pins_not_connected_yet.erase(it);
		}

		// for each output of the gate, replace the pin name with one of the new cell
		//
		pins_not_connected_yet = cell->outputs;
		for (auto& output : gate.outputs) {

			// randomly pick one output
			auto it = pins_not_connected_yet.begin();
			std::advance(it, Randomize::rand(0, pins_not_connected_yet.size()));

			// create the following pair: new pin name, old net/pin name assignment
			outputs.insert(std::make_pair(
						*it,
						output.second
					));

			// also remove this pin from the set of yet unconnected ones
			pins_not_connected_yet.erase(it);
		}

		// replace the pin mappings in the gate
		gate.inputs = std::move(inputs);
		gate.outputs = std::move(outputs);

		// finally, also replace cell in gate
		gate.cell = cell;
	}
}

// 2) swap output connectivity:
// 	select two outputs (of different gates), possibly with same fan-out degree
// 	swap the output nets
//
// note that consider_fanout is passed by value / as copy; hence, data.parameters.consider_fanout is not affected/overwritten
void Randomize::randomizeHelperSwapOutputs(bool consider_fanout, Data::Netlist& netlist) {
	bool found;
	unsigned trials, trials_stop;

	found = false;
	trials = 0;
	trials_stop = netlist.gates.size() * Randomize::TRIALS_LIMIT_FACTOR;

	while (!found) {
		trials++;

		// break handler
		if (trials == trials_stop) {
			// so far the fan out might have had to match;
			// but now, since many trials had already been done, drop that constraint, and reset trials counter
			if (consider_fanout) {
				consider_fanout = false;
				trials = 0;
			}
			// fan out has been already ignored (for 2nd part of trials); still no suitable pair of gates found; abort
			else {
				break;
			}
		}

		// pick two gates randomly
		//
		Data::Gate& gate_a = netlist.gates[
			Randomize::rand(0, netlist.gates.size())
				];
		Data::Gate& gate_b = netlist.gates[
			Randomize::rand(0, netlist.gates.size())
				];

		// first, gates shouldn't be the same
		if (gate_a.name != gate_b.name) {

			// randomly select one output net for each gate
			//
			auto output_a = gate_a.outputs.begin();
			std::advance(output_a, Randomize::rand(0, gate_a.outputs.size()));
			auto output_b = gate_b.outputs.begin();
			std::advance(output_b, Randomize::rand(0, gate_b.outputs.size()));

			// possibly consider fanout, see also above
			//
			// check for the same number of children/fan-out on the current graph (not yet updated, but that is OK since this
			// very gate modification is also not done yet)
			// 
			bool const same_fanout = (netlist.nodes[output_a->second].children.size() == netlist.nodes[output_b->second].children.size());
			if (!consider_fanout || (consider_fanout && same_fanout)) {

				found = true;

				std::cout << "Randomize>    Randomly picked gates: \"" << gate_a.name << "\" and \"" << gate_b.name << "\"" << std::endl;
				std::cout << "Randomize>     Randomly picked output nets/pins: \"" << output_a->second << "\" and \"" << output_b->second << "\"" << std::endl;
				std::cout << "Randomize>     Fan-outs of those nets/pins: " << netlist.nodes[output_a->second].children.size() << " and " << netlist.nodes[output_b->second].children.size() << std::endl;

				// finally, swap the outputs
				std::swap(output_a->second, output_b->second);
			}
		}
	}

	if (!found) {
		std::cout << "Randomize>    Warning: could not find any suitable pair of gates to swap outputs; skipping operation ..." << std::endl;
	}
}

// 3) swap input connectivity:
// 	select two inputs (same or different gates)
// 	swap the input nets
void Randomize::randomizeHelperSwapInputs(Data::Netlist& netlist) {

	// pick two gates randomly, could be even the same
	//
	Data::Gate& gate_a = netlist.gates[
		Randomize::rand(0, netlist.gates.size())
			];
	Data::Gate& gate_b = netlist.gates[
		Randomize::rand(0, netlist.gates.size())
			];

	// randomly select one input net for each gate
	//
	auto input_a = gate_a.inputs.begin();
	std::advance(input_a, Randomize::rand(0, gate_a.inputs.size()));
	auto input_b = gate_b.inputs.begin();
	std::advance(input_b, Randomize::rand(0, gate_b.inputs.size()));

	std::cout << "Randomize>    Randomly picked gates: \"" << gate_a.name << "\" and \"" << gate_b.name << "\"" << std::endl;
	std::cout << "Randomize>     Randomly picked input nets/pins: \"" << input_a->second << "\" and \"" << input_b->second << "\"" << std::endl;

	// finally, swap the inputs
	std::swap(input_a->second, input_b->second);
}

// 4) delete gate:
// 	replace all output nets with some of the input nets
void Randomize::randomizeHelperDeleteGate(Data::Netlist& netlist) {
	bool found;
	unsigned trials, trials_stop;

	found = false;
	trials = 0;
	trials_stop = netlist.gates.size() * Randomize::TRIALS_LIMIT_FACTOR;

	while (!found && (trials < trials_stop)) {
		trials++;

		// pick a gate randomly
		//
		Data::Gate& gate = netlist.gates[
			Randomize::rand(0, netlist.gates.size())
				];

		// sanity check against more outputs than inputs; if so, we cannot replace all outputs with some input (for such cases, we would
		// have to (randomly) select further nets from the netlist -- this is doable, but would impose checking for loops, and is
		// just not implemented as of now since Nangate (or other regular libraries) have no gates with more outputs than inputs 
		//
		// note that this also captures cells with zero inputs but some output, like TIE cells -- deleting them would make some
		// other cells' inputs dangling
		//
		if (gate.outputs.size() > gate.inputs.size()) {
			continue;
		}

		// check whether that gate connects to some PI/PO; if so, ignore that gate for simplicity
		//
		// check for sinks/drivers from the current graph (not yet updated, but that is OK since this very gate modification is also
		// not done yet)
		bool PI_found = false;
		for (auto const* driver : netlist.nodes[gate.name].parents) {

			if (driver->type == Data::Node::Type::PI) {
				PI_found = true;
				break;
			}
		}
		bool PO_found = false;
		for (auto const* sink : netlist.nodes[gate.name].children) {

			if (sink->type == Data::Node::Type::PO) {
				PO_found = true;
				break;
			}
		}
		if (PI_found || PO_found) {
			continue;
		}

		// otherwise, gate is valid to delete
		found = true;

		std::cout << "Randomize>    Randomly picked gate: \"" << gate.name << "\"" << std::endl;

		// replace the input net of any subsequent gate (which was originally driven by the gate to be deleted) with a randomly
		// selected input of that gate -- thereby the driver of the gate to be deleted becomes the driver of the subsequent gate
		//
		std::unordered_map<std::string, std::string> inputs = gate.inputs;
		for (auto const& output : gate.outputs) {

			// randomly pick one of the remaining input nets
			auto input = inputs.begin();
			std::advance(input, Randomize::rand(0, inputs.size()));
			// store name of input net separately
			std::string input_net = input->second;
			// remove input net from map of inputs still available
			inputs.erase(input);

			if (Randomize::DBG) {
				std::cout << "DBG>     Handle the following output of the gate to be deleted: \"" << output.second << "\"" << std::endl;
				std::cout << "DBG>      Leverage the following input of the gate to be deleted: \"" << input_net << "\"" << std::endl;
			}

			// revise subsequent gates (sinks)
			//
// (TODO) for some reason, this quick access to all sinks had not been working in these sense that any modification "sink_input.second =
// input_net" did not carry over ...
//
//			for (auto* sink : netlist.nodes[output.second].children) {
//				for (auto& sink_input : sink->gate->inputs) {
//
			for (auto& gate : netlist.gates) {

				for (auto& sink_input : gate.inputs) {

					// the input of that gate matches the output of the gate to be deleted; that gate is a sink
					if (sink_input.second == output.second) {

						if (Randomize::DBG) {
							std::cout << "DBG>      Replace the following sink's input with that input above: \"";
							std::cout << gate.name << "\"" << std::endl;
							std::cout << "DBG>       Previous sink input: \"" << sink_input.second << "\"" << std::endl;
						}

						// replace that input net with the selected input
						sink_input.second = input_net;

						if (Randomize::DBG) {
							std::cout << "DBG>       New sink input: \"" << sink_input.second << "\"" << std::endl;
						}
					}
				}
			}

			// delete that wire from the netlist
			for (auto iter = netlist.wires.begin(); iter != netlist.wires.end(); ++iter) {
				if ((*iter) == output.second) {

					if (Randomize::DBG) {
						std::cout << "DBG>      Delete the following wire from the netlist: \"" << *iter << "\"" << std::endl;
					}

					netlist.wires.erase(iter);
					break;
				}
			}
		}

		// finally, delete that gate from the netlist
		for (auto iter = netlist.gates.begin(); iter != netlist.gates.end(); ++iter) {
			if ((*iter).name == gate.name) {

				if (Randomize::DBG) {
					std::cout << "DBG>      Delete the following gate from the netlist: \"" << (*iter).name << "\"" << std::endl;
				}

				netlist.gates.erase(iter);
				break;
			}
		}
	}

	if (!found) {
		std::cout << "Randomize>    Warning: could not find any suitable gate to delete; skipping operation ..." << std::endl;
		return;
	}
}

// 5) insert gate:
// 	pick some net randomly
// 	insert gate within net, i.e., decompose net into input and output part
// 	pick remaining input nets for gate randomly from netlist
void Randomize::randomizeHelperInsertGate(std::unordered_map<std::string, Data::Cell> const& cells, Data::Netlist& netlist) {
	bool found;
	unsigned trials, trials_stop;
	Data::Cell const* cell;
	std::string random_net, new_net;

	found = false;
	trials = 0;
	trials_stop = cells.size() * Randomize::TRIALS_LIMIT_FACTOR;

	while (!found && (trials < trials_stop)) {
		trials++;

		// randomly pick cell type 
		//
		auto iter = cells.begin();
		std::advance(iter, Randomize::rand(0, cells.size()));
		cell = &(iter->second);

		// there should be at least one input (that is because insertion is done by putting the new gate within an already existing
		// net)
		//
		// note that this captures cells like TIE cells -- inserting them would require insertion of subsequent sinks at once,
		// otherwise their output would remain dangling
		//
		if (cell->inputs.empty()) {
			continue;
		}
		// also sanity check against more outputs than inputs
		else if (cell->outputs.size() > cell->inputs.size()) {
			continue;
		}
		else {
			found = true;
		}
	}

	if (!found) {
		std::cout << "Randomize>    Warning: could not find any suitable pair of gates to swap outputs; skipping operation ..." << std::endl;
	}
	else {
		std::cout << "Randomize>    Randomly picked cell type: \"" << cell->type << "\"" << std::endl;

		// new gate
		Data::Gate new_gate = Data::Gate();
		// assign cell
		new_gate.cell = cell;

		// randomly generate name
		new_gate.name = Randomize::randName(netlist.random_names_already_taken);

		std::cout << "Randomize>     Random " << Randomize::RAND_NAME_SIZE << "-character name for new gate: \"" << new_gate.name  << "\"" << std::endl;

		// for each of the cell's outputs, randomly pick a net where the gate should be inserted into
		//
		std::set<std::string> inputs = cell->inputs;
		for (auto const& output : cell->outputs) {

			random_net = netlist.wires[
				Randomize::rand(0, netlist.wires.size())
					];

			std::cout << "Randomize>     Re-wire the following net: \"" << random_net << "\"" << std::endl;

			// inserting the new gate into that net requires the following two steps
			//

			// 1) assign that net as an input for the new gate
			//
			// also pick randomly among the input pins not yet assigned
			auto iter = inputs.begin();
			std::advance(iter, Randomize::rand(0, inputs.size()));
			
			new_gate.inputs.insert(std::make_pair(
						*iter,
						random_net
					));

			std::cout << "Randomize>      Connect this net with the following input pin: \"" << *iter << "\"" << std::endl;

			// the input is assigned, remove the pin from the set of inputs yet to be assigned
			inputs.erase(iter);

			// 2) replace all the matching sink inputs for that net with an new net defined for the new gate's output
			//
			// derive the new net name
			new_net = new_gate.name + "_" + output;

			std::cout << "Randomize>      Re-connect all sinks previously connected with this net using the following new net: \"" << new_net << "\"" << std::endl;

			// as with Randomize::randomizeHelperDeleteGate, work directly on gates, not on graph
			//
			for (auto& gate : netlist.gates) {

				for (auto& sink_input : gate.inputs) {

					// the input of that gate matches the net where the gate is inserted into
					if (sink_input.second == random_net) {

						// replace that input net with the new net
						sink_input.second = new_net;
					}
				}
			}

			// 3) assign the new net as output for the new gate
			//
			new_gate.outputs.insert(std::make_pair(
						output,
						new_net
					));

			// 4) insert the new net as wire into the netlist
			//
			netlist.wires.emplace_back(std::move(new_net));
		}

		// now, for the remaining inputs pins, also pick randomly some nets to connect with
		//
		for (auto const& input : inputs) {

			random_net = netlist.wires[
				Randomize::rand(0, netlist.wires.size())
					];

			new_gate.inputs.insert(std::make_pair(
						input,
						random_net
					));

			std::cout << "Randomize>     Connect the remaining input pin \"" << input << "\" with the following net: \"" << random_net << "\"" << std::endl;
		}

		// finally, memorize the gate in the netlist
		netlist.gates.emplace_back(std::move(new_gate));
	}
}

void Randomize::evaluateHD(
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

	if (Randomize::DBG) {
		std::cout << "DBG> Evaluate HD ..." << std::endl;
	}

	// run multiple iterations
	//
	HD_local = 0.0;
	for (unsigned it = 1; it <= iterations; it++) {

		if (Randomize::DBG) {
			std::cout << "DBG> Iteration: " << it << "/" << iterations << std::endl;
		}

		// first, randomly assign the same bits to the input nodes in the original and the current graph
		//
		for (auto const& input : inputs) {
			orig_nodes_copy[input].bit = nodes_copy[input].bit = static_cast<bool>(Randomize::rand(0, 2));

			if (Randomize::DBG_VERBOSE) {
				std::cout << "DBG>  Assign the following random bit to PI \"" << input << "\": " << orig_nodes_copy[input].bit << std::endl;
			}
		}

		// second, evaluate all bit values for both graphs
		//
		if (Randomize::DBG) {
			std::cout << "DBG> Evaluate original graph/netlist" << std::endl;
		}

		Randomize::evaluateHDHelper(orig_nodes_copy, orig_nodes_topology, lazy_evaluation);

		if (Randomize::DBG) {
			std::cout << "DBG> Done" << std::endl;
			std::cout << "DBG> Evaluate current graph/netlist" << std::endl;
		}

		Randomize::evaluateHDHelper(nodes_copy, nodes_copy_topology, lazy_evaluation);

		if (Randomize::DBG) {
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
			if (Randomize::DBG_VERBOSE) {

				std::cout << "DBG>  Original graph, PO \"" << output << "\": " << orig_nodes_copy[output].bit << std::endl;
				std::cout << "DBG>  Current graph, PO \"" << output << "\": " << nodes_copy[output].bit << std::endl;
			}
		}
		// HD of this current iteration
		HD_curr_iter /= outputs.size();

		if (Randomize::DBG) {
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
	//std::cout << "Randomize>  HD for current thread: " << HD_local << std::endl;

	m.unlock();

	if (Randomize::DBG) {
		std::cout << "DBG> HD for this thread: " << HD_local << std::endl;
		std::cout << "DBG> Evaluate HD done" << std::endl;
	}
}

void Randomize::evaluateHDHelper(std::unordered_map<std::string, Data::Node>& nodes, std::vector< std::vector<Data::Node const*> > const& nodes_topology, bool const& lazy_evaluation) {
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

		if (Randomize::DBG_VERBOSE) {
			std::cout << "DBG>  Evaluate Boolean assignments -- current graph index: " << index << std::endl;
		}

		for (auto const* node_ptr : nodes_topology[index]) {

			// work directly on the node reference, not the node pointer from the topology
			//
			auto& node = nodes[node_ptr->name];

			// sanity check for nodes with current index
			if (node.index != index) {
				std::cout << "Randomize> Error -- the following node has an unexpected index: \"" << node.name << "\"" << std::endl;
			}

			// sanity check to consider only wire/PO nodes
			if (!(node.type == Data::Node::Type::Wire || node.type == Data::Node::Type::PO)) {
				std::cout << "Randomize> Error -- the following node is neither a wire node nor a PO node: \"" << node.name << "\"" << std::endl;
			}

			if (Randomize::DBG_VERBOSE) {
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
				std::cout << "Randomize> Error -- the following wire/PO node has more than one parent: \"" << node.name << "\"" << std::endl;
			}

			// parent is gate
			if (node.parents[0]->type == Data::Node::Type::Gate) {

				auto const* gate = node.parents[0]->gate;

				if (Randomize::DBG_VERBOSE) {
					std::cout << "DBG>    Matching driver: " << gate->name << std::endl;
				}

				// check among all outputs of the gate
				output_found = false;
				for (auto const& output : gate->outputs) {

					if (output.second == node.name) {

						if (Randomize::DBG_VERBOSE) {
							std::cout << "DBG>     Driver output: " << output.first << std::endl;
						}

						// sanity check for multiple driver
						if (output_found) {
							std::cout << "Randomize> Error -- the following wire/PO node has more than one driver: \"" << node.name << "\"" << std::endl;
						}
						output_found = true;

						// copy Boolean function string
						function = gate->cell->functions.find(output.first)->second;

						if (Randomize::DBG_VERBOSE) {
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

							if (Randomize::DBG_VERBOSE) {
								std::cout << "DBG>      Current input: " << input.first << std::endl;
								std::cout << "DBG>       Driven by the following node: " << input.second << std::endl;
								std::cout << "DBG>       Revised output function: " << function << std::endl;
							}
						}

						// evaluate Boolean value and assign to node
						node.bit = Randomize::evaluateString(function, lazy_evaluation);

						if (Randomize::DBG_VERBOSE) {
							std::cout << "DBG>      Final function value: " << node.bit << std::endl;
						}
					}
				}
				// log in case no driver was found
				if (!output_found) {
					std::cout << "Randomize> Error -- the following wire/PO node has no driver: \"" << node.name << "\"" << std::endl;
				}
			}
			// parent is PI -- just copy Boolean value
			else if (node.parents[0]->type == Data::Node::Type::PI) {
				node.bit = node.parents[0]->bit;
			}
			// parent is neither PI nor gate; error
			else {
				std::cout << "Randomize> Error -- the following node has an incompatible parent: \"" << node.name << "\"" << std::endl;
			}
		}
	}
}

bool Randomize::evaluateString(std::string function, bool const& lazy_evaluation) {
	std::string::size_type pos_begin, pos_end;
	bool substring_result;
	int inverted_bits;

	if (Randomize::DBG_VERBOSE) {
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

		if (Randomize::DBG_VERBOSE) {
			std::cout << "DBG>  pos_begin: " << pos_begin << std::endl;
			std::cout << "DBG>  pos_end: " << pos_end << std::endl;
			//std::cout << "DBG>   Substring before recursion: " << function.substr(pos_begin + 1, pos_end - pos_begin - 1) << std::endl;
		}

		substring_result = Randomize::evaluateString(function.substr(pos_begin + 1, pos_end - pos_begin - 1), lazy_evaluation);

		if (Randomize::DBG_VERBOSE) {
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

		if (Randomize::DBG_VERBOSE) {
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
			std::cout << "Randomize> Error -- the following Boolean (sub-)string could not be parsed correctly: \"" << function << "\"" << std::endl;
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
							std::cout << "Randomize> Error -- the following Boolean (sub-)string could not be parsed correctly: \"" << function << "\"" << std::endl;
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
							std::cout << "Randomize> Error -- the following Boolean (sub-)string could not be parsed correctly: \"" << function << "\"" << std::endl;
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
							std::cout << "Randomize> Error -- the following Boolean (sub-)string could not be parsed correctly: \"" << function << "\"" << std::endl;
							return false;
						}
					}

				// OR operation with some bit inversion error
				default:
					std::cout << "Randomize> Error -- the following Boolean (sub-)string could not be parsed correctly: \"" << function << "\"" << std::endl;
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
							std::cout << "Randomize> Error -- the following Boolean (sub-)string could not be parsed correctly: \"" << function << "\"" << std::endl;
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
							std::cout << "Randomize> Error -- the following Boolean (sub-)string could not be parsed correctly: \"" << function << "\"" << std::endl;
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
							std::cout << "Randomize> Error -- the following Boolean (sub-)string could not be parsed correctly: \"" << function << "\"" << std::endl;
							return false;
						}
					}

				// AND operation with some bit inversion error
				default:
					std::cout << "Randomize> Error -- the following Boolean (sub-)string could not be parsed correctly: \"" << function << "\"" << std::endl;
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
						std::cout << "Randomize> Error -- the following Boolean (sub-)string could not be parsed correctly: \"" << function << "\"" << std::endl;
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
						std::cout << "Randomize> Error -- the following Boolean (sub-)string could not be parsed correctly: \"" << function << "\"" << std::endl;
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
						std::cout << "Randomize> Error -- the following Boolean (sub-)string could not be parsed correctly: \"" << function << "\"" << std::endl;
						return false;
					}

				// XOR operation with some bit inversion error
				default:
					std::cout << "Randomize> Error -- the following Boolean (sub-)string could not be parsed correctly: \"" << function << "\"" << std::endl;
					return false;
			}
		}
		// unknown operator or other error
		else {
			std::cout << "Randomize> Error -- the following Boolean (sub-)string could not be parsed correctly: \"" << function << "\"" << std::endl;
			return false;
		}
	}
}

bool Randomize::checkGraphForCycles(Data::Node const* node) {

	if (Randomize::DBG_VERBOSE) {
		std::cout << "DBG> Check graph for cycles; consider node: " << node->name << std::endl;
	}

	// node not visited/checked yet
	//
	//
	if (!node->visited) {

		if (Randomize::DBG_VERBOSE) {
			std::cout << "DBG>  Proceed with node \"" << node->name <<"\";";
			std::cout << " not visited yet; mark as visited and as part of recursion" << std::endl;
		}

		// mark as visited/checked, and also mark as part of this recursion
		node->visited = node->recursion = true;

		// now, check all children in depth-first manner
		//
		for (unsigned c = 0; c < node->children.size(); c++) {
			auto const* child = node->children[c];

			if (Randomize::DBG_VERBOSE) {
				std::cout << "DBG>   Consider node \"" << node->name << "\"'s child: \"" << child->name << "\"";
				std::cout << "; child " << c + 1 << "/" << node->children.size() << std::endl;
			}

			// child not visited yet; check recursively whether some cycle can be found
			//
			if (!child->visited && Randomize::checkGraphForCycles(child)) {

				if (Randomize::DBG_VERBOSE) {
					std::cout << "DBG> Return from recursive check of node \"" << node->name << "\"'s child: \"" << child->name << "\"";
					std::cout << "; a cycle was found ..." << std::endl;
				}

				return true;
			}
			// child already visited; in case it has been visited during the current recursive call, then we found a cycle/backedge
			// http://www.geeksforgeeks.org/detect-cycle-in-a-graph/
			//
			else if (child->recursion) {

				if (Randomize::DBG_VERBOSE) {
					std::cout << "DBG>   Cycle found; passed this node \"" << child->name << "\" already during recursion" << std::endl;
				}

				return true;
			}
			// child already visited, but not part of the recursion anymore; represents a transitive edge from one parent node
			// to some child node, which is fine
			// 
			else {
				if (Randomize::DBG_VERBOSE) {
					std::cout << "DBG>   Cleared node \"" << node->name << "\"'s child: " << child->name;
					std::cout << "; child " << c + 1 << "/" << node->children.size() << std::endl;
				}
			}
		}
	}

	if (Randomize::DBG_VERBOSE) {
		std::cout << "DBG> Check graph for cycles; DONE consider node: \"" << node->name << "\"" << std::endl;
	}

	// after return from recursion; mark as "not anymore part of a recursion"
	node->recursion = false;

	return false;
}

void Randomize::determGraphOrder(Data::Netlist const& netlist) {

	if (Randomize::DBG_VERBOSE) {
		std::cout << "DBG> Determining the graph order, in ascending order from global source to global sink ..." << std::endl;
	}

	// global source has index 0 by definition
	netlist.nodes[Data::STRINGS_GLOBAL_SOURCE].index = 0;

	// recursively handle all node, starting from global source
	Randomize::determGraphOrderRec(&(netlist.nodes[Data::STRINGS_GLOBAL_SOURCE]));

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
	if (Randomize::DBG_VERBOSE) {

		std::cout << "DBG> Print topology of netlist graph: " << std::endl;

		for (unsigned i = 0; i < netlist.topology.size(); i++) {
			for (auto const* node : netlist.topology[i]) {

				if (Randomize::DBG_VERBOSE) {
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

void Randomize::initGraph(Data::Netlist& netlist) {

	if (Randomize::DBG) {
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

	// sanity check whether nodes had been inserted / can be found
	if (Randomize::DBG) {

		if (netlist.nodes.find(Data::STRINGS_GLOBAL_SOURCE) == netlist.nodes.end()) {
			std::cout << "DBG>  Error: the following node was not inserted/found: \"" << Data::STRINGS_GLOBAL_SOURCE << "\"" << std::endl;
		}
		if (netlist.nodes.find(Data::STRINGS_GLOBAL_SINK) == netlist.nodes.end()) {
			std::cout << "DBG>  Error: the following node was not inserted/found: \"" << Data::STRINGS_GLOBAL_SINK << "\"" << std::endl;
		}
	}

	// add inputs as nodes
	// 
	for (auto const& input : netlist.inputs) {

		netlist.nodes.insert(std::make_pair(
					input,
					Data::Node(input, Data::Node::Type::PI)
				));

		// sanity check whether nodes had been inserted / can be found
		if (Randomize::DBG) {
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

		netlist.nodes.insert(std::make_pair(
					output,
					Data::Node(output, Data::Node::Type::PO)
				));

		// sanity check whether nodes had been inserted / can be found
		if (Randomize::DBG) {
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

		netlist.nodes.insert(std::make_pair(
					gate.name,
					Data::Node(gate.name, Data::Node::Type::Gate)
				));

		// sanity check whether nodes had been inserted / can be found
		if (Randomize::DBG) {
			if (netlist.nodes.find(gate.name) == netlist.nodes.end()) {
				std::cout << "DBG>  Error: the following node was not inserted/found: \"" << gate.name << "\"" << std::endl;
			}
		}

		// also memorize pointer to gate in node
		netlist.nodes[gate.name].gate = &gate;
	}

	// add wires as nodes
	//
	// important to note: the data structure for netlist.nodes is std::unordered_map, that means in case a wire with the same name as a
	// PI/PO is to be inserted, that wire will be ignored, since the PI/PO node has been already inserted above
	//
	for (auto const& wire : netlist.wires) {

		netlist.nodes.insert(std::make_pair(
					wire,
					Data::Node(wire, Data::Node::Type::Wire)
				));

		// sanity check whether nodes had been inserted / can be found
		if (Randomize::DBG) {
			if (netlist.nodes.find(wire) == netlist.nodes.end()) {
				std::cout << "DBG>  Error: the following node was not inserted/found: \"" << wire << "\"" << std::endl;
			}
		}
	}

	// connect graph based on connectivity of gates
	for (auto const& gate : netlist.gates) {
		auto& gate_node = netlist.nodes[gate.name];

		if (Randomize::DBG_VERBOSE) {
			std::cout << "DBG>  Gate: \"" << gate.name << "\"" << std::endl;
		}

		// check all the inputs of the gate
		//
		// gate.inputs: cell pin is key, pin/net name is value
		for (auto const& input_iter : gate.inputs) {
			auto node_iter = netlist.nodes.find(input_iter.second);

			if (Randomize::DBG_VERBOSE) {
				std::cout << "DBG>   Input pin: \"" << input_iter.first << "\"" << std::endl;
			}

			// there's a node matching the input of the gate
			if (node_iter != netlist.nodes.end()) {

				if (Randomize::DBG_VERBOSE) {
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

		// check all the outputs of the gate
		//
		// gate.outputs: cell pin is key, pin/net name is value
		for (auto const& output_iter : gate.outputs) {
			auto const& node_iter = netlist.nodes.find(output_iter.second);

			if (Randomize::DBG_VERBOSE) {
				std::cout << "DBG>   Output pin: \"" << output_iter.first << "\"" << std::endl;
			}

			// there's a node matching the output of the gate
			if (node_iter != netlist.nodes.end()) {

				if (Randomize::DBG_VERBOSE) {
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
	if (Randomize::DBG) {

		std::cout << "DBG> Print netlist graph: " << std::endl;

		// count all edges
		int edges = 0;
		for (auto const& node_iter : netlist.nodes) {
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
		std::cout << "DBG>  Nodes: " << netlist.nodes.size() << std::endl;
		std::cout << "DBG>  Edges: " << edges << std::endl;
		std::cout << "DBG> " << std::endl;
	}
}
