#include "test2_lib.h"

std::string test2_print() {
    return "called test2_print";
}

// Do not call this in the unittest (leave uncovered for testing purposes).
std::string test2_uncovered() {
    return "called test2_uncovered";
}
