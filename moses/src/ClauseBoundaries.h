#ifndef moses_ClauseBoundaries_h
#define moses_ClauseBoundaries_h

#include <list>
#include <ostream>
#include <fstream>
#include <string>
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include "InputFileStream.h"

namespace Moses
{
    // Collection of alignment pairs, ordered by source index
    class ClauseBoundaries
    {
        public:
        std::vector< std::vector<int> > m_clauseBoundaries;

        ClauseBoundaries();
        std::vector< std::vector<int> > GetClauseBoundaries() const;
        void SetClauseBoundaries(std::vector< std::vector<int> >);
        int ReadClauseBoundaries(std::istream& in);
    };

}
#endif


