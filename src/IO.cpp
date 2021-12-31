
#include "IO.hpp"
#include "Data.hpp"
#include "Main.hpp"

// TODO use regex for various parsing/matching tasks instead of find, find_first_of, find_last_of
//

// parse program parameters and test files
void IO::parseParametersFiles(Data& data, int const& argc, char** argv) {
	int arg_index;

	// parse binary name
	data.binary = argv[0];

	// print help if not all mandatory parameters are provided
	if (argc < 6) {
		IO::printHelp(data);
		exit(1);
	}

	// read in mandatory parameters
	data.files.in_netlist = argv[1];
	data.files.golden_netlist = argv[2];
	data.files.cells_inputs = argv[3];
	data.files.cells_outputs = argv[4];
	data.files.cells_functions = argv[5];

	// test input files
	IO::testFile(data.files.in_netlist);
	IO::testFile(data.files.golden_netlist);
	IO::testFile(data.files.cells_inputs);
	IO::testFile(data.files.cells_outputs);
	IO::testFile(data.files.cells_functions);

	// read in all optional arguments
	//
	arg_index = 6;
	while (arg_index < argc) {

		std::string arg_string = argv[arg_index];
		std::string::size_type arg_start = arg_string.find('=');

		// sanity check whether argument can be parsed
		if ((arg_string != "--help") && (arg_start == std::string::npos)) {
			std::cout << "IO> Error: the following argument cannot be parsed as optional parameter: \"" << arg_string << "\"" << std::endl;
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
		else if (parameter == "--HD_sampling_iterations") {
			try {
				data.parameters.HD_sampling_iterations = std::stoi(value);
			}
			catch (std::invalid_argument) {
				std::cout << "IO> Error: provide an integer value for the optional parameter ``HD_sampling_iterations''; currently provided: \"" << value << "\"" << std::endl;
				exit(1);
			}
		}
		else if (parameter == "--lazy_Boolean_evaluation") {
			if (value == "1" || value == "true") {
				data.parameters.lazy_Boolean_evaluation = true;
			}
			else if (value == "0" || value == "false") {
				data.parameters.lazy_Boolean_evaluation = false;
			}
			else {
				std::cout << "IO> Error: provide a Boolean value for the optional parameter ``lazy_Boolean_evaluation''; currently provided: \"";
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
	std::cout << "IO> Parameter ``golden_netlist'': " << data.files.golden_netlist << std::endl;
//	std::cout << "IO> Parameter ``cells.inputs'': " << data.files.cells_inputs << std::endl;
//	std::cout << "IO> Parameter ``cells.outputs'': " << data.files.cells_outputs << std::endl;
//	std::cout << "IO> Parameter ``cells.functions'': " << data.files.cells_functions << std::endl;
	std::cout << "IO> Parameter ``threads'': " << data.parameters.threads << std::endl;
	std::cout << "IO> Parameter ``HD_sampling_iterations'': " << data.parameters.HD_sampling_iterations << std::endl;
	std::cout << "IO> Parameter ``lazy_Boolean_evaluation'': " << data.parameters.lazy_Boolean_evaluation << std::endl;
	std::cout << "IO> " << std::endl;
};

void IO::printHelp(Data const& data) {
	std::cout << "IO> Usage: " << data.binary << " netlist.v golden_netlist.v cells.inputs cells.outputs cells.functions [";
	std::cout << "--threads=" << data.parameters.threads << " ";
	std::cout << "--HD_sampling_iterations=" << data.parameters.HD_sampling_iterations << " ";
	std::cout << "--lazy_Boolean_evaluation=" << data.parameters.lazy_Boolean_evaluation << "]" << std::endl;
	std::cout << "IO> " << std::endl;
	std::cout << "IO> Mandatory parameter ``netlist.v'': Netlist to evaluate against golden netlist" << std::endl;
	std::cout << "IO> Mandatory parameter ``golden_netlist.v'': Golden or reference netlist for HD evaluation" << std::endl;
	std::cout << "IO> Mandatory parameter ``cells.inputs'': Cells and their inputs" << std::endl;
	std::cout << "IO> Mandatory parameter ``cells.outputs'': Cells and their outputs" << std::endl;
	std::cout << "IO> Mandatory parameter ``cells.functions'': Cells and their output functions" << std::endl;
	std::cout << "IO> Optional parameter ``threads'': Threads for parallel runs; default value: " << data.parameters.threads << std::endl;
	std::cout << "IO> Optional parameter ``HD_sampling_iterations'': Iterations for HD evaluation; default value: " << data.parameters.HD_sampling_iterations << std::endl;
	std::cout << "IO> Optional parameter ``lazy_Boolean_evaluation'': parsing of Boolean strings is short-cut whenever possible during the HD evaluation, but in case there are unsupported or erroneous parts in the Boolean function strings, the HD values will be wrong -- it's recommend to use this feature only once regular evaluation was tried, which reports on such errors; default value: " << data.parameters.lazy_Boolean_evaluation << std::endl;
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
	std::cout << "IO> Parsing cells and their inputs ...";
	std::cout << std::endl;

	while (std::getline(in, line)) {

		Data::Cell cell;
		std::istringstream linestream(line);

		// first is cell type
		linestream >> cell.type;

		// memorize type and driving strength separately; first ensure that the regexp for cell and driving strength matches
		//
		if (std::regex_match(cell.type, IO::REGEX_CELL_DRIVING_STRENGTH)) {

			// there could be multiple "X" in the type, e.g., for XOR2_X1, so consider only the last occurrence
			std::string::size_type start_pos = cell.type.find_last_of('X');

			// in case there's a '_' right before the 'X', drop that '_' from the type
			if (cell.type[start_pos - 1] == '_') {
				cell.type_wo_strength = cell.type.substr(0, start_pos - 1);
			}
			else {
				cell.type_wo_strength = cell.type.substr(0, start_pos);
			}

			// finally, try to parse strength as integer; skip the 'X'
			try {
				cell.strength = std::stoi(cell.type.substr(start_pos + 1));
			}
			catch (std::invalid_argument) {
				std::cout << "IO>  Error -- the driving strength of the following cell could not been parsed: ";
				std::cout << "\"" << cell.type << "\" -- this should not happen, check your cell files!" << std::endl;
				exit(1);
			}
		}
		// if regexp fails, the cell and its driving strength could not be parsed
		else {
			std::cout << "IO>  Error -- the driving strength of the following cell could not been parsed: ";
			std::cout << "\"" << cell.type << "\" -- this should not happen, check your cell files!" << std::endl;
			exit(1);
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

	std::cout << "IO> Parsing cells and their outputs ...";
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

		// sanity check on zero outputs
		if (cell.outputs.empty()) {
			std::cout << "IO>  Error -- the following cell has no outputs: \"" << cell.type << "\"" << std::endl;
			exit(1);
		}

		// update existing cells or log warning
		//
		auto const iter = data.cells.find(cell.type);
		if (iter != data.cells.end()) {
			iter->second.outputs = cell.outputs;
		}
		else {
			std::cout << "IO>  Error -- the following cell has not been seen during the previous parsing operation: ";
			std::cout << "\"" << cell.type << "\" -- this should not happen, check your cell files for consistency!" << std::endl;
			exit(1);
		}
	}
	
	// close outputs file
	in.close();

	// 3) functions
	in.open(data.files.cells_functions.c_str());

	std::cout << "IO> Parsing cells and their functions ...";
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
				//std::cout << " (" << iter->second.type_wo_strength << "; " << iter->second.strength << ")";
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
				std::cout << "IO>  Error -- the following cell has a different number of outputs and output functions: ";
				std::cout << "\"" << iter->second.type << "\" -- this should not happen, check your cell files for consistency!" << std::endl;
				exit(1);

			}

			// cross check actual functions with output pins
			for (auto const& output : iter->second.outputs) {

				if (iter->second.functions.find(output) == iter->second.functions.end()) {
					std::cout << "IO>  Error -- the following cell is missing a function for an output pin: ";
					std::cout << "\"" << iter->second.type << "\" -- this should not happen, check your cell files for consistency!" << std::endl;
					exit(1);
				}
			}
		}
		else {
			std::cout << "IO>  Error -- the following cell has not been seen during the previous parsing operation: ";
			std::cout << "\"" << cell.type << "\" -- this should not happen, check your cell files for consistency!" << std::endl;
			exit(1);
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

			std::cout << "IO_DBG>  \"" << cell.type << "\"";
			std::cout << " (strength: " << cell.strength << ", type_wo_strength: \"" << cell.type_wo_strength << "\") --";

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

void IO::parseNetlist(std::unordered_map<std::string, Data::Cell> const& cells, Data::Netlist& netlist, std::string const& file) {
	std::ifstream in;
	std::string line;
	std::string tmpstr;
	bool statement_done;

	in.open(file.c_str());

	std::cout << "IO> Parsing the netlist \"" << file << "\" ...";
	std::cout << std::endl;

	// 0) parse the module name
	while (std::getline(in, line)) {

		// skip all the irrelevant lines
		if (!(line.find("module ") != std::string::npos && line.find("end") == std::string::npos)) {
			continue;
		}
		// process the one relevant line
		else {
			std::istringstream linestream(line);

			// ensure that line does not begin with comment markup
			if (line.find("//") == 0) {
				continue;
			}

			// drop "module";
			linestream >> tmpstr;

			// parse the module name
			linestream >> tmpstr;

			netlist.module_name = tmpstr;
		}
	}

	// dbg log
	//
	if (IO::DBG) {
		std::cout << "IO_DBG> Print module name: " << netlist.module_name << std::endl;
	}
	
	// reset file handler
	in.clear();
	in.seekg(0, in.beg);
	// reset other flags
	statement_done = true;

	// 1) parse inputs, line by line
	//
	while (std::getline(in, line)) {

		// skip all the irrelevant lines
		if (statement_done && line.find("input ") == std::string::npos) {
			continue;
		}

		// process all the relevant lines
		else {
			std::istringstream linestream(line);

			// ensure that line does not begin with comment markup
			if (line.find("//") == 0) {
				continue;
			}

			// drop "input" for very first line
			if (statement_done) {
				linestream >> tmpstr;
			}

			// then, any number of inputs can follow
			while (!linestream.eof()) {

				linestream >> tmpstr;

				// store input
				netlist.inputs.emplace_back(
						// drop last character (could/should be "," or ";")
						tmpstr.substr(0, tmpstr.length() - 1)
					);

				// check for ";" which declares that it's the final input
				if (tmpstr.find(";") != std::string::npos) {
					statement_done = true;
					break;
				}
				// otherwise, the statement continues and, for the end of line, the next line will be considered as well
				else {
					statement_done = false;
				}
			}
		}
	}

	// dbg log
	//
	if (IO::DBG) {

		std::cout << "IO_DBG> Print all inputs: " << std::endl;

		for (auto const& port : netlist.inputs) {

			std::cout << "IO_DBG>  \"" << port << "\"";
			std::cout << std::endl;
		}
	}
	
	// reset file handler
	in.clear();
	in.seekg(0, in.beg);
	// reset other flags
	statement_done = true;

	// 2) parse outputs, line by line
	//
	while (std::getline(in, line)) {

		// skip all the irrelevant lines
		if (statement_done && line.find("output ") == std::string::npos) {
			continue;
		}
		// process all the relevant lines
		else {
			std::istringstream linestream(line);

			// ensure that line does not begin with comment markup
			if (line.find("//") == 0) {
				continue;
			}

			// drop "output" for very first line
			if (statement_done) {
				linestream >> tmpstr;
			}

			// then, any number of outputs can follow
			while (!linestream.eof()) {

				linestream >> tmpstr;

				// store output
				netlist.outputs.emplace_back(
						// drop last character (could/should be "," or ";")
						tmpstr.substr(0, tmpstr.length() - 1)
					);

				// check for ";" which declares that it's the final output
				if (tmpstr.find(";") != std::string::npos) {
					statement_done = true;
					break;
				}
				// otherwise, the statement continues and, for the end of line, the next line will be considered as well
				else {
					statement_done = false;
				}
			}
		}
	}

	// dbg log
	//
	if (IO::DBG) {

		std::cout << "IO_DBG> Print all outputs: " << std::endl;

		for (auto const& port : netlist.outputs) {

			std::cout << "IO_DBG>  \"" << port << "\"";
			std::cout << std::endl;
		}
	}
	
	// reset file handler
	in.clear();
	in.seekg(0, in.beg);
	// reset other flags
	statement_done = true;

	// 3) parse wires, line by line
	//
	while (std::getline(in, line)) {

		// skip all the irrelevant lines
		if (statement_done && line.find("wire ") == std::string::npos) {
			continue;
		}
		// process all the relevant lines
		else {
			std::istringstream linestream(line);

			// ensure that line does not begin with comment markup
			if (line.find("//") == 0) {
				continue;
			}

			// drop "wire" for very first line
			if (statement_done) {
				linestream >> tmpstr;
			}

			// then, any number of wires can follow
			while (!linestream.eof()) {

				linestream >> tmpstr;

				// store wire
				netlist.wires.emplace_back(
						// drop last character (could/should be "," or ";")
						tmpstr.substr(0, tmpstr.length() - 1)
					);

				// check for ";" which declares that it's the final wire
				if (tmpstr.find(";") != std::string::npos) {
					statement_done = true;
					break;
				}
				// otherwise, the statement continues and, for the end of line, the next line will be considered as well
				else {
					statement_done = false;
				}
			}
		}
	}

	// dbg log
	//
	if (IO::DBG) {

		std::cout << "IO_DBG> Print all wires: " << std::endl;

		for (auto const& wire : netlist.wires) {

			std::cout << "IO_DBG>  \"" << wire << "\"";
			std::cout << std::endl;
		}
	}

	// reset file handler
	in.clear();
	in.seekg(0, in.beg);
	// reset other flags
	statement_done = true;

	// 4) parse gates, line by line
	//
	Data::Gate new_gate;

	while (std::getline(in, line)) {

		// skip all the irrelevant lines
		if (statement_done && !std::regex_search(line, IO::REGEX_CELL_DRIVING_STRENGTH)) {
			continue;
		}
		// process all the relevant lines
		else {
			std::istringstream linestream(line);

			// ensure that line does not begin with comment markup
			if (line.find("//") == 0) {
				continue;
			}

			// parse gate name and type from very first line
			if (statement_done) {

				//// dbg
				//std::cout << "START: " << line << std::endl;

				// new gate instance
				new_gate = Data::Gate();

				// gate type
				std::string gate_type;
				linestream >> gate_type;
				
				// gate name, initialize w/ prefix to avoid name clashes w/ PI/PO or wire nodes, which
				// may have same names
				new_gate.name = "_gate__";

				// parse and update gate name
				linestream >> tmpstr;
				new_gate.name += tmpstr;

				// find related cell
				auto iter = cells.find(gate_type);
				if (iter != cells.end()) {
					new_gate.cell = &(iter->second);
				}
				// report error otherwise
				else {
					std::cout << "IO>  Error: the gate \"" << new_gate.name << "\" is of type \"" << gate_type << "\"";
					std::cout << ", but this type is not covered in your cells.inputs and cells.outputs files ..." << std::endl;
					std::cout << "IO>  Check your library and cells.inputs/outputs files" << std::endl;
					exit(1);
				}
			}

			// all lines contains some output/input pin and its connectivity
			//

			// check all the output pins of the related cell
			for (auto const& pin : new_gate.cell->outputs) {

				std::string pin_ = "." + pin;

				if (line.find(pin_) != std::string::npos) {

					// memorize the output pin and the net/pins it's connected to
					//
					std::string::size_type pos_begin = line.find_first_of('(', line.find(pin_) + pin_.length()) + 1;
					std::string::size_type pos_end = line.find_first_of(')', pos_begin);

					// remove heading/trailing whitespaces, if any, using stream operation
					std::string connected;
					std::istringstream connected_stream(line.substr(pos_begin, pos_end - pos_begin));
					connected_stream >> connected;

					new_gate.outputs.insert(std::make_pair(
								pin,
								connected
							));
				}
			}

			// check all the input pins of the related cell
			for (auto const& pin : new_gate.cell->inputs) {

				std::string pin_ = "." + pin;

				if (line.find(pin_) != std::string::npos) {

					// memorize the input pin and the net/pins it's connected to
					//
					std::string::size_type pos_begin = line.find_first_of('(', line.find(pin_) + pin_.length()) + 1;
					std::string::size_type pos_end = line.find_first_of(')', pos_begin);

					// remove heading/trailing whitespaces, if any, using stream operation
					std::string connected;
					std::istringstream connected_stream(line.substr(pos_begin, pos_end - pos_begin));
					connected_stream >> connected;

					new_gate.inputs.insert(std::make_pair(
								pin,
								connected
							));
				}
			}

			// final line has ");"
			if (line.find(");") != std::string::npos) {

				//// dbg
				//std::cout << "STOP: " << line << std::endl;

				// memorize the gate
				netlist.gates.emplace_back(new_gate);

				statement_done = true;
			}
			// otherwise, the statement continues and the next line will be considered as well
			else {
				statement_done = false;
			}
		}
	}

	// dbg log
	//
	if (IO::DBG) {

		std::cout << "IO_DBG> Print all gates: " << std::endl;

		for (Data::Gate const& gate : netlist.gates) {

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
	
	// reset file handler
	in.clear();
	in.seekg(0, in.beg);
	// reset other flags
	statement_done = true;

	// 5) parse assignments, if any, line by line
	//
	while (std::getline(in, line)) {

		// skip all the irrelevant lines
		if (!(line.find("assign ") != std::string::npos && line.find(';') != std::string::npos)) {
			continue;
		}
		// process all the relevant lines
		else {
			std::istringstream linestream(line);
			std::string PO, PI;

			// ensure that line does not begin with comment markup
			if (line.find("//") == 0) {
				continue;
			}

			// drop "assign";
			linestream >> tmpstr;

			// parse the PO name
			linestream >> PO;

			// drop "=";
			linestream >> tmpstr;

			// parse the PI name
			linestream >> PI;

			netlist.assignments.insert( std::make_pair(PO, PI) );
		}
	}

	// dbg log
	//
	if (IO::DBG) {

		std::cout << "IO_DBG> Print all assignments: " << std::endl;

		for (auto const& assignment : netlist.assignments) {

			std::cout << "IO_DBG>  \"" << assignment.first << "\" = \"" << assignment.second << "\"";
			std::cout << std::endl;
		}
	}

	// finally, close file
	in.close();

	std::cout << "IO> Done" << std::endl;
	std::cout << "IO>  Input ports: " << netlist.inputs.size() << std::endl;
	std::cout << "IO>  Output ports: " << netlist.outputs.size() << std::endl;
	std::cout << "IO>  Wires: " << netlist.wires.size() << std::endl;
	std::cout << "IO>  Gates: " << netlist.gates.size() << std::endl;
	std::cout << "IO>  PO-to-PI assignments: " << netlist.assignments.size() << std::endl;
	std::cout << "IO> " << std::endl;

	// sanity checks
	if (netlist.inputs.empty() || netlist.outputs.empty()) {
		std::cout << "IO> No ports found; exiting ..." << std::endl;
		std::cout << "IO> Check your netlist file \"" << file << "\"" << std::endl;
		exit(1);
	}
	if (netlist.wires.empty()) {
		std::cout << "IO> No wires found; exiting ..." << std::endl;
		std::cout << "IO> Check your netlist file \"" << file << "\"" << std::endl;
		exit(1);
	}
	if (netlist.gates.empty()) {
		std::cout << "IO> No gates found; exiting ..." << std::endl;
		std::cout << "IO> Check your netlist file \"" << file << "\"" << std::endl;
		exit(1);
	}
};
