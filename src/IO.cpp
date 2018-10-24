
#include "IO.hpp"
#include "Data.hpp"
#include "Randomize.hpp"

// parse program parameters and test files
void IO::parseParametersFiles(Data& data, int const& argc, char** argv) {
	int arg_index;

	// parse binary name
	data.binary = argv[0];

	// print help if not all mandatory parameters are provided
	if (argc < 5) {
		IO::printHelp(data);
		exit(1);
	}

	// read in mandatory parameters
	data.files.in_netlist = argv[1];
	data.files.cells_inputs = argv[2];
	data.files.cells_outputs = argv[3];
	data.files.cells_functions = argv[4];

	// test input files
	IO::testFile(data.files.in_netlist);
	IO::testFile(data.files.cells_inputs);
	IO::testFile(data.files.cells_outputs);
	IO::testFile(data.files.cells_functions);

	// read in all optional arguments
	//
	arg_index = 5;
	while (arg_index < argc) {

		std::string arg_string = argv[arg_index];
		std::string::size_type arg_start = arg_string.find('=');

		// sanity check whether argument can be parsed
		if ((arg_string != "--help") && (arg_start == std::string::npos)) {
			std::cout << "IO> Error: the following argument cannot be parsed as optional parameter: \"" << arg_string << "\" -- make sure that the parameters are in the form of, e.g., --intermediate_output_HD_step=" << data.parameters.intermediate_output_HD_step << std::endl;
			exit(1);
		}

		// now, decompose string into parameter and value
		std::string parameter = arg_string.substr(0, arg_start);
		std::string value = arg_string.substr(arg_start + 1);

		if (IO::DBG) {
			std::cout << "DBG> parameter: \"" << parameter << "\"" << std::endl;
			std::cout << "DBG> value: \"" << value << "\"" << std::endl;
		}

		// try to interpret parameter
		//
		if (parameter == "--threads") {
			try {
				data.parameters.threads = std::stoi(value);
			}
			catch (std::invalid_argument) {
				std::cout << "IO> Error: provide an integer value for the optional parameter ``threads''; currently provided: \"" << value << "\"" << std::endl;
				exit(1);
			}
		}
		else if (parameter == "--intermediate_output_HD_step") {
			try {
				data.parameters.intermediate_output_HD_step = std::stod(value);
			}
			catch (std::invalid_argument) {
				std::cout << "IO> Error: provide a double value for the optional parameter ``intermediate_output_HD_step''; currently provided: \"" << value << "\"" << std::endl;
				exit(1);
			}
		}
		else if (parameter == "--HD_target") {
			try {
				data.parameters.HD_target = std::stod(value);
			}
			catch (std::invalid_argument) {
				std::cout << "IO> Error: provide a double value for the optional parameter ``HD_target''; currently provided: \"" << value << "\"" << std::endl;
				exit(1);
			}
		}
		else if (parameter == "--acceptance_ratio") {
			try {
				data.parameters.acceptance_ratio = std::stod(value);
			}
			catch (std::invalid_argument) {
				std::cout << "IO> Error: provide a double value for the optional parameter ``acceptance_ratio''; currently provided: \"" << value << "\"" << std::endl;
				exit(1);
			}
		}
		else if (parameter == "--HD_sampling_iterations") {
			try {
				data.parameters.HD_sampling_iterations = std::stoi(value);
			}
			catch (std::invalid_argument) {
				std::cout << "IO> Error: provide an integer value for the optional parameter ``HD_sampling_iterations''; currently provided: \"" << value << "\"" << std::endl;
				exit(1);
			}
		}
		else if (parameter == "--random_op") {
			try {
				data.parameters.random_op = std::stoi(value);
			}
			catch (std::invalid_argument) {
				std::cout << "IO> Error: provide an integer value for the optional parameter ``random_op''; currently provided: \"" << value << "\"" << std::endl;
				exit(1);
			}
		}
		// Boolean parameters
		else if (parameter == "--also_output_scrambled_netlists") {

			if (value == "1" || value == "true") {
				data.parameters.also_output_scrambled_netlists = true;
			}
			else if (value == "0" || value == "false") {
				data.parameters.also_output_scrambled_netlists = false;
			}
			else {
				std::cout << "IO> Error: provide a Boolean value for the optional parameter ``also_output_scrambled_netlists''; currently provided: \"";
				std::cout << value << "\"" << std::endl;
				exit(1);
			}
		}
		else if (parameter == "--consider_fanout") {

			if (value == "1" || value == "true") {
				data.parameters.consider_fanout = true;
			}
			else if (value == "0" || value == "false") {
				data.parameters.consider_fanout = false;
			}
			else {
				std::cout << "IO> Error: provide a Boolean value for the optional parameter ``consider_fanout''; currently provided: \"";
				std::cout << value << "\"" << std::endl;
				exit(1);
			}
		}
		else if (parameter == "--consider_driving_strength") {
			if (value == "1" || value == "true") {
				data.parameters.consider_driving_strength = true;
			}
			else if (value == "0" || value == "false") {
				data.parameters.consider_driving_strength = false;
			}
			else {
				std::cout << "IO> Error: provide a Boolean value for the optional parameter ``consider_driving_strength''; currently provided: \"";
				std::cout << value << "\"" << std::endl;
				exit(1);
			}
		}
		// special parameter help
		else if (parameter == "--help") {
			IO::printHelp(data);
			exit(1);
		}
		// finally, catch any unsupported parameter
		else {
			std::cout << "IO> Error: the following optional parameter is not supported: \"" << arg_string << "\" -- check your input and also call the binary without any parameters to obtain a help/overview" << std::endl;
			exit(1);
		}

		// consider next argument
		arg_index++;
	}

	std::cout << "IO> Parameter ``netlist.v'': " << data.files.in_netlist << std::endl;
	std::cout << "IO> Parameter ``cells.inputs'': " << data.files.cells_inputs << std::endl;
	std::cout << "IO> Parameter ``cells.outputs'': " << data.files.cells_outputs << std::endl;
	std::cout << "IO> Parameter ``cells.functions'': " << data.files.cells_functions << std::endl;
	std::cout << "IO> Parameter ``also_output_scrambled_netlists'': " << data.parameters.also_output_scrambled_netlists << std::endl;
	std::cout << "IO> Parameter ``threads'': " << data.parameters.threads << std::endl;
	std::cout << "IO> Parameter ``intermediate_output_HD_step'': " << data.parameters.intermediate_output_HD_step << std::endl;
	std::cout << "IO> Parameter ``HD_target'': " << data.parameters.HD_target << std::endl;
	std::cout << "IO> Parameter ``acceptance_ratio'': " << data.parameters.acceptance_ratio << std::endl;
	std::cout << "IO> Parameter ``HD_sampling_iterations'': " << data.parameters.HD_sampling_iterations << std::endl;
	std::cout << "IO> Parameter ``consider_fanout'': " << data.parameters.consider_fanout << std::endl;
	std::cout << "IO> Parameter ``consider_driving_strength'': " << data.parameters.consider_driving_strength << std::endl;
	std::cout << "IO> Parameter ``random_op'': " << data.parameters.random_op << std::endl;
	std::cout << "IO>  Replace underlying cell type: " << static_cast<unsigned>(Randomize::RandomOperation::ReplaceCell) << std::endl;
	std::cout << "IO>  Swap outputs for a pair of gates: " << static_cast<unsigned>(Randomize::RandomOperation::SwapOutputs) << std::endl;
	std::cout << "IO>  Swap inputs for a pair of gates: " << static_cast<unsigned>(Randomize::RandomOperation::SwapInputs) << std::endl;
	std::cout << "IO>  Delete a gate: " << static_cast<unsigned>(Randomize::RandomOperation::DeleteGate) << std::endl;
	std::cout << "IO>  Insert a gate: " << static_cast<unsigned>(Randomize::RandomOperation::InsertGate) << std::endl;
	std::cout << "IO>  Randomly any of the above operation: " << Data::parameters::DEFAULT_RANDOM_OP << std::endl;
	std::cout << "IO> " << std::endl;
};

