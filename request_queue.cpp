#include "request_queue.h"

using namespace std;

RequestQueue::RequestQueue(const SearchServer& search_server) : server_(search_server), empty_requests_(0),
                                                                current_time_(0) {
}

vector<Document> RequestQueue::AddFindRequest(const string& raw_query, DocumentStatus status) {
    return QueryProcess(raw_query, server_.FindTopDocuments(raw_query, status));
}

vector<Document> RequestQueue::AddFindRequest(const string& raw_query) {
    return QueryProcess(raw_query, server_.FindTopDocuments(raw_query));
}

int RequestQueue::GetNoResultRequests() const {
    return empty_requests_;
}

RequestQueue::QueryResult::QueryResult(uint64_t timestamp, const string& raw_query, const vector<Document>& result,
                                       bool empty)
        : timestamp(timestamp), raw_query(raw_query),
          result(result),
          empty(empty) {
}

vector<Document> RequestQueue::QueryProcess(const string& raw_query, const vector<Document>& result) {
    ++current_time_;
    QueryResult query_result(current_time_, raw_query, result, result.empty());
    while (!requests_.empty() && min_in_day_ <= current_time_ - requests_.front().timestamp) {
        if (requests_.front().empty) {
            --empty_requests_;
        }
        requests_.pop_front();
    }
    requests_.push_back(query_result);
    if (result.empty()) {
        ++empty_requests_;
    }
    return result;
}