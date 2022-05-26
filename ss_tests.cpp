#include "ss_tests.h"
#include "my_assert.h"
#include "search_server.h"
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include <numeric>
#include <cmath>

using namespace std;

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
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    RUN_TEST(TestAddDocument);
    RUN_TEST(TestMinusWords);
    RUN_TEST(TestMatchDocument);
    RUN_TEST(TestRelevanceSorting);
    RUN_TEST(TestCalculateRating);
    RUN_TEST(TestPredicate);
    RUN_TEST(TestDocumentStatusFilter);
    RUN_TEST(TestRelevanceCalculation);
}

void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    // Сначала убеждаемся, что поиск слова, не входящего в список стоп-слов,
    // находит нужный документ
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    // Затем убеждаемся, что поиск этого же слова, входящего в список стоп-слов,
    // возвращает пустой результат
    {
        SearchServer server("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("in"s).empty(), "Check stop words setting algorithm"s);
    }
}

void TestAddDocument() {
    const int doc0_id = 42;
    const string content0 = "cat in the city"s;
    const vector<int> ratings0 = {1, 2, 3};
    const int doc1_id = 24;
    const string content1 = "dog in the city"s;
    const vector<int> ratings1 = {3, 5, 6};
    {
        SearchServer server;
        ASSERT_EQUAL(server.GetDocumentCount(), 0);
        server.AddDocument(doc0_id, content0, DocumentStatus::ACTUAL, ratings0);
        server.AddDocument(doc1_id, content1, DocumentStatus::ACTUAL, ratings1);
        ASSERT_EQUAL_HINT(server.GetDocumentCount(), 2, "Document addition failed"s);
        const auto found_docs = server.FindTopDocuments("old cat"s);
        ASSERT_EQUAL(found_docs.size(), 1);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc0_id);
    }
    {
        SearchServer server;
        server.AddDocument(doc0_id, content0, DocumentStatus::ACTUAL, ratings0);
        server.AddDocument(doc1_id, content1, DocumentStatus::ACTUAL, ratings1);
        const auto found_docs = server.FindTopDocuments("grey parrot"s);
        ASSERT_HINT(found_docs.empty(), "Check AddDocument method"s);
    }
}

void TestMinusWords() {
    const int doc0_id = 42;
    const string content0 = "cat in the city"s;
    const vector<int> ratings0 = {1, 2, 3};
    const int doc1_id = 24;
    const string content1 = "dog in the city"s;
    const vector<int> ratings1 = {3, 5, 6};
    {
        SearchServer server;
        server.AddDocument(doc0_id, content0, DocumentStatus::ACTUAL, ratings0);
        server.AddDocument(doc1_id, content1, DocumentStatus::ACTUAL, ratings1);
        const auto found_docs = server.FindTopDocuments("city cat -dog"s);
        ASSERT_EQUAL(found_docs.size(), 1);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL_HINT(doc0.id, doc0_id, "Check minus-words processing"s);
    }
    {
        SearchServer server;
        server.AddDocument(doc0_id, content0, DocumentStatus::ACTUAL, ratings0);
        server.AddDocument(doc1_id, content1, DocumentStatus::ACTUAL, ratings1);
        const auto found_docs = server.FindTopDocuments("city cat and dog"s);
        ASSERT_EQUAL(found_docs.size(), 2);
        const Document& doc0 = found_docs[0];
        const Document& doc1 = found_docs[1];
        ASSERT_EQUAL(doc0.id, doc1_id);
        ASSERT_EQUAL(doc1.id, doc0_id);
    }
}