void IO::printHelp(Data const& data) {
	std::cout << "IO> Usage: " << data.binary << " netlist.v cells.inputs cells.outputs cells.functions [";
	std::cout << "--also_output_scrambled_netlists=" << data.parameters.also_output_scrambled_netlists << " ";
	std::cout << "--threads=" << data.parameters.threads << " ";
	std::cout << "--intermediate_output_HD_step=" << data.parameters.intermediate_output_HD_step << " ";
	std::cout << "--HD_target=" << data.parameters.HD_target << " ";
	std::cout << "--acceptance_ratio=" << data.parameters.acceptance_ratio << " ";
	std::cout << "--HD_sampling_iterations=" << data.parameters.HD_sampling_iterations << " ";
	std::cout << "--consider_fanout=" << data.parameters.consider_fanout << " ";
	std::cout << "--consider_driving_strength=" << data.parameters.consider_driving_strength << " ";
	std::cout << "--random_op=" << data.parameters.random_op << "]" << std::endl;
	std::cout << "IO> " << std::endl;
	std::cout << "IO> Mandatory parameter ``netlist.v'': Netlist to be randomized" << std::endl;
	std::cout << "IO> Mandatory parameter ``cells.inputs'': Cells and their inputs" << std::endl;
	std::cout << "IO> Mandatory parameter ``cells.outputs'': Cells and their outputs" << std::endl;
	std::cout << "IO> Mandatory parameter ``cells.functions'': Cells and their output functions" << std::endl;
	std::cout << "IO> Optional parameter ``also_output_scrambled_netlists'': Besides, randomized netlists, also generate scrambled netlists; default value: " << data.parameters.also_output_scrambled_netlists << std::endl;
	std::cout << "IO> Optional parameter ``threads'': Threads for parallel runs; default value: " << data.parameters.threads << std::endl;
	std::cout << "IO> Optional parameter ``intermediate_output_HD_step'': Write out intermediate results/netlist, for every X step from HD 0.0 to ``HD_target''; default value: " << data.parameters.intermediate_output_HD_step << std::endl;
	std::cout << "IO> Optional parameter ``HD_target'': Target value for HD [0.0 - 1.0]; default value: " << data.parameters.HD_target << std::endl;
	std::cout << "IO> Optional parameter ``acceptance_ratio'': Ratio for accepting some modification with inferior HD [0.0 - 1.0]; default value: " << data.parameters.acceptance_ratio << std::endl;
	std::cout << "IO> Optional parameter ``HD_sampling_iterations'': Iterations for HD evaluation; default value: " << data.parameters.HD_sampling_iterations << std::endl;
	std::cout << "IO> Optional parameter ``consider_fanout'': when swapping outputs for a pair of gate, try to match the fan-out for those outputs; default value: " << data.parameters.consider_fanout << std::endl;
	std::cout << "IO> Optional parameter ``consider_driving_strength'': when replacing the underlying cell type, try to keep the same driving strength; default value: " << data.parameters.consider_driving_strength << std::endl;
	std::cout << "IO> Optional parameter ``random_op'': Integer code for the random operation to be applied; possible values: " << std::endl;
	std::cout << "IO>  Replace underlying cell type: " << static_cast<unsigned>(Randomize::RandomOperation::ReplaceCell) << std::endl;
	std::cout << "IO>  Swap outputs for a pair of gates: " << static_cast<unsigned>(Randomize::RandomOperation::SwapOutputs) << std::endl;
	std::cout << "IO>  Swap inputs for a pair of gates: " << static_cast<unsigned>(Randomize::RandomOperation::SwapInputs) << std::endl;
	std::cout << "IO>  Delete a gate: " << static_cast<unsigned>(Randomize::RandomOperation::DeleteGate) << std::endl;
	std::cout << "IO>  Insert a gate: " << static_cast<unsigned>(Randomize::RandomOperation::InsertGate) << std::endl;
	std::cout << "IO>   Note that there's no default value for ``random_op'' -- in case this parameter is not provided, the above operations will be picked randomly" << std::endl;
	std::cout << "IO> " << std::endl;
}

