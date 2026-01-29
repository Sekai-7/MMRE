#include "MultiResourceTimelineCache.h"
#include "DataIngestionManager.h"

#include <iostream>
#include <cstdlib>

int main() {
    if (std::getenv("MMRE_CONTAINER") == nullptr) {
        std::cerr << "Error: Not running in MMRE Docker container. Please set MMRE_CONTAINER=1." << std::endl;
        return 1;
    }
    return 1;
}