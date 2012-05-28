#include "BleuScorer.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <climits>
#include <fstream>
#include <iostream>
#include <stdexcept>

#include "util/check.hh"
#include "Ngram.h"
#include "Reference.h"
#include "Util.h"
#include "Vocabulary.h"

namespace {

// configure regularisation
const char KEY_REFLEN[] = "reflen";
const char REFLEN_AVERAGE[] = "average";
const char REFLEN_SHORTEST[] = "shortest";
const char REFLEN_CLOSEST[] = "closest";

} // namespace

BleuScorer::BleuScorer(const string& config)
    : StatisticsBasedScorer("BLEU", config),
      m_ref_length_type(CLOSEST) {
  const string reflen = getConfig(KEY_REFLEN, REFLEN_CLOSEST);
  if (reflen == REFLEN_AVERAGE) {
    m_ref_length_type = AVERAGE;
  } else if (reflen == REFLEN_SHORTEST) {
    m_ref_length_type = SHORTEST;
  } else if (reflen == REFLEN_CLOSEST) {
    m_ref_length_type = CLOSEST;
  } else {
    throw runtime_error("Unknown reference length strategy: " + reflen);
  }
}

BleuScorer::~BleuScorer() {}

size_t BleuScorer::CountNgrams(const string& line, NgramCounts& counts,
                               unsigned int n)
{
  assert(n > 0);
  vector<int> encoded_tokens;
  TokenizeAndEncode(line, encoded_tokens);
  for (size_t k = 1; k <= n; ++k) {
    //ngram order longer than sentence - no point
    if (k > encoded_tokens.size()) {
      continue;
    }
    for (size_t i = 0; i < encoded_tokens.size()-k+1; ++i) {
      vector<int> ngram;
      for (size_t j = i; j < i+k && j < encoded_tokens.size(); ++j) {
        ngram.push_back(encoded_tokens[j]);
      }
      counts.Add(ngram);
    }
  }
  return encoded_tokens.size();
}

void BleuScorer::setReferenceFiles(const vector<string>& referenceFiles)
{
  // Make sure reference data is clear
  m_references.reset();
  mert::VocabularyFactory::GetVocabulary()->clear();

  //load reference data
  for (size_t i = 0; i < referenceFiles.size(); ++i) {
    TRACE_ERR("Loading reference from " << referenceFiles[i] << endl);

    if (!OpenReference(referenceFiles[i].c_str(), i)) {
      throw runtime_error("Unable to open " + referenceFiles[i]);
    }
  }
}

bool BleuScorer::OpenReference(const char* filename, size_t file_id) {
  ifstream ifs(filename);
  if (!ifs) {
    cerr << "Cannot open " << filename << endl;
    return false;
  }
  return OpenReferenceStream(&ifs, file_id);
}

bool BleuScorer::OpenReferenceStream(istream* is, size_t file_id) {
  if (is == NULL) return false;

  string line;
  size_t sid = 0;
  while (getline(*is, line)) {
    line = applyFactors(line);
    if (file_id == 0) {
      Reference* ref = new Reference;
      m_references.push_back(ref);    // Take ownership of the Reference object.
    }
    if (m_references.size() <= sid) {
      cerr << "Reference " << file_id << "has too many sentences." << endl;
      return false;
    }
    NgramCounts counts;
    size_t length = CountNgrams(line, counts, kBleuNgramOrder);

    //for any counts larger than those already there, merge them in
    for (NgramCounts::const_iterator ci = counts.begin(); ci != counts.end(); ++ci) {
      const NgramCounts::Key& ngram = ci->first;
      const NgramCounts::Value newcount = ci->second;

      NgramCounts::Value oldcount = 0;
      m_references[sid]->get_counts()->Lookup(ngram, &oldcount);
      if (newcount > oldcount) {
        m_references[sid]->get_counts()->operator[](ngram) = newcount;
      }
    }
    //add in the length
    m_references[sid]->push_back(length);
    if (sid > 0 && sid % 100 == 0) {
      TRACE_ERR(".");
    }
    ++sid;
  }
  return true;
}

void BleuScorer::prepareStats(size_t sid, const string& text, ScoreStats& entry)
{
  if (sid >= m_references.size()) {
    stringstream msg;
    msg << "Sentence id (" << sid << ") not found in reference set";
    throw runtime_error(msg.str());
  }
  NgramCounts testcounts;
  // stats for this line
  vector<ScoreStatsType> stats(kBleuNgramOrder * 2);
  string sentence = applyFactors(text);
  const size_t length = CountNgrams(sentence, testcounts, kBleuNgramOrder);

  const int reference_len = CalcReferenceLength(sid, length);
  stats.push_back(reference_len);

  //precision on each ngram type
  for (NgramCounts::const_iterator testcounts_it = testcounts.begin();
       testcounts_it != testcounts.end(); ++testcounts_it) {
    const NgramCounts::Value guess = testcounts_it->second;
    const size_t len = testcounts_it->first.size();
    NgramCounts::Value correct = 0;

    NgramCounts::Value v = 0;
    if (m_references[sid]->get_counts()->Lookup(testcounts_it->first, &v)) {
      correct = min(v, guess);
    }
    stats[len * 2 - 2] += correct;
    stats[len * 2 - 1] += guess;
  }
  entry.set(stats);
}

float BleuScorer::calculateScore(const vector<int>& comps) const
{
  CHECK(comps.size() == kBleuNgramOrder * 2 + 1);

  float logbleu = 0.0;
  for (int i = 0; i < kBleuNgramOrder; ++i) {
    if (comps[2*i] == 0) {
      return 0.0;
    }
    logbleu += log(comps[2*i]) - log(comps[2*i+1]);

  }
  logbleu /= kBleuNgramOrder;
  // reflength divided by test length
  const float brevity = 1.0 - static_cast<float>(comps[kBleuNgramOrder * 2]) / comps[1];
  if (brevity < 0.0) {
    logbleu += brevity;
  }
  return exp(logbleu);
}

int BleuScorer::CalcReferenceLength(size_t sentence_id, size_t length) {
  switch (m_ref_length_type) {
    case AVERAGE:
      return m_references[sentence_id]->CalcAverage();
      break;
    case CLOSEST:
      return m_references[sentence_id]->CalcClosest(length);
      break;
    case SHORTEST:
      return m_references[sentence_id]->CalcShortest();
      break;
    default:
      cerr << "unknown reference types." << endl;
      exit(1);
  }
}

void BleuScorer::DumpCounts(ostream* os,
                            const NgramCounts& counts) const {
  for (NgramCounts::const_iterator it = counts.begin();
       it != counts.end(); ++it) {
    *os << "(";
    const NgramCounts::Key& keys = it->first;
    for (size_t i = 0; i < keys.size(); ++i) {
      if (i != 0) {
        *os << " ";
      }
      *os << keys[i];
    }
    *os << ") : " << it->second << ", ";
  }
  *os << endl;
}

float sentenceLevelBleuPlusOne(const vector<float>& stats) {
  CHECK(stats.size() == kBleuNgramOrder * 2 + 1);

  float logbleu = 0.0;
  for (int j = 0; j < kBleuNgramOrder; j++) {
    logbleu += log(stats[2 * j] + 1.0) - log(stats[2 * j + 1] + 1.0);
  }
  logbleu /= kBleuNgramOrder;
  const float brevity = 1.0 - stats[(kBleuNgramOrder * 2)] / stats[1];

  if (brevity < 0.0) {
    logbleu += brevity;
  }
  return exp(logbleu);
}
