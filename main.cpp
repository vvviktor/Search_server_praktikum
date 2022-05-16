// search_server_s3_t1_v1.cpp

#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;

string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result;
    cin >> result;
    ReadLine();
    return result;
}

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

struct Document {
    Document() = default;

    Document(int id, double relevance, int rating)
            : id(id), relevance(relevance), rating(rating) {
    }

    int id = 0;
    double relevance = 0.0;
    int rating = 0;
};

template<typename StringContainer>
set<string> MakeUniqueNonEmptyStrings(const StringContainer& strings) {
    set<string> non_empty_strings;
    for (const string& str: strings) {
        if (!str.empty()) {
            non_empty_strings.insert(str);
        }
    }
    return non_empty_strings;
}

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};

class SearchServer {
public:
    inline static constexpr int INVALID_DOCUMENT_ID = -1;

    SearchServer() = default;

    template<typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words)
            : stop_words_(MakeUniqueNonEmptyStrings(stop_words)) {
    }

    explicit SearchServer(const string& stop_words_text)
            : SearchServer(SplitIntoWords(stop_words_text))  // Invoke delegating constructor from string container
    {
    }

    [[nodiscard]] bool
    AddDocument(int document_id, const string& document, DocumentStatus status, const vector<int>& ratings) {
        if (document_id < 0 || documents_.count(document_id) || !IsValidWord(document)) {
            return false;
        }
        const vector<string> words = SplitIntoWordsNoStop(document);
        if (!IsValidDocument(words)) {
            return false;
        }
        const double inv_word_count = 1.0 / words.size();
        for (const string& word: words) {
            word_to_document_freqs_[word][document_id] += inv_word_count;
        }
        documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status});
        document_index_to_id_[doc_counter_] = document_id;
        ++doc_counter_;
        return true;
    }

    template<typename DocumentPredicate>
    [[nodiscard]] bool
    FindTopDocuments(const string& raw_query, DocumentPredicate document_predicate, vector<Document>& result) const {
        Query query;
        if (!ParseQuery(raw_query, query)) {
            return false;
        }
        auto matched_documents = FindAllDocuments(query, document_predicate);

        sort(matched_documents.begin(), matched_documents.end(), [](const Document& lhs, const Document& rhs) {
            if (abs(lhs.relevance - rhs.relevance) < 1e-6) {
                return lhs.rating > rhs.rating;
            } else {
                return lhs.relevance > rhs.relevance;
            }
        });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        result = matched_documents;
        return true;
    }

    [[nodiscard]] bool
    FindTopDocuments(const string& raw_query, DocumentStatus status, vector<Document>& result) const {
        return FindTopDocuments(raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
            return document_status == status;
        }, result);
    }

    [[nodiscard]] bool FindTopDocuments(const string& raw_query, vector<Document>& result) const {
        return FindTopDocuments(raw_query, DocumentStatus::ACTUAL, result);
    }

    int GetDocumentCount() const {
        return documents_.size();
    }

    int GetDocumentId(int index) const {
        if (index < 0 || index > documents_.size() - 1) {
            return INVALID_DOCUMENT_ID;
        }
        return document_index_to_id_.at(index);
    }

    [[nodiscard]] bool
    MatchDocument(const string& raw_query, int document_id, tuple<vector<string>, DocumentStatus>& result) const {
        if (!IsValidWord(raw_query)) {
            return false;
        }
        Query query;
        if (!ParseQuery(raw_query, query)) {
            return false;
        }
        vector<string> matched_words;
        for (const string& word: query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.push_back(word);
            }
        }
        for (const string& word: query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.clear();
                break;
            }
        }
        result = {matched_words, documents_.at(document_id).status};
        return true;
    }

