#include <cstdint>
#include <random>
#include <algorithm>
#include <iostream>
#include <cstring>
#include <chrono>
#include <limits>
#include <vector>
#include <iomanip>
#include <string>
#include <utility>
#include <thread>

#include "libraries/ska_sort.hpp"
#include "libraries/ksort.h"
#include <boost/sort/sort.hpp>
#include "polite_sort_uint64.h"
#include "conventional_radix_sort_uint64.h"

#include "line_chart_ChatGPT.h"

#define rskey( x ) x
KRADIX_SORT_INIT( ksort, uint64_t, rskey, sizeof( uint64_t ) )

static std::random_device random_device;
static std::mt19937_64 rng( std::random_device() );
static std::uniform_int_distribution<uint64_t> distribution_full;
static std::uniform_int_distribution<uint32_t> distribution_limited_range;

static void fuzz_sorted( uint64_t *begin, size_t *end, size_t swaps )
{
  size_t count = end - begin;
  for( std::size_t i = 0; i < swaps; ++i )
    std::swap( begin[ distribution_full( rng ) % count ], begin[ distribution_full( rng ) % count ] );
}

int main()
{
  const size_t element_counts[] = {
    1000, 5000, 10000, 50000, 100000, 500000, 1000000, 5000000, 10000000, 50000000, 100000000
  };
  const unsigned int run_counts[] { 60, 60, 30, 30, 15, 15, 7, 7, 4, 4, 3 };
  const char *series_labels[] = { "ska_sort", "spreadsort", "kradix_sort", "conventional", "polite_sort" };
  const char* x_ticks[] = { "1K", "5K", "10K", "50K", "100K", "500K", "1M", "5M", "10M", "50M", "100M" };

  size_t max_element_count = element_counts[ sizeof( element_counts ) / sizeof( *element_counts ) - 1 ];
  std::vector<uint64_t> original_data( max_element_count );
  std::vector<uint64_t> mutable_data( max_element_count );

  std::vector<std::vector<double>> results_random( 5 );
  std::vector<std::vector<double>> results_limited_range_random( 5 );
  std::vector<std::vector<double>> results_fully_presorted( 5 );
  std::vector<std::vector<double>> results_mostly_presorted( 5 );
  std::vector<std::vector<double>> results_moderately_presorted( 5 );

  #define BENCH( sort_function, results_vector )                                                                     \
  do                                                                                                                 \
  {                                                                                                                  \
    std::this_thread::sleep_for( std::chrono::milliseconds( 500 ) );                                                 \
    double lowest_result = std::numeric_limits<double>::max();                                                       \
    for( unsigned int run = 0; run < run_counts[ i ]; ++run )                                                        \
    {                                                                                                                \
      std::copy( original_data.begin(), original_data.begin() + element_counts[ i ], mutable_data.begin() );         \
      auto start = std::chrono::steady_clock::now();                                                                 \
      sort_function( &mutable_data[ 0 ], &mutable_data[ 0 ] + element_counts[ i ] );                                 \
      auto end = std::chrono::steady_clock::now();                                                                   \
      auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>( end - start ).count();                         \
      lowest_result = std::min( (double)ns / element_counts[ i ], lowest_result );                                   \
      if( !std::is_sorted( mutable_data.begin(), mutable_data.begin() + element_counts[ i ] ) )                      \
         std::cout << #sort_function << " failed to sort!\n";                                                        \
    }                                                                                                                \
    results_vector.push_back( lowest_result );                                                                       \
    std::cout << "    " << std::left << std::setw( 40 ) << #sort_function << ": " << std::right << std::setw( 4 ) << \
      std::fixed << std::setprecision( 2 ) << lowest_result << " ns\n";                                              \
  } while( 0 )                                                                                                       \

  // Random data.

  std::cout << "Random data:\n";
  std::generate( original_data.begin(), original_data.end(), [&](){ return distribution_full( rng ); } );

  for( unsigned int i = 0; i < sizeof( element_counts ) / sizeof( *element_counts ); ++i )
  {
    std::cout << "  " << element_counts[ i ] << " elements:\n";
    BENCH( ska_sort, results_random[ 0 ] );
    BENCH( boost::sort::spreadsort::integer_sort, results_random[ 1 ] );
    BENCH( radix_sort_ksort, results_random[ 2 ] );
    BENCH( conventional_radix_sort, results_random[ 3 ] );
    BENCH( polite_sort, results_random[ 4 ] );
  }

  // Range-limited random data.

  std::cout << "Random limited-range data:\n";
  std::generate( original_data.begin(), original_data.end(), [&](){ return distribution_limited_range( rng ); } );

  for( unsigned int i = 0; i < sizeof( element_counts ) / sizeof( *element_counts ); ++i )
  {
    std::cout << "  " << element_counts[ i ] << " elements:\n";
    BENCH( ska_sort, results_limited_range_random[ 0 ] );
    BENCH( boost::sort::spreadsort::integer_sort, results_limited_range_random[ 1 ] );
    BENCH( radix_sort_ksort, results_limited_range_random[ 2 ] );
    BENCH( conventional_radix_sort, results_limited_range_random[ 3 ] );
    BENCH( polite_sort, results_limited_range_random[ 4 ] );
  }

  // Presorted data.

  std::cout << "Fully presorted data:\n";
  std::generate( original_data.begin(), original_data.end(), [&](){ return distribution_full( rng ); } );
  radix_sort_ksort( &original_data[ 0 ], &original_data[ 0 ] + max_element_count );

  for( unsigned int i = 0; i < sizeof( element_counts ) / sizeof( *element_counts ); ++i )
  {
    std::cout << "  " << element_counts[ i ] << " elements:\n";
    BENCH( ska_sort, results_fully_presorted[ 0 ] );
    BENCH( boost::sort::spreadsort::integer_sort, results_fully_presorted[ 1 ] );
    BENCH( radix_sort_ksort, results_fully_presorted[ 2 ] );
    BENCH( conventional_radix_sort, results_fully_presorted[ 3 ] );
    BENCH( polite_sort, results_fully_presorted[ 4 ] );
  }

  // Mostly presorted data.

  std::cout << "Mostly presorted data:\n";
  std::generate( original_data.begin(), original_data.end(), [&](){ return distribution_full( rng ); } );
  radix_sort_ksort( &original_data[ 0 ], &original_data[ 0 ] + max_element_count );
  fuzz_sorted( &original_data[ 0 ], &original_data[ 0 ] + max_element_count, max_element_count / 50 );

  for( unsigned int i = 0; i < sizeof( element_counts ) / sizeof( *element_counts ); ++i )
  {
    std::cout << "  " << element_counts[ i ] << " elements:\n";
    BENCH( ska_sort, results_mostly_presorted[ 0 ] );
    BENCH( boost::sort::spreadsort::integer_sort, results_mostly_presorted[ 1 ] );
    BENCH( radix_sort_ksort, results_mostly_presorted[ 2 ] );
    BENCH( conventional_radix_sort, results_mostly_presorted[ 3 ] );
    BENCH( polite_sort, results_mostly_presorted[ 4 ] );
  }

  // Moderately presorted data.

  std::cout << "Moderately presorted data:\n";
  std::generate( original_data.begin(), original_data.end(), [&](){ return distribution_full( rng ); } );
  radix_sort_ksort( &original_data[ 0 ], &original_data[ 0 ] + max_element_count );
  fuzz_sorted( &original_data[ 0 ], &original_data[ 0 ] + max_element_count, max_element_count / 10 );

  for( unsigned int i = 0; i < sizeof( element_counts ) / sizeof( *element_counts ); ++i )
  {
    std::cout << "  " << element_counts[ i ] << " elements:\n";
    BENCH( ska_sort, results_moderately_presorted[ 0 ] );
    BENCH( boost::sort::spreadsort::integer_sort, results_moderately_presorted[ 1 ] );
    BENCH( radix_sort_ksort, results_moderately_presorted[ 2 ] );
    BENCH( conventional_radix_sort, results_moderately_presorted[ 3 ] );
    BENCH( polite_sort, results_moderately_presorted[ 4 ] );
  }

  std::cout << "Generating charts.\n";

  write_svg_line_chart(
      "random.svg",
      "Random Data",
      "Number of elements",
      "Nanoseconds per element",
      results_random,
      series_labels,
      x_ticks
  );

  write_svg_line_chart(
      "random_limited_range.svg",
      "Random Limited-Range Data",
      "Number of elements",
      "Nanoseconds per element",
      results_limited_range_random,
      series_labels,
      x_ticks
  );

  write_svg_line_chart(
      "fully_presorted.svg",
      "Fully Presorted Data",
      "Number of elements",
      "Nanoseconds per element",
      results_fully_presorted,
      series_labels,
      x_ticks
  );

  write_svg_line_chart(
      "mostly_presorted.svg",
      "Mostly Presorted Data",
      "Number of elements",
      "Nanoseconds per element",
      results_mostly_presorted,
      series_labels,
      x_ticks
  );

  write_svg_line_chart(
      "moderately_presorted.svg",
      "Moderately Presorted Data",
      "Number of elements",
      "Nanoseconds per element",
      results_moderately_presorted,
      series_labels,
      x_ticks
  );

  std::cout << "Done\n";
}