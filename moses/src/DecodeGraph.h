// $Id$
// vim:tabstop=2

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

#ifndef moses_DecodeGraph_h
#define moses_DecodeGraph_h

#include "util/check.hh"
#include <list>
#include <iterator>
#include "TypeDef.h"

namespace Moses
{

class DecodeStep;

//! list of DecodeStep s which factorizes the translation
class DecodeGraph
{
protected:
  std::list<const DecodeStep*> m_steps;
  size_t m_position;
  size_t m_maxChartSpan;

  //MSPnew : field for min span
  size_t m_minChartSpan;

public:
  /**
    * position: The position of this graph within the decode sequence.
    **/
  DecodeGraph(size_t position)
    : m_position(position)
    , m_maxChartSpan(NOT_FOUND)
  {}

  // for chart decoding
  DecodeGraph(size_t position, size_t maxChartSpan)
    : m_position(position)
    , m_maxChartSpan(maxChartSpan)
  {}

    //MSPnew : for chart decoding with min span
	DecodeGraph(size_t position, size_t maxChartSpan, size_t minChartSpan)
	: m_position(position)
	, m_maxChartSpan(maxChartSpan)
	, m_minChartSpan(minChartSpan) //min span field
	{}

  //! iterators
  typedef std::list<const DecodeStep*>::iterator iterator;
  typedef std::list<const DecodeStep*>::const_iterator const_iterator;
  const_iterator begin() const {
    return m_steps.begin();
  }
  const_iterator end() const {
    return m_steps.end();
  }

  virtual ~DecodeGraph();

  //! Add another decode step to the graph
  void Add(const DecodeStep *decodeStep) {
    m_steps.push_back(decodeStep);
  }

  size_t GetSize() const {
    return m_steps.size();
  }

  size_t GetMaxChartSpan() const {
    CHECK(m_maxChartSpan != NOT_FOUND);
    return m_maxChartSpan;
  }

  //MSPnew : min span accessors
  void SetMinChartSpan(size_t minSpan){
    m_minChartSpan = minSpan;
  }

  size_t GetMinChartSpan() const {
    return m_minChartSpan;
  }
  //end of new

  size_t GetPosition() const {
    return m_position;
  }

};


}
#endif
