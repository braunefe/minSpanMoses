// $Id$
// vim:tabstop=2
/***********************************************************************
 Moses - factored phrase-based language decoder
 Copyright (C) 2010 Hieu Hoang

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

#include <vector>
#include "ChartCell.h"
#include "ChartTranslationOptionCollection.h"
#include "ChartCellCollection.h"
#include "InputType.h"
#include "WordsRange.h"
#include "SentenceStats.h"
#include "TranslationSystem.h"
#include "ChartRuleLookupManager.h"

#include <boost/shared_ptr.hpp>

namespace Moses
{

class ChartHypothesis;
class ChartTrellisDetourQueue;
class ChartTrellisNode;
class ChartTrellisPath;
class ChartTrellisPathList;

class ChartManager
{
private:
  static void CreateDeviantPaths(boost::shared_ptr<const ChartTrellisPath>,
                                 ChartTrellisDetourQueue &);

  static void CreateDeviantPaths(boost::shared_ptr<const ChartTrellisPath>,
                                 const ChartTrellisNode &,
                                 ChartTrellisDetourQueue &);

  InputType const& m_source; /**< source sentence to be translated */
  ChartCellCollection m_hypoStackColl;
  ChartTranslationOptionCollection m_transOptColl; /**< pre-computed list of translation options for the phrases in this sentence */
  std::auto_ptr<SentenceStats> m_sentenceStats;
  const TranslationSystem* m_system;
  clock_t m_start; /**< starting time, used for logging */
  std::vector<ChartRuleLookupManager*> m_ruleLookupManagers;
  unsigned m_hypothesisId; /* For handing out hypothesis ids to ChartHypothesis */

public:
  ChartManager(InputType const& source, const TranslationSystem* system);
  ~ChartManager();
  void ProcessSentence();
  void AddXmlChartOptions();
  const ChartHypothesis *GetBestHypothesis() const;
  void CalcNBest(size_t count, ChartTrellisPathList &ret,bool onlyDistinct=0) const;

  void GetSearchGraph(long translationId, std::ostream &outputSearchGraphStream) const;
	void FindReachableHypotheses( const ChartHypothesis *hypo, std::map<unsigned,bool> &reachable ) const; /* auxilliary function for GetSearchGraph */

  const InputType& GetSource() const {
    return m_source;
  }
  const TranslationSystem* GetTranslationSystem() const {
    return m_system;
  }

  SentenceStats& GetSentenceStats() const {
    return *m_sentenceStats;
  }
  /***
   * to be called after processing a sentence (which may consist of more than just calling ProcessSentence() )
   */
  void CalcDecoderStatistics() const;
  void ResetSentenceStats(const InputType& source) {
    m_sentenceStats = std::auto_ptr<SentenceStats>(new SentenceStats(source));
  }

  unsigned GetNextHypoId() { return m_hypothesisId++; }
};

}

