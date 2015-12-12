#include <iostream>
#include <algorithm>

inline int calc(int a, int b, int c, int d, int e)
{
    return a + b * (c * c) + (d * d * d) - e;
}

int main(int, char **)
{
    int vals[] = { 2, 3, 5, 7, 9 };

    std::cout << "Synacor Challenge ruins equation solver." << std::endl;
    std::cout << "Solving equation a + b * c^2 + d^3 - e = 399" << std::endl;
    std::cout << " using 2, 3, 5, 7, 9 as candidates..." << std::endl << std::endl;

    do
    {
        int val = calc(vals[0], vals[1], vals[2], vals[3], vals[4]);

        if (val == 399)
        {
            std::cout << '\r' << vals[0] << " + " << vals[1] << " * " << vals[2] << "^2 + " << vals[3] << "^3 - " << vals[4] << " = " << val << std::endl;
            return 0;
        }        
    }
    while (std::next_permutation(vals, vals + 5));

    std::cout << "Could not solve equation :(." << std::endl;
    return 1;
}