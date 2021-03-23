/* Argument parsing. I decided to use cxxopts, a lightweight parser:
 * 	https://github.com/jarro2783/cxxopts/
 *
 * I also tried using boost, but I found cxxopts' interface much more intuitive,
 * and its output was also much better. The only problem is the high compilation
 * time.
 */

#include <thread>
#include "args.h"
#include "cxxopts.hpp"

using namespace std;

#ifdef DEBUG
#define DEFAULT_NUM_THREADS 1
#else
#define DEFAULT_NUM_THREADS thread::hardware_concurrency()
#endif

size_t parse_memory(const string& s) {
	size_t i = 0;
	size_t value = stoi(s, &i);
	switch (s[i]) {
		case 'G':
			value *= 1024;
		case 'M':
			value *= 1024;
		case 'K':
			value *= 1024;
		case 0:
			break;
		default:
			throw cxxopts::OptionParseException("invalid memory: " + s);
	}
	return value;
}

bool Args::parse(int argc, char** argv) {
	// Fuck 80 chars limit.
	try {
		cxxopts::Options cmd("kvm-fuzz", "kvm-fuzz: fuzz x86_64 closed-source applications with hardware acceleration\n");
		cmd.add_options("Available")
			("j,jobs", "Number of threads to use", cxxopts::value<int>(jobs)->default_value(to_string(DEFAULT_NUM_THREADS)))
			("m,memory", "Virtual machine memory limit", cxxopts::value<string>()->default_value("8M"))
			("k,kernel", "Kernel path", cxxopts::value<string>(kernel_path)->default_value("./kernel/kernel"), "path")
			("i,input", "Input folder (initial corpus)", cxxopts::value<string>(input_dir)->default_value("./corpus"), "dir")
			("o,output", "Output folder (crashes)", cxxopts::value<string>(output_dir)->default_value("./crashes"), "dir")
			("f,file", "Memory loaded files for the target. Set once for each file, or as a list: -f file1,file2", cxxopts::value<vector<string>>(memory_files), "path")
			("b,basic-blocks", "Path to file containing a list of basic blocks for code coverage", cxxopts::value<string>(basic_blocks_path), "path")
			("binary", "File to run", cxxopts::value<string>(binary_path))
			("args", "Args passed to binary", cxxopts::value<vector<string>>(binary_argv))
			("h,help", "Print usage")
		;

		// Set positional arguments
		cmd.parse_positional({"binary", "args"});

		// Set custom usage help
		cmd.custom_help("[ options ]")
		   .positional_help("-- /path/to/fuzzed_binary [ args ]");

		auto options = cmd.parse(argc, argv);

		// Display help
		if (options.count("help") || !options.count("binary")) {
			cout << cmd.help() << endl;
			return false;
		}

		// Parse special arguments
		memory = parse_memory(options["memory"].as<string>());
		binary_argv.insert(binary_argv.begin(), binary_path);
		if (basic_blocks_path.empty()) {
			size_t pos = binary_path.rfind("/") + 1;
			string binary_name = binary_path.substr(pos == string::npos ? 0 : pos);
			basic_blocks_path = "./basic_blocks_" + binary_name + ".txt";
		}

	} catch (cxxopts::OptionException& e) {
		cout << "error: " << e.what() << endl;
		return false;
	}

	return true;
}