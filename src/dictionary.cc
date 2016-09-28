/**
 * Copyright (c) 2016-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#include "dictionary.h"

#include <assert.h>

#include <iostream>
#include <algorithm>
#include <iterator>
#include <unordered_map>
#include <cctype>

const std::string Dictionary::EOS = "</s>";
const std::string Dictionary::BOW = "<";
const std::string Dictionary::EOW = ">";

Dictionary::Dictionary(std::shared_ptr<Args> args) {
  args_ = args;
  size_ = 0;
  nwords_ = 0;
  nlabels_ = 0;
  ntokens_ = 0;
  word2int_.resize(MAX_VOCAB_SIZE);
  for (int32_t i = 0; i < MAX_VOCAB_SIZE; i++) {
    word2int_[i] = -1;
  }
}

int32_t Dictionary::find(const std::string& w) {
  int32_t h = hash(w) % MAX_VOCAB_SIZE;
  while (word2int_[h] != -1 && words_[word2int_[h]].word != w) {
    h = (h + 1) % MAX_VOCAB_SIZE;
  }
  return h;
}

/* modified to parse word labels 
 * ddu
 */
void Dictionary::add(const std::string& w) {
  int32_t h = find(w);
  ntokens_++;
  if (word2int_[h] == -1) {
    entry e;
    e.word = w;
    e.count = 1;

    if( w.find(args_->label) == 0) {
        cur_label_ = w;
        e.type = entry_type::label;
        e.nlabels = 1;
        e.labels.push_back(w);
    }
    else {
        e.type = entry_type::word;
        e.nlabels = 1;
        e.labels.push_back(cur_label_);
    }
    words_.push_back(e);
    word2int_[h] = size_++;
  } else {
    auto e = words_[word2int_[h]];
    if( e.type == entry_type::label ) {
      cur_label_ = e.word;
    }
    else if( std::find( e.labels.begin(), e.labels.end(), cur_label_) == e.labels.end() ) {
        words_[word2int_[h]].labels.push_back(cur_label_);
        ++(words_[word2int_[h]].nlabels);
    }
    words_[word2int_[h]].count++;
  }
}

int32_t Dictionary::nwords() {
  return nwords_;
}

int32_t Dictionary::nlabels() {
  return nlabels_;
}

int64_t Dictionary::ntokens() {
  return ntokens_;
}

// get all labels of a word
// return sorted label ids
std::vector<int32_t> Dictionary::getLabels( int32_t i) {
  assert(i >= 0);
  assert(i < nwords_);
  auto labels = words_[i].labels;
  std::vector<int32_t> label_ids;
  for( auto label : labels) {
      int32_t wid = getId(label);
      assert( wid >= nwords_ );  // 
      label_ids.push_back( wid-nwords_);
  }
  std::sort( label_ids.begin(), label_ids.end());
  return label_ids;
}

const std::vector<int32_t>& Dictionary::getNgrams(int32_t i) {
  assert(i >= 0);
  assert(i < nwords_);
  return words_[i].subwords;
}

const std::vector<int32_t> Dictionary::getNgrams(const std::string& word) {
  std::vector<int32_t> ngrams;
  int32_t i = getId(word);
  if (i >= 0) {
    ngrams = words_[i].subwords;
  } else {
    computeNgrams(BOW + word + EOW, ngrams);
  }
  return ngrams;
}

bool Dictionary::discard(int32_t id, real rand) {
  assert(id >= 0);
  assert(id < nwords_);
  if (args_->model == model_name::sup) return false;
  return rand > pdiscard_[id];
}

int32_t Dictionary::getId(const std::string& w) {
  int32_t h = find(w);
  return word2int_[h];
}

entry_type Dictionary::getType(int32_t id) {
  assert(id >= 0);
  assert(id < size_);
  return words_[id].type;
}

std::string Dictionary::getWord(int32_t id) {
  assert(id >= 0);
  assert(id < size_);
  return words_[id].word;
}

uint32_t Dictionary::hash(const std::string& str) {
  uint32_t h = 2166136261;
  for (size_t i = 0; i < str.size(); i++) {
    h = h ^ uint32_t(str[i]);
    h = h * 16777619;
  }
  return h;
}

void Dictionary::computeNgrams(const std::string& word,
                               std::vector<int32_t>& ngrams) {
  for (size_t i = 0; i < word.size(); i++) {
    std::string ngram;
    if ((word[i] & 0xC0) == 0x80) continue;
    for (size_t j = i, n = 1; j < word.size() && n <= args_->maxn; n++) {
      ngram.push_back(word[j++]);
      while (j < word.size() && (word[j] & 0xC0) == 0x80) {
        ngram.push_back(word[j++]);
      }
      if (n >= args_->minn) {
        int32_t h = hash(ngram) % args_->bucket;
        ngrams.push_back(nwords_ + h);
      }
    }
  }
}

void Dictionary::initNgrams() {
  for (size_t i = 0; i < size_; i++) {
    std::string word = BOW + words_[i].word + EOW;
    words_[i].subwords.push_back(i);
    computeNgrams(word, words_[i].subwords);
  }
}

bool Dictionary::readWord(std::istream& in, std::string& word)
{
  char c;
  std::streambuf& sb = *in.rdbuf();
  word.clear();
  while ((c = sb.sbumpc()) != EOF) {
    if (c == ' ' || c == '\n' || c == '\r' || c == '\t' || c == '\v' || c == '\f' || c == '\0') {
      if (word.empty()) {
        if (c == '\n') {
          word += EOS;
          return true;
        }
        continue;
      } else {
        if (c == '\n')
          sb.sungetc();
        return true;
      }
    }
    word.push_back(c);
  }
  // trigger eofbit
  in.get();
  return !word.empty();
}

