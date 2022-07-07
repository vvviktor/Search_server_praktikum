#include "process_queries.h"

using namespace std;

vector<vector<Document>> ProcessQueries(
        const SearchServer& search_server,
        const vector<string>& queries) {
    vector<vector<Document>> ret(queries.size());
    auto f = [&search_server](const string& str) { return search_server.FindTopDocuments(str); };
    transform(execution::par, queries.begin(), queries.end(), ret.begin(), f);
    return ret;
}

list<Document> ProcessQueriesJoined(const SearchServer& search_server,
                                    const std::vector<std::string>& queries) {
    list<Document> ret;
    for (const auto& docs: ProcessQueries(search_server, queries)) {
        auto temp = new Document[docs.size()];
        auto f = [](const Document& doc) {
            return doc;
        };
        transform(execution::par, docs.begin(), docs.end(), temp, f);
        for (auto it = temp; it != temp + docs.size(); ++it) {
            ret.push_front(*it);
        }
        delete[] temp;
    }
    ret.reverse();
    return ret;
}

/*list<Document> ProcessQueriesJoined(const SearchServer& search_server,
                                    const std::vector<std::string>& queries) {
    vector<list<Document>> temp_result(queries.size());
    list<Document> final_result;
    auto tr = [&search_server](const string& str) {
        list<Document> temp;
        for (auto doc: search_server.FindTopDocuments(str)) {
            temp.insert(temp.end(), doc);
        }
        return temp;
    };
    transform(execution::par, queries.begin(), queries.end(), temp_result.begin(), tr);
    for (auto list: temp_result) {
        final_result.splice(final_result.end(), list);
    }
    return final_result;
}*/
