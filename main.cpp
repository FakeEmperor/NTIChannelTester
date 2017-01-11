#include <iostream>
#include "cmd.h"
#include "tests/config.h"
#if !TESTS_ACTIVE

/*
 std::vector<char*> lol = {
		"-d",
		"-io_source",
		"\"source.txt\"",
		"-in_encoded",
		"encoded.txt",
		"-io_noised",
		"noised.txt",
		"-in_decoded",
		"decoded.txt",
		"-out_report",
		"report.txt",
		"-max_source_size",
		"500",
		"-noise_levels",
		"0,0.9,0.1,0.6",
		"-num_sources",
		"2"
	};
*/

int main(int argc, char** argv)
{
	nti::cmd::NTICommandLine cmd;
	try
	{
		cmd.start(const_cast<const char**>(argv+1), argc-1);
	} catch (std::runtime_error &e) {
		std::cerr << "[ERROR]! " << e.what();
	}

	
}

#endif