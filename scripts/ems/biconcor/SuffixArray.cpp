#include "SuffixArray.h"

#include <fstream>
#include <string>
#include <stdlib.h>
#include <cstring>

namespace {

const int LINE_MAX_LENGTH = 10000;

} // namespace

using namespace std;

SuffixArray::SuffixArray()
    : m_array(NULL),
      m_index(NULL),
      m_buffer(NULL),
      m_wordInSentence(NULL),
      m_sentence(NULL),
      m_sentenceLength(NULL),
      m_vcb(),
      m_size(0),
      m_sentenceCount(0) { }

SuffixArray::~SuffixArray()
{
  free(m_array);
  free(m_index);
  free(m_wordInSentence);
  free(m_sentence);
  free(m_sentenceLength);
}

void SuffixArray::Create(const string& fileName )
{
  m_vcb.StoreIfNew( "<uNk>" );
  m_endOfSentence = m_vcb.StoreIfNew( "<s>" );

  ifstream textFile;
  char line[LINE_MAX_LENGTH];

  // count the number of words first;
  textFile.open(fileName.c_str());

  if (!textFile) {
    cerr << "no such file or directory " << fileName << endl;
    exit(1);
  }

  istream *fileP = &textFile;
  m_size = 0;
  m_sentenceCount = 0;
  while(!fileP->eof()) {
    SAFE_GETLINE((*fileP), line, LINE_MAX_LENGTH, '\n');
    if (fileP->eof()) break;
    vector< WORD_ID > words = m_vcb.Tokenize( line );
    m_size += words.size() + 1;
    m_sentenceCount++;
  }
  textFile.close();
  cerr << m_size << " words (incl. sentence boundaries)" << endl;

  // allocate memory
  m_array = (WORD_ID*) calloc( sizeof( WORD_ID ), m_size );
  m_index = (INDEX*) calloc( sizeof( INDEX ), m_size );
  m_wordInSentence = (char*) calloc( sizeof( char ), m_size );
  m_sentence = (INDEX*) calloc( sizeof( INDEX ), m_size );
  m_sentenceLength = (char*) calloc( sizeof( char ), m_sentenceCount );

  // fill the array
  int wordIndex = 0;
  int sentenceId = 0;
  textFile.open(fileName.c_str());

  if (!textFile) {
    cerr << "no such file or directory " << fileName << endl;
    exit(1);
  }

  fileP = &textFile;
  while(!fileP->eof()) {
    SAFE_GETLINE((*fileP), line, LINE_MAX_LENGTH, '\n');
    if (fileP->eof()) break;
    vector< WORD_ID > words = m_vcb.Tokenize( line );
    vector< WORD_ID >::const_iterator i;

    for( i=words.begin(); i!=words.end(); i++) {
      m_index[ wordIndex ] = wordIndex;
      m_sentence[ wordIndex ] = sentenceId;
      m_wordInSentence[ wordIndex ] = i-words.begin();
      m_array[ wordIndex++ ] = *i;
    }
    m_index[ wordIndex ] = wordIndex;
    m_array[ wordIndex++ ] = m_endOfSentence;
    m_sentenceLength[ sentenceId++ ] = words.size();
  }
  textFile.close();
  cerr << "done reading " << wordIndex << " words, " << sentenceId << " sentences." << endl;
  // List(0,9);

  // sort
  m_buffer = (INDEX*) calloc( sizeof( INDEX ), m_size );

  if (m_buffer == NULL) {
    cerr << "cannot allocate memory to m_buffer" << endl;
    exit(1);
  }

  Sort( 0, m_size-1 );
  free( m_buffer );
  cerr << "done sorting" << endl;
}

