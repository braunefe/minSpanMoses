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

#include <fstream>
#include <string>
#include <iterator>
#include <algorithm>
#include "Loader.h"
#include "LoaderFactory.h"
#include "PhraseDictionaryMinSpan.h"
#include "FactorCollection.h"
#include "Word.h"
#include "Util.h"
#include "InputFileStream.h"
#include "StaticData.h"
#include "WordsRange.h"
#include "UserMessage.h"
#include "../CYKPlusParser/ChartRuleLookupManagerMinSpan.h"

using namespace std;

namespace Moses
{

bool PhraseDictionaryMinSpan::Load(const std::vector<FactorType> &input
                                , const std::vector<FactorType> &output
                                , const string &filePath
                                , const vector<float> &weight
                                , size_t tableLimit
                                , const LMList &languageModels
                                , const WordPenaltyProducer* wpProducer)
{
  m_filePath = filePath;
  m_tableLimit = tableLimit;


  // data from file
  InputFileStream inFile(filePath);

  std::auto_ptr<RuleTableLoader> loader =
      RuleTableLoaderFactory::Create(filePath);

	//new : use overloaded function using phrase dictionary min span
  bool ret = loader->Load(input, output, inFile, weight, tableLimit,
                          languageModels, wpProducer, 1, *this);
  return ret;
}

TargetPhraseCollection &PhraseDictionaryMinSpan::GetOrCreateTargetPhraseCollection(const Phrase &source, const TargetPhrase &target)
{
  PhraseDictionaryNodeSCFG &currNode = GetOrCreateNode(source, target);
  return currNode.GetOrCreateTargetPhraseCollection();
}

PhraseDictionaryNodeSCFG &PhraseDictionaryMinSpan::GetOrCreateNode(const Phrase &source, const TargetPhrase &target)
{
  const size_t size = source.GetSize();

  const AlignmentInfo &alignmentInfo = target.GetAlignmentInfo();
  AlignmentInfo::const_iterator iterAlign = alignmentInfo.begin();

  PhraseDictionaryNodeSCFG *currNode = &m_collection;
  for (size_t pos = 0 ; pos < size ; ++pos) {
    const Word& word = source.GetWord(pos);

    if (word.IsNonTerminal()) {
      // indexed by source label 1st
      const Word &sourceNonTerm = word;

      //new : inserted for testing
        if(!(iterAlign != target.GetAlignmentInfo().end()))
        {
            VERBOSE(1,"Bad rule : " << source << " ||| " << target << endl);
            //std::cout << "Bad rule : " << source << " ||| " << target << std::endl;
        }

      assert(iterAlign != target.GetAlignmentInfo().end());
       //new : inserted for testing
        if(!(iterAlign->first == pos))
        {
            VERBOSE(1,"Bad rule : " << source << " ||| " << target << endl);
            //std::cout << "Bad rule : " << source << " ||| " << target << std::endl;
        }
      assert(iterAlign->first == pos);
      size_t targetNonTermInd = iterAlign->second;
      ++iterAlign;
      const Word &targetNonTerm = target.GetWord(targetNonTermInd);

       //new : inserted for testing
        if(!targetNonTerm.IsNonTerminal())
        {
           VERBOSE(1,"Bad rule : " << source << " ||| " << target << endl);
        }


      currNode = currNode->GetOrCreateChild(sourceNonTerm, targetNonTerm);
    } else {
      currNode = currNode->GetOrCreateChild(word);
    }

    assert(currNode != NULL);
  }

  return *currNode;
}

void PhraseDictionaryMinSpan::InitializeForInput(const InputType& /* input */)
{
  // Nothing to do: sentence-specific state is stored in ChartRuleLookupManager
}

PhraseDictionaryMinSpan::~PhraseDictionaryMinSpan()
{
  CleanUp();
}

void PhraseDictionaryMinSpan::CleanUp()
{
  // Nothing to do: sentence-specific state is stored in ChartRuleLookupManager
}

ChartRuleLookupManager *PhraseDictionaryMinSpan::CreateRuleLookupManager(
  const InputType &sentence,
  const ChartCellCollection &cellCollection)
{
  return new ChartRuleLookupManagerMinSpan(sentence, cellCollection, *this);
}

void PhraseDictionaryMinSpan::SortAndPrune()
{
  if (GetTableLimit())
  {
    m_collection.Sort(GetTableLimit());
  }
}

TO_STRING_BODY(PhraseDictionaryMinSpan);

// friend
ostream& operator<<(ostream& out, const PhraseDictionaryMinSpan& phraseDict)
{
  typedef PhraseDictionaryNodeSCFG::TerminalMap TermMap;
  typedef PhraseDictionaryNodeSCFG::NonTerminalMap NonTermMap;

  const PhraseDictionaryNodeSCFG &coll = phraseDict.m_collection;
  for (NonTermMap::const_iterator p = coll.m_nonTermMap.begin(); p != coll.m_nonTermMap.end(); ++p) {
    const Word &sourceNonTerm = p->first.first;
    out << sourceNonTerm;
  }
  for (TermMap::const_iterator p = coll.m_sourceTermMap.begin(); p != coll.m_sourceTermMap.end(); ++p) {
    const Word &sourceTerm = p->first;
    out << sourceTerm;
  }
  return out;
}

}
