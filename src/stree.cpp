#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <map>
#include <string>
#include <vector>

static bool printFrequency = true;
static bool printPrefix = true;

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
    "  -b\n"
    "      Use bash brace expansion format, e.g. turning \"foo\", \"bar\", \"baz\"\n"
    "      into \"{foo,ba{r,z}}\n"
    "  -g\n"
    "      Create representation suitable for graphviz, e.g. turning \"foo\", \"bar\", \"baz\"\n"
    "      into \"digraph { foo;ba->{r;z}}\"\n"
    "\n"
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
  bash,
  graphviz
};
static structureStyle_t structureStyle = linewise;
void setParentheses() { structureStyle = parentheses; }
void setBash() { structureStyle = bash; }
void setGraphviz() { structureStyle = graphviz; }

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
void dump( std::ostream &out, std::string current, std::string prefix,
           const charNode_c *node, bool isRootNode )
{
  // Nodes need to exist
  if ( !node || !node->count() ) return;

  // If there is just one possible an non-optional continuation of the string, we do not want to put
  // it on an extra line, but add it to the current one.
  //
  // We can only do this if...
  if ( node->next().size() == 1 &&             // ...there is only one way to contiune and...
       node->next().begin()->second.count() == // ...the current string can not end here.
       node->count() )
  {
    // We implement this by recursion
    dump( out, current + node->next().begin()->first, prefix,
          &node->next().begin()->second, isRootNode );
    return;
  }

  // The root node in graphviz needs special consideration
  if ( isRootNode && structureStyle == graphviz )
     out << "digraph {";

  // Invariants at this point:
  // - The way root -> node represents the string prefix+current of which the prefix part has
  //   already been printed, but the current part has not.
  // - prefix+current is a complete string from the input data set, or it is the longest
  //   common prefix of at least two such strings.

  // Some output formats require that we surround the current string with something:
  if ( structureStyle == parentheses )
    out << "(";

  // print current string, possible repeating the prefix along the way, and decorate the thing with
  // frequencies.
  if ( prependFrequency )
  {
    if ( structureStyle == linewise )
      out << std::setw( 8 ) << std::left; // neat vertical alignment
    out << node->count();
    if ( !current.empty() )
       out << " ";
  }

  if ( repeatPrefix )
    out << prefix << current;
  else if ( structureStyle == linewise )
    // Use whitespace instead
    out << std::string( prefix.length(), ' ' ) << current;
  else
    out << current;

  if ( appendFrequency )
  {
    if ( !current.empty() || prependFrequency )
      out << " ";
    out << node->count();
  }

  if ( structureStyle == linewise )
    out << "\n";

  // We may need to recurse.
  if ( !node->next().empty() )
  {
    if ( structureStyle == graphviz && !current.empty() )
      out << " -> {";
    if ( structureStyle == bash )
    {
      // bash output compresses "foo foolish" to "foo{,lish}". To determine if node represents a
      // string in its own right, we check if is more often in the input data set than the sum of
      // the children.
      std::size_t nextCount = 0;
      for ( charNodes_c::const_iterator it = node->next().begin();
            it != node->next().end();
            ++it )
        nextCount += it->second.count();
      if ( nextCount < node->count() )
        out << "{,";
      else
        out << "{";
    }

    // Now dump each child, sorted by frequency if is visible unless the user says no
    std::vector< charNodes_c::const_iterator > children;
    for ( charNodes_c::const_iterator it = node->next().begin();
          it != node->next().end();
          ++it )
      children.push_back( it );
    if ( ( prependFrequency || appendFrequency ) && !forceAlphabetically )
      sort( children.begin(), children.end(), orderByCount );
    std::vector< charNodes_c::const_iterator >::const_iterator cit;
    for ( cit = children.begin(); cit != children.end(); ++cit )
    {
      if ( cit != children.begin() )
        if ( structureStyle == graphviz )
          out << ";";
        else if ( structureStyle == bash )
          out << ",";

      dump( out, std::string( 1, ( *cit )->first ), prefix + current,
            &( *cit )->second, false );
    }
    if ( structureStyle == graphviz && !current.empty() ||
         structureStyle == bash )
      out << "}";
  }

  if ( structureStyle == parentheses )
    out << ")";
  if ( isRootNode )
  {
    if ( structureStyle == graphviz || ( structureStyle == bash && !isRootNode ) )
      out << "}";
    out << "\n";
  }
}

int main( int argc, char *argv[] )
{
  optionSetter[ "-h" ] = usage;
  optionSetter[ "-a" ] = setForceAlphabetically;
  optionSetter[ "-s" ] = setNoRepeatPrefix;
  optionSetter[ "-f" ] = setPrependFrequency;
  optionSetter[ "-F" ] = setAppendFrequency;
  optionSetter[ "-p" ] = setParentheses;
  optionSetter[ "-b" ] = setBash;
  optionSetter[ "-g" ] = setGraphviz;

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

  dump( std::cout, "", "", &root, true );
}