void Dictionary::readFromFile(std::istream& in) {
  std::string word;
  int64_t minThreshold = 1;
  while (readWord(in, word)) {
    add(word);
    if (ntokens_ % 1000000 == 0 && args_->verbose > 1) {
      std::cout << "\rRead " << ntokens_  / 1000000 << "M words" << std::flush;
    }
    if (size_ > 0.75 * MAX_VOCAB_SIZE) {
      threshold(minThreshold++);
    }
  }

  std::cout << "\rRead " << ntokens_  / 1000000 << "M words" << std::endl;
  threshold(args_->minCount);
  initTableDiscard();
  initNgrams();
  std::cout << "Number of words:  " << nwords_ << std::endl;
  std::cout << "Number of labels: " << nlabels_ << std::endl;
  if (size_ == 0) {
    std::cerr << "Empty vocabulary. Try a smaller -minCount value." << std::endl;
    exit(EXIT_FAILURE);
  }
}

void Dictionary::threshold(int64_t t) {
  sort(words_.begin(), words_.end(), [](const entry& e1, const entry& e2) {
      if (e1.type != e2.type) return e1.type < e2.type;
      return e1.count > e2.count;
    });
  words_.erase(remove_if(words_.begin(), words_.end(), [&](const entry& e) {
        return e.type == entry_type::word && e.count < t;
      }), words_.end());
  words_.shrink_to_fit();
  size_ = 0;
  nwords_ = 0;
  nlabels_ = 0;
  for (int32_t i = 0; i < MAX_VOCAB_SIZE; i++) {
    word2int_[i] = -1;
  }
  for (auto it = words_.begin(); it != words_.end(); ++it) {
    int32_t h = find(it->word);
    word2int_[h] = size_++;
    if (it->type == entry_type::word) nwords_++;
    if (it->type == entry_type::label) nlabels_++;
  }
}

void Dictionary::initTableDiscard() {
  pdiscard_.resize(size_);
  for (size_t i = 0; i < size_; i++) {
    real f = real(words_[i].count) / real(ntokens_);
    pdiscard_[i] = sqrt(args_->t / f) + args_->t / f;
  }
}

std::vector<int64_t> Dictionary::getCounts(entry_type type) {
  std::vector<int64_t> counts;
  for (auto& w : words_) {
    if (w.type == type) counts.push_back(w.count);
  }
  return counts;
}

void Dictionary::addNgrams(std::vector<int32_t>& line, int32_t n) {
  int32_t line_size = line.size();
  for (int32_t i = 0; i < line_size; i++) {
    uint64_t h = line[i];
    for (int32_t j = i + 1; j < line_size && j < i + n; j++) {
      h = h * 116049371 + line[j];
      line.push_back(nwords_ + (h % args_->bucket));
    }
  }
}

int32_t Dictionary::getLine(std::istream& in,
                            std::vector<int32_t>& words,
                            std::vector<int32_t>& labels,
                            std::minstd_rand& rng) {
  std::uniform_real_distribution<> uniform(0, 1);
  std::string token;
  int32_t ntokens = 0;
  words.clear();
  labels.clear();
  if (in.eof()) {
    in.clear();
    in.seekg(std::streampos(0));
  }
  while (readWord(in, token)) {
    if (token == EOS) break;
    int32_t wid = getId(token);
    if (wid < 0) continue;
    entry_type type = getType(wid);
    ntokens++;
    if (type == entry_type::word && !discard(wid, uniform(rng))) {
      words.push_back(wid);
    }
    if (type == entry_type::label) {
      labels.push_back(wid - nwords_);
    }
    if (words.size() > MAX_LINE_SIZE && args_->model != model_name::sup) break;
  }
  return ntokens;
}

std::string Dictionary::getLabel(int32_t lid) {
  assert(lid >= 0);
  assert(lid < nlabels_);
  return words_[lid + nwords_].word;
}

void Dictionary::save(std::ostream& out) {
  out.write((char*) &size_, sizeof(int32_t));
  out.write((char*) &nwords_, sizeof(int32_t));
  out.write((char*) &nlabels_, sizeof(int32_t));
  out.write((char*) &ntokens_, sizeof(int64_t));
  for (int32_t i = 0; i < size_; i++) {
    entry e = words_[i];
    out.write(e.word.data(), e.word.size() * sizeof(char));
    out.put(0);
    out.write((char*) &(e.count), sizeof(int64_t));
    out.write((char*) &(e.type), sizeof(entry_type));
  }
}

void Dictionary::load(std::istream& in) {
  words_.clear();
  for (int32_t i = 0; i < MAX_VOCAB_SIZE; i++) {
    word2int_[i] = -1;
  }
  in.read((char*) &size_, sizeof(int32_t));
  in.read((char*) &nwords_, sizeof(int32_t));
  in.read((char*) &nlabels_, sizeof(int32_t));
  in.read((char*) &ntokens_, sizeof(int64_t));
  for (int32_t i = 0; i < size_; i++) {
    char c;
    entry e;
    while ((c = in.get()) != 0) {
      e.word.push_back(c);
    }
    in.read((char*) &e.count, sizeof(int64_t));
    in.read((char*) &e.type, sizeof(entry_type));
    words_.push_back(e);
    word2int_[find(e.word)] = i;
  }
  initTableDiscard();
  initNgrams();
}
