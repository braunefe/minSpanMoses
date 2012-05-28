/***********************************************************************
  Moses - factored phrase-based language decoder
  Copyright (C) 2009 University of Edinburgh

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

#include <sstream>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <vector>
#include <stdlib.h>
#include <assert.h>
#include <cstring>
#include <set>

#include "SafeGetline.h"
#include "tables-core.h"
#include "PhraseAlignment.h"
#include "score.h"
#include "InputFileStream.h"

using namespace std;

#define LINE_MAX_LENGTH 100000

Vocabulary vcbT;
Vocabulary vcbS;

class LexicalTable
{
public:
  map< WORD_ID, map< WORD_ID, double > > ltable;
  void load( char[] );
  double permissiveLookup( WORD_ID wordS, WORD_ID wordT ) {
    // cout << endl << vcbS.getWord( wordS ) << "-" << vcbT.getWord( wordT ) << ":";
    if (ltable.find( wordS ) == ltable.end()) return 1.0;
    if (ltable[ wordS ].find( wordT ) == ltable[ wordS ].end()) return 1.0;
    // cout << ltable[ wordS ][ wordT ];
    return ltable[ wordS ][ wordT ];
  }
};

vector<string> tokenize( const char [] );

void writeCountOfCounts( const char* fileNameCountOfCounts );
void processPhrasePairs( vector< PhraseAlignment > & , ostream &phraseTableFile);
PhraseAlignment* findBestAlignment(const PhraseAlignmentCollection &phrasePair );
void outputPhrasePair(const PhraseAlignmentCollection &phrasePair, float, int, ostream &phraseTableFile );
double computeLexicalTranslation( const PHRASE &, const PHRASE &, PhraseAlignment * );
double computeUnalignedPenalty( const PHRASE &, const PHRASE &, PhraseAlignment * );
set<string> functionWordList;
void loadFunctionWords( const char* fileNameFunctionWords );
double computeUnalignedFWPenalty( const PHRASE &, const PHRASE &, PhraseAlignment * );
void calcNTLengthProb(const vector< PhraseAlignment* > &phrasePairs
                      , map<size_t, map<size_t, float> > &sourceProb
                      , map<size_t, map<size_t, float> > &targetProb);
LexicalTable lexTable;
bool inverseFlag = false;
bool hierarchicalFlag = false;
bool wordAlignmentFlag = false;
bool goodTuringFlag = false;
bool kneserNeyFlag = false;
#define COC_MAX 10
bool logProbFlag = false;
int negLogProb = 1;
bool lexFlag = true;
bool unalignedFlag = false;
bool unalignedFWFlag = false;
bool outputNTLengths = false;
int countOfCounts[COC_MAX+1];
int totalDistinct = 0;
float minCountHierarchical = 0;

int main(int argc, char* argv[])
{
  cerr << "Score v2.0 written by Philipp Koehn\n"
       << "scoring methods for extracted rules\n";

  if (argc < 4) {
    cerr << "syntax: score extract lex phrase-table [--Inverse] [--Hierarchical] [--LogProb] [--NegLogProb] [--NoLex] [--GoodTuring coc-file] [--KneserNey coc-file] [--WordAlignment] [--UnalignedPenalty] [--UnalignedFunctionWordPenalty function-word-file] [--MinCountHierarchical count] [--OutputNTLengths] \n";
    exit(1);
  }
  char* fileNameExtract = argv[1];
  char* fileNameLex = argv[2];
  char* fileNamePhraseTable = argv[3];
  char* fileNameCountOfCounts;
  char* fileNameFunctionWords;

  for(int i=4; i<argc; i++) {
    if (strcmp(argv[i],"inverse") == 0 || strcmp(argv[i],"--Inverse") == 0) {
      inverseFlag = true;
      cerr << "using inverse mode\n";
    } else if (strcmp(argv[i],"--Hierarchical") == 0) {
      hierarchicalFlag = true;
      cerr << "processing hierarchical rules\n";
    } else if (strcmp(argv[i],"--WordAlignment") == 0) {
      wordAlignmentFlag = true;
      cerr << "outputing word alignment" << endl;
    } else if (strcmp(argv[i],"--NoLex") == 0) {
      lexFlag = false;
      cerr << "not computing lexical translation score\n";
    } else if (strcmp(argv[i],"--GoodTuring") == 0) {
      goodTuringFlag = true;
      if (i+1==argc) { 
        cerr << "ERROR: specify count of count files for Good Turing discounting!\n";
        exit(1);
      }
      fileNameCountOfCounts = argv[++i];
      cerr << "adjusting phrase translation probabilities with Good Turing discounting\n";
    } else if (strcmp(argv[i],"--KneserNey") == 0) {
      kneserNeyFlag = true;
      if (i+1==argc) { 
        cerr << "ERROR: specify count of count files for Kneser Ney discounting!\n";
        exit(1);
      }
      fileNameCountOfCounts = argv[++i];
      cerr << "adjusting phrase translation probabilities with Kneser Ney discounting\n";
    } else if (strcmp(argv[i],"--UnalignedPenalty") == 0) {
      unalignedFlag = true;
      cerr << "using unaligned word penalty\n";
    } else if (strcmp(argv[i],"--UnalignedFunctionWordPenalty") == 0) {
      unalignedFWFlag = true;
      if (i+1==argc) { 
        cerr << "ERROR: specify count of count files for Kneser Ney discounting!\n";
        exit(1);
      }
      fileNameFunctionWords = argv[++i];
      cerr << "using unaligned function word penalty with function words from " << fileNameFunctionWords << endl;
    } else if (strcmp(argv[i],"--LogProb") == 0) {
      logProbFlag = true;
      cerr << "using log-probabilities\n";
    } else if (strcmp(argv[i],"--NegLogProb") == 0) {
      logProbFlag = true;
      negLogProb = -1;
      cerr << "using negative log-probabilities\n";
    } else if (strcmp(argv[i],"--MinCountHierarchical") == 0) {
      minCountHierarchical = atof(argv[++i]);
      cerr << "dropping all phrase pairs occurring less than " << minCountHierarchical << " times\n";
      minCountHierarchical -= 0.00001; // account for rounding
    } else if (strcmp(argv[i],"--OutputNTLengths") == 0) {
      outputNTLengths = true;
    } else {
      cerr << "ERROR: unknown option " << argv[i] << endl;
      exit(1);
    }
  }

  // lexical translation table
  if (lexFlag)
    lexTable.load( fileNameLex );

  // function word list
  if (unalignedFWFlag)
    loadFunctionWords( fileNameFunctionWords );

  // compute count of counts for Good Turing discounting
  if (goodTuringFlag || kneserNeyFlag) {
    for(int i=1; i<=COC_MAX; i++) countOfCounts[i] = 0;
  }

  // sorted phrase extraction file
  Moses::InputFileStream extractFile(fileNameExtract);

  if (extractFile.fail()) {
    cerr << "ERROR: could not open extract file " << fileNameExtract << endl;
    exit(1);
  }
  istream &extractFileP = extractFile;

  // output file: phrase translation table
	ostream *phraseTableFile;

	if (strcmp(fileNamePhraseTable, "-") == 0) {
		phraseTableFile = &cout;
	}
	else {
		ofstream *outputFile = new ofstream();
		outputFile->open(fileNamePhraseTable);
		if (outputFile->fail()) {
			cerr << "ERROR: could not open file phrase table file "
					 << fileNamePhraseTable << endl;
			exit(1);
		}
		phraseTableFile = outputFile;
	}
	
  // loop through all extracted phrase translations
  float lastCount = 0.0f;
  vector< PhraseAlignment > phrasePairsWithSameF;
  int i=0;
  char line[LINE_MAX_LENGTH],lastLine[LINE_MAX_LENGTH];
  lastLine[0] = '\0';
  PhraseAlignment *lastPhrasePair = NULL;
  while(true) {
    if (extractFileP.eof()) break;
    if (++i % 100000 == 0) cerr << "." << flush;
    SAFE_GETLINE((extractFileP), line, LINE_MAX_LENGTH, '\n', __FILE__);
    if (extractFileP.eof())	break;

    // identical to last line? just add count
    if (strcmp(line,lastLine) == 0) {
      lastPhrasePair->count += lastCount;
      continue;
    }
    strcpy( lastLine, line );

    // create new phrase pair
    PhraseAlignment phrasePair;
    phrasePair.create( line, i );
    lastCount = phrasePair.count;

    // only differs in count? just add count
    if (lastPhrasePair != NULL && lastPhrasePair->equals( phrasePair )) {
      lastPhrasePair->count += phrasePair.count;
      continue;
    }

    // if new source phrase, process last batch
    if (lastPhrasePair != NULL &&
        lastPhrasePair->GetSource() != phrasePair.GetSource()) {
      processPhrasePairs( phrasePairsWithSameF, *phraseTableFile );
      phrasePairsWithSameF.clear();
      lastPhrasePair = NULL;
    }

    // add phrase pairs to list, it's now the last one
    phrasePairsWithSameF.push_back( phrasePair );
    lastPhrasePair = &phrasePairsWithSameF.back();
  }
  processPhrasePairs( phrasePairsWithSameF, *phraseTableFile );
	
	phraseTableFile->flush();
	if (phraseTableFile != &cout) {
		(dynamic_cast<ofstream*>(phraseTableFile))->close();
		delete phraseTableFile;
	}

  // output count of count statistics
  if (goodTuringFlag || kneserNeyFlag) {
    writeCountOfCounts( fileNameCountOfCounts );
  }
}

void writeCountOfCounts( const char* fileNameCountOfCounts )
{
  // open file
	ofstream countOfCountsFile;
	countOfCountsFile.open(fileNameCountOfCounts);
	if (countOfCountsFile.fail()) {
		cerr << "ERROR: could not open count-of-counts file "
				 << fileNameCountOfCounts << endl;
    return;
	}

  // Kneser-Ney needs the total number of phrase pairs
  countOfCountsFile << totalDistinct << endl;

  // write out counts
  for(int i=1; i<=COC_MAX; i++) {
    countOfCountsFile << countOfCounts[ i ] << endl;
  }
	countOfCountsFile.close();
}

void processPhrasePairs( vector< PhraseAlignment > &phrasePair, ostream &phraseTableFile )
{
  if (phrasePair.size() == 0) return;

  // group phrase pairs based on alignments that matter
  // (i.e. that re-arrange non-terminals)
  PhrasePairGroup phrasePairGroup;
  
  float totalSource = 0;

  //cerr << "phrasePair.size() = " << phrasePair.size() << endl;
  
  // loop through phrase pairs
  for(size_t i=0; i<phrasePair.size(); i++) {
    // add to total count
    PhraseAlignment &currPhrasePair = phrasePair[i];
    
    totalSource += phrasePair[i].count;
    
    // check for matches
    //cerr << "phrasePairGroup.size() = " << phrasePairGroup.size() << endl;
    
    PhraseAlignmentCollection phraseAlignColl;
    phraseAlignColl.push_back(&currPhrasePair);
    pair<PhrasePairGroup::iterator, bool> retInsert;
    retInsert = phrasePairGroup.insert(phraseAlignColl);
    if (!retInsert.second)
    { // already exist. Add to that collection instead
      PhraseAlignmentCollection &existingColl = const_cast<PhraseAlignmentCollection&>(*retInsert.first);
      existingColl.push_back(&currPhrasePair);
    }
    
  }

  // output the distinct phrase pairs, one at a time
  const PhrasePairGroup::SortedColl &sortedColl = phrasePairGroup.GetSortedColl();
  PhrasePairGroup::SortedColl::const_iterator iter;

  for(iter = sortedColl.begin(); iter != sortedColl.end(); ++iter) 
  {
    const PhraseAlignmentCollection &group = **iter;
    outputPhrasePair( group, totalSource, phrasePairGroup.GetSize(), phraseTableFile );

  }
  
}

PhraseAlignment* findBestAlignment(const PhraseAlignmentCollection &phrasePair )
{
  float bestAlignmentCount = -1;
  PhraseAlignment* bestAlignment;
  
  for(int i=0; i<phrasePair.size(); i++) {
    if (phrasePair[i]->count > bestAlignmentCount) {
      bestAlignmentCount = phrasePair[i]->count;
      bestAlignment = phrasePair[i];
    }
  }
  
  return bestAlignment;
}


void calcNTLengthProb(const map<size_t, map<size_t, size_t> > &lengths
                      , size_t total
                      , map<size_t, map<size_t, float> > &probs)
{
  map<size_t, map<size_t, size_t> >::const_iterator iterOuter;
  for (iterOuter = lengths.begin(); iterOuter != lengths.end(); ++iterOuter)
  {
    size_t sourcePos = iterOuter->first;
    const map<size_t, size_t> &inner = iterOuter->second;
    
    map<size_t, size_t>::const_iterator iterInner;
    for (iterInner = inner.begin(); iterInner != inner.end(); ++iterInner)
    {
      size_t length = iterInner->first;
      size_t count = iterInner->second;
      float prob = (float) count / (float) total;
      probs[sourcePos][length] = prob;
    }
  }
}

void calcNTLengthProb(const vector< PhraseAlignment* > &phrasePairs
                      , map<size_t, map<size_t, float> > &sourceProb
                      , map<size_t, map<size_t, float> > &targetProb)
{
  map<size_t, map<size_t, size_t> > sourceLengths, targetLengths;
  // 1st = position in source phrase, 2nd = length, 3rd = count
  map<size_t, size_t> totals;
  // 1st = position in source phrase, 2nd = total counts
  // each source pos should have same count?
  
  vector< PhraseAlignment* >::const_iterator iterOuter;
  for (iterOuter = phrasePairs.begin(); iterOuter != phrasePairs.end(); ++iterOuter)
  {
    const PhraseAlignment &phrasePair = **iterOuter;
    const std::map<size_t, std::pair<size_t, size_t> > &ntLengths = phrasePair.GetNTLengths();
    
    std::map<size_t, std::pair<size_t, size_t> >::const_iterator iterInner;
    for (iterInner = ntLengths.begin(); iterInner != ntLengths.end(); ++iterInner)
    {
      size_t sourcePos = iterInner->first;
      size_t sourceLength = iterInner->second.first;
      size_t targetLength = iterInner->second.second;
      
      sourceLengths[sourcePos][sourceLength]++;
      targetLengths[sourcePos][targetLength]++;

      totals[sourcePos]++;
    }
  }
    
  if (totals.size() == 0)
  { // no non-term. Don't bother
    return;
  }

  size_t total = totals.begin()->second;
  if (totals.size() > 1)
  {
    assert(total == (++totals.begin())->second );
  }
  
  calcNTLengthProb(sourceLengths, total, sourceProb);
  calcNTLengthProb(targetLengths, total, targetProb);
  
}

void outputNTLengthProbs(ostream &phraseTableFile, const map<size_t, map<size_t, float> > &probs, const string &prefix)
{
  map<size_t, map<size_t, float> >::const_iterator iterOuter;
  for (iterOuter = probs.begin(); iterOuter != probs.end(); ++iterOuter)
  {
    size_t sourcePos = iterOuter->first;
    const map<size_t, float> &inner = iterOuter->second;
    
    map<size_t, float>::const_iterator iterInner;
    for (iterInner = inner.begin(); iterInner != inner.end(); ++iterInner)
    {
      size_t length = iterInner->first;
      float prob = iterInner->second;

      phraseTableFile << sourcePos << "|" << prefix << "|" << length << "=" << prob << " ";
    }
  }

}

void outputPhrasePair(const PhraseAlignmentCollection &phrasePair, float totalCount, int distinctCount, ostream &phraseTableFile )
{
  if (phrasePair.size() == 0) return;

  PhraseAlignment *bestAlignment = findBestAlignment( phrasePair );
    
  // compute count
  float count = 0;
  for(size_t i=0; i<phrasePair.size(); i++) {
    count += phrasePair[i]->count;
  }

  // collect count of count statistics
  if (goodTuringFlag || kneserNeyFlag) {
    totalDistinct++;
    int countInt = count + 0.99999;
    if(countInt <= COC_MAX)
      countOfCounts[ countInt ]++;
  }

  // output phrases
  const PHRASE &phraseS = phrasePair[0]->GetSource();
  const PHRASE &phraseT = phrasePair[0]->GetTarget();

  // do not output if hierarchical and count below threshold
  if (hierarchicalFlag && count < minCountHierarchical) {
    for(int j=0; j<phraseS.size()-1; j++) {
      if (isNonTerminal(vcbS.getWord( phraseS[j] )))
        return;
    }
  }

  // source phrase (unless inverse)
  if (! inverseFlag) {
    for(int j=0; j<phraseS.size(); j++) {
      phraseTableFile << vcbS.getWord( phraseS[j] );
      phraseTableFile << " ";
    }
    phraseTableFile << "||| ";
  }

  // target phrase
  for(int j=0; j<phraseT.size(); j++) {
    phraseTableFile << vcbT.getWord( phraseT[j] );
    phraseTableFile << " ";
  }
  phraseTableFile << "||| ";

  // source phrase (if inverse)
  if (inverseFlag) {
    for(int j=0; j<phraseS.size(); j++) {
      phraseTableFile << vcbS.getWord( phraseS[j] );
      phraseTableFile << " ";
    }
    phraseTableFile << "||| ";
  }

  // lexical translation probability
  if (lexFlag) {
    double lexScore = computeLexicalTranslation( phraseS, phraseT, bestAlignment);
    phraseTableFile << ( logProbFlag ? negLogProb*log(lexScore) : lexScore );
  }

  // unaligned word penalty
  if (unalignedFlag) {
    double penalty = computeUnalignedPenalty( phraseS, phraseT, bestAlignment);
    phraseTableFile << " " << ( logProbFlag ? negLogProb*log(penalty) : penalty );
  }

  // unaligned function word penalty
  if (unalignedFWFlag) {
    double penalty = computeUnalignedFWPenalty( phraseS, phraseT, bestAlignment);
    phraseTableFile << " " << ( logProbFlag ? negLogProb*log(penalty) : penalty );
  }

  phraseTableFile << " ||| ";

  // alignment info for non-terminals
  if (! inverseFlag) {
    if (hierarchicalFlag) {
      // always output alignment if hiero style, but only for non-terms
      assert(phraseT.size() == bestAlignment->alignedToT.size() + 1);
      for(int j = 0; j < phraseT.size() - 1; j++) {
        if (isNonTerminal(vcbT.getWord( phraseT[j] ))) {
          if (bestAlignment->alignedToT[ j ].size() != 1) {
            cerr << "Error: unequal numbers of non-terminals. Make sure the text does not contain words in square brackets (like [xxx])." << endl;
            phraseTableFile.flush();
            assert(bestAlignment->alignedToT[ j ].size() == 1);
          }
          int sourcePos = *(bestAlignment->alignedToT[ j ].begin());
          phraseTableFile << sourcePos << "-" << j << " ";
        }
      }
    } else if (wordAlignmentFlag) {
      // alignment info in pb model
      for(int j=0; j<bestAlignment->alignedToT.size(); j++) {
        const set< size_t > &aligned = bestAlignment->alignedToT[j];
        for (set< size_t >::const_iterator p(aligned.begin()); p != aligned.end(); ++p) {
          phraseTableFile << *p << "-" << j << " ";
        }
      }
    }
  }

  // counts
  
  phraseTableFile << " ||| " << totalCount << " " << count;
  if (kneserNeyFlag) 
    phraseTableFile << " " << distinctCount;
  
  // nt lengths  
  if (outputNTLengths)
  {
    phraseTableFile << " ||| ";

    if (!inverseFlag)
    {
      map<size_t, map<size_t, float> > sourceProb, targetProb;
      // 1st sourcePos, 2nd = length, 3rd = prob

      calcNTLengthProb(phrasePair, sourceProb, targetProb);
      
      outputNTLengthProbs(phraseTableFile, sourceProb, "S");
      outputNTLengthProbs(phraseTableFile, targetProb, "T");
    }    
  }
  
  phraseTableFile << endl;
}

double computeUnalignedPenalty( const PHRASE &phraseS, const PHRASE &phraseT, PhraseAlignment *alignment )
{
  // unaligned word counter
  double unaligned = 1.0;
  // only checking target words - source words are caught when computing inverse
  for(int ti=0; ti<alignment->alignedToT.size(); ti++) {
    const set< size_t > & srcIndices = alignment->alignedToT[ ti ];
    if (srcIndices.empty()) {
      unaligned *= 2.718;
    }
  }
  return unaligned;
}

double computeUnalignedFWPenalty( const PHRASE &phraseS, const PHRASE &phraseT, PhraseAlignment *alignment )
{
  // unaligned word counter
  double unaligned = 1.0;
  // only checking target words - source words are caught when computing inverse
  for(int ti=0; ti<alignment->alignedToT.size(); ti++) {
    const set< size_t > & srcIndices = alignment->alignedToT[ ti ];
    if (srcIndices.empty() && functionWordList.find( vcbT.getWord( phraseT[ ti ] ) ) != functionWordList.end()) {
      unaligned *= 2.718;
    }
  }
  return unaligned;
}

void loadFunctionWords( const char *fileName )
{
  cerr << "Loading function word list from " << fileName;
  ifstream inFile;
  inFile.open(fileName);
  if (inFile.fail()) {
    cerr << " - ERROR: could not open file\n";
    exit(1);
  }
  istream *inFileP = &inFile;

  char line[LINE_MAX_LENGTH];
  while(true) {
    SAFE_GETLINE((*inFileP), line, LINE_MAX_LENGTH, '\n', __FILE__);
    if (inFileP->eof()) break;
    vector<string> token = tokenize( line );
    if (token.size() > 0)
      functionWordList.insert( token[0] );
  }
  inFile.close();

  cerr << " - read " << functionWordList.size() << " function words\n";
  inFile.close();
}

double computeLexicalTranslation( const PHRASE &phraseS, const PHRASE &phraseT, PhraseAlignment *alignment )
{
  // lexical translation probability
  double lexScore = 1.0;
  int null = vcbS.getWordID("NULL");
  // all target words have to be explained
  for(int ti=0; ti<alignment->alignedToT.size(); ti++) {
    const set< size_t > & srcIndices = alignment->alignedToT[ ti ];
    if (srcIndices.empty()) {
      // explain unaligned word by NULL
      lexScore *= lexTable.permissiveLookup( null, phraseT[ ti ] );
    } else {
      // go through all the aligned words to compute average
      double thisWordScore = 0;
      for (set< size_t >::const_iterator p(srcIndices.begin()); p != srcIndices.end(); ++p) {
        thisWordScore += lexTable.permissiveLookup( phraseS[ *p ], phraseT[ ti ] );
      }
      lexScore *= thisWordScore / (double)srcIndices.size();
    }
  }
  return lexScore;
}

void LexicalTable::load( char *fileName )
{
  cerr << "Loading lexical translation table from " << fileName;
  ifstream inFile;
  inFile.open(fileName);
  if (inFile.fail()) {
    cerr << " - ERROR: could not open file\n";
    exit(1);
  }
  istream *inFileP = &inFile;

  char line[LINE_MAX_LENGTH];

  int i=0;
  while(true) {
    i++;
    if (i%100000 == 0) cerr << "." << flush;
    SAFE_GETLINE((*inFileP), line, LINE_MAX_LENGTH, '\n', __FILE__);
    if (inFileP->eof()) break;

    vector<string> token = tokenize( line );
    if (token.size() != 3) {
      cerr << "line " << i << " in " << fileName
           << " has wrong number of tokens, skipping:\n"
           << token.size() << " " << token[0] << " " << line << endl;
      continue;
    }

    double prob = atof( token[2].c_str() );
    WORD_ID wordT = vcbT.storeIfNew( token[0] );
    WORD_ID wordS = vcbS.storeIfNew( token[1] );
    ltable[ wordS ][ wordT ] = prob;
  }
  cerr << endl;
}

std::pair<PhrasePairGroup::Coll::iterator,bool> PhrasePairGroup::insert ( const PhraseAlignmentCollection& obj )
{
  std::pair<iterator,bool> ret = m_coll.insert(obj);
  
  if (ret.second)
  { // obj inserted. Also add to sorted vector
    const PhraseAlignmentCollection &insertedObj = *ret.first;
    m_sortedColl.push_back(&insertedObj);
  }
  
  return ret;
}


