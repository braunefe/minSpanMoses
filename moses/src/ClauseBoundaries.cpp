#include <list>
#include <iostream>
#include <ostream>
#include <fstream>
#include <string>
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include "ClauseBoundaries.h"

using namespace std;

namespace Moses
{

    //my string tokenizer
void TokenizeMyString(const string& str,
                      vector< vector<string> > & intervals,
                      const string& delimiters)
  {

     string clause_delimiter = "S";
     string interval_delimiter = ",";

    //vector containing clauses with begin label and comas
    vector<string> clauses;
    vector<string> :: iterator itr_clauses;

    //limits of intervals where clause boundaries are in
    vector<string> limits;

    // Skip delimiters at beginning.
    string::size_type lastPos = str.find_first_not_of(delimiters, 0);

    // Find first "non-delimiter".
    string::size_type pos = str.find_first_of(delimiters, lastPos);

    while (string::npos != pos || string::npos != lastPos)
    {
        // Found a token, add it to the vector.

        //new : inserted for testing
        //cout << "found clause" << str.substr(lastPos, pos - lastPos) << endl;
        clauses.push_back(str.substr(lastPos, pos - lastPos));
        // Skip delimiters.  Note the "not_of"
        lastPos = str.find_first_not_of(delimiters, pos);
        // Find next "non-delimiter"
        pos = str.find_first_of(delimiters, lastPos);
    }

    for(itr_clauses = clauses.begin();itr_clauses != clauses.end();itr_clauses++)
    {
        string currentClause = *itr_clauses;

        //new : inserted for testing
        //cout << "current clause" << currentClause << endl;
        lastPos = currentClause.find_first_not_of(interval_delimiter, 0);
        pos = currentClause.find_first_of(interval_delimiter, lastPos);

        while (string::npos != pos || string::npos != lastPos)
        {
            //new : inserted for testing
            //cout << "current position " << currentClause.substr(lastPos, pos - lastPos) << endl;
            // Found a token, add it to the vector if it is not the clause_delimiter
            if(currentClause.substr(lastPos, pos - lastPos) != clause_delimiter)
            {
                //new : inserted for testing
                //cout << "pushed limit" << currentClause.substr(lastPos, pos - lastPos) << endl;
                limits.push_back(currentClause.substr(lastPos, pos - lastPos));}
                // Skip delimiters.  Note the "not_of"
                lastPos = currentClause.find_first_not_of(interval_delimiter, pos);
                // Find next "non-delimiter"
                pos = currentClause.find_first_of(interval_delimiter, lastPos);
        }
        intervals.push_back(limits);
        limits.clear();
        //new : inserted for testing
        vector<vector<string> > :: iterator itr_intervals;
        for(itr_intervals = intervals.begin(); itr_intervals != intervals.end(); itr_intervals++)
        {
            vector<string> current_limit = *itr_intervals;
            vector<string> :: iterator itr_current_limit;
            for(itr_current_limit = current_limit.begin(); itr_current_limit != current_limit.end(); itr_current_limit++)
            {
                string current_string = *itr_current_limit;
                //cout << "Current String" << current_string << endl;
            }

        }
    }
  }

ClauseBoundaries::ClauseBoundaries(){}

void ClauseBoundaries::SetClauseBoundaries(vector< vector<int> > cb)
{
    m_clauseBoundaries = cb;
}

std::vector< std::vector<int> > ClauseBoundaries::GetClauseBoundaries() const
  {
	//new : inserted for testing
	//std::cout << "return GetClauseBoundaries "  << std::endl;
	//std::cout << "value &m_clauseBoundaries " << m_clauseBoundaries  << std::endl;
	return m_clauseBoundaries;
  }

int ClauseBoundaries::ReadClauseBoundaries(std::istream& in)
{
	std::string line;
	//new : inserted for testing
	//std::cout << "ReadClauseBoundaries" << std::endl;
	//read one line : if eof return 0
	if (getline(in, line, '\n').eof())
	return 0;
	//std::cout << "no eof" << std::endl;
	//tokenize line
	vector<vector<string> > clauseMarkersStrings;
	TokenizeMyString(line, clauseMarkersStrings, " ");

	//convert into ints and put into vector
	vector<vector<int> > myMarkersInt;
	vector<vector<string> > :: iterator overIntervals;
	for(overIntervals=clauseMarkersStrings.begin(); overIntervals!=clauseMarkersStrings.end(); overIntervals++)
	{
		vector<string> intervalMarkers = *overIntervals;
		vector<string> :: iterator itr_intervals;
		vector<int> intMarkers;
		//new : inserted for testing
		for(itr_intervals=intervalMarkers.begin(); itr_intervals!=intervalMarkers.end(); itr_intervals++)
		{
		    string marker = *itr_intervals;
		    int intMarker = atoi(marker.c_str());
            //std::cout << "marker: " << intMarker << std::endl;
		    intMarkers.push_back(intMarker);
		}
		myMarkersInt.push_back(intMarkers);
	}
	//set m_clauseBoundaries (field of ClauseBoundaries) to int vector
	SetClauseBoundaries(myMarkersInt);
	return 1;
}


}
