/***********************************************************************
 Moses - statistical machine translation system
 Copyright (C) 2006-2012 University of Edinburgh

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

#include "ChartRuleLookupManager.h"
#include "ChartTranslationOptionList.h"
#include "NonTerminal.h"
#include "RuleTable/UTrieNode.h"
#include "RuleTable/UTrie.h"
#include "Scope3Parser/ApplicableRuleTrie.h"
#include "Scope3Parser/StackLattice.h"
#include "Scope3Parser/StackLatticeBuilder.h"
#include "Scope3Parser/StackLatticeSearcher.h"
#include "Scope3Parser/VarSpanTrieBuilder.h"
#include "StaticData.h"

#include <memory>
#include <vector>

namespace Moses
{

class InputType;
class ChartCellCollection;
class ChartHypothesisCollection;
class WordsRange;

class Scope3Parser : public ChartRuleLookupManager
{
 public:
  Scope3Parser(const InputType &sentence,
               const ChartCellCollection &cellColl,
               const RuleTableUTrie &ruleTable,
               size_t maxChartSpan)
      : ChartRuleLookupManager(sentence, cellColl)
      , m_ruleTable(ruleTable)
      , m_maxChartSpan(maxChartSpan)
  {
    Init();
  }

  //MSPnew : optional minSpan argument set to 0
  void GetChartRuleCollection(
    const WordsRange &range,
    ChartTranslationOptionList &outColl,
    size_t minSpan=0);

 private:
  // Define a callback type for use by StackLatticeSearcher.
  struct MatchCallback
  {
    public:
      MatchCallback(const WordsRange &range,
                    ChartTranslationOptionList &out)
          : m_range(range)
          , m_out(out)
          , m_tpc(NULL) {}
      void operator()(const StackVec &stackVec)
      {
        m_out.Add(*m_tpc, stackVec, m_range);
      }
      const WordsRange &m_range;
      ChartTranslationOptionList &m_out;
      const TargetPhraseCollection *m_tpc;
  };

  void Init();
  void InitRuleApplicationVector();
  void FillSentenceMap(const Sentence &, SentenceMap &);
  void AddRulesToCells(const ApplicableRuleTrie &, std::pair<int, int>, int,
                       int);

  const RuleTableUTrie &m_ruleTable;
  std::vector<std::vector<std::vector<
    std::pair<const UTrieNode *, const VarSpanNode *> > > > m_ruleApplications;
  std::auto_ptr<VarSpanNode> m_varSpanTrie;
  StackVec m_emptyStackVec;
  const size_t m_maxChartSpan;
  StackLattice m_lattice;
  StackLatticeBuilder m_latticeBuilder;
  std::vector<VarSpanNode::NonTermRange> m_ranges;
  std::vector<std::vector<bool> > m_quickCheckTable;
};

}  // namespace Moses