private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };
    const set<string> stop_words_;
    map<string, map<int, double>> word_to_document_freqs_;
    map<int, DocumentData> documents_;
    int doc_counter_ = 0;
    map<int, int> document_index_to_id_;

    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }

    static bool IsValidWord(const string& word) {
        // A valid word must not contain special characters
        return none_of(word.begin(), word.end(), [](char c) {
            return c >= '\0' && c < ' ';
        });
    }

    static bool ProperMinusUsage(const string& text) {
        return !((text[0] == '-' && text[1] == '-') || (text == "-"s));
    }

    static bool IsValidDocument(const vector<string>& document) {
        for (const auto& word: document) {
            if (!ProperMinusUsage(word)) {
                return false;
            }
        }
        return true;
    }

    vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
        for (const string& word: SplitIntoWords(text)) {
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }

    static int ComputeAverageRating(const vector<int>& ratings) {
        if (ratings.empty()) {
            return 0;
        }
        int rating_sum = 0;
        for (const int rating: ratings) {
            rating_sum += rating;
        }
        return rating_sum / static_cast<int>(ratings.size());
    }

    struct QueryWord {
        string data;
        bool is_minus;
        bool is_stop;
    };

    bool ParseQueryWord(string text, QueryWord& query_word) const {
        bool is_minus = false;
        // Word shouldn't be empty
        if (text[0] == '-') {
            if (!ProperMinusUsage(text)) {
                return false;
            }
            is_minus = true;
            text = text.substr(1);
        }
        query_word = {text, is_minus, IsStopWord(text)};
        return true;
    }

    struct Query {
        set<string> plus_words;
        set<string> minus_words;
    };

    bool ParseQuery(const string& text, Query& query) const {
        if (!IsValidWord(text)) {
            return false;
        }
        for (const string& word: SplitIntoWords(text)) {
            QueryWord query_word;
            if (!ParseQueryWord(word, query_word)) {
                query.plus_words.clear();
                query.minus_words.clear();
                return false;
            }
            if (!query_word.is_stop) {
                if (query_word.is_minus) {
                    query.minus_words.insert(query_word.data);
                } else {
                    query.plus_words.insert(query_word.data);
                }
            }
        }
        return true;
    }

    // Existence required
    double ComputeWordInverseDocumentFreq(const string& word) const {
        return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
    }

    template<typename DocumentPredicate>
    vector<Document> FindAllDocuments(const Query& query, DocumentPredicate document_predicate) const {
        map<int, double> document_to_relevance;
        for (const string& word: query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto [document_id, term_freq]: word_to_document_freqs_.at(word)) {
                const auto& document_data = documents_.at(document_id);
                if (document_predicate(document_id, document_data.status, document_data.rating)) {
                    document_to_relevance[document_id] += term_freq * inverse_document_freq;
                }
            }
        }

        for (const string& word: query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            for (const auto [document_id, _]: word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(document_id);
            }
        }

        vector<Document> matched_documents;
        for (const auto [document_id, relevance]: document_to_relevance) {
            matched_documents.push_back({document_id, relevance, documents_.at(document_id).rating});
        }
        return matched_documents;
    }
};

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
        ASSERT_EQUAL(server.GetDocumentCount(), 0);
        ASSERT_HINT(server.AddDocument(doc0_id, content0, DocumentStatus::ACTUAL, ratings0),
                    "Unsuccessful addition of valid document."s);
        ASSERT_EQUAL_HINT(server.GetDocumentCount(), 1, "Unsuccessful addition of valid document."s);
    }
    {
        SearchServer server;
        ASSERT_EQUAL(server.GetDocumentCount(), 0);
        ASSERT_HINT(!server.AddDocument(doc1_id, content1, DocumentStatus::ACTUAL, ratings1),
                    "Successful addition of invalid document. Check negative id filtering"s);
        ASSERT_EQUAL_HINT(server.GetDocumentCount(), 0, "Invalid document saved"s);
    }
    {
        SearchServer server;
        ASSERT_EQUAL(server.GetDocumentCount(), 0);
        (void) server.AddDocument(doc0_id, content0, DocumentStatus::ACTUAL, ratings0);
        ASSERT_HINT(!server.AddDocument(doc0_id, content0, DocumentStatus::ACTUAL, ratings0),
                    "Successful double addition of valid document. Check double addition filtering"s);
        ASSERT_EQUAL(server.GetDocumentCount(), 1);
    }
    {
        SearchServer server;
        ASSERT_EQUAL(server.GetDocumentCount(), 0);
        ASSERT_HINT(!server.AddDocument(doc2_id, content2, DocumentStatus::ACTUAL, ratings2),
                    "Successful addition of invalid document. Check --prefix filtering"s);
        ASSERT_EQUAL_HINT(server.GetDocumentCount(), 0, "Invalid document saved"s);
    }
    {
        SearchServer server;
        ASSERT_EQUAL(server.GetDocumentCount(), 0);
        ASSERT_HINT(!server.AddDocument(doc3_id, content3, DocumentStatus::ACTUAL, ratings3),
                    "Successful addition of invalid document. Check standalone '-' filtering"s);
        ASSERT_EQUAL_HINT(server.GetDocumentCount(), 0, "Invalid document saved"s);
    }
    {
        SearchServer server;
        ASSERT_EQUAL(server.GetDocumentCount(), 0);
        ASSERT_HINT(!server.AddDocument(doc4_id, content4, DocumentStatus::ACTUAL, ratings4),
                    "Successful addition of invalid document. Check ASCII 0-31 filtering"s);
        ASSERT_EQUAL_HINT(server.GetDocumentCount(), 0, "Invalid document saved"s);
    }
}

