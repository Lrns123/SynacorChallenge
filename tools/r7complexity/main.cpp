#include <iostream>
#include <array>
#include <atomic>
#include <bitset>
#include <vector>
#include <thread>
#include <mutex>
#include <fstream>

namespace
{
    struct ComplexityData
    {
        unsigned long long recursiveIterations;
        unsigned long long memoizedIterations;
        unsigned long long linearIterations;
        unsigned short result;
    };

    std::array<ComplexityData, 32768> complexityArray;

    std::mutex arrayMutex;
    std::atomic<unsigned short> nextNumber(0);

    using AnswerTable = std::array<unsigned short, 32768 * 4 + 2>;
    using RecursionTable = std::array<unsigned long long, 32768 * 4 + 2>;
    using MemoizationTable = std::bitset<32768 * 4 + 2>;
}

auto buildTable(unsigned short r0, unsigned short r1, unsigned short r7, AnswerTable &table)
{
    for (unsigned i = 0, n = 32768; i != n; ++i)
        table[i] = (i + 1) % 32768;

    for (unsigned i = 1, n = r0 + 1; i != n; ++i)
    {
        table[i << 15] = table[(i - 1) << 15 | r7];
        for (unsigned j = 1, m = i == r0 ? r1 + 1 : 32768; j != m; ++j)
            table[i << 15 | j] = table[(i - 1) << 15 | table[i << 15 | (j - 1)]];
    }

    return table;
}

unsigned long long simulateRecursive(unsigned short r0, unsigned short r1, unsigned short r7, const AnswerTable &table, RecursionTable &counts)
{
    size_t idx = r0 << 15 | r1;
    if (counts[idx])
        return counts[idx];

    if (r0 == 0)
        return counts[idx] = 1;

    if (r1 == 0)
        return counts[idx] = 1 + simulateRecursive(r0 - 1, r7, r7, table, counts);

    return counts[idx] = 1 + simulateRecursive(r0, r1 - 1, r7, table, counts) + simulateRecursive(r0 - 1, table[r0 << 15 | (r1 - 1)], r7, table, counts);
}

unsigned long long simulateMemoized(unsigned short r0, unsigned short r1, unsigned short r7, const AnswerTable &table, MemoizationTable &hits)
{
    size_t idx = r0 << 15 | r1;
    if (hits[idx])
        return 1;

    hits[idx] = true;

    if (r0 == 0)
        return 1;

    if (r1 == 0)
        return 1 + simulateMemoized(r0 - 1, r7, r7, table, hits);

    return 1 + simulateMemoized(r0, r1 - 1, r7, table, hits) + simulateMemoized(r0 - 1, table[r0 << 15 | (r1 - 1)], r7, table, hits);
}

unsigned long long simulateLinear(unsigned short r0, unsigned short r1)
{
    return r0 * 32768 + r1 + 1;
}

int main(int argc, char **argv)
{
    std::cout << "Synacor Challenge R7 (teleporter) complexity estimator" << std::endl;

    if (argc < 2)
    {
        std::cout << "Usage: " << argv[0] << " <output file>" << std::endl;
        return 1;
    }

    std::cout << "Estimating complexity..." << std::endl;
    std::vector<std::thread> threads;
    for (size_t i = 0, n = std::thread::hardware_concurrency(); i != n; ++i)
    {
        threads.emplace_back([]()
        {
            AnswerTable table;
            RecursionTable counts;
            MemoizationTable hits;

            while (true)
            {
                unsigned short num = nextNumber++;
                if (num > 32767)
                    break;

                counts.fill(0);
                hits.reset();

                buildTable(4, 1, num, table);
                auto recursive = simulateRecursive(4, 1, num, table, counts);
                auto memoized = simulateMemoized(4, 1, num, table, hits);
                auto linear = simulateLinear(4, 1);
               
                std::lock_guard<std::mutex> guard(arrayMutex);
                complexityArray[num] = { recursive, memoized, linear, table[4 << 15 | 1] };
            }
        });
    }

    while (nextNumber < 32768)
    {
        std::cout << '\r' << nextNumber << " / 32767";
        std::this_thread::yield();
    }

    for (auto &thread : threads)
        thread.join();

    std::cout << "\rDone.                " << std::endl;

    std::ofstream ofs(argv[1]);
    if (!ofs)
    {
        std::cout << "Could not open output file" << std::endl;
        return 1;
    }

    ofs << "r7,recursive,memoized,linear,result\n";

    for (size_t i = 0; i != 32768; ++i)
    {
        const auto &entry = complexityArray[i];
        ofs << i << ',' << entry.recursiveIterations << ',' << entry.memoizedIterations << ',' << entry.linearIterations << ',' << entry.result << '\n';
    }

    ofs.close();

    std::cout << "Complexity data saved to " << argv[1] << std::endl;
}
