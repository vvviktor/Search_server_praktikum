#include <iostream>
#include <string>
#include <vector>
#include "search_server.h"
#include "paginator.h"
#include "request_queue.h"

using namespace std;

#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)
#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))
#define ASSERT(expr) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)
#define ASSERT_HINT(expr, hint) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))
#define RUN_TEST(func)  RunTestImpl((func), #func)

template<typename T>
void RunTestImpl(const T& func, const string& func_str) {
    func();
    cerr << func_str << " OK"s << endl;
}

template<typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const string& t_str, const string& u_str, const string& file,
                     const string& func, unsigned line, const string& hint) {
    if (t != u) {
        cerr << boolalpha;
        cerr << file << "("s << line << "): "s << func << ": "s;
        cerr << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
        cerr << t << " != "s << u << "."s;
        if (!hint.empty()) {
            cerr << " Hint: "s << hint;
        }
        cerr << endl;
        abort();
    }
}

void AssertImpl(bool value, const string& expr_str, const string& file, const string& func, unsigned line,
                const string& hint) {
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

void TestSearchServerConstructor() {
    try {
        const vector<string> stop_words = {"\12n"s, "\26H\13"s, "G\7F"s};
        SearchServer server(stop_words);
    } catch (const invalid_argument& e) {
        cerr << "Search server constructor error: " << e.what() << endl;
    }
}

void TestAddInvalidDocument() {
    const int doc0_id = 42;
    const string content0 = "cat-dog in the city"s;
    const vector<int> ratings0 = {1, 2, 3};
    const int doc1_id = -24;
    const string content1 = "cat in the city"s;
    const vector<int> ratings1 = {3, 5, 6};
    const int doc2_id = 52;
    const string content2 = "cat-dog in the --city"s;
    const vector<int> ratings2 = {1, 2, 3};
    const int doc3_id = 133;
    const string content3 = "cat-dog in the - city"s;
    const vector<int> ratings3 = {1, 2, 3};
    const int doc4_id = 133;
    const string content4 = "dog in the N\27O\26W ! cat city"s;
    const vector<int> ratings4 = {1, 2, 3};
    {
        SearchServer server;
        try {
            server.AddDocument(doc1_id, content1, DocumentStatus::ACTUAL, ratings1);
        } catch (const invalid_argument& e) {
            cerr << "AddDocument error: "s << e.what() << endl;
        }
    }
    {
        SearchServer server;
        try {
            server.AddDocument(doc0_id, content0, DocumentStatus::ACTUAL, ratings0);
            server.AddDocument(doc0_id, content0, DocumentStatus::ACTUAL, ratings0);
        } catch (const invalid_argument& e) {
            cerr << "AddDocument error: "s << e.what() << endl;
        }
    }
    {
        SearchServer server;
        try {
            server.AddDocument(doc3_id, content3, DocumentStatus::ACTUAL, ratings3);
        } catch (const invalid_argument& e) {
            cerr << "AddDocument error: "s << e.what() << endl;
        }
    }
    {
        SearchServer server;
        try {
            server.AddDocument(doc4_id, content4, DocumentStatus::ACTUAL, ratings4);
        } catch (const invalid_argument& e) {
            cerr << "AddDocument error: "s << e.what() << endl;
        }
    }
}

void TestInvalidQuery() {
    const int doc0_id = 42;
    const string content0 = "cat-dog in the city"s;
    const vector<int> ratings0 = {1, 2, 3};
    {
        SearchServer server;
        server.AddDocument(doc0_id, content0, DocumentStatus::ACTUAL, ratings0);

        const auto result = server.FindTopDocuments("cat-dog in the city"s);
        ASSERT_EQUAL_HINT(result.size(), 1, "Ignoring of valid query."s);
    }
    {
        try {
            SearchServer server;
            server.AddDocument(doc0_id, content0, DocumentStatus::ACTUAL, ratings0);
            const auto result = server.FindTopDocuments("cat --in the city"s);
        } catch (const invalid_argument& e) {
            cerr << "Query error: " << e.what() << endl;
        }
    }
    {
        try {
            SearchServer server;
            server.AddDocument(doc0_id, content0, DocumentStatus::ACTUAL, ratings0);
            const auto result = server.FindTopDocuments("dog in the - city"s);
        } catch (const invalid_argument& e) {
            cerr << "Query error: " << e.what() << endl;
        }
    }
    {
        try {
            SearchServer server;
            server.AddDocument(doc0_id, content0, DocumentStatus::ACTUAL, ratings0);
            const auto result = server.FindTopDocuments("dog in the N\27O\26W ! cat city"s);
        } catch (const invalid_argument& e) {
            cerr << "Query error: " << e.what() << endl;
        }
    }
}

void TestMatchDocumentInvalidQuery() {
    const int doc0_id = 42;
    const string content0 = "cat-dog in the city"s;
    const vector<int> ratings0 = {1, 2, 3};
    {
        SearchServer server;
        server.AddDocument(doc0_id, content0, DocumentStatus::ACTUAL, ratings0);
        const auto [matched_words, status] = server.MatchDocument("cat-dog in the city"s, 42);
        ASSERT_HINT(!matched_words.empty(), "Ignoring of valid query."s);
        ASSERT_EQUAL_HINT(matched_words.size(), 4, "Matched words number is incorrect."s);
        ASSERT_EQUAL(matched_words[0], "cat-dog"s);
        ASSERT_EQUAL(matched_words[1], "city"s);
        ASSERT_EQUAL(matched_words[2], "in"s);
        ASSERT_EQUAL(matched_words[3], "the"s);
    }
    {
        try {
            SearchServer server;
            server.AddDocument(doc0_id, content0, DocumentStatus::ACTUAL, ratings0);
            const auto [matched_words, status] = server.MatchDocument("cat --in the city"s, 42);
        } catch (const invalid_argument& e) {
            cerr << "MatchDocument error: "s << e.what() << endl;
        }
    }
    {
        try {
            SearchServer server;
            server.AddDocument(doc0_id, content0, DocumentStatus::ACTUAL, ratings0);
            const auto [matched_words, status] = server.MatchDocument("dog in the - city"s, 42);
        } catch (const invalid_argument& e) {
            cerr << "MatchDocument error: "s << e.what() << endl;
        }
    }
    {
        try {
            SearchServer server;
            server.AddDocument(doc0_id, content0, DocumentStatus::ACTUAL, ratings0);
            const auto [matched_words, status] = server.MatchDocument("dog in the N\27O\26W ! cat city"s, 42);
        } catch (const invalid_argument& e) {
            cerr << "MatchDocument error: "s << e.what() << endl;
        }
    }
}

void TestGetDocumentId() {
    const int doc0_id = 42;
    const string content0 = "young cat in the city"s;
    const vector<int> ratings0 = {1, 1, 1};
    const int doc1_id = 24;
    const string content1 = "young parrot in the city"s;
    const vector<int> ratings1 = {1, 1, 1};
    const int doc2_id = 33;
    const string content2 = "dog in the city"s;
    const vector<int> ratings2 = {1, 1, 1};
    {
        SearchServer server;
        ASSERT_EQUAL(server.GetDocumentCount(), 0);
        server.AddDocument(doc0_id, content0, DocumentStatus::ACTUAL, ratings0);
        server.AddDocument(doc1_id, content1, DocumentStatus::ACTUAL, ratings1);
        server.AddDocument(doc2_id, content2, DocumentStatus::ACTUAL, ratings2);
        ASSERT_EQUAL(server.GetDocumentCount(), 3);
        ASSERT_EQUAL(server.GetDocumentId(2), 33);
        ASSERT_EQUAL(server.GetDocumentId(1), 24);
        ASSERT_EQUAL(server.GetDocumentId(0), 42);
        try {
            server.GetDocumentId(-5);
        } catch (const out_of_range& e) {
            cerr << "GetDocumentId error: Request is out of range."s << endl;
        }
        try {
            server.GetDocumentId(6);
        } catch (const out_of_range& e) {
            cerr << "GetDocumentId error: Request is out of range."s << endl;
        }
    }
}

void TestSearchServer() {
    RUN_TEST(TestSearchServerConstructor);
    RUN_TEST(TestAddInvalidDocument);
    RUN_TEST(TestInvalidQuery);
    RUN_TEST(TestMatchDocumentInvalidQuery);
    RUN_TEST(TestGetDocumentId);
}

int main() {
    TestSearchServer();
    SearchServer search_server("and in at"s);
    RequestQueue request_queue(search_server);

    search_server.AddDocument(1, "curly cat curly tail"s, DocumentStatus::ACTUAL, {7, 2, 7});
    search_server.AddDocument(2, "curly dog and fancy collar"s, DocumentStatus::ACTUAL, {1, 2, 3});
    search_server.AddDocument(3, "big cat fancy collar "s, DocumentStatus::ACTUAL, {1, 2, 8});
    search_server.AddDocument(4, "big dog sparrow Eugene"s, DocumentStatus::IRRELEVANT, {1, 3, 2});
    search_server.AddDocument(5, "big dog sparrow Vasiliy"s, DocumentStatus::ACTUAL, {1, 1, 1});

    // 1439 запросов с нулевым результатом
    for (int i = 0; i < 1439; ++i) {
        request_queue.AddFindRequest("empty request"s);
    }
    // все еще 1439 запросов с нулевым результатом
    request_queue.AddFindRequest("curly dog"s);
    // новые сутки, первый запрос удален, 1438 запросов с нулевым результатом
    request_queue.AddFindRequest("big collar"s);
    // первый запрос удален, 1437 запросов с нулевым результатом
    request_queue.AddFindRequest("sparrow"s);
    cout << "Total empty requests: "s << request_queue.GetNoResultRequests() << endl;
    return 0;
}

