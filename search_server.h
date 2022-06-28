#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <set>
#include <map>
#include <algorithm>
#include <stdexcept>
#include <cmath>
#include <iterator>
#include <execution>
#include <type_traits>
#include <cassert>
#include "document.h"
#include "string_processing.h"
#include "log_duration.h"

static const int MAX_RESULT_DOCUMENT_COUNT = 5;
static const double EPSILON = 1e-6;

class SearchServer {
public:
    SearchServer() = default; // Этот конструктор был нужен для удобства тестирования.

    template<typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words);

    explicit SearchServer(std::string_view stop_words_text);

    explicit SearchServer(const std::string& stop_words_text);

    void AddDocument(int document_id, std::string_view document, DocumentStatus status,
                     const std::vector<int>& ratings);

    void RemoveDocument(int document_id);

    template<class ExecutionPolicy>
    void RemoveDocument(ExecutionPolicy&& policy, int document_id);

    template<typename DocumentPredicate>
    std::vector<Document>
    FindTopDocuments(std::string_view raw_query, DocumentPredicate document_predicate) const;

    std::vector<Document> FindTopDocuments(std::string_view raw_query, DocumentStatus status) const;

    std::vector<Document> FindTopDocuments(std::string_view raw_query) const;

    const std::map<std::string_view, double>& GetWordFrequencies(int document_id) const;

    int GetDocumentCount() const;

    std::tuple<std::vector<std::string_view>, DocumentStatus>
    MatchDocument(std::string_view raw_query, int document_id) const;

    template<class ExecutionPolicy>
    std::tuple<std::vector<std::string_view>, DocumentStatus>
    MatchDocument(ExecutionPolicy&& policy, std::string_view raw_query, int document_id) const;

    std::set<int>::const_iterator begin() const;

    std::set<int>::const_iterator end() const;

private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };

    std::set<std::string, std::less<>> stop_words_;
    std::map<std::string, std::map<int, double>, std::less<>> word_to_document_freqs_;
    std::map<int, std::map<std::string, double, std::less<>>> document_to_word_freqs_;
    std::map<int, DocumentData> documents_;
    std::set<int> document_ids_;

    bool IsStopWord(std::string_view word) const;

    static bool IsValidWord(std::string_view word);

    std::vector<std::string_view> SplitIntoWordsNoStop(std::string_view text) const;

    static int ComputeAverageRating(const std::vector<int>& ratings);

    struct QueryWord {
        std::string_view data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(std::string_view text) const;

    struct Query {
        std::set<std::string_view> plus_words;
        std::set<std::string_view> minus_words;
    };

    struct QueryPar {
        std::vector<std::string_view> plus_words;
        std::vector<std::string_view> minus_words;
    };

    Query ParseQuery(std::string_view text) const;

    QueryPar ParseQueryPar(std::string_view text) const;

    double ComputeWordInverseDocumentFreq(const std::string& word) const;

    template<typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const Query& query, DocumentPredicate document_predicate) const;
};

template<typename StringContainer>
SearchServer::SearchServer(const StringContainer& stop_words)
        : stop_words_(MakeUniqueNonEmptyStrings(stop_words)) {
    if (!std::all_of(stop_words_.begin(), stop_words_.end(), [](const auto& word) {
        return IsValidWord(word);
    })) {
        using namespace std::literals::string_literals;
        throw std::invalid_argument("Forbidden characters in stop-words."s);
    }
}

