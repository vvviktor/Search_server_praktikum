#include "remove_duplicates.h"
#include <set>
#include <string>
#include <iostream>
#include <map>

using namespace std;

void RemoveDuplicates(SearchServer& search_server) {
    map<int, set<string>> ids_to_words;
    set<int> ids_to_remove;

    for (const int id: search_server) {
        for (const auto& [word, freq]: search_server.GetWordFrequencies(id)) {
            ids_to_words[id].insert(word);
        }
    }

    map<int, set<string>> ids_to_words2 = ids_to_words;

    for (const auto& [id, words]: ids_to_words) {
        for (const auto& [id2, words2]: ids_to_words2) {
            if (id2 == id) {
                continue;
            }
            if (words2 == words) {
                ids_to_remove.insert(id2);
                ids_to_words.erase(id2);
                cout << "Found duplicate document id "s << id2 << endl;
            }
        }
    }

    for (const int id: ids_to_remove) {
        search_server.RemoveDocument(id);
    }
}
