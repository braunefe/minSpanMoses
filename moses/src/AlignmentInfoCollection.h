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

#include "AlignmentInfo.h"

#include <set>

namespace Moses
{

// Singleton collection of all AlignmentInfo objects.
class AlignmentInfoCollection
{
 public:
  static AlignmentInfoCollection &Instance() { return s_instance; }

  // Returns a pointer to an AlignmentInfo object with the same source-target
  // alignment pairs as given in the argument.  If the collection already
  // contains such an object then returns a pointer to it; otherwise a new
  // one is inserted.
  const AlignmentInfo *Add(const std::set<std::pair<size_t,size_t> > &);

  // Returns a pointer to an empty AlignmentInfo object.
  const AlignmentInfo &GetEmptyAlignmentInfo() const;

 private:
  typedef std::set<AlignmentInfo, AlignmentInfoOrderer> AlignmentInfoSet;

  // Only a single static variable should be created.
  AlignmentInfoCollection();

  static AlignmentInfoCollection s_instance;
  AlignmentInfoSet m_collection;
  const AlignmentInfo *m_emptyAlignmentInfo;
};

}