// good ol' quick sort
void SuffixArray::Sort(INDEX start, INDEX end)
{
  if (start == end) return;
  INDEX mid = (start+end+1)/2;
  Sort( start, mid-1 );
  Sort( mid, end );

  // merge
  int i = start;
  int j = mid;
  int k = 0;
  int length = end-start+1;
  while( k<length ) {
    if (i == mid ) {
      m_buffer[ k++ ] = m_index[ j++ ];
    } else if (j > end ) {
      m_buffer[ k++ ] = m_index[ i++ ];
    } else {
      if (CompareIndex( m_index[i], m_index[j] ) < 0) {
        m_buffer[ k++ ] = m_index[ i++ ];
      } else {
        m_buffer[ k++ ] = m_index[ j++ ];
      }
    }
  }

  memcpy( ((char*)m_index) + sizeof( INDEX ) * start,
          ((char*)m_buffer), sizeof( INDEX ) * (end-start+1) );
}

int SuffixArray::CompareIndex( INDEX a, INDEX b ) const
{
  // skip over identical words
  INDEX offset = 0;
  while( a+offset < m_size &&
         b+offset < m_size &&
         m_array[ a+offset ] == m_array[ b+offset ] ) {
    offset++;
  }

  if( a+offset == m_size ) return -1;
  if( b+offset == m_size ) return 1;
  return CompareWord( m_array[ a+offset ], m_array[ b+offset ] );
}

inline int SuffixArray::CompareWord( WORD_ID a, WORD_ID b ) const
{
  // cerr << "c(" << m_vcb.GetWord(a) << ":" << m_vcb.GetWord(b) << ")=" << m_vcb.GetWord(a).compare( m_vcb.GetWord(b) ) << endl;
  return m_vcb.GetWord(a).compare( m_vcb.GetWord(b) );
}

int SuffixArray::Count( const vector< WORD > &phrase )
{
  INDEX dummy;
  return LimitedCount( phrase, m_size, dummy, dummy, 0, m_size-1 );
}

bool SuffixArray::MinCount( const vector< WORD > &phrase, INDEX min )
{
  INDEX dummy;
  return LimitedCount( phrase, min, dummy, dummy, 0, m_size-1 ) >= min;
}

bool SuffixArray::Exists( const vector< WORD > &phrase )
{
  INDEX dummy;
  return LimitedCount( phrase, 1, dummy, dummy, 0, m_size-1 ) == 1;
}

int SuffixArray::FindMatches( const vector< WORD > &phrase, INDEX &firstMatch, INDEX &lastMatch, INDEX search_start, INDEX search_end )
{
  return LimitedCount( phrase, m_size, firstMatch, lastMatch, search_start, search_end );
}

int SuffixArray::LimitedCount( const vector< WORD > &phrase, INDEX min, INDEX &firstMatch, INDEX &lastMatch, INDEX search_start, INDEX search_end )
{
  // cerr << "FindFirst\n";
  INDEX start = search_start;
  INDEX end = (search_end == -1) ? (m_size-1) : search_end;
  INDEX mid = FindFirst( phrase, start, end );
  // cerr << "done\n";
  if (mid == m_size) return 0; // no matches
  if (min == 1) return 1;      // only existance check

  int matchCount = 1;

  //cerr << "before...\n";
  firstMatch = FindLast( phrase, mid, start, -1 );
  matchCount += mid - firstMatch;

  //cerr << "after...\n";
  lastMatch = FindLast( phrase, mid, end, 1 );
  matchCount += lastMatch - mid;

  return matchCount;
}

SuffixArray::INDEX SuffixArray::FindLast( const vector< WORD > &phrase, INDEX start, INDEX end, int direction )
{
  end += direction;
  while(true) {
    INDEX mid = ( start + end + (direction>0 ? 0 : 1) )/2;

    int match = Match( phrase, mid );
    int matchNext = Match( phrase, mid+direction );
    //cerr << "\t" << start << ";" << mid << ";" << end << " -> " << match << "," << matchNext << endl;

    if (match == 0 && matchNext != 0) return mid;

    if (match == 0) // mid point is a match
      start = mid;
    else
      end = mid;
  }
}

SuffixArray::INDEX SuffixArray::FindFirst( const vector< WORD > &phrase, INDEX &start, INDEX &end )
{
  while(true) {
    INDEX mid = ( start + end + 1 )/2;
    //cerr << "FindFirst(" << start << ";" << mid << ";" << end << ")\n";
    int match = Match( phrase, mid );

    if (match == 0) return mid;
    if (start >= end && match != 0 ) return m_size;

    if (match > 0)
      start = mid+1;
    else
      end = mid-1;
  }
}

