#include "search_server.h"
#include <numeric>

using namespace std;

SearchServer::SearchServer(string_view stop_words_text)
        : SearchServer(SplitIntoWords(stop_words_text)) {
}

SearchServer::SearchServer(const string& stop_words_text)
        : SearchServer(SplitIntoWords(stop_words_text)) {
}

void SearchServer::AddDocument(int document_id, string_view document, DocumentStatus status,
                               const vector<int>& ratings) {

    if (document_id < 0) {
        throw invalid_argument("Negative document ID."s);
    } else if (documents_.count(document_id) > 0) {
        throw invalid_argument("Double addition of the document."s);
    }

    const vector<std::string_view> words = SplitIntoWordsNoStop(document);
    const double inv_word_count = 1.0 / words.size();

    for (string_view word: words) {
        word_to_document_freqs_[string(word)][document_id] += inv_word_count;
        document_to_word_freqs_[document_id][string(word)] += inv_word_count;
    }

    documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status});
    document_ids_.insert(document_id);
}

void SearchServer::RemoveDocument(int document_id) {

    if (!document_ids_.count(document_id)) {
        throw invalid_argument("Attempt to remove non-existing ID."s);
    }

    for (const auto& [word, freqs]: document_to_word_freqs_.at(document_id)) {
        word_to_document_freqs_.at(word).erase(document_id);
    }

    document_to_word_freqs_.erase(document_id);
    documents_.erase(document_id);
    document_ids_.erase(document_id);
}

vector<Document> SearchServer::FindTopDocuments(string_view raw_query, DocumentStatus status) const {
    return FindTopDocuments(
            raw_query,
            [status](int document_id, DocumentStatus document_status, int rating) {
                return document_status == status;
            });
}

vector<Document> SearchServer::FindTopDocuments(string_view raw_query) const {
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

const map<string_view, double>& SearchServer::GetWordFrequencies(int document_id) const {

    if (!document_to_word_freqs_.count(document_id)) {
        static const map<string_view, double>& dummy = {};
        return dummy;
    }

    return (map<std::string_view, double>&) document_to_word_freqs_.at(document_id);
}


int SearchServer::GetDocumentCount() const {
    return static_cast<int>(documents_.size());
}

tuple<vector<string_view>, DocumentStatus>
SearchServer::MatchDocument(string_view raw_query, int document_id) const {

    if (!document_ids_.count(document_id)) {
        throw out_of_range("Document ID is out of range."s);
    }

    const Query query = ParseQuery(raw_query);
    vector<string_view> matched_words;

    if (any_of(query.minus_words.begin(), query.minus_words.end(),
               [this, document_id](std::string_view word) {
                   return document_to_word_freqs_.at(document_id).count(word);
               })) {
        return tuple{matched_words, documents_.at(document_id).status};
    }

    for (const string_view word: query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (document_to_word_freqs_.at(document_id).count(word)) {
            matched_words.push_back(word);
        }
    }

    return tuple{matched_words, documents_.at(document_id).status};
}

set<int>::const_iterator SearchServer::begin() const {
    return document_ids_.begin();
}

set<int>::const_iterator SearchServer::end() const {
    return document_ids_.end();
}

bool SearchServer::IsStopWord(string_view word) const {
    return stop_words_.count(word) > 0;
}

bool SearchServer::IsValidWord(string_view word) {
    // A valid word must not contain special characters
    return none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
    });
}

vector<string_view> SearchServer::SplitIntoWordsNoStop(string_view text) const {
    vector<string_view> words;
    for (const string_view word: SplitIntoWordsView(text)) {
        if (!IsValidWord(word)) {
            throw invalid_argument("Document contains forbidden characters."s);
        }
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}

int SearchServer::ComputeAverageRating(const std::vector<int>& ratings) {
    if (ratings.empty()) {
        return 0;
    }
    return accumulate(ratings.begin(), ratings.end(), 0) / static_cast<int>(ratings.size());
}

SearchServer::QueryWord SearchServer::ParseQueryWord(string_view text) const {
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

SearchServer::Query SearchServer::ParseQuery(string_view text) const {
    Query query;
    for (const string_view word: SplitIntoWordsView(text)) {
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

SearchServer::QueryPar SearchServer::ParseQueryPar(std::string_view text) const {
    QueryPar query;
    for (const string_view word: SplitIntoWordsView(text)) {
        const QueryWord query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                query.minus_words.emplace_back(query_word.data);
            } else {
                query.plus_words.emplace_back(query_word.data);
            }
        }
    }
    return query;
}

double SearchServer::ComputeWordInverseDocumentFreq(const string& word) const {
    return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}