void TestMatchDocument() {
    const int doc0_id = 42;
    const string content0 = "old white cat in the city"s;
    const vector<int> ratings0 = {1, 2, 3};
    {
        SearchServer server;
        server.AddDocument(doc0_id, content0, DocumentStatus::ACTUAL, ratings0);
        const auto [matched_words, status] = server.MatchDocument("young white cat in the city"s, 42);
        ASSERT_HINT(status == DocumentStatus::ACTUAL, "Wrong status"s);
        ASSERT_EQUAL_HINT(matched_words.size(), 5, "Matched words number is incorrect."s);
        ASSERT_EQUAL(matched_words[0], "cat"s);
        ASSERT_EQUAL(matched_words[1], "city"s);
        ASSERT_EQUAL(matched_words[2], "in"s);
        ASSERT_EQUAL(matched_words[3], "the"s);
        ASSERT_EQUAL(matched_words[4], "white"s);
    }
    {
        SearchServer server("in the"s);
        server.AddDocument(doc0_id, content0, DocumentStatus::ACTUAL, ratings0);
        const auto [matched_words, status] = server.MatchDocument("young white cat in the city"s, 42);
        ASSERT_EQUAL_HINT(matched_words.size(), 3, "Stop words in result. Check stop words filtering"s);
        ASSERT_EQUAL(matched_words[0], "cat"s);
        ASSERT_EQUAL(matched_words[1], "city"s);
        ASSERT_EQUAL(matched_words[2], "white"s);
    }
    {
        SearchServer server;
        server.AddDocument(doc0_id, content0, DocumentStatus::ACTUAL, ratings0);
        const auto [matched_words, status] = server.MatchDocument("young white cat in the -city"s, 42);
        ASSERT_HINT(matched_words.empty(),
                    "Result of a query containing minus-words must be empty. Check MatchDocument method."s);
    }
}

void TestRelevanceSorting() {
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
        SearchServer server("in the"s);
        server.AddDocument(doc0_id, content0, DocumentStatus::ACTUAL, ratings0);
        server.AddDocument(doc1_id, content1, DocumentStatus::ACTUAL, ratings1);
        server.AddDocument(doc2_id, content2, DocumentStatus::ACTUAL, ratings2);
        const auto found_docs = server.FindTopDocuments("young city cat"s);
        ASSERT_EQUAL(found_docs.size(), 3);
        const Document& doc0 = found_docs[0];
        const Document& doc1 = found_docs[1];
        const Document& doc2 = found_docs[2];
        ASSERT_HINT((doc0.relevance > doc1.relevance) && (doc1.relevance > doc2.relevance),
                    "Wrong relevance sorting order. Check FindTopDocuments method."s);
    }
}

void TestCalculateRating() {
    const int doc0_id = 42;
    const string content0 = "young cat in the city"s;
    const vector<int> ratings0 = {1, 2, 3};
    const int doc1_id = 24;
    const string content1 = "dog in the city"s;
    const vector<int> ratings1 = {3, 5, 6};
    {
        SearchServer server;
        server.AddDocument(doc0_id, content0, DocumentStatus::ACTUAL, ratings0);
        server.AddDocument(doc1_id, content1, DocumentStatus::ACTUAL, ratings1);
        const auto found_docs = server.FindTopDocuments("young city cat"s);
        ASSERT_EQUAL(found_docs.size(), 2);
        const Document& doc0 = found_docs[0];
        const Document& doc1 = found_docs[1];
        ASSERT_EQUAL_HINT(doc0.rating,
                          accumulate(ratings0.begin(), ratings0.end(), 0) / static_cast<int>(ratings0.size()),
                          "Incorrect rating calculation."s);
        ASSERT_EQUAL_HINT(doc1.rating,
                          accumulate(ratings1.begin(), ratings1.end(), 0) / static_cast<int>(ratings1.size()),
                          "Incorrect rating calculation."s);
    }
}

void TestPredicate() {
    const int doc0_id = 42;
    const string content0 = "young cat in the city"s;
    const vector<int> ratings0 = {1, 2, 3};
    const int doc1_id = 24;
    const string content1 = "dog in the city"s;
    const vector<int> ratings1 = {3, 5, 6};
    const int doc2_id = 33;
    const string content2 = "young parrot in the city"s;
    const vector<int> ratings2 = {2, 2, 3};
    {
        SearchServer server;
        server.AddDocument(doc0_id, content0, DocumentStatus::ACTUAL, ratings0);
        server.AddDocument(doc1_id, content1, DocumentStatus::ACTUAL, ratings1);
        server.AddDocument(doc2_id, content2, DocumentStatus::ACTUAL, ratings2);
        const auto found_docs = server.FindTopDocuments("young city cat"s,
                                                        [](int document_id, DocumentStatus status, int rating) {
                                                            return document_id % 2 == 0;
                                                        });
        ASSERT_EQUAL(found_docs.size(), 2);
        const Document& doc0 = found_docs[0];
        const Document& doc1 = found_docs[1];
        ASSERT_HINT(doc0.id % 2 == 0, "Incorrect predicate filtering."s);
        ASSERT_HINT(doc1.id % 2 == 0, "Incorrect predicate filtering."s);
    }
    {
        SearchServer server;
        server.AddDocument(doc0_id, content0, DocumentStatus::ACTUAL, ratings0);
        server.AddDocument(doc1_id, content1, DocumentStatus::ACTUAL, ratings1);
        server.AddDocument(doc2_id, content2, DocumentStatus::ACTUAL, ratings2);
        const auto found_docs = server.FindTopDocuments("young city cat"s,
                                                        [](int document_id, DocumentStatus status, int rating) {
                                                            return document_id % 2 != 0;
                                                        });
        ASSERT_EQUAL(found_docs.size(), 1);
        const Document& doc0 = found_docs[0];
        ASSERT_HINT(doc0.id % 2 != 0, "Incorrect predicate filtering."s);
    }
}

