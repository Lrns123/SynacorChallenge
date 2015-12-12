#include <iostream>
#include <cstdlib>
#include <thread>
#include <atomic>
#include <vector>

namespace
{
    std::atomic<unsigned short> nextNumber(1);
    std::atomic<unsigned short> goalNumber(0xFFFF);
    std::atomic<bool> finished(false);
}


/*
VM Bytecode:

156b: set R0 4
156e: set R1 1
1571: call 178b
1573: eq R1 R0 6
....
178b: jt R0 1793
178e: add R0 R1 1
1792: ret
1793: jt R1 17a0
1796: add R0 R0 7fff
179a: set R1 R7
179d: call 178b
179f: ret
17a0: push R0
17a2: add R1 R1 7fff
17a6: call 178b
17a8: set R1 R0
17ab: pop R0
17ad: add R0 R0 7fff
17b1: call 178b
17b3: ret


C Translation:
static unsigned short r7;
unsigned short runRecursive(unsigned short r0, unsigned short r1)
{
    // [1]
    if (r0 == 0)
        return (r1 + 1) % 32768;
        
    // [2]
    if (r1 == 0)
        return runRecursive(r0 - 1, r7);

    // [3]
    return runRecursive(r0 - 1, runRecursive(r0, r1 - 1));
}
*/

// Linearized version of the above recursive implementation.
// This performs the above algorithm in reverse order (starting at r0 = 0, and r1 = 0 and working its way up),
//  thereby avoiding the need of recursion, at the cost of needing 'a lot' of memory.
unsigned short runLinear(unsigned short r0, unsigned short r1, unsigned short r7)
{
    auto table = std::unique_ptr<unsigned short[]>(new unsigned short[32768 * r0 + r1 + 1]);
    
    // [1]
    for (unsigned i = 0, n = 32768; i != n; ++i)
        table[i] = (i + 1) % 32768;


    for (unsigned i = 1, n = r0 + 1; i != n; ++i)
    {
        // [2]
        table[i << 15] = table[(i - 1) << 15 | r7];

        // [3]
        for (unsigned j = 1, m = i == r0 ? r1 + 1: 32768; j != m; ++j)
            table[i << 15 | j] = table[(i - 1) << 15 | table[i << 15 | (j - 1)]];
    }

    return table[r0 << 15 | r1];
}

int main(int argc, char ** argv)
{
    nextNumber = argc > 1 ? atoi(argv[1]) : 1;
    std::vector<std::thread> threads;
    
    std::cout << "Synacor Challenge R7 (teleporter) bruteforcer." << std::endl;    
    std::cout << "Starting at " << nextNumber << " using " << std::thread::hardware_concurrency() << " threads." << std::endl;

    // Spawn worker threads
    for (unsigned i = 0; i != std::thread::hardware_concurrency(); ++i)
    {
        threads.emplace_back([]()
        {
            while (!finished)
            {
                int num = nextNumber++;
                if (num > 32767)
                    finished = true;
                else if (runLinear(4, 1, num) == 6)
                {
                    finished = true;
                    goalNumber = num;
                }
            }
        });
    }

    // Show progress
    while (!finished)
    {
        std::cout << '\r' << nextNumber;
        std::this_thread::yield();
    }

    std::cout << '\r';

    for (auto &thread : threads)
        thread.join();

    std::cout << "Bruteforce completed!" << std::endl;

    if (goalNumber != 0xFFFF)
        std::cout << "R7 should be set to " << goalNumber << '.' << std::endl;
    else
        std::cout << "R7 could not be bruteforced :(." << std::endl;

    return 0;
}