#include <cassert>
#include "mylib/main.hpp"

int main() {
    auto s = mylib::hello("Vaasu");
    assert(s == "Hello, Vaasu!");
    return 0;
}