void TestDocumentStatusFilter() {
    const int doc0_id = 42;
    const string content0 = "young cat in the city"s;
    const vector<int> ratings0 = {1, 2, 3};
    const int doc1_id = 24;
    const string content1 = "dog in the city"s;
    const vector<int> ratings1 = {3, 5, 6};
    const int doc2_id = 33;
    const string content2 = "young parrot in the city"s;
    const vector<int> ratings2 = {2, 2, 3};
    {
        SearchServer server;
        server.AddDocument(doc0_id, content0, DocumentStatus::ACTUAL, ratings0);
        server.AddDocument(doc1_id, content1, DocumentStatus::IRRELEVANT, ratings1);
        server.AddDocument(doc2_id, content2, DocumentStatus::ACTUAL, ratings2);
        const auto found_docs = server.FindTopDocuments("young city cat"s, DocumentStatus::ACTUAL);
        ASSERT_EQUAL(found_docs.size(), 2);
        const Document& doc0 = found_docs[0];
        const Document& doc1 = found_docs[1];
        ASSERT_EQUAL_HINT(doc0.id, 42, "Incorrect status filtering."s);
        ASSERT_EQUAL_HINT(doc1.id, 33, "Incorrect status filtering."s);
    }
    {
        SearchServer server;
        server.AddDocument(doc0_id, content0, DocumentStatus::ACTUAL, ratings0);
        server.AddDocument(doc1_id, content1, DocumentStatus::IRRELEVANT, ratings1);
        server.AddDocument(doc2_id, content2, DocumentStatus::ACTUAL, ratings2);
        const auto found_docs = server.FindTopDocuments("young city cat"s, DocumentStatus::BANNED);
        ASSERT_HINT(found_docs.empty(), "Incorrect status filtering."s);
    }
}

void TestRelevanceCalculation() {
    const int doc0_id = 42;
    const string content0 = "young cat in the city"s;
    const vector<int> ratings0 = {1, 2, 3};
    const int doc1_id = 24;
    const string content1 = "dog in the city"s;
    const vector<int> ratings1 = {3, 5, 6};
    const int doc2_id = 33;
    const string content2 = "young parrot in the city"s;
    const vector<int> ratings2 = {2, 2, 3};
    {
        SearchServer server("in the"s);
        server.AddDocument(doc0_id, content0, DocumentStatus::ACTUAL, ratings0);
        server.AddDocument(doc1_id, content1, DocumentStatus::ACTUAL, ratings1);
        server.AddDocument(doc2_id, content2, DocumentStatus::ACTUAL, ratings2);
        const auto found_docs = server.FindTopDocuments("young city cat"s);
        ASSERT_EQUAL(found_docs.size(), 3);
        const Document& doc0 = found_docs[0];
        const Document& doc1 = found_docs[1];
        const Document& doc2 = found_docs[2];
        const double doc0_relevance = log(3.0 / 2) * (1.0 / 3) + log(3.0) * (1.0 / 3);
        const double doc1_relevance = 0;
        const double doc2_relevance = log(3.0 / 2) * (1.0 / 3);
        ASSERT_EQUAL_HINT(doc0.relevance, doc0_relevance, "Incorrect relevance calculation. Check TF*IDF algorithm."s);
        ASSERT_EQUAL_HINT(doc1.relevance, doc2_relevance, "Incorrect relevance calculation. Check TF*IDF algorithm."s);
        ASSERT_EQUAL_HINT(doc2.relevance, doc1_relevance, "Incorrect relevance calculation. Check TF*IDF algorithm."s);
    }
}