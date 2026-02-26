#ifndef CONVENTIONAL_SORT_UINT64_H
#define CONVENTIONAL_SORT_UINT64_H

#include <stdint.h>
#include <stdbool.h>

#include "polite_sort_uint64.h"

typedef union
{
  size_t count;
  uint64_t *begin;
} bucket_counts_or_begins_ty;

static void conventional_radix_sort_internal( uint64_t *begin, uint64_t *end, unsigned int shift, bucket_counts_or_begins_ty *bucket_counts_or_begins )
{
  uint64_t *bucket_ends[ 256 ] = {};

  for( int i = 0; i < 256; ++i )
    bucket_counts_or_begins[ i ].count = 0;

  // TODO: Benchmark against the unrolled version below.
  // for( uint64_t *el = begin; el != end; ++el )
  //   ++bucket_counts_or_begins[ ( *el >> shift ) & 0xFF ].count;

  uint64_t *el = begin;
  while( end - el >= 8 )
  {
    uint64_t x0 = el[ 0 ];
    uint64_t x1 = el[ 1 ];
    uint64_t x2 = el[ 2 ];
    uint64_t x3 = el[ 3 ];
    uint64_t x4 = el[ 4 ];
    uint64_t x5 = el[ 5 ];
    uint64_t x6 = el[ 6 ];
    uint64_t x7 = el[ 7 ];
    ++bucket_counts_or_begins[ x0 >> shift & 0xFFull ].count;
    ++bucket_counts_or_begins[ x1 >> shift & 0xFFull ].count;
    ++bucket_counts_or_begins[ x2 >> shift & 0xFFull ].count;
    ++bucket_counts_or_begins[ x3 >> shift & 0xFFull ].count;
    ++bucket_counts_or_begins[ x4 >> shift & 0xFFull ].count;
    ++bucket_counts_or_begins[ x5 >> shift & 0xFFull ].count;
    ++bucket_counts_or_begins[ x6 >> shift & 0xFFull ].count;
    ++bucket_counts_or_begins[ x7 >> shift & 0xFFull ].count;
    el += 8;
  }

  while( el != end )
  {
    ++bucket_counts_or_begins[ ( *el >> shift ) & 0xFF ].count;
    ++el;
  }

  int nonempty_bucket_count = 0;
  uint64_t *bucket_begin = begin;
  for( int i = 0; i < 256; ++i )
  {
    size_t bucket_count = bucket_counts_or_begins[ i ].count;
    nonempty_bucket_count += (bool)bucket_count;
    bucket_counts_or_begins[ i ].begin = bucket_begin;
    bucket_begin += bucket_count;
    bucket_ends[ i ] = bucket_begin;
  }

  if( nonempty_bucket_count > 1 )
    for( int i = 0; i < 256; i++ )
    {
      while( bucket_counts_or_begins[ i ].begin != bucket_ends[ i ] )
      {
        int destination_child_bucket;
        if( ( destination_child_bucket = *bucket_counts_or_begins[ i ].begin >> shift & 0xFFull ) != i )
        {
          uint64_t temp = *bucket_counts_or_begins[ i ].begin, swap;
          do
          {
            swap = temp;
            temp = *bucket_counts_or_begins[ destination_child_bucket ].begin;
            *( bucket_counts_or_begins[ destination_child_bucket ].begin++ ) = swap;

            // Fetch the next destination-bucket so that it's ready for use next time we need it.
            // This optimization proved to be significant in benchmarking.
            __builtin_prefetch( bucket_counts_or_begins[ destination_child_bucket ].begin );

            destination_child_bucket = temp >> shift & 0xFFull;
          } while( destination_child_bucket != i );

          *( bucket_counts_or_begins[ i ].begin++ ) = temp;
        }
        else
          ++bucket_counts_or_begins[ i ].begin;
      }
    }

  if( shift != 0 )
  {
    shift -= 8;
    bucket_begin = begin;
    for( int i = 0; i < 256; ++i )
    {
      size_t bucket_count = bucket_ends[ i ] - bucket_begin;
      if( bucket_count > 64 )
        conventional_radix_sort_internal( bucket_begin, bucket_ends[ i ], shift, bucket_counts_or_begins );
      else if( bucket_count > 1 )
        sort_small_bucket( bucket_begin, bucket_ends[ i ] );

      bucket_begin = bucket_ends[ i ];
    }
  }
}

void conventional_radix_sort( uint64_t *begin, uint64_t *end )
{
  if( end - begin <= 64 )
    sort_small_bucket( begin, end );
  else
  {
    bucket_counts_or_begins_ty bucket_counts_or_begins[ 256 ];
    conventional_radix_sort_internal( begin, end, 56, bucket_counts_or_begins );
  }
}

#endif