void IO::testFile(std::string const& file) {
	std::ifstream in;

	in.open(file.c_str());
	if (!in.good()) {
		std::cout << "IO> ";
		std::cout << "No such file: " << file << std::endl;
		exit(1);
	}
	in.close();
}

void IO::parseCells(Data& data) {
	std::ifstream in;
	std::string line;
	std::string tmpstr;

	in.open(data.files.cells_inputs.c_str());

	// 1) inputs
	std::cout << "IO> Parsing the cells and their inputs ...";
	std::cout << std::endl;

	while (std::getline(in, line)) {

		Data::Cell cell;
		std::istringstream linestream(line);

		// first is cell type
		linestream >> cell.type;

		// also memorize type and driving strength separately
		// presume that driving strength comes after '_'
		std::string::size_type driving_strength = cell.type.find('_');
		if (driving_strength != std::string::npos) {
			cell.type_wo_strength = cell.type.substr(0, driving_strength);
			cell.strength = cell.type.substr(driving_strength);
		}
		// otherwise, driving strength could not be parsed
		else {
			cell.type_wo_strength = cell.type;
			std::cout << "IO>  Warning -- the driving strength of the following cell could not been parsed: ";
			std::cout << "\"" << cell.type << "\" -- this should not happen, check your cell files!" << std::endl;
		}

		// then, any number of I/O pins can follow
		while (!linestream.eof()) {

			linestream >> tmpstr;
			cell.inputs.insert(tmpstr);
		}

		// memorize parsed cell
		//
		data.cells.insert(std::make_pair(
					cell.type,
					cell
				));
	}
	
	// close inputs file
	in.close();

	// 2) outputs
	in.open(data.files.cells_outputs.c_str());

	std::cout << "IO> Parsing the cells and their outputs ...";
	std::cout << std::endl;

	while (std::getline(in, line)) {

		Data::Cell cell;
		std::istringstream linestream(line);

		// first is cell type
		linestream >> cell.type;

		// then, any number of I/O pins can follow
		while (!linestream.eof()) {

			linestream >> tmpstr;
			cell.outputs.insert(tmpstr);
		}

		// update existing cells or log warning
		//
		auto const iter = data.cells.find(cell.type);
		if (iter != data.cells.end()) {
			iter->second.outputs = cell.outputs;
		}
		else {
			std::cout << "IO>  Warning -- the following cell has not been seen during the previous parsing operation: ";
			std::cout << "\"" << cell.type << "\" -- this should not happen, check your cell files for consistency!" << std::endl;
		}
	}
	
	// close outputs file
	in.close();

	// 3) functions
	in.open(data.files.cells_functions.c_str());

	std::cout << "IO> Parsing the cells and their functions ...";
	std::cout << std::endl;

	while (std::getline(in, line)) {

		Data::Cell cell;
		std::istringstream linestream(line);
		bool start = true;
		std::string function;
		std::string output_pin;

		// first is cell type
		linestream >> cell.type;

		// then, any number of output functions can follow
		//
		// functions are encapsulated within " ", so parse for beginning and end "
		//
		while (!linestream.eof()) {

			linestream >> tmpstr;

			// current word is start or end of new function
			if (tmpstr.find('"') != std::string::npos) {

				// start word, contains output pin
				if (start) {
					output_pin = tmpstr;
					start = false;
				}
				// second, independent occurrence of "; stop word
				else if (!start) {

					// consider last word of function
					function += tmpstr;

					//if (IO::DBG) {
					//	std::cout << "IO_DBG>  \"" << cell.type << "\" FUNCTION: \"" << function << "\"" << std::endl;
					//}

					// memorize function, but drop the " at the beginning and the end
					cell.functions.insert(std::make_pair(
								output_pin.substr(1),
								std::string(function.begin(), function.end() - 1)
								//function.substr(0, function.find('"'))
							));

					// reset start
					start = true;
				}
			}
			else {
				// = occurs after output pin; reset function string
				if (tmpstr.find('=') != std::string::npos) {
					function = "";
				}
				// regular word in between, build up current function
				else {
					function += tmpstr + " ";
				}
			}
		}

		// update existing cells or log warning
		//
		auto const iter = data.cells.find(cell.type);
		if (iter != data.cells.end()) {
			iter->second.functions = cell.functions;

			if (IO::DBG) {
				std::cout << "IO_DBG>  \"" << iter->second.type << "\"";
				std::cout << " (" << iter->second.type_wo_strength << "; " << iter->second.strength << ")";
				std::cout << " OUT = ( ";
				for (auto const& output : iter->second.outputs) {
					std::cout << "\"" << output << "\" ";
				}
				std::cout << ")";
				std::cout << " FUNCTIONS = ( ";
				for (auto const& function : iter->second.functions) {
					std::cout << "\"" << function.first << "\" = \"" << function.second << "\" ";
				}
				std::cout << ")" << std::endl;
			}

			// cross check number of functions with number of output pins
			if (iter->second.functions.size() != iter->second.outputs.size()) {
				std::cout << "IO>  Warning -- the following cell has a different number of outputs and output functions: ";
				std::cout << "\"" << iter->second.type << "\" -- this should not happen, check your cell files for consistency!" << std::endl;

			}

			// cross check actual functions with output pins
			for (auto const& output : iter->second.outputs) {

				if (iter->second.functions.find(output) == iter->second.functions.end()) {
					std::cout << "IO>  Warning -- the following cell is missing a function for an output pin: ";
					std::cout << "\"" << iter->second.type << "\" -- this should not happen, check your cell files for consistency!" << std::endl;
				}
			}
		}
		else {
			std::cout << "IO>  Warning -- the following cell has not been seen during the previous parsing operation: ";
			std::cout << "\"" << cell.type << "\" -- this should not happen, check your cell files for consistency!" << std::endl;
		}
	}
	
	// close functions file
	in.close();

	// dbg log of parsed tuples
	//
	if (IO::DBG) {

		std::cout << "IO_DBG> Print all cells: " << std::endl;

		for (auto const& cell_iter : data.cells) {
			Data::Cell const& cell = cell_iter.second;

			std::cout << "IO_DBG>  \"" << cell.type << "\" --";

			std::cout << " OUT: ( ";
			for (auto const& output : cell.outputs) {
				std::cout << "\"" << output << "\" = \"" << cell.functions.find(output)->second << "\" ";
			}
			std::cout << ")";
			std::cout << " IN: ( ";
			for (auto const& input : cell.inputs) {
				std::cout << "\"" << input << "\" ";
			}
			std::cout << ")";

			std::cout << std::endl;
		}
	}

	std::cout << "IO> Done" << std::endl;
	std::cout << "IO>  Cells: " << data.cells.size() << std::endl;
	std::cout << "IO> " << std::endl;

	// sanity check
	if (data.cells.empty()) {
		std::cout << "IO> No cells found; exiting ..." << std::endl;
		std::cout << "IO> Check your cells files \"" << data.files.cells_inputs << "\", \"" << data.files.cells_outputs << "\"" << std::endl;
		exit(1);
	}
}

