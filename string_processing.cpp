#include "string_processing.h"

using namespace std;

vector<string> SplitIntoWords(const string& text) {
    vector<string> words;
    string word;
    for (const char c: text) {
        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        } else {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }

    return words;
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


