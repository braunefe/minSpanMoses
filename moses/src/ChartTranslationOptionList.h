/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2006 University of Edinburgh

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
***********************************************************************/

#pragma once

#include "ChartTranslationOption.h"
#include "StackVec.h"

#include <vector>

namespace Moses
{

class TargetPhraseCollection;
class WordsRange;

//! a list of target phrases that is trsnalated from the same source phrase
class ChartTranslationOptionList
{
 public:
  ChartTranslationOptionList(size_t);
  ~ChartTranslationOptionList();

  const ChartTranslationOption &Get(size_t i) const { return *m_collection[i]; }

  //! number of translation options
  size_t GetSize() const { return m_size; }

  void Add(const TargetPhraseCollection &, const StackVec &,
           const WordsRange &);

  void Clear();
  void ShrinkToLimit();
  void ApplyThreshold();

 private:
  typedef std::vector<ChartTranslationOption*> CollType;

  struct ScoreThresholdPred
  {
    ScoreThresholdPred(float threshold) : m_thresholdScore(threshold) {}
    bool operator()(const ChartTranslationOption *option)
    {
      return option->GetEstimateOfBestScore() >= m_thresholdScore;
    }
    float m_thresholdScore;
  };

  CollType m_collection;
  size_t m_size;
  float m_scoreThreshold;
  const size_t m_ruleLimit;
};

}
