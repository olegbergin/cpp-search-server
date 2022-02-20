#include "request_queue.h"

using namespace std;

RequestQueue::RequestQueue(const SearchServer& search_server)
    : search_server_(search_server)
    , no_results_requests_(0)
    , current_time_(0)
    {
}

vector<Document> RequestQueue::AddFindRequest(const string& raw_query, DocumentStatus status) {
    return AddFindRequest(
        raw_query,
        [status](int document_id, DocumentStatus document_status, int rating) {
            return document_status == status;
        });
}

vector<Document> RequestQueue::AddFindRequest(const string& raw_query) {
    return AddFindRequest(raw_query, DocumentStatus::ACTUAL);
}

int RequestQueue::GetNoResultRequests() const {
    return no_results_requests_;
}

void RequestQueue::AddRequest(int results_num) {
    ++current_time_;
    while (!requests_.empty() && min_in_day_ <= current_time_ - requests_.front().timestamp) {
        if (0 == requests_.front().results) {
            --no_results_requests_;
        }
        requests_.pop_front();
    }
    requests_.push_back({ current_time_, results_num });
    if (0 == results_num) {
        ++no_results_requests_;
    }
}
