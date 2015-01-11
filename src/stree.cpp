#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

static bool printFrequency = true;
static bool printPrefix = true;    // Bei false statt dessen nur Whitespace

void usage()
{
  std::cerr <<
    "NAME\n"
    "  stree - Build and display a prefix trie from a list of strings\n"
    "\n"
    "SYNOPSIS\n"
    "  stree [-a] [-s] [-p] [-f] [-F] file\n"
    "  stree -h\n"
    "\n"
    "DESCRIPTION\n"
    "  stree is building a prefix trie from strings. The strings are read either\n"
    "  from the given file or stdin.\n"
    "\n"
    "  The strings are managed by their common prefixes, resulting in a tree\n"
    "  structure that will be written to stdout in a configurable way.\n"
    "\n"
    "OPTIONS\n"
    "  -a	\n"
    "      Sort output alphabetically. This is the default, unless one of -f/-F is\n"
    "      given.\n"
    "\n"
    "  -s	\n"
    "      If multiple strings have the same prefix but different continuations,\n"
    "      they are usually written on their own line with the common prefix\n"
    "      repeated, e.g. the string \"foo\", \"bar\", and \"baz\" would give:\n"
    "          1 foo\n"
    "          2 ba\n"
    "          1 bar\n"
    "          1 baz\n"
    "      With -s, the common prefix is replaced by spaces, e.g.\n"
    "          1 foo\n"
    "          2 ba\n"
    "          1   r\n"
    "          1   z\n"
    "      Spaces are left out if -p or -b are used\n"
    "\n"
    "  -p\n"
    "      Use parentheses to reveal structure, e.g. turning \"foo\", \"bar\", \"baz\"\n"
    "      into \"((foo)(ba(r)(z)))\"\n"
    "\n"
    "  -f	Prepend the frequency to each line of output. Also sorts by frequency,\n"
    "      unless -a is also used.\n"
    "\n"
    "  -F\n"
    "      Just as -f, but append the frequency, rather than prepending it. This\n"
    "      can be uesful together with -s if you view the output with an editor\n"
    "      that is capable to fold by indent.\n"
    "\n"
    "  -h  Print this help and exit\n"
    "\n"
    "AUTHOR\n"
    "  Benjamin King (benjaminking@web.de)\n";
  exit( 1 );

  ///// TODO @@@ Fix logic for bash output
  /////  "\n"
  /////  "  -b	Like -p, but using bash syntax, e.g. \"{foo,bar{r,z}}\"\n"
}

/*
  For each bhaviour, there is a static bool signifying to what it has been set.
  Each option also has a function to set the corresponding behaviour.

  All these functions are stored in a map for later execution.
*/
static std::map< std::string, void(*)() > optionSetter;

static bool forceAlphabetically = false;
void setForceAlphabetically() { forceAlphabetically = true; }

static bool repeatPrefix = true;
void setNoRepeatPrefix() { repeatPrefix = false; }

static bool appendFrequency = false;
void setAppendFrequency() { appendFrequency = true; }

static bool prependFrequency = false;
void setPrependFrequency() { prependFrequency = true; }

enum structureStyle_t
{
  linewise,
  parentheses,
  bash
};
static structureStyle_t structureStyle = linewise;
void setParentheses() { structureStyle = parentheses; }
void setBash() { structureStyle = bash; }

/*
  A charNode_c represents a node in the trie of strings.

  It represents some prefix of all strings stored below it, has a count for the number of them and a
  map of charNode_c's that can follow.
*/
class charNode_c;
typedef std::map< char, charNode_c > charNodes_c;
class charNode_c
{
  unsigned int _count;
  charNodes_c _next;

public:
  charNode_c() : _count( 0 ) {}
  charNodes_c& next()             { return _next; }
  const charNodes_c& next() const { return _next; }
  bool operator++()               { ++_count; }
  unsigned int count() const      { return _count; }
};

bool orderByCount( const charNodes_c::const_iterator &lhs, const charNodes_c::const_iterator &rhs )
{
  return lhs->second.count() > rhs->second.count();
}

void read( std::istream &in, charNode_c &root )
{
  std::string s;
  while ( getline( in, s ) )
  {
    ++root;
    charNode_c *current = &root;
    for ( int i = 0; i < s.length(); ++i )
    {
      // Enter the string while counting the charcters
      current = &current->next()[ s[ i ] ];
      ++*current;
    }
  }
}

