#include "remove_duplicates.h"

using namespace std;

void RemoveDuplicates(SearchServer& search_server) {
    set<int> documents_to_remove;
    map<set<string>, int> words_to_document;
    for (const int document_id : search_server) {
        set<string> document_words;
        const map<string, double> word_freqs = search_server.GetWordFrequencies(document_id);
        for (auto [word, freqs] : word_freqs) {
            document_words.insert(word);
        }
        if (words_to_document.count(document_words)) {
            int prev_doc_id = words_to_document[document_words];
            if (prev_doc_id > document_id) {
                words_to_document[document_words] = document_id;
                documents_to_remove.insert(prev_doc_id);
            }
            else {
                documents_to_remove.insert(document_id);
            }
        }
        else {
            words_to_document[document_words] = document_id;
        }
    }
    for (int document_id : documents_to_remove) {
        cout << "Found duplicate document id " << document_id << endl;
        search_server.RemoveDocument(document_id);
    }
}