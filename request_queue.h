#pragma once

#include <vector>
#include <string>
#include <deque>
#include "search_server.h"
#include "document.h"

class RequestQueue {
public:
    explicit RequestQueue(const SearchServer& search_server);

    // сделаем "обёртки" для всех методов поиска, чтобы сохранять результаты для нашей статистики
    template<typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) {
        return QueryProcess(raw_query, server_.FindTopDocuments(raw_query, document_predicate));
    }

    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status);

    std::vector<Document> AddFindRequest(const std::string& raw_query);

    int GetNoResultRequests() const;

private:
    struct QueryResult {
        QueryResult() = default;

        QueryResult(uint64_t timestamp, const std::string& raw_query, const std::vector<Document>& result, bool empty);

        uint64_t timestamp = 0;
        std::string raw_query;
        std::vector<Document> result;
        bool empty = false;
    };

    std::deque<QueryResult> requests_;
    const static int min_in_day_ = 1440;
    const SearchServer& server_;
    int empty_requests_;
    uint64_t current_time_;

    std::vector<Document> QueryProcess(const std::string& raw_query, const std::vector<Document>& result);
};
