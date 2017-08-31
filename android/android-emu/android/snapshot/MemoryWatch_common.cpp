#include "android/snapshot/MemoryWatch.h"

hva2gpa_t hva2gpa_call = 0;
gpa2hva_t gpa2hva_call = 0;

void set_address_translation_funcs(
        hva2gpa_t hva2gpa,
        gpa2hva_t gpa2hva) {
    hva2gpa_call = hva2gpa;
    gpa2hva_call = gpa2hva;
}
