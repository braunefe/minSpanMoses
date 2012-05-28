/***********************************************************************
  Moses - factored phrase-based language decoder
  Copyright (C) 2011 University of Edinburgh

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
#ifndef moses_ChartRuleLookupManagerMinSpan_h
#define moses_ChartRuleLookupManagerMinSpan_h

#include <vector>

#if HAVE_CONFIG_H
#include "config.h"
#ifdef USE_BOOST_POOL
#include <boost/pool/object_pool.hpp>
#endif
#endif

#include "ChartRuleLookupManagerCYKPlus.h"
#include "DotChart.h"
#include "DotChartInMemory.h"
#include "NonTerminal.h"
#include "../RuleTable/PhraseDictionaryNodeSCFG.h"
#include "../RuleTable/PhraseDictionaryMinSpan.h"

//new : include ClauseBoundary class
//#include "ClauseBoundaries.h"

namespace Moses
{

class ChartTranslationOptionList;
class DottedRuleColl;
class WordsRange;

// Implementation of ChartRuleLookupManager for in-memory rule tables.
class ChartRuleLookupManagerMinSpan : public ChartRuleLookupManagerCYKPlus
{
public:
  ChartRuleLookupManagerMinSpan(const InputType &sentence,
                               const ChartCellCollection &cellColl,
                               const PhraseDictionaryMinSpan &ruleTable);

  ~ChartRuleLookupManagerMinSpan();

  virtual void GetChartRuleCollection(
    const WordsRange &range,
    ChartTranslationOptionList &outColl,
    size_t minSpan);

private:
  void ExtendPartialRuleApplication(
    const DottedRuleInMemory &prevDottedRule,
    size_t startPos,
    size_t endPos,
    size_t stackInd,
    DottedRuleColl &dottedRuleColl);

    //new : add clause boundaries vector as field of class ChartRuleLookupManagerMinSpan
    std::vector< std::vector<int> > m_clauseBounds;

    std::vector<DottedRuleColl*> m_dottedRuleColls;
    const PhraseDictionaryMinSpan &m_ruleTable;
#ifdef USE_BOOST_POOL
  // Use object pools to allocate the DottedRule and CoveredChartSpan objects
  // for this sentence.  We allocate a lot of them and this has been seen to
  // significantly improve performance, especially for multithreaded decoding.
  boost::object_pool<DottedRuleInMemory> m_dottedRulePool;
#endif
};

}  // namespace Moses

#endif
