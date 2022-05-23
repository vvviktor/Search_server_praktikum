#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>
#include <numeric>

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
    SearchServer() = default; // Этот конструктор был нужен для удобства тестирования.

    template<typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words)
            : stop_words_(MakeUniqueNonEmptyStrings(stop_words)) {
        if (!all_of(stop_words_.begin(), stop_words_.end(), [](const auto& word) {
            return IsValidWord(word);
        })) {
            throw invalid_argument("Forbidden characters in stop-words."s);
        }
    }

    explicit SearchServer(const string& stop_words_text)
            : SearchServer(SplitIntoWords(stop_words_text)) {
    }

    void AddDocument(int document_id, const string& document, DocumentStatus status,
                     const vector<int>& ratings) {
        if (document_id < 0) {
            throw invalid_argument("Negative document ID."s);
        } else if (documents_.count(document_id) > 0) {
            throw invalid_argument("Double addition of the document."s);
        }
        const vector<string> words = SplitIntoWordsNoStop(document);
        const double inv_word_count = 1.0 / words.size();
        for (const string& word: words) {
            word_to_document_freqs_[word][document_id] += inv_word_count;
        }
        documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status});
        document_ids_.push_back(document_id);
    }

    template<typename DocumentPredicate>
    vector<Document> FindTopDocuments(const string& raw_query, DocumentPredicate document_predicate) const {
        const Query query = ParseQuery(raw_query);
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
        return matched_documents;
    }

    vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus status) const {
        return FindTopDocuments(
                raw_query,
                [status](int document_id, DocumentStatus document_status, int rating) {
                    return document_status == status;
                });
    }

    vector<Document> FindTopDocuments(const string& raw_query) const {
        return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
    }

    int GetDocumentCount() const {
        return documents_.size();
    }

    int GetDocumentId(int index) const {
        return document_ids_.at(index);
    }

    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query, int document_id) const {
        const Query query = ParseQuery(raw_query);
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
        return tuple{matched_words, documents_.at(document_id).status};
    }

private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };

    const set<string> stop_words_;
    map<string, map<int, double>> word_to_document_freqs_;
    map<int, DocumentData> documents_;
    vector<int> document_ids_;

    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }

    static bool IsValidWord(const string& word) {
        // A valid word must not contain special characters
        return none_of(word.begin(), word.end(), [](char c) {
            return c >= '\0' && c < ' ';
        });
    }

    vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
        for (const string& word: SplitIntoWords(text)) {
            if (!IsValidWord(word)) {
                throw invalid_argument("Document contains forbidden characters."s);
            }
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
        return accumulate(ratings.begin(), ratings.end(), 0) / static_cast<int>(ratings.size());
    }

    struct QueryWord {
        string data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(string text) const {
        if (text.empty()) {
            throw invalid_argument("Empty request."s);
        }
        bool is_minus = false;
        if (text[0] == '-') {
            is_minus = true;
            text = text.substr(1);
        }
        if (text.empty()) {
            throw invalid_argument("Standalone '-' in request."s);
        }
        if (text[0] == '-') {
            throw invalid_argument("'--' in request."s);
        }
        if (!IsValidWord(text)) {
            throw invalid_argument("Forbidden characters in request."s);
        }
        return QueryWord{text, is_minus, IsStopWord(text)};
    }

    struct Query {
        set<string> plus_words;
        set<string> minus_words;
    };

    Query ParseQuery(const string& text) const {
        Query query;
        for (const string& word: SplitIntoWords(text)) {
            const QueryWord query_word = ParseQueryWord(word);
            if (!query_word.is_stop) {
                if (query_word.is_minus) {
                    query.minus_words.insert(query_word.data);
                } else {
                    query.plus_words.insert(query_word.data);
                }
            }
        }
        return query;
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

template<typename It>
class IteratorRange {
public:
    explicit IteratorRange(It begin, It end, size_t size) : begin_(begin), end_(end), size_(size) {
    }

    It begin() const {
        return begin_;
    }

    size_t size() const {
        return size_;
    }

    It end() const {
        return end_;
    }

private:
    It begin_;
    It end_;
    size_t size_;
};

template<typename It>
class Paginator {
public:
    explicit Paginator(It begin, It end, size_t size) {
        for (auto page_begin_it = begin; page_begin_it != end;) {
            if (distance(page_begin_it, end) >= size) {
                It page_end_it = next(page_begin_it, size);
                pages_.push_back(IteratorRange(page_begin_it, page_end_it, size));
                advance(page_begin_it, size);
            } else {
                It page_end_it = next(page_begin_it, distance(page_begin_it, end));
                pages_.push_back(IteratorRange(page_begin_it, page_end_it, distance(page_begin_it, end)));
                break;
            }
        }
    }

    auto begin() const {
        return pages_.begin();
    }

    auto end() const {
        return pages_.end();
    }

private:
    vector<IteratorRange<It>> pages_;
};

ostream& operator<<(ostream& out, const Document& document) {
    out << "{ "s
        << "document_id = "s << document.id << ", "s
        << "relevance = "s << document.relevance << ", "s
        << "rating = "s << document.rating << " }"s;
    return out;
}

template<typename It>
ostream& operator<<(ostream& out, const IteratorRange<It> page) {
    for (auto it = page.begin(); it != page.end(); ++it) {
        out << *it;
    }
    return out;
}

template<typename Container>
auto Paginate(const Container& c, size_t page_size) {
    return Paginator(c.begin(), c.end(), page_size);
}

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

// ==================== для примера =========================

void PrintDocument(const Document& document) {
    cout << "{ "s
         << "document_id = "s << document.id << ", "s
         << "relevance = "s << document.relevance << ", "s
         << "rating = "s << document.rating << " }"s << endl;
}

int main() {
    TestSearchServer();
    SearchServer search_server("and with"s);

    search_server.AddDocument(1, "funny pet and nasty rat"s, DocumentStatus::ACTUAL, {7, 2, 7});
    search_server.AddDocument(2, "funny pet with curly hair"s, DocumentStatus::ACTUAL, {1, 2, 3});
    search_server.AddDocument(3, "big cat nasty hair"s, DocumentStatus::ACTUAL, {1, 2, 8});
    search_server.AddDocument(4, "big dog cat Vladislav"s, DocumentStatus::ACTUAL, {1, 3, 2});
    search_server.AddDocument(5, "big dog hamster Borya"s, DocumentStatus::ACTUAL, {1, 1, 1});

    const auto search_results = search_server.FindTopDocuments("curly dog"s);
    int page_size = 2;
    const auto pages = Paginate(search_results, page_size);

    // Выводим найденные документы по страницам
    for (auto page = pages.begin(); page != pages.end(); ++page) {
        cout << *page << endl;
        cout << "Page break"s << endl;
    }
}