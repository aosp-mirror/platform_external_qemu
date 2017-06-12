#pragma once

#include <inttypes.h>

void add_hva_gpa(void* hva, uint64_t gpa, uint64_t len);
void remove_hva_gpa(uint64_t gpa, uint64_t len);
uint64_t hva2gpa(void* hva, bool* found);
void* gpa2hva(uint64_t gpa, bool* found);
