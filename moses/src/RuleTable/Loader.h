/***********************************************************************
 Moses - statistical machine translation system
 Copyright (C) 2006-2011 University of Edinburgh

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

#include "RuleTable/Trie.h"
#include "TypeDef.h"
//MSPnew : include phrase dictionary min span
#include "PhraseDictionaryMinSpan.h"

#include <istream>
#include <vector>

namespace Moses
{

class LMList;
class WordPenaltyProducer;

// Abstract base class defining RuleTableLoader interface.  Friend of
// RuleTableTrie.
class RuleTableLoader
{
 public:
  virtual ~RuleTableLoader() {}

  virtual bool Load(const std::vector<FactorType> &input,
                    const std::vector<FactorType> &output,
                    std::istream &inStream,
                    const std::vector<float> &weight,
                    size_t tableLimit,
                    const LMList &languageModels,
                    const WordPenaltyProducer* wpProducer,
                    RuleTableTrie &) = 0;


  //MSPnew overload load function wiht phrase dictionary min span
  virtual bool Load(const std::vector<FactorType> &input,
                    const std::vector<FactorType> &output,
                    std::istream &inStream,
                    const std::vector<float> &weight,
                    size_t tableLimit,
                    const LMList &languageModels,
                    const WordPenaltyProducer* wpProducer,
                    bool IsMinSpan,
                    PhraseDictionaryMinSpan &) = 0;



 protected:
  // Provide access to RuleTableTrie's private SortAndPrune function.
  void SortAndPrune(RuleTableTrie &ruleTable) {
    ruleTable.SortAndPrune();
  }

  //MSPnew : overloaded : provide access to PhraseDictionarMinSpan's private SortAndPrune function.
  void SortAndPrune(bool IsMinSpan, PhraseDictionaryMinSpan &ruleTable) {
    ruleTable.SortAndPrune();
  }

  // Provide access to RuleTableTrie's private
  // GetOrCreateTargetPhraseCollection function.
  TargetPhraseCollection &GetOrCreateTargetPhraseCollection(
      RuleTableTrie &ruleTable
      , const Phrase &source
      , const TargetPhrase &target
      , const Word &sourceLHS) {
    return ruleTable.GetOrCreateTargetPhraseCollection(source, target, sourceLHS);
  }

   //MSPnew :: overloaded : provide access to PhraseDictionarySCFG's private
  // GetOrCreateTargetPhraseCollection function.
  TargetPhraseCollection &GetOrCreateTargetPhraseCollection(bool IsMinSpan,
      PhraseDictionaryMinSpan &ruleTable, const Phrase &source,
      const TargetPhrase &target) {
    return ruleTable.GetOrCreateTargetPhraseCollection(source, target);
  }
};

}  // namespace Moses
