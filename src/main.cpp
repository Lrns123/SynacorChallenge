#include <iostream>
#include "VMDebugger.hpp"
#include "SynacorVM.hpp"

int main(int argc, char ** argv)
{
    try
    {
        if (argc < 2)
        {
            VMDebugger debugger;
            debugger.runShell();
        }
        else
        {
            SynacorVM vm;

            std::cout << "Loading binary... ";
            std::cout << vm.loadBinary(argv[1]) << " words" << std::endl;;

            std::cout << "Executing..." << std::endl << std::endl;
            vm.run();

            std::cout << std::endl << std::endl << "Execution completed..." << std::endl;
        }
    }
    catch (const std::exception &e)
    {
        std::cout << std::endl << " --- EXCEPTION ---" << std::endl << e.what() << std::endl;
        return 1;
    }
    catch (...)
    {
        std::cout << std::endl << " --- EXCEPTION ---" << std::endl << "<Unknown exception>" << std::endl;
        return 1;
    }

    return 0;
}