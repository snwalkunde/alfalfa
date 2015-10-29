#ifndef FRAME_DB_HH
#define FRAME_DB_HH

#include <map>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <boost/multi_index/tag.hpp>

#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>

#include "db.hh"
#include "optional.hh"
#include "frame_info.hh"
#include "dependency_tracking.hh"
#include "protobufs/alfalfa.pb.h"

using namespace std;
using namespace boost::multi_index;
using boost::multi_index_container;

using namespace google::protobuf::io;

struct FrameData_OutputHashExtractor
{
  typedef const size_t result_type;

  const result_type & operator()( const FrameInfo & fd ) const { return fd.target_hash().output_hash; }
  result_type & operator()( FrameInfo * fd ) { return fd->target_hash().output_hash; }
};

struct FrameData_SourceHashExtractor
{
  typedef const SourceHash result_type;

  const result_type & operator()( const FrameInfo & fd ) const { return fd.source_hash(); }
  result_type & operator()( FrameInfo * fd ) { return fd->source_hash(); }
};

struct FrameData_TargetHashExtractor
{
  typedef const TargetHash result_type;

  const result_type & operator()( const FrameInfo & fd ) const { return fd.target_hash(); }
  result_type & operator()( FrameInfo * fd ) { return fd->target_hash(); }
};

struct FrameDataSetSequencedTag;

typedef multi_index_container
<
  FrameInfo,
  indexed_by
  <
    hashed_non_unique<FrameData_OutputHashExtractor>,
    hashed_non_unique<FrameData_SourceHashExtractor, std::hash<SourceHash>, std::equal_to<SourceHash> >,
    sequenced<tag<FrameDataSetSequencedTag> >
  >
> FrameDataSet;

// typedef FrameDataSet::nth_index<0>::type FrameDataSetByFrameName;
typedef FrameDataSet::nth_index<0>::type FrameDataSetByOutputHash;
typedef FrameDataSet::nth_index<1>::type FrameDataSetBySourceHash;
typedef FrameDataSet::index<FrameDataSetSequencedTag>::type FrameDataSetSequencedAccess;

class FrameDataSetSourceHashSearch
{
private:
  class FrameDataSetSourceHashSearchIterator
    : public std::iterator<std::forward_iterator_tag, FrameInfo>
  {
  private:
    size_t stage_;
    SourceHash source_hash_;
    FrameDataSetBySourceHash & data_set_;
    FrameDataSetBySourceHash::iterator itr_;
    FrameDataSetBySourceHash::iterator begin_, current_end_;

  public:
    FrameDataSetSourceHashSearchIterator( FrameDataSetBySourceHash & data_set,
      SourceHash source_hash, bool end );

    FrameDataSetSourceHashSearchIterator( const FrameDataSetSourceHashSearchIterator & it );

    FrameDataSetSourceHashSearchIterator & operator++();
    FrameDataSetSourceHashSearchIterator operator++( int );

    bool operator==( const FrameDataSetSourceHashSearchIterator & rhs ) const;
    bool operator!=( const FrameDataSetSourceHashSearchIterator & rhs ) const;

    const FrameInfo & operator*() const;
    const FrameInfo * operator->() const;
  };

  SourceHash source_hash_;
  FrameDataSetBySourceHash & data_set_;
  FrameDataSetSourceHashSearchIterator begin_iterator_, end_iterator_;

public:
  typedef FrameDataSetSourceHashSearchIterator iterator;

  FrameDataSetSourceHashSearch( FrameDataSetBySourceHash & data_set, SourceHash source_hash );

  iterator begin() { return begin_iterator_; }
  iterator end() { return end_iterator_; }
};

class FrameDB : public BasicDatabase<FrameInfo, AlfalfaProtobufs::FrameInfo,
  FrameDataSet, FrameDataSetSequencedTag>
{
public:
  FrameDB( const std::string & filename, const std::string & magic_number, OpenMode mode = OpenMode::READ )
    : BasicDatabase<FrameInfo, AlfalfaProtobufs::FrameInfo,
      FrameDataSet, FrameDataSetSequencedTag>( filename, magic_number, mode )
  {}
  
  vector<std::string> ivf_files();

  std::pair<FrameDataSetByOutputHash::iterator, FrameDataSetByOutputHash::iterator>
  search_by_output_hash( const size_t & output_hash );

  std::pair<FrameDataSetSourceHashSearch::iterator, FrameDataSetSourceHashSearch::iterator>
  search_by_decoder_hash( const DecoderHash & decoder_hash );
};

#endif /* FRAME_DB */
