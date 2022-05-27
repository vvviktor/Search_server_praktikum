#include "my_assert.h"

using namespace std;

void AssertImpl(bool value, const std::string& expr_str, const std::string& file, const std::string& func,
                unsigned line, const std::string& hint) {
    if (!value) {
        cerr << file << "("s << line << "): "s << func << ": "s;
        cerr << "ASSERT("s << expr_str << ") failed."s;
        if (!hint.empty()) {
            cerr << " Hint: "s << hint;
        }
        cerr << endl;
        abort();
    }
}