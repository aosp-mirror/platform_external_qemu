#include "test1_lib.h"

std::string test1_print() {
    return "called test1_print";
}

// Do not call this in the unittest (leave uncovered for testing purposes).
std::string test1_uncovered() {
    return "called test1_uncovered";
}