/*
  Print the prefix tree to stdout.

  Given a charNode_c *node, the character it represents 'c' and the prefix that is common to all
  strings represented by node, the subnodes of node are written to stdout.

  if 'c' is NUL, node is assumed to be the root of the prefix tree.
*/
void dump( const charNode_c *node, const char c, std::string prefix )
{
  // Nodes need to exist
  if ( !node || !node->count() ) return;

  if ( structureStyle == parentheses )
    printf( "(" );
  else if ( structureStyle == bash )
    printf( "{" );

  if ( prependFrequency )
    printf( "%-8i ", node->count() );

  if ( repeatPrefix )
    // Repeat the prefix
    printf( "%s", prefix.c_str() );
  else if ( structureStyle == linewise )
    // Use whitespace instead
    printf( "%s", std::string( prefix.length(), ' ' ).c_str() );
  else
  {
    // Do not print anything at all
  }

  // If this is not the root node, there is a character to print. It is also appended to the prefix
  // for later subnodes.
  if ( c )
  {
    printf( "%c", c );
    prefix += c;
  }

  /*
    As long as there is just a single character following with the same count of strings, it should
    be added to the current line instead of doing a proper recursion, e.g. the strings "foo", "bar",
    "baz" will give
      foo
      ba
        r
        z
    and not
      f
       o
        o
      b
       a
        r
        z
   This is handled by the following while loop.
  */
  charNodes_c::const_iterator it = node->next().begin();
  while ( it != node->next().end() )
  {
    if ( it->second.count() == node->count() )
    {
      // Since node->count() >= the sum of counts of all its children, this means that there is only
      // on possible and mandatory continuation for node. We will print it in the same line.
      printf( "%c", it->first );
      prefix += it->first;
      // Reset node and loop iterator to the next node
      node = &it->second;
      it = node->next().begin();
    }
    else
    {
      // dump children recursively
      if ( appendFrequency )
      {
        if ( c )
          printf( " " );
        printf( "%i", node->count() );
      }

      if ( structureStyle == linewise )
        printf( "\n" );

      std::vector< charNodes_c::const_iterator > children;
      for ( ;it != node->next().end(); ++it )
        children.push_back( it );

      // Sort by frequency if is visible unless the user says no
      if ( ( prependFrequency || appendFrequency ) && !forceAlphabetically )
        sort( children.begin(), children.end(), orderByCount );

      std::vector< charNodes_c::const_iterator >::const_iterator cit;
      for ( cit = children.begin(); cit != children.end(); ++cit )
      {
        if ( structureStyle == bash && cit != children.begin() )
          printf( "," );
        dump( &(*cit)->second, (*cit)->first, prefix );
      }

      if ( structureStyle == parentheses )
        printf( ")" );
      if ( structureStyle == bash )
        printf( "}" );
      //if ( !c && structureStyle != linewise )
      //  printf( "\n" );

      // And we are done
      return;
    }
  }

  // Only when no childs had to be written recursively
  if ( appendFrequency )
  {
    if ( c )
      printf( " " );
    printf( "%i", node->count() );
  }

  switch ( structureStyle )
  {
    case parentheses: printf( ")" ); break;
    case bash: printf( "}" ); break;
    default: printf( "\n" );
  }

  if ( !c && structureStyle != linewise )
    printf( "\n" );
}

int main( int argc, char *argv[] )
{
  optionSetter[ "-h" ] = usage;
  optionSetter[ "-a" ] = setForceAlphabetically;
  optionSetter[ "-s" ] = setNoRepeatPrefix;
  optionSetter[ "-f" ] = setPrependFrequency;
  optionSetter[ "-F" ] = setAppendFrequency;
  optionSetter[ "-p" ] = setParentheses;
  ///// TODO @@@ Does not work correctly (dreaded commas...)
  /////optionSetter[ "-b" ] = setBash;

  int i;
  for ( i = 1; i < argc; ++i )
  {
    if ( strcmp( argv[ i ], "--" ) == 0 )
    {
      // End of options
      ++i;
      break;
    }
    if ( optionSetter.count( argv[ i ] ) )
      optionSetter[ argv[ i ] ]();
    else
      break;
  }

  charNode_c root;
  if ( i == argc )
  {
    // read from stdin
    read( std::cin, root );
  }
  else
  {
    for( ; i < argc; ++i )
    {
      std::fstream file( argv[ i ] );
      read( file, root );
      file.close();
    }
  }

  // Baum ausgeben
  dump( &root, '\000', "" );
}