int SuffixArray::Match( const vector< WORD > &phrase, INDEX index )
{
  INDEX pos = m_index[ index ];
  for(INDEX i=0; i<phrase.size() && i+pos<m_size; i++) {
    int match = CompareWord( m_vcb.GetWordID( phrase[i] ), m_array[ pos+i ] );
    // cerr << "{" << index << "+" << i << "," << pos+i << ":" << match << "}" << endl;
    if (match != 0)
      return match;
  }
  return 0;
}

void SuffixArray::List(INDEX start, INDEX end)
{
  for(INDEX i=start; i<=end; i++) {
    INDEX pos = m_index[ i ];
    // cerr << i << ":" << pos << "\t";
    for(int j=0; j<5 && j+pos<m_size; j++) {
      cout << " " << m_vcb.GetWord( m_array[ pos+j ] );
    }
    // cerr << "\n";
  }
}

void SuffixArray::Save(const string& fileName ) const
{
  FILE *pFile = fopen ( fileName.c_str() , "w" );
  if (pFile == NULL) {
    cerr << "Cannot open " << fileName << endl;
    exit(1);
  }

  fwrite( &m_size, sizeof(INDEX), 1, pFile );
  fwrite( m_array, sizeof(WORD_ID), m_size, pFile ); // corpus
  fwrite( m_index, sizeof(INDEX), m_size, pFile );   // suffix array
  fwrite( m_wordInSentence, sizeof(char), m_size, pFile); // word index
  fwrite( m_sentence, sizeof(INDEX), m_size, pFile); // sentence index

  fwrite( &m_sentenceCount, sizeof(INDEX), 1, pFile );
  fwrite( m_sentenceLength, sizeof(char), m_sentenceCount, pFile); // sentence length
  fclose( pFile );

  m_vcb.Save( fileName + ".src-vcb" );
}

void SuffixArray::Load(const string& fileName )
{
  FILE *pFile = fopen ( fileName.c_str() , "r" );
  if (pFile == NULL) {
    cerr << "no such file or directory " << fileName << endl;
    exit(1);
  }

  cerr << "loading from " << fileName << endl;

  fread( &m_size, sizeof(INDEX), 1, pFile );
  cerr << "words in corpus: " << m_size << endl;
  m_array = (WORD_ID*) calloc( sizeof( WORD_ID ), m_size );
  m_index = (INDEX*) calloc( sizeof( INDEX ), m_size );
  m_wordInSentence = (char*) calloc( sizeof( char ), m_size );
  m_sentence = (INDEX*) calloc( sizeof( INDEX ), m_size );

  if (m_array == NULL) {
    cerr << "Error: cannot allocate memory to m_array" << endl;
    exit(1);
  }

  if (m_index == NULL) {
    cerr << "Error: cannot allocate memory to m_index" << endl;
    exit(1);
  }

  if (m_wordInSentence == NULL) {
    cerr << "Error: cannot allocate memory to m_wordInSentence" << endl;
    exit(1);
  }

  if (m_sentence == NULL) {
    cerr << "Error: cannot allocate memory to m_sentence" << endl;
    exit(1);
  }

  fread( m_array, sizeof(WORD_ID), m_size, pFile ); // corpus
  fread( m_index, sizeof(INDEX), m_size, pFile );   // suffix array
  fread( m_wordInSentence, sizeof(char), m_size, pFile); // word index
  fread( m_sentence, sizeof(INDEX), m_size, pFile); // sentence index

  fread( &m_sentenceCount, sizeof(INDEX), 1, pFile );
  cerr << "sentences in corpus: " << m_sentenceCount << endl;
  m_sentenceLength = (char*) calloc( sizeof( char ), m_sentenceCount );

  if (m_sentenceLength == NULL) {
    cerr << "Error: cannot allocate memory to m_sentenceLength" << endl;
    exit(1);
  }

  fread( m_sentenceLength, sizeof(char), m_sentenceCount, pFile); // sentence length
  fclose( pFile );

  m_vcb.Load( fileName + ".src-vcb" );
}
