#include "dispatchers/metaprogramming/IMeta.h"

bool IMeta::registerAnalyzer(identifier_t identifier, const analyzer_builder &make_analyzer) {
    // STUB, metaprogramming can't register stuff. Just coun't the analyzers for size to work.
    _size++;
    return true;
}

size_t IMeta::size() {
    return _size;
}

void IMeta::clear() {
    _size = 0;
}