void TestInvalidQuery() {
    const int doc0_id = 42;
    const string content0 = "cat-dog in the city"s;
    const vector<int> ratings0 = {1, 2, 3};
    {
        SearchServer server;
        (void) server.AddDocument(doc0_id, content0, DocumentStatus::ACTUAL, ratings0);
        vector<Document> result;
        ASSERT_HINT(server.FindTopDocuments("cat-dog in the city"s, result), "Ignoring of valid query."s);
        ASSERT_EQUAL(result.size(), 1);
    }
    {
        SearchServer server;
        (void) server.AddDocument(doc0_id, content0, DocumentStatus::ACTUAL, ratings0);
        vector<Document> result;
        ASSERT_HINT(!server.FindTopDocuments("cat --in the city"s, result),
                    "Processing of invalid query. Check --prefix filtering"s);
        ASSERT_EQUAL(result.size(), 0);
    }
    {
        SearchServer server;
        (void) server.AddDocument(doc0_id, content0, DocumentStatus::ACTUAL, ratings0);
        vector<Document> result;
        ASSERT_HINT(!server.FindTopDocuments("dog in the - city"s, result),
                    "Processing of invalid query. Check standalone '-' filtering"s);
        ASSERT_EQUAL(result.size(), 0);
    }
    {
        SearchServer server;
        (void) server.AddDocument(doc0_id, content0, DocumentStatus::ACTUAL, ratings0);
        vector<Document> result;
        ASSERT_HINT(!server.FindTopDocuments("dog in the N\27O\26W ! cat city"s, result),
                    "Processing of invalid query. Check ASCII 0-31 filtering"s);
        ASSERT_EQUAL(result.size(), 0);
    }
}

void TestMatchDocumentInvalidQuery() {
    const int doc0_id = 42;
    const string content0 = "cat-dog in the city"s;
    const vector<int> ratings0 = {1, 2, 3};
    {
        SearchServer server;
        (void) server.AddDocument(doc0_id, content0, DocumentStatus::ACTUAL, ratings0);
        tuple<vector<string>, DocumentStatus> result;
        ASSERT_HINT(server.MatchDocument("cat-dog in the city"s, 42, result), "Ignoring of valid query."s);
        const auto [matched_words, status] = result;
        ASSERT_EQUAL_HINT(matched_words.size(), 4, "Matched words number is incorrect."s);
        ASSERT_EQUAL(matched_words[0], "cat-dog"s);
        ASSERT_EQUAL(matched_words[1], "city"s);
        ASSERT_EQUAL(matched_words[2], "in"s);
        ASSERT_EQUAL(matched_words[3], "the"s);
    }
    {
        SearchServer server;
        (void) server.AddDocument(doc0_id, content0, DocumentStatus::ACTUAL, ratings0);
        tuple<vector<string>, DocumentStatus> result;
        ASSERT_HINT(!server.MatchDocument("cat --in the city"s, 42, result),
                    "Processing of invalid query. Check --prefix filtering"s);
        const auto [matched_words, status] = result;
        ASSERT(matched_words.empty());
        ASSERT(status == DocumentStatus::ACTUAL);
    }
    {
        SearchServer server;
        (void) server.AddDocument(doc0_id, content0, DocumentStatus::ACTUAL, ratings0);
        tuple<vector<string>, DocumentStatus> result;
        ASSERT_HINT(!server.MatchDocument("dog in the - city"s, 42, result),
                    "Processing of invalid query. Check standalone '-' filtering"s);
        const auto [matched_words, status] = result;
        ASSERT(matched_words.empty());
        ASSERT(status == DocumentStatus::ACTUAL);
    }
    {
        SearchServer server;
        (void) server.AddDocument(doc0_id, content0, DocumentStatus::ACTUAL, ratings0);
        tuple<vector<string>, DocumentStatus> result;
        ASSERT_HINT(!server.MatchDocument("dog in the N\27O\26W ! cat city"s, 42, result),
                    "Processing of invalid query. Check ASCII 0-31 filtering"s);
        const auto [matched_words, status] = result;
        ASSERT(matched_words.empty());
        ASSERT(status == DocumentStatus::ACTUAL);
    }
}

void TestGetDocumentId () {
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
        (void) server.AddDocument(doc0_id, content0, DocumentStatus::ACTUAL, ratings0);
        (void) server.AddDocument(doc1_id, content1, DocumentStatus::ACTUAL, ratings1);
        (void) server.AddDocument(doc2_id, content2, DocumentStatus::ACTUAL, ratings2);
        ASSERT_EQUAL(server.GetDocumentCount(), 3);
        ASSERT_EQUAL(server.GetDocumentId(2), 33);
        ASSERT_EQUAL(server.GetDocumentId(1), 24);
        ASSERT_EQUAL(server.GetDocumentId(0), 42);
        ASSERT_EQUAL_HINT(server.GetDocumentId(-5), -1, "Incorrect handling of negative index query."s);
        ASSERT_EQUAL_HINT(server.GetDocumentId(6), -1, "Incorrect out of range index query"s);
    }
}

void TestSearchServer() {
    RUN_TEST(TestAddInvalidDocument);
    RUN_TEST(TestInvalidQuery);
    RUN_TEST(TestMatchDocumentInvalidQuery);
    RUN_TEST(TestGetDocumentId);
}

// ==================== для примера =========================

void PrintDocument(const Document& document) {
    cout << "{ "s
         << "document_id = "s << document.id << ", "s
         << "relevance = "s << document.relevance << ", "s
         << "rating = "s << document.rating << " }"s << endl;
}

int main() {
    TestSearchServer();
}