template<typename DocumentPredicate>
std::vector<Document>
SearchServer::FindTopDocuments(std::string_view raw_query, DocumentPredicate document_predicate) const {
    const Query query = ParseQuery(raw_query);
    auto matched_documents = FindAllDocuments(query, document_predicate);

    sort(matched_documents.begin(), matched_documents.end(), [](const Document& lhs, const Document& rhs) {
        if (std::abs(lhs.relevance - rhs.relevance) < EPSILON) {
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

template<class ExecutionPolicy>
void SearchServer::RemoveDocument(ExecutionPolicy&& policy, int document_id) {

    if (!document_ids_.count(document_id)) {
        using namespace std::literals::string_literals;
        throw std::invalid_argument("Attempt to remove non-existing ID."s);
    }

    std::vector<std::string> words_to_process;
    words_to_process.reserve(document_to_word_freqs_.at(document_id).size());

    for (const auto& [word, freq]: document_to_word_freqs_.at(document_id)) {
        words_to_process.emplace_back(word);
    }

    std::for_each(std::forward<ExecutionPolicy>(policy), words_to_process.begin(),
                  words_to_process.end(), [this, document_id](auto& word) {
                word_to_document_freqs_.at(word).erase(document_id);
            });

    document_to_word_freqs_.erase(document_id);
    documents_.erase(document_id);
    document_ids_.erase(document_id);
}

template<class ExecutionPolicy>
std::tuple<std::vector<std::string_view>, DocumentStatus>
SearchServer::MatchDocument(ExecutionPolicy&& policy, std::string_view raw_query, int document_id) const {
    assert(std::is_execution_policy_v<ExecutionPolicy>);  // В тренажере этот assert прерывает выполнение программы, то есть на самом деле параллельные алгоритмы не выполняются.
    if constexpr(std::is_same_v<std::decay_t<ExecutionPolicy>,  // 26-й тест в тренажере успешно выполняется только с этим условием.
            std::execution::sequenced_policy>) {
        return MatchDocument(raw_query, document_id);
    }

    if (!document_ids_.count(document_id)) {
        using namespace std::literals::string_literals;
        throw std::out_of_range("Document ID is out of range."s);
    }

    const QueryPar query = ParseQueryPar(raw_query);
    std::vector<std::string_view> matched_words(query.plus_words.size());

    if (std::any_of(std::forward<ExecutionPolicy>(policy), query.minus_words.begin(), query.minus_words.end(),
                    [this, document_id](std::string_view word) {
                        return document_to_word_freqs_.at(document_id).count(word);
                    })) {
        matched_words.clear();
        return std::tuple{matched_words, documents_.at(document_id).status};
    }

    auto it = std::copy_if(std::forward<ExecutionPolicy>(policy), query.plus_words.begin(), query.plus_words.end(),
                           matched_words.begin(),
                           [this, document_id](std::string_view word) {
                               return document_to_word_freqs_.at(document_id).count(word);
                           });
    matched_words.erase(it, matched_words.end());
    std::sort(std::forward<ExecutionPolicy>(policy), matched_words.begin(), matched_words.end());
    matched_words.erase(std::unique(std::forward<ExecutionPolicy>(policy), matched_words.begin(), matched_words.end()),
                        matched_words.end());

    return std::tuple{matched_words, documents_.at(document_id).status};
}

template<typename DocumentPredicate>
std::vector<Document>
SearchServer::FindAllDocuments(const Query& query, DocumentPredicate document_predicate) const {
    std::map<int, double> document_to_relevance;
    for (std::string_view word: query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        const double inverse_document_freq = ComputeWordInverseDocumentFreq(std::string(word));
        for (const auto [document_id, term_freq]: word_to_document_freqs_.at(std::string(word))) {
            const auto& document_data = documents_.at(document_id);
            if (document_predicate(document_id, document_data.status, document_data.rating)) {
                document_to_relevance[document_id] += term_freq * inverse_document_freq;
            }
        }
    }

    for (std::string_view word: query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        for (const auto [document_id, _]: word_to_document_freqs_.at(std::string(word))) {
            document_to_relevance.erase(document_id);
        }
    }

    std::vector<Document> matched_documents;
    for (const auto [document_id, relevance]: document_to_relevance) {
        matched_documents.push_back({document_id, relevance, documents_.at(document_id).rating});
    }
    return matched_documents;
}

