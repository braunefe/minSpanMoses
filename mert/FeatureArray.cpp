/*
 *  FeatureArray.cpp
 *  mert - Minimum Error Rate Training
 *
 *  Created by Nicola Bertoldi on 13/05/08.
 *
 */

#include <fstream>
#include "FeatureArray.h"
#include "FileStream.h"
#include "Util.h"

FeatureArray::FeatureArray()
    : m_index(""), m_num_features(0), m_sparse_flag(false) {}

FeatureArray::~FeatureArray() {}

void FeatureArray::savetxt(ostream* os)
{
  *os << FEATURES_TXT_BEGIN << " " << m_index << " " << m_array.size()
          << " " << m_num_features << " " << m_features << endl;
  for (featarray_t::iterator i = m_array.begin(); i != m_array.end(); ++i) {
    i->savetxt(os);
    *os << endl;
  }
  *os << FEATURES_TXT_END << endl;
}

void FeatureArray::savebin(ostream* os)
{
  *os << FEATURES_BIN_BEGIN << " " << m_index << " " << m_array.size()
          << " " << m_num_features << " " << m_features << endl;
  for (featarray_t::iterator i = m_array.begin(); i != m_array.end(); ++i)
    i->savebin(os);

  *os << FEATURES_BIN_END << endl;
}


void FeatureArray::save(ostream* os, bool bin)
{
  if (size() <= 0) return;
  if (bin) {
    savebin(os);
  } else {
    savetxt(os);
  }
}

void FeatureArray::save(const string &file, bool bin)
{
  ofstream ofs(file.c_str(), ios::out);
  if (!ofs) {
    cerr << "Failed to open " << file << endl;
    exit(1);
  }
  ostream *os = &ofs;
  save(os, bin);
  ofs.close();
}

void FeatureArray::save(bool bin)
{
  save(&cout, bin);
}

void FeatureArray::loadbin(istream* is, size_t n)
{
  FeatureStats entry(m_num_features);
  for (size_t i = 0 ; i < n; i++) {
    entry.loadbin(is);
    add(entry);
  }
}

void FeatureArray::loadtxt(istream* is, size_t n)
{
  FeatureStats entry(m_num_features);

  for (size_t i = 0; i < n; i++) {
    entry.loadtxt(is);
    add(entry);
    if (entry.getSparse().size()>0)
      m_sparse_flag = true;
  }
}

void FeatureArray::load(istream* is)
{
  size_t number_of_entries = 0;
  bool binmode = false;

  string substring, stringBuf;
  string::size_type loc;

  getline(*is, stringBuf);
  if (!is->good()) {
    return;
  }

  if (!stringBuf.empty()) {
    if ((loc = stringBuf.find(FEATURES_TXT_BEGIN)) == 0) {
      binmode = false;
    } else if ((loc = stringBuf.find(FEATURES_BIN_BEGIN)) == 0) {
      binmode = true;
    } else {
      TRACE_ERR("ERROR: FeatureArray::load(): Wrong header");
      return;
    }
    getNextPound(stringBuf, substring);
    getNextPound(stringBuf, substring);
    m_index = substring;
    getNextPound(stringBuf, substring);
    number_of_entries = atoi(substring.c_str());
    getNextPound(stringBuf, substring);
    m_num_features = atoi(substring.c_str());
    m_features = stringBuf;
  }

  if (binmode) {
    loadbin(is, number_of_entries);
  } else {
    loadtxt(is, number_of_entries);
  }

  getline(*is, stringBuf);
  if (!stringBuf.empty()) {
    if ((loc = stringBuf.find(FEATURES_TXT_END)) != 0 &&
        (loc = stringBuf.find(FEATURES_BIN_END)) != 0) {
      TRACE_ERR("ERROR: FeatureArray::load(): Wrong footer");
      return;
    }
  }
}

void FeatureArray::load(const string &file)
{
  TRACE_ERR("loading data from " << file << endl);
  inputfilestream input_stream(file); // matches a stream with a file. Opens the file
  istream* is = &input_stream;
  load(is);
  input_stream.close();
}

void FeatureArray::merge(FeatureArray& e)
{
  //dummy implementation
  for (size_t i = 0; i < e.size(); i++)
    add(e.get(i));
}

bool FeatureArray::check_consistency() const
{
  const size_t sz = NumberOfFeatures();
  if (sz == 0)
    return true;

  for (featarray_t::const_iterator i = m_array.begin(); i != m_array.end(); i++) {
    if (i->size() != sz)
      return false;
  }
  return true;
}
