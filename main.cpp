#include <iostream>
#include "cmd.h"
#include "tests/config.h"
#if !TESTS_ACTIVE

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