void IO::parseNetlist(Data& data) {
	std::ifstream in;
	std::string line;
	std::string tmpstr;

	in.open(data.files.in_netlist.c_str());

	std::cout << "IO> Parsing the netlist ...";
	std::cout << std::endl;

	// 0) parse the module name
	while (std::getline(in, line)) {

		// skip all the irrelevant lines
		if (!(line.find("module") != std::string::npos && line.find("end") == std::string::npos)) {
			continue;
		}
		// process the one relevant line
		else {
			std::istringstream linestream(line);

			// drop "module";
			linestream >> tmpstr;

			// parse the module name
			linestream >> tmpstr;

			data.netlist.module_name = tmpstr;
		}
	}
	
	// reset file handler
	in.clear() ;
	in.seekg(0, in.beg);

	// dbg log
	//
	if (IO::DBG) {
		std::cout << "IO_DBG> Print module name: " << data.netlist.module_name << std::endl;
	}

	// 1) parse inputs, line by line
	//
	while (std::getline(in, line)) {

		// skip all the irrelevant lines
		if (!(line.find("input") != std::string::npos && line.find(";") != std::string::npos)) {
			continue;
		}
		// process all the relevant lines
		else {
			std::istringstream linestream(line);

			// drop "input";
			linestream >> tmpstr;

			// parse the input name, without the final ";"
			linestream >> tmpstr;
			tmpstr = tmpstr.substr(0, tmpstr.find(";"));

			data.netlist.inputs.emplace_back(tmpstr);
		}
	}
	
	// reset file handler
	in.clear() ;
	in.seekg(0, in.beg);

	// dbg log
	//
	if (IO::DBG) {

		std::cout << "IO_DBG> Print all inputs: " << std::endl;

		for (auto const& port : data.netlist.inputs) {

			std::cout << "IO_DBG>  \"" << port << "\"";
			std::cout << std::endl;
		}
	}

	// 2) parse outputs, line by line
	//
	while (std::getline(in, line)) {

		// skip all the irrelevant lines
		if (!(line.find("output") != std::string::npos && line.find(";") != std::string::npos)) {
			continue;
		}
		// process all the relevant lines
		else {
			std::istringstream linestream(line);

			// drop "output";
			linestream >> tmpstr;

			// parse the output name, without the final ";"
			linestream >> tmpstr;
			tmpstr = tmpstr.substr(0, tmpstr.find(";"));

			data.netlist.outputs.emplace_back(tmpstr);
		}
	}
	
	// reset file handler
	in.clear() ;
	in.seekg(0, in.beg);

	// dbg log
	//
	if (IO::DBG) {

		std::cout << "IO_DBG> Print all outputs: " << std::endl;

		for (auto const& port : data.netlist.outputs) {

			std::cout << "IO_DBG>  \"" << port << "\"";
			std::cout << std::endl;
		}
	}

	// 3) parse wires, line by line
	//
	while (std::getline(in, line)) {

		// skip all the irrelevant lines
		if (!(line.find("wire") != std::string::npos && line.find(";") != std::string::npos)) {
			continue;
		}
		// process all the relevant lines
		else {
			std::istringstream linestream(line);

			// drop "wire";
			linestream >> tmpstr;

			// parse the wire name, without the final ";"
			linestream >> tmpstr;
			tmpstr = tmpstr.substr(0, tmpstr.find(";"));

			data.netlist.wires.emplace_back(tmpstr);
		}
	}

	// reset file handler
	in.clear() ;
	in.seekg(0, in.beg);

	// dbg log
	//
	if (IO::DBG) {

		std::cout << "IO_DBG> Print all wires: " << std::endl;

		for (auto const& wire : data.netlist.wires) {

			std::cout << "IO_DBG>  \"" << wire << "\"";
			std::cout << std::endl;
		}
	}

	// 4) parse gates, line by line
	//
	Data::Gate new_gate;

	while (std::getline(in, line)) {

		// skip all the irrelevant lines
		if (!(line.find("),") != std::string::npos || line.find("));") != std::string::npos)) {
			continue;
		}
		// process all the relevant lines
		else {
			std::istringstream linestream(line);

			// differentiate the different lines
			//
			// 1st line has driving strength for cell
			if (line.find("_X") != std::string::npos) {

				// dbg
				//std::cout << "START: " << line << std::endl;

				// new_gate new instance
				new_gate = Data::Gate();

				// gate type
				linestream >> tmpstr;
				// gate name
				linestream >> new_gate.name;

				// find related cell
				auto iter = data.cells.find(tmpstr);
				if (iter != data.cells.end()) {
					new_gate.cell = &(iter->second);
				}
				// report error otherwise
				else {
					std::cout << "IO>  Error: the gate \"" << new_gate.name << "\" is of type \"" << tmpstr << "\"";
					std::cout << ", but this type is not covered in your cells.inputs and cells.outputs files ..." << std::endl;
					std::cout << "IO>  Exiting; check you library and cells.inputs/outputs files" << std::endl;
					exit(1);
				}
			}

			// all lines contains some output/input pin and its connectivity
			//
			// check all the output pins of the related cell
			for (auto const& pin : new_gate.cell->outputs) {

				if (line.find("." + pin + "(") != std::string::npos) {

					// memorize the output pin and the net/pins it's connected to
					//
					std::string::size_type pos_begin = line.find_last_of("(") + 1;
					std::string::size_type pos_end = line.find_first_of(")");

					// remove heading/trailing whitespaces, if any, using stream operation
					std::string connected;
					std::istringstream connected_stream(line.substr(pos_begin, pos_end - pos_begin));
					connected_stream >> connected;

					new_gate.outputs.insert(std::make_pair(
								pin,
								connected
							));

					break;
				}
			}
			// check all the input pins of the related cell
			for (auto const& pin : new_gate.cell->inputs) {

				if (line.find("." + pin + "(") != std::string::npos) {

					// memorize the input pin and the net/pins it's connected to
					//
					std::string::size_type pos_begin = line.find_last_of("(") + 1;
					std::string::size_type pos_end = line.find_first_of(")");

					// remove heading/trailing whitespaces, if any, using stream operation
					std::string connected;
					std::istringstream connected_stream(line.substr(pos_begin, pos_end - pos_begin));
					connected_stream >> connected;

					new_gate.inputs.insert(std::make_pair(
								pin,
								connected
							));

					break;
				}
			}

			// final line has "));"
			if (line.find("));") != std::string::npos) {

				// dbg
				//std::cout << "STOP: " << line << std::endl;

				// memorize the gate
				data.netlist.gates.emplace_back(new_gate);
			}
		}
	}

	// finally, close file
	in.close();

	// dbg log
	//
	if (IO::DBG) {

		std::cout << "IO_DBG> Print all gates: " << std::endl;

		for (Data::Gate const& gate : data.netlist.gates) {

			std::cout << "IO_DBG>  \"" << gate.cell->type << "\" \"" << gate.name << "\"";

			std::cout << " OUT = ( ";
			for (auto const& output : gate.outputs) {
				std::cout << "\"" << output.first << "\": \"" << output.second << "\" ";
			}
			std::cout << ")";
			std::cout << " IN = ( ";
			for (auto const& input : gate.inputs) {
				std::cout << "\"" << input.first << "\": \"" << input.second << "\" ";
			}
			std::cout << ")";

			std::cout << std::endl;
		}
	}

	std::cout << "IO> Done" << std::endl;
	std::cout << "IO>  Input ports: " << data.netlist.inputs.size() << std::endl;
	std::cout << "IO>  Output ports: " << data.netlist.outputs.size() << std::endl;
	std::cout << "IO>  Wires: " << data.netlist.wires.size() << std::endl;
	std::cout << "IO>  Gates: " << data.netlist.gates.size() << std::endl;
	std::cout << "IO> " << std::endl;

	// sanity checks
	if (data.netlist.inputs.empty() || data.netlist.outputs.empty()) {
		std::cout << "IO> No ports found; exiting ..." << std::endl;
		std::cout << "IO> Check your netlist file \"" << data.files.in_netlist << "\"" << std::endl;
		exit(1);
	}
	if (data.netlist.wires.empty()) {
		std::cout << "IO> No wires found; exiting ..." << std::endl;
		std::cout << "IO> Check your netlist file \"" << data.files.in_netlist << "\"" << std::endl;
		exit(1);
	}
	if (data.netlist.gates.empty()) {
		std::cout << "IO> No gates found; exiting ..." << std::endl;
		std::cout << "IO> Check your netlist file \"" << data.files.in_netlist << "\"" << std::endl;
		exit(1);
	}
};

void IO::writeNetlist(Data& data, double const& HD, unsigned const& iterations, bool scramble) {
	std::ofstream out;
	std::string out_file;
	// for scrambling
	std::unordered_set<std::string> random_names_already_taken;
	std::vector<std::string> wires;
	std::vector<Data::Gate> gates;

	// try to set locale
	try {
		out.imbue(std::locale(std::string(Data::LOCALE).c_str()));
	}
	catch (std::runtime_error) {
		std::cout << "IO>" << std::endl;
		std::cout << "IO> Warning: failed to set locale \"" << Data::LOCALE << "\" ..." << std::endl;
		std::cout << "IO>" << std::endl;
	}

	// derive output file name
	//
	// drop folder from input file
	std::string::size_type start_pos = data.files.in_netlist.find_last_of('/');
	// skip "/" character, if any
	if (start_pos == std::string::npos) {
		start_pos = 0;
	}
	else {
		start_pos++;
	}
	// assume that file type/suffice is 2 characters long (".v")
	out_file = data.files.in_netlist.substr(start_pos, data.files.in_netlist.length() - start_pos - 2);
	out_file += "_rand_HD_";
	out_file += std::to_string(HD);
	if (scramble) {
		out_file += "_scrambled";
	}
	out_file += ".v";

	if (scramble) {
		std::cout << "Randomize>  Writing out randomized and scrambled netlist \"" << out_file << "\" ..." << std::endl;
	}
	else {
		std::cout << "Randomize>  Writing out randomized netlist \"" << out_file << "\" ..." << std::endl;
	}

	out.open(out_file.c_str());

	if (scramble) {
		out << "// Randomized and scrambled netlist" << std::endl;
	}
	else {
		out << "// Randomized netlist" << std::endl;
	}
	out << "//" << std::endl;
	out << "// Parameter ``netlist.v'': " << data.files.in_netlist << std::endl;
	out << "// Parameter ``cells.inputs'': " << data.files.cells_inputs << std::endl;
	out << "// Parameter ``cells.outputs'': " << data.files.cells_outputs << std::endl;
	out << "// Parameter ``cells.functions'': " << data.files.cells_functions << std::endl;
	out << "// Parameter ``also_output_scrambled_netlists'': " << data.parameters.also_output_scrambled_netlists << std::endl;
	out << "// Parameter ``threads'': " << data.parameters.threads << std::endl;
	out << "// Parameter ``intermediate_output_HD_step'': " << data.parameters.intermediate_output_HD_step << std::endl;
	out << "// Parameter ``HD_target'': " << data.parameters.HD_target << std::endl;
	out << "// Parameter ``acceptance_ratio'': " << data.parameters.acceptance_ratio << std::endl;
	out << "// Parameter ``HD_sampling_iterations'': " << data.parameters.HD_sampling_iterations << std::endl;
	out << "// Parameter ``consider_fanout'': " << data.parameters.consider_fanout << std::endl;
	out << "// Parameter ``consider_driving_strength'': " << data.parameters.consider_driving_strength << std::endl;
	out << "// Parameter ``random_op'': " << data.parameters.random_op << std::endl;
	out << "//  Replace underlying cell type: " << static_cast<unsigned>(Randomize::RandomOperation::ReplaceCell) << std::endl;
	out << "//  Swap outputs for a pair of gates: " << static_cast<unsigned>(Randomize::RandomOperation::SwapOutputs) << std::endl;
	out << "//  Swap inputs for a pair of gates: " << static_cast<unsigned>(Randomize::RandomOperation::SwapInputs) << std::endl;
	out << "//  Delete a gate: " << static_cast<unsigned>(Randomize::RandomOperation::DeleteGate) << std::endl;
	out << "//  Insert a gate: " << static_cast<unsigned>(Randomize::RandomOperation::InsertGate) << std::endl;
	out << "//  Randomly any of the above operation: " << Data::parameters::DEFAULT_RANDOM_OP << std::endl;
	out << "//" << std::endl;
	out << "//" << std::endl;
	out << "// Achieved the following HD for this randomized netlist: " << HD << std::endl;
	out << "//" << std::endl;
	out << "// Randomization iterations: " << iterations << std::endl;
	out << "//" << std::endl;
	out << "// Replaced cell type for that many gates: " << data.netlist_modifications.replacedCells << std::endl;
	out << "// Swapped outputs for that many pairs of gates: " << data.netlist_modifications.swappedOutputs << std::endl;
	out << "// Swapped inputs for that many pairs of gates: " << data.netlist_modifications.swappedInputs << std::endl;
	out << "// Deleted that many gates: " << data.netlist_modifications.deletedGates << std::endl;
	out << "// Inserted that many gates: " << data.netlist_modifications.insertedGates << std::endl;
	out << "//" << std::endl;
	out << std::endl;

	if (scramble) {
		out << "// port names and their order remain as is; they are not scrambled" << std::endl;
	}
	out << "module " << data.netlist.module_name << "_rand (" << std::endl;

	// output all inputs for module ports
	for (auto const& input : data.netlist.inputs) {
		out << input << "," << std::endl;
	}

	// output all outputs for module ports
	for (unsigned i = 1; i <= data.netlist.outputs.size(); i++) {

		// the last output has no comma following, and it also closes the port list
		if (i == data.netlist.outputs.size()) {
			out << data.netlist.outputs[i - 1] << ");" << std::endl;
		}
		else {
			out << data.netlist.outputs[i - 1] << "," << std::endl;
		}
	}
	out << std::endl;

	// output all inputs
	if (scramble) {
		out << "// port names and their order remain as is; they are not scrambled" << std::endl;
	}
	for (auto const& input : data.netlist.inputs) {
		out << "input " << input << ";" << std::endl;
	}
	out << std::endl;

	// output all outputs
	if (scramble) {
		out << "// port names and their order remain as is; they are not scrambled" << std::endl;
	}
	for (auto const& output : data.netlist.outputs) {
		out << "output " << output << ";" << std::endl;
	}
	out << std::endl;

	// for scrambling, keep track of already used random names
	//
	// also consider the names already taken for some random operations on the netlist; we're working on a local copy such that the
	// random names generated and used for this call to IO::writeNetlist are not memorized within
	// data.netlist.random_names_already_taken as this would remain throughout the remaining program flow -- in other words, only the
	// random names already fixed in the netlist should be considered, all other names are free to be used
	//
	if (scramble) {

		random_names_already_taken = data.netlist.random_names_already_taken;

		// also back-up original wires/gates such that they can be restored after
		wires = data.netlist.wires;
		gates = data.netlist.gates;
	}

	// if needed, scramble wire name, and rewrite all affected gate inputs/outputs
	//
	// directly work on data.netlist.wires data, which is restored later on
	//
	if (scramble) {

		for (auto& wire : data.netlist.wires) {
			std::string orig_name;

			// memorize old, original name
			orig_name = std::move(wire);

			// replace with new random name
			wire = Randomize::randName(random_names_already_taken);

			// also rewrite all affected gate inputs/outputs
			for (auto& gate : data.netlist.gates) {

				for (auto& input : gate.inputs) {
					if (input.second == orig_name) {
						input.second = wire;
					}
				}

				for (auto& output : gate.outputs) {
					if (output.second == orig_name) {
						output.second = wire;
					}
				}
			}
		}

		// finally, also shuffle order of wires
		std::random_shuffle(data.netlist.wires.begin(), data.netlist.wires.end());

		// log note on scrambled wires
		out << "// wire names and their order is scrambled" << std::endl;
	}
	// output wires, scrambled or not
	for (auto const& wire : data.netlist.wires) {
		out << "wire " << wire << ";" << std::endl;
	}
	out << std::endl;

	// if needed, restore original wires
	if (scramble) {
		data.netlist.wires = std::move(wires);
	}

	// if needed, scramble gates names
	//
	// directly work on data.netlist.gates data, which is restored later on
	if (scramble) {

		for (auto& gate : data.netlist.gates) {
			gate.name = Randomize::randName(random_names_already_taken);
		}

		// also shuffle order of gates
		std::random_shuffle(data.netlist.gates.begin(), data.netlist.gates.end());

		// log note on scrambled gates
		out << "// gate names and their order is scrambled" << std::endl;
	}

	// output gates, scrambled or not
	for (auto const& gate: data.netlist.gates) {
		unsigned outputs_remaining = gate.outputs.size();
		unsigned inputs_remaining = gate.inputs.size();

		out << gate.cell->type << " ";
		out << gate.name << " ";

		out << "(";

		for (auto const& input : gate.inputs) {

			// corner case: no outputs are present and this is the last input, so end port mapping already here
			if (outputs_remaining == 0 && inputs_remaining == 1) {
				out << "." << input.first << "(" << input.second << ")";
			}
			else {
				out << "." << input.first << "(" << input.second << "), ";
				inputs_remaining--;
			}
		}

		// only one output
		if (outputs_remaining == 1) {
			out << "." << gate.outputs.begin()->first << "(" << gate.outputs.begin()->second << ")";
		}
		// multiple outputs (or zero outputs)
		else {
			for (auto const& output : gate.outputs) {

				if (outputs_remaining == 1) {
					out << "." << output.first << "(" << output.second << ")";
				}
				else {
					out << "." << output.first << "(" << output.second << "), ";
					outputs_remaining--;
				}
			}
		}

		out << ");" << std::endl;
	}
	out << std::endl;

	// if needed, restore original gates
	if (scramble) {
		data.netlist.gates = std::move(gates);
	}

	out << "endmodule" << std::endl;

	out.close();

	std::cout << "Randomize>  Done" << std::endl;
	std::cout << "Randomize>" << std::endl;
}
