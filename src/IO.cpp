
#include "IO.hpp"
#include "Data.hpp"

// parse program parameters and test files
void IO::parseParametersFiles(Data& data, int const& argc, char** argv) {

	// print command-line parameters
	if (argc < 6) {
		std::cout << "IO> Usage: " << argv[0] << " netlist.v cells.inputs cells.outputs out.v [threads]" << std::endl;
		std::cout << "IO> " << std::endl;
		std::cout << "IO> Mandatory parameter ``netlist.v'': Netlist to be randomized" << std::endl;
		std::cout << "IO> Mandatory parameter ``cells.inputs'': All cells and all their inputs" << std::endl;
		std::cout << "IO> Mandatory parameter ``cells.outputs'': All cells and all their outputs" << std::endl;
		std::cout << "IO> Mandatory parameter ``cells.functions'': All cells and all their output functions" << std::endl;
		std::cout << "IO> Mandatory parameter ``out.v'': Randomized netlist" << std::endl;
		std::cout << "IO> Optional parameter ``threads'': Threads for parallel runs" << std::endl;
		exit(1);
	}

	// read in mandatory parameters
	data.files.in_netlist = argv[1];
	data.files.cells_inputs = argv[2];
	data.files.cells_outputs = argv[3];
	data.files.cells_functions = argv[4];
	data.files.out_netlist = argv[5];

	// read in optional arguments
	if (argc == 7) {
		data.threads = std::stoi(argv[6]);
	}

	// test input files
	IO::testFile(data.files.in_netlist);
	IO::testFile(data.files.cells_inputs);
	IO::testFile(data.files.cells_outputs);
	IO::testFile(data.files.cells_functions);
};

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

		// then, any number of I/O pins can follow
		while (!linestream.eof()) {

			linestream >> tmpstr;
			cell.inputs.emplace_back(tmpstr);
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
			cell.outputs.emplace_back(tmpstr);
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
		bool stop = false;
		std::string function;

		// first is cell type
		linestream >> cell.type;

		// then, any number of output functions can follow
		//
		// functions are encapsulated within " ", so parse for beginning and end "
		//
		while (!linestream.eof()) {

			linestream >> tmpstr;

			// current word is start, end, or both start and end of new function
			if (tmpstr.find('"') != std::string::npos) {

				// start, init string
				if (start) {
					function = tmpstr;
					start = false;

					// start word is also stop word
					if (std::count(function.begin(), function.end(), '"') == 2) {
						stop = true;
					}
				}
				// second, independent occurrence of "; stop
				else if (!start) {
					stop = true;
				}

				// stop, finalize string
				if (stop) {

					// consider last word of function, but only for functions spread across multiple words, where
					// there's currently only one occurrence of "
					if (std::count(function.begin(), function.end(), '"') == 1) {
						function += " " + tmpstr;
					}

					//if (IO::DBG) {
					//	std::cout << "IO_DBG>  \"" << cell.type << "\" FUNCTION: \"" << function << "\"" << std::endl;
					//}

					// memorize function, but drop the " at the beginning and the end
					cell.functions.emplace_back(std::string(function.begin() + 1, function.end() - 1));

					// reset start/stop
					start = true;
					stop = false;
				}
			}
			else {
				// regular word in between, part of current function
				function += " " + tmpstr;
			}
		}

		// update existing cells or log warning
		//
		auto const iter = data.cells.find(cell.type);
		if (iter != data.cells.end()) {
			iter->second.functions = cell.functions;

			// cross check number of functions with number of output pins
			if (iter->second.functions.size() != iter->second.outputs.size()) {
				std::cout << "IO>  Warning -- the following cell has a different number of outputs and output functions: ";
				std::cout << "\"" << iter->second.type << "\" -- this should not happen, check your cell files for consistency!" << std::endl;

				if (IO::DBG) {
					std::cout << "IO_DBG>  \"" << iter->second.type << "\"";
					std::cout << " OUT = ( ";
					for (auto const& output : iter->second.outputs) {
						std::cout << "\"" << output << "\" ";
					}
					std::cout << ")";
					std::cout << " FUNCTIONS = ( ";
					for (auto const& function : iter->second.functions) {
						std::cout << "\"" << function << "\" ";
					}
					std::cout << ")" << std::endl;
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

			std::cout << "IO_DBG>  \"" << cell.type << "\"";

			std::cout << " OUT = ( ";
			for (unsigned i = 0; i < cell.outputs.size(); i++) {
				std::cout << "\"" << cell.outputs[i] << "\" = \"" << cell.functions[i] << "\" ";
			}
			std::cout << ")";
			std::cout << " IN = ( ";
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

			data.netlist.inputs.insert(tmpstr);
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

			data.netlist.outputs.insert(tmpstr);
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

			data.netlist.wires.insert(tmpstr);
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
	Data::Cell *cell = nullptr;

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

				// make that new_gate is a new instance
				new_gate = Data::Gate();

				// gate type
				linestream >> new_gate.type;
				// gate name
				linestream >> new_gate.name;

				// find related cell
				auto iter = data.cells.find(new_gate.type);
				if (iter != data.cells.end()) {
					cell = &(iter->second);
				}
				// report error otherwise
				else {
					std::cout << "IO>  Error: the gate \"" << new_gate.name << "\" is of type \"" << new_gate.type << "\"";
					std::cout << ", but this type is not covered in your cells.inputs and cells.outputs files ..." << std::endl;
					std::cout << "IO>  Exiting; check you library and cells.inputs/outputs files" << std::endl;
					exit(1);
				}
			}

			// all lines contains some output/input pin and its connectivity
			//
			// check all the output pins of the related cell
			for (auto const& pin : cell->outputs) {

				if (line.find("." + pin + "(") != std::string::npos) {

					// memorize the output pin and the net/pins it's connected to
					//
					size_t pos_begin = line.find_last_of("(") + 1;
					size_t pos_end = line.find_first_of(")");

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
			for (auto const& pin : cell->inputs) {

				if (line.find("." + pin + "(") != std::string::npos) {

					// memorize the input pin and the net/pins it's connected to
					//
					size_t pos_begin = line.find_last_of("(") + 1;
					size_t pos_end = line.find_first_of(")");

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

			std::cout << "IO_DBG>  \"" << gate.type << "\" \"" << gate.name << "\"";

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

void IO::writeNetlist(Data& data) {
	std::ofstream out;

	std::cout << "Randomize>" << std::endl;
	std::cout << "Randomize> Writing out netlist ..." << std::endl;

	out.open(data.files.out_netlist.c_str());

	out << "// Randomized netlist -- HD around TODO" << std::endl;
	out << "//" << std::endl;
	out << std::endl;
	out << "module " << data.netlist.module_name << " (" << std::endl;

	// output all inputs for module ports
	for (auto const& input : data.netlist.inputs) {
		out << input << "," << std::endl;
	}

	// output all outputs for module ports
	unsigned count = 1;
	for (auto const& output : data.netlist.outputs) {

		// the last output has no comma following, and it also closes the port list
		if (count == data.netlist.outputs.size()) {
			out << output << ");" << std::endl;
		}
		else {
			out << output << "," << std::endl;
			count++;
		}
	}
	out << std::endl;

	// output all inputs
	for (auto const& input : data.netlist.inputs) {
		out << "input " << input << ";" << std::endl;
	}
	out << std::endl;

	// output all outputs
	for (auto const& output : data.netlist.outputs) {
		out << "output " << output << ";" << std::endl;
	}
	out << std::endl;

	// output all wires
	for (auto const& wire : data.netlist.wires) {
		out << "wire " << wire << ";" << std::endl;
	}
	out << std::endl;

	// output all gates 
	for (auto const& gate: data.netlist.gates) {
		unsigned outputs_remaining = gate.outputs.size();
		unsigned inputs_remaining = gate.inputs.size();

		out << gate.type << " ";
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
	out << "endmodule" << std::endl;

	out.close();

	std::cout << "Randomize> Done" << std::endl;
	std::cout << "Randomize>" << std::endl;
}
