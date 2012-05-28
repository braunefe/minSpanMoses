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

#include "ChartRuleLookupManagerMinSpan.h"

#include "../RuleTable/PhraseDictionarySCFG.h"
#include "../RuleTable/PhraseDictionaryMinSpan.h"
#include "InputType.h"
#include "ChartTranslationOptionList.h"
#include "CellCollection.h"
#include "DotChartInMemory.h"
#include "StaticData.h"
#include "NonTerminal.h"
#include "ChartCellCollection.h"
#include "ClauseBoundaries.h"
#include "Util.h"


namespace Moses
{

ChartRuleLookupManagerMinSpan::ChartRuleLookupManagerMinSpan(
  const InputType &src,
  const ChartCellCollection &cellColl,
  const PhraseDictionaryMinSpan &ruleTable)
  : ChartRuleLookupManagerCYKPlus(src, cellColl)
  , m_ruleTable(ruleTable)
{
  assert(m_dottedRuleColls.size() == 0);
  size_t sourceSize = src.GetSize();
  m_dottedRuleColls.resize(sourceSize);


 //MSPnew : if clause boundary option is on, set clause boundaries vector
    //otherwise set to empty vector
  const StaticData& staticData = StaticData::Instance();
        //check if option is on
    if (staticData.GetParam("clause-bounds").size() == 1)
    {
      m_clauseBounds = src.GetClauseBoundaries()->GetClauseBoundaries();
    }
    else
    {
        std::vector< std::vector<int> > empty_bounds;
        m_clauseBounds = empty_bounds;
    }

  const PhraseDictionaryNodeSCFG &rootNode = m_ruleTable.GetRootNode();

  for (size_t ind = 0; ind < m_dottedRuleColls.size(); ++ind) {
#ifdef USE_BOOST_POOL
    DottedRuleInMemory *initDottedRule = m_dottedRulePool.malloc();
    new (initDottedRule) DottedRuleInMemory(rootNode);
#else
    DottedRuleInMemory *initDottedRule = new DottedRuleInMemory(rootNode);
#endif

    DottedRuleColl *dottedRuleColl = new DottedRuleColl(sourceSize - ind + 1);
    dottedRuleColl->Add(0, initDottedRule); // init rule. stores the top node in tree

    m_dottedRuleColls[ind] = dottedRuleColl;
  }
}

ChartRuleLookupManagerMinSpan::~ChartRuleLookupManagerMinSpan()
{
  RemoveAllInColl(m_dottedRuleColls);
}

void ChartRuleLookupManagerMinSpan::GetChartRuleCollection(
  const WordsRange &range,
  ChartTranslationOptionList &outColl,
  size_t minSpan)
{
  size_t relEndPos = range.GetEndPos() - range.GetStartPos();
  size_t absEndPos = range.GetEndPos();

  //MSPnew : get start position of considered range
  size_t startOfFirst = range.GetStartPos();

  // MAIN LOOP. create list of nodes of target phrases

  // get list of all rules that apply to spans at same starting position
  DottedRuleColl &dottedRuleCol = *m_dottedRuleColls[range.GetStartPos()];
  const DottedRuleList &expandableDottedRuleList = dottedRuleCol.GetExpandableDottedRuleList();

  const ChartCellLabel &sourceWordLabel = GetCellCollection().Get(WordsRange(absEndPos, absEndPos)).GetSourceWordLabel();

  // loop through the rules
  // (note that expandableDottedRuleList can be expanded as the loop runs
  //  through calls to ExtendPartialRuleApplication())
  for (size_t ind = 0; ind < expandableDottedRuleList.size(); ++ind) {
    // rule we are about to extend
    const DottedRuleInMemory &prevDottedRule = *expandableDottedRuleList[ind];
    // we will now try to extend it, starting after where it ended
    size_t startPos = prevDottedRule.IsRoot()
                    ? range.GetStartPos()
                    : prevDottedRule.GetWordsRange().GetEndPos() + 1;

    // search for terminal symbol
    // (if only one more word position needs to be covered)
    if (startPos == absEndPos) {

      // look up in rule dictionary, if the current rule can be extended
      // with the source word in the last position
      const Word &sourceWord = sourceWordLabel.GetLabel();
      const PhraseDictionaryNodeSCFG *node = prevDottedRule.GetLastNode().GetChild(sourceWord);

      // if we found a new rule -> create it and add it to the list
      if (node != NULL) {
				// create the rule
#ifdef USE_BOOST_POOL
        DottedRuleInMemory *dottedRule = m_dottedRulePool.malloc();
        new (dottedRule) DottedRuleInMemory(*node, sourceWordLabel,
                                            prevDottedRule);
#else
        DottedRuleInMemory *dottedRule = new DottedRuleInMemory(*node,
                                                                sourceWordLabel,
                                                                prevDottedRule);
#endif
        dottedRuleCol.Add(relEndPos+1, dottedRule);
      }
    }

    // search for non-terminals
    size_t endPos, stackInd;

    // span is already complete covered? nothing can be done
    if (startPos > absEndPos)
      continue;

    else if (startPos == range.GetStartPos() && range.GetEndPos() > range.GetStartPos()) {
      // We're at the root of the prefix tree so won't try to cover the full
      // span (i.e. we don't allow non-lexical unary rules).  However, we need
      // to match non-unary rules that begin with a non-terminal child, so we
      // do that in two steps: during this iteration we search for non-terminals
      // that cover all but the last source word in the span (there won't
      // already be running nodes for these because that would have required a
      // non-lexical unary rule match for an earlier span).  Any matches will
      // result in running nodes being appended to the list and on subsequent
      // iterations (for this same span), we'll extend them to cover the final
      // word.
      endPos = absEndPos - 1;
      stackInd = relEndPos;
    }
    else
    {
      endPos = absEndPos;
      stackInd = relEndPos + 1;
    }

    ExtendPartialRuleApplication(prevDottedRule, startPos, endPos, stackInd,
                                 dottedRuleCol);
  }

  // list of rules that that cover the entire span
  DottedRuleList &rules = dottedRuleCol.Get(relEndPos + 1);

  // look up target sides for the rules
  size_t rulesLimit = StaticData::Instance().GetRuleLimit();
  DottedRuleList::const_iterator iterRule;
  for (iterRule = rules.begin(); iterRule != rules.end(); ++iterRule) {
    const DottedRuleInMemory &dottedRule = **iterRule;
    //std::cout << "Dotted rule is : " << dottedRule << std::endl;

    const PhraseDictionaryNodeSCFG &node = dottedRule.GetLastNode();
    //std::cout << "Phrase dictionary node is : " << node << std::endl;

    // look up target sides
    const TargetPhraseCollection *targetPhraseCollection = node.GetTargetPhraseCollection();

     //MSPnew : look at total covered span in order to determine if we are greater as the min span condition
    int end = dottedRule.GetWordsRange().GetEndPos();

    //MSPnew : determine start position : iterate over previous rule until first word is reached
    //make a copy of covered chart span
    DottedRule currentDottedRule = dottedRule;	

    int span = end - startOfFirst + 1;
    //std::cout << "Span is : " << span << std::endl;

    //MSPnew : see if option clause-boundaries is on
        //make instance of StaticData
    const StaticData& staticData = StaticData::Instance();
        //check if option is on
    if (staticData.GetParam("clause-bounds").size() == 1)
    {
        //MSPnew : check if span crosses clause boundary
            //get clause boundaries form InputType src
        bool crosses = 0;

        std::vector<std::vector<int> >::iterator iter_boundaries;

        for(iter_boundaries = m_clauseBounds.begin(); iter_boundaries!=m_clauseBounds.end(); iter_boundaries++)
        {
                std::vector<int> boundaries = *iter_boundaries;
                std::vector<int> :: iterator itr_bounds = boundaries.begin();

                while(itr_bounds!=boundaries.end())
                {
                    int boundary1 = *itr_bounds;
                    //MSPnew
                    //std::cout << "First boundary : " << boundary1 << std::endl;
                    itr_bounds++;
                     //MSPnew : should be no problem to read boundary2 because there are always 2 boundaries in a clause
                    int boundary2 = *itr_bounds;
                    //std::cout << "Second boundary : " << boundary2 << std::endl;
                    itr_bounds++;

                    //MSPnew : terminals in rule can go up to boundary2 + 2
                    bool isEndMatch = MatchesInterval(end, boundary2);

		    bool isInsideStart = IsInside(startOfFirst, boundary1 + 1, boundary2 + 1);
		    bool isInsideEnd = IsInside(end, boundary1 + 1, boundary2 + 1);

		    if( (isInsideStart && !isInsideEnd) || (!isInsideStart && isInsideEnd) )
		    {
			crosses = 1;
		    }	

		    //MSPnew : only end of clause has to match
                    if( isEndMatch == 1 )
                    {
                        if ((targetPhraseCollection != NULL) && (crosses == 0) && (span > minSpan))
                        {
                            	AddCompletedRule(dottedRule, *targetPhraseCollection, range, outColl);
                        }
                    }
		   
        	}
    	}
    }
    else
    {
	//std::cout << "Not in clause boundary mode !" << std::endl;
	
        //MSPnew : do not look for clause boundaries
        if(span > minSpan)
        {
            if (targetPhraseCollection != NULL)
            {
                  AddCompletedRule(dottedRule, *targetPhraseCollection, range, outColl);
            }
            else
            {
                //std::cout << "Target Phrase Collection is null ! " << std::endl;
            }
        }
        else
        {
            //std::cout << "GCR : span : " << span << " too small." << endl;
        }
    }

    //MSPnew : removed here : do not give the complete target rule collection.
    // add the fully expanded rule (with lexical target side)
    //if (targetPhraseCollection != NULL) {
    // outColl.Add(*targetPhraseCollection, dottedRule,
    //              GetCellCollection(), adhereTableLimit, rulesLimit);
   }
    dottedRuleCol.Clear(relEndPos+1);
    outColl.ShrinkToLimit();
}

// Given a partial rule application ending at startPos-1 and given the sets of
// source and target non-terminals covering the span [startPos, endPos],
// determines the full or partial rule applications that can be produced through
// extending the current rule application by a single non-terminal.
void ChartRuleLookupManagerMinSpan::ExtendPartialRuleApplication(
  const DottedRuleInMemory &prevDottedRule,
  size_t startPos,
  size_t endPos,
  size_t stackInd,
  DottedRuleColl & dottedRuleColl)
{
  // source non-terminal labels for the remainder
  const NonTerminalSet &sourceNonTerms =
    GetSentence().GetLabelSet(startPos, endPos);

  // target non-terminal labels for the remainder
  const ChartCellLabelSet &targetNonTerms =
    GetCellCollection().Get(WordsRange(startPos, endPos)).GetTargetLabelSet();

  // note where it was found in the prefix tree of the rule dictionary
  const PhraseDictionaryNodeSCFG &node = prevDottedRule.GetLastNode();

  const PhraseDictionaryNodeSCFG::NonTerminalMap & nonTermMap =
    node.GetNonTerminalMap();

  const size_t numChildren = nonTermMap.size();
  if (numChildren == 0) {
    return;
  }
  const size_t numSourceNonTerms = sourceNonTerms.size();
  const size_t numTargetNonTerms = targetNonTerms.GetSize();
  const size_t numCombinations = numSourceNonTerms * numTargetNonTerms;

  // We can search by either:
  //   1. Enumerating all possible source-target NT pairs that are valid for
  //      the span and then searching for matching children in the node,
  // or
  //   2. Iterating over all the NT children in the node, searching
  //      for each source and target NT in the span's sets.
  // We'll do whichever minimises the number of lookups:
  if (numCombinations <= numChildren*2) {

		// loop over possible source non-terminal labels (as found in input tree)
    NonTerminalSet::const_iterator p = sourceNonTerms.begin();
    NonTerminalSet::const_iterator sEnd = sourceNonTerms.end();
    for (; p != sEnd; ++p) {
      const Word & sourceNonTerm = *p;

      // loop over possible target non-terminal labels (as found in chart)
      ChartCellLabelSet::const_iterator q = targetNonTerms.begin();
      ChartCellLabelSet::const_iterator tEnd = targetNonTerms.end();
      for (; q != tEnd; ++q) {
        const ChartCellLabel &cellLabel = q->second;

        // try to match both source and target non-terminal
        const PhraseDictionaryNodeSCFG * child =
          node.GetChild(sourceNonTerm, cellLabel.GetLabel());

        // nothing found? then we are done
        if (child == NULL) {
          continue;
        }

        // create new rule
#ifdef USE_BOOST_POOL
        DottedRuleInMemory *rule = m_dottedRulePool.malloc();
        new (rule) DottedRuleInMemory(*child, cellLabel, prevDottedRule);
#else
        DottedRuleInMemory *rule = new DottedRuleInMemory(*child, cellLabel,
                                                          prevDottedRule);
#endif
        dottedRuleColl.Add(stackInd, rule);
      }
    }
  }
  else
  {
    // loop over possible expansions of the rule
    PhraseDictionaryNodeSCFG::NonTerminalMap::const_iterator p;
    PhraseDictionaryNodeSCFG::NonTerminalMap::const_iterator end =
      nonTermMap.end();
    for (p = nonTermMap.begin(); p != end; ++p) {
      // does it match possible source and target non-terminals?
      const PhraseDictionaryNodeSCFG::NonTerminalMapKey &key = p->first;
      const Word &sourceNonTerm = key.first;
      if (sourceNonTerms.find(sourceNonTerm) == sourceNonTerms.end()) {
        continue;
      }
      const Word &targetNonTerm = key.second;
      const ChartCellLabel *cellLabel = targetNonTerms.Find(targetNonTerm);
      if (!cellLabel) {
        continue;
      }

      // create new rule
      const PhraseDictionaryNodeSCFG &child = p->second;
#ifdef USE_BOOST_POOL
      DottedRuleInMemory *rule = m_dottedRulePool.malloc();
      new (rule) DottedRuleInMemory(child, *cellLabel, prevDottedRule);
#else
      DottedRuleInMemory *rule = new DottedRuleInMemory(child, *cellLabel,
                                                        prevDottedRule);
#endif
      dottedRuleColl.Add(stackInd, rule);
    }
  }
}

}  // namespace Moses
