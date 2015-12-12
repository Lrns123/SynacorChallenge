#include <vector>
#include <iostream>
#include <chrono>

// Bit flags to denote pending operators
enum Operator
{
    MaxNum = (1 << 24) - 1,
    Add = 1 << 24,
    Sub = 1 << 25,
    Mul = 1 << 26,
};

// Vault grid values
size_t vaultGrid[4][4] = 
{
    {Mul, 8, Sub, 1},
    {4, Mul, 11, Mul},
    {Add, 4, Sub, 18},
    {22, Sub, 9, Mul},
};

class InvalidPathException
{};

int mergeWeight(int value, int cellValue)
{
    if (value > MaxNum) // Value has a pending operator
    {
        if (cellValue > MaxNum)     // Two operators in a row -> invalid path
            throw InvalidPathException();

        // Apply pending operator
        if (value & Add)
            return (value & MaxNum) + cellValue;
        
        if (value & Sub)
            return (value & MaxNum) - cellValue;

        if (value & Mul)
            return (value & MaxNum) * cellValue;

        throw InvalidPathException();

    }
    
    if (cellValue <= MaxNum)    // Two numbers in a row -> invalid path
        throw InvalidPathException();

    // Add pending operator
    return value | cellValue;
    
}

enum Direction
{
    North,
    South,
    East,
    West
};

class StackHelper
{
    std::vector<Direction> &m_stack;

public:
    StackHelper(std::vector<Direction> & stack, Direction dir)
        : m_stack(stack)
    {
        m_stack.push_back(dir);
    }

    ~StackHelper()
    {
        m_stack.pop_back();
    }
};

void search(int x, int y, int weight, size_t maxDepth, std::vector<Direction> &stack, std::vector<Direction> &shortest)
{
    if (shortest.size() != 0 && stack.size() >= shortest.size())
        return;

    weight = mergeWeight(weight, vaultGrid[y][x]);

    // Special cases
    if ((weight & MaxNum) > 32767) // Max value exceeded
        return;

    if (x == 0 && y == 3 && stack.size() != 0) // Cannot return to start
        return;
    

    if (x == 3 && y == 0) // Vault reached
    {
        if (weight == 30 && (shortest.size() == 0 || shortest.size() > stack.size()))
            shortest = stack;

        return;
    }

    if (stack.size() >= maxDepth)
        return;

    // Recurse into neighbouring cells (depth-first search)
    if (x > 0)
    {
        StackHelper helper(stack, West);
        search(x - 1, y, weight, maxDepth, stack, shortest);
    }

    if (x < 3)
    {
        StackHelper helper(stack, East);
        search(x + 1, y, weight, maxDepth, stack, shortest);
    }

    if (y > 0)
    {
        StackHelper helper(stack, North);
        search(x, y - 1, weight, maxDepth, stack, shortest);
    }

    if (y < 3)
    {
        StackHelper helper(stack, South);
        search(x, y + 1, weight, maxDepth, stack, shortest);
    }
}


int main(int argc, char **argv)
{
    std::cout << "Synacor Challenge vault solver." << std::endl;
    std::cout << "Starting scan for vault solution..." << std::endl;

    size_t maxSteps = argc > 1 ? atoi(argv[1]) : 15;
    std::cout << "Searching for shortest solution with up to " << maxSteps << " steps." << std::endl;

    try
    {
        using namespace std::chrono;

        std::vector<Direction> stack;
        std::vector<Direction> shortest;

        auto start = high_resolution_clock::now();
        // Run depth-first search through the grid.
        search(0, 3, Add, maxSteps, stack, shortest);
        auto end = high_resolution_clock::now();

        std::cout << "Search completed in " << duration_cast<milliseconds>(end - start).count() << " ms" << std::endl << std::endl;

        if (shortest.size() == 0)
            std::cout << "No solutions found. Try increasing the maximum number of steps." << std::endl;
        else
        {
            static const char *directionStrings[] = { "go north", "go south", "go east", "go west" };

            std::cout << "Solution found! (" << shortest.size() << " steps)" << std::endl;
            for (auto dir : shortest)
                std::cout << directionStrings[dir] << std::endl;
            
        }
    }
    catch (const InvalidPathException &)
    {
        std::cout << "Invalid path encountered" << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cout << "Error: Exception occurred: " << e.what() << std::endl;
    }
    catch (...)
    {
        std::cout << "Error: Unknown exception occurred" << std::endl;
    }
}