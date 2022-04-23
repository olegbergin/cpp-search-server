#include "search_server.h"

using namespace std;

SearchServer::SearchServer(const string& stop_words_text)
    : SearchServer(SplitIntoWords(stop_words_text))
{
}

void SearchServer::AddDocument(int document_id, const string& document, DocumentStatus status, const vector<int>& ratings) {
    if (document_id < 0) {
        throw invalid_argument("документ с отрицательным id"s);
    }
    if (documents_.count(document_id) > 0) {
        throw invalid_argument("документ c таким id уже существует"s);
    }

    vector<string> words = SplitIntoWordsNoStop(document);
    const double inv_word_count = 1.0 / words.size();
    for (const string& word : words) {
        word_to_document_freqs_[word][document_id] += inv_word_count;
        doc_to_word_freq_[document_id][word] += inv_word_count;
    }
    documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });
    document_ids_.emplace(document_id);
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query, DocumentStatus status) const {
    return FindTopDocuments(
        raw_query,
        [status](int document_id, DocumentStatus document_status, int rating) {
            return document_status == status;
        });
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query) const {
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

size_t SearchServer::GetDocumentCount() const {
    return documents_.size();
}

std::set<int>::const_iterator SearchServer::begin() const {
    return document_ids_.begin();
}

std::set<int>::const_iterator SearchServer::end() const {
    return document_ids_.end();
}

const map<string, double>& SearchServer::GetWordFrequencies(int document_id) {

    static const map<string, double> s_empty;
    if (!doc_to_word_freq_.count(document_id)) {
        return s_empty;
    }
    return doc_to_word_freq_.at(document_id);
}

tuple<vector<string>, DocumentStatus> SearchServer::MatchDocument(const string& raw_query, int document_id) const {
    const Query query = ParseQuery(raw_query);

    vector<string> matched_words;
    for (const string& word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.push_back(word);
        }
    }
    for (const string& word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.clear();
            break;
        }
    }
    return tuple{ matched_words, documents_.at(document_id).status };
}

    /*void SearchServer::RemoveDocument(int document_id){
        if (!document_ids_.count(document_id)) {
            return;
        }
        for (auto& [word, freq] : doc_to_word_freq_[document_id]) {
            word_to_document_freqs_[word].erase(document_id);
        }
        document_ids_.erase(document_id);
        documents_.erase(document_id);
        doc_to_word_freq_.erase(document_id);
    }*/
void SearchServer::RemoveDocument(int document_id) {
    if (!document_ids_.count(document_id)) {
        return;
    }
    for (auto& [word, ids_freqs] : word_to_document_freqs_) {
        ids_freqs.erase(document_id);
    }
    document_ids_.erase(document_id);
    documents_.erase(document_id);
    doc_to_word_freq_.erase(document_id);
}

bool SearchServer::IsStopWord(const string& word) const {
    return stop_words_.count(word) > 0;
}

bool SearchServer::IsValidWord(const string& word) {
    // A valid word must not contain special characters
    if (word[0] == '-') {
        if (word[1] == '-' || word.length() == 1) {
            return false;
        }
    }
    return none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
        });
}

vector<string> SearchServer::SplitIntoWordsNoStop(const string& text ) const {
    vector<string> result;
    for (const string& word : SplitIntoWords(text)) {
        if (!IsValidWord(word)) {
            throw invalid_argument("недопустимые символы в тексте добавляемого документа"s);
        }
        if (!IsStopWord(word)) {
            result.push_back(word);
        }
    }
    return result;
}

int SearchServer::ComputeAverageRating(const vector<int>& ratings) {
    if (ratings.empty()) {
        return 0;
    }
    int rating_sum = accumulate(ratings.begin(), ratings.end(), 0);
    return rating_sum / static_cast<int>(ratings.size());
}

SearchServer::QueryWord SearchServer::ParseQueryWord(string text) const {
    bool is_minus = false;
    if (IsValidWord(text)) {
        if (text[0] == '-') {
            is_minus = true;
            text = text.substr(1);
        }
    }
    else {
        throw invalid_argument("запрос не корректен"s);
    }
    return { text, is_minus, IsStopWord(text) };
}

SearchServer::Query SearchServer::ParseQuery(const string& text) const {
    Query query;
    for (const string& word : SplitIntoWords(text)) {
        const QueryWord query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                query.minus_words.insert(query_word.data);
            }
            else {
                query.plus_words.insert(query_word.data);
            }
        }
        return query;
    }
}

double SearchServer::ComputeWordInverseDocumentFreq(const string& word) const {
    return std::log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}