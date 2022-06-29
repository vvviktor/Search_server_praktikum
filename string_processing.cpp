#include "string_processing.h"

using namespace std;

vector<string> SplitIntoWords(string_view str) {
    vector<string> result;
    str.remove_prefix(min(str.size(), str.find_first_not_of(' ')));
    const int64_t pos_end = str.npos;

    while (!str.empty()) {
        int64_t space = str.find(' ');
        result.push_back(space == pos_end ? string(str) : string(str.substr(0, str.find(' '))));
        str.remove_prefix(min(str.size(), str.find(' ')));
        str.remove_prefix(min(str.size(), str.find_first_not_of(' ')));
    }

    return result;
}

vector<string_view> SplitIntoWordsView(string_view str) {
    vector<string_view> result;
    str.remove_prefix(min(str.size(), str.find_first_not_of(' ')));
    const int64_t pos_end = str.npos;

    while (!str.empty()) {
        int64_t space = str.find(' ');
        result.push_back(space == pos_end ? str : str.substr(0, str.find(' ')));
        str.remove_prefix(min(str.size(), str.find(' ')));
        str.remove_prefix(min(str.size(), str.find_first_not_of(' ')));
    }

    return result;
}


