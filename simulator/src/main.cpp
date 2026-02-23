#include <omp.h>
#include <iostream>

int main(int argc, char* argv[]) {
#ifdef _OPENMP
    std::cout << "OpenMP enabled: " << _OPENMP << "\n";
    std::cout << "max threads: " << omp_get_max_threads() << "\n";
#else
    std::cout << "OpenMP NOT enabled\n";
#endif

    return 0;
}