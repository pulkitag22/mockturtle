#include "experiments.hpp"

#include <mockturtle/networks/mig.hpp>
#include <mockturtle/algorithms/cut_rewriting.hpp>
#include <mockturtle/algorithms/mig_algebraic_rewriting.hpp>
#include <mockturtle/algorithms/node_resynthesis/mig_npn.hpp>
#include <mockturtle/algorithms/mig_resub.hpp>
#include <mockturtle/io/aiger_reader.hpp>
#include <mockturtle/views/depth_view.hpp>
#include <lorina/lorina.hpp>

#include <fmt/format.h>
#include <vector>

/*! \brief A benchmark
 */
template<class Ntk>
class benchmark
{
public:
  explicit benchmark( Ntk const& ntk, std::string const& name )
    : ntk_( ntk )
    , name_( name )
  {
  }

  std::string name() const
  {
    return name_;
  }

  Ntk clone() const
  {
    return cleanup_dangling( ntk_ );
  }

  Ntk const& get() const
  {
    return ntk_;
  }

protected:
  Ntk ntk_;
  std::string name_;
};

struct agent_statistics
{
  mockturtle::stopwatch<>::duration time_total{0};
  uint64_t size_before{0u};
  uint64_t size_after{0u};

  uint64_t depth_before{0u};
  uint64_t depth_after{0u};

  void report() const
  {
    fmt::print( "[i] time: {:>8.2f}\n[i] size:  {:6d} -> {:6d} ({:d})\n[i] depth: {:6d} -> {:6d} ({:d})\n",
                mockturtle::to_seconds( time_total ),
                size_before, size_after, int64_t( size_before ) - int64_t( size_after ),
                depth_before, depth_after, int64_t(depth_after) - depth_before );
  }
};

/*! \brief Algebraic depth rewriting for MIGs
 */
class mig_alg_rw
{
public:
  template<typename Ntk>
  void run( Ntk& ntk )
  {
    mockturtle::mig_algebraic_depth_rewriting_params ps;
    ps.strategy = mockturtle::mig_algebraic_depth_rewriting_params::dfs;
    ps.allow_area_increase = false;
    mockturtle::mig_algebraic_depth_rewriting( ntk, ps );
  }
};

/*! \brief Boolean resubstitution for MIGs
 */
class mig_rs
{
public:
  template<typename Ntk>
  void run( Ntk& ntk )
  {
    mockturtle::resubstitution_params ps;
    ps.max_pis = 8u;
    ps.max_inserts = 1u;
    ps.progress = true;

    mockturtle::mig_resubstitution( ntk, ps );
    ntk = cleanup_dangling( ntk );
  }
};

/*! \brief Boolean rewriting for MIGs
 */
class mig_rw
{
public:
  template<typename Ntk>
  void run( Ntk& ntk )
  {
    mockturtle::mig_npn_resynthesis resyn;
    mockturtle::cut_rewriting_params ps;
    ps.cut_enumeration_ps.cut_size = 4;
    cut_rewriting_with_compatibility_graph( ntk, resyn, ps );
    ntk = cleanup_dangling( ntk );
  }
};

namespace detail
{

template<typename Ntk, typename T>
void run_agents( Ntk& ntk, T&& agent )
{
  agent.run( ntk );
}

template<typename Ntk, typename T, typename ... Ts>
void run_agents( Ntk& ntk, T&& agent, Ts ... ts )
{
  agent.run( ntk );
  run_agents<Ntk, Ts...>( ntk, std::forward<Ts>(ts)... );
}

} /* detail */

template<typename... AgentTypes>
class flow
{
public:
  template<typename... T>
  explicit flow()
  {
    static_assert( ( sizeof...( AgentTypes ) > 0 ), "at least one agent must be specified" );
    st.resize( sizeof...( AgentTypes ) );
  }

  template<typename Benchmark>
  void run( Benchmark const& bm )
  {
    auto ntk = bm.clone();

    std::cout << "size_before = " << ntk.size() << std::endl;

    std::apply( [&]( auto&&... args ){
        detail::run_agents( ntk, std::forward<decltype( args )>( args )... );
      },
      agents );

    std::cout << "size_after = " << ntk.size() << std::endl;
  }

private:
  std::tuple<AgentTypes...> agents;
  std::vector<agent_statistics> st;
};

#if 0
class league
{
public:
  using network_type = mockturtle::mig_network;
  using benchmark_type = std::shared_ptr<benchmark<network_type>>;
  using agent_type = std::shared_ptr<agent<network_type>>;

public:
  explicit league()
  {
  }

  void run()
  {
    /* load benchmarks */
    fmt::print( "[i] agents = {}\n", agents.size() );
    fmt::print( "[i] benchmarks = {}\n", benchmarks.size() );

    for  ( auto const& benchmark : benchmarks )
    {
      for ( auto const& agent : agents )
      {
        fmt::print( "[i] execute agent `{}` on benchmark `{}`\n", agent->name(), benchmark->name() );        
        agent->run( benchmark->network() );
      }
    }

    /* print results */
    for ( auto const& agent : agents )
    {
      agent->report();
    }
  }

  void add_benchmark( network_type const& ntk, std::string const& name )
  {
    benchmarks.emplace_back( std::make_shared<benchmark<network_type>>( ntk, name ) );
  }

  template<class T>
  void add_agent()
  {
    agents.emplace_back( std::make_shared<T>() );
  }

protected:
  std::vector<benchmark_type> benchmarks;
  std::vector<agent_type> agents;
};
#endif

int main()
{
  /* define some optimization flows */
  flow<mig_rw, mig_rs, mig_rw, mig_rs, mig_rw, mig_rs> f0;
  flow<mig_rw, mig_rw, mig_rw, mig_rs, mig_rs, mig_rs> f1;

  /* add benchmarks */
  for ( auto const& benchmark : experiments::epfl_benchmarks( experiments::adder | experiments::bar | experiments::cavlc | experiments::dec | experiments::priority ) )
  {
    fmt::print( "[i] processig {}\n", benchmark );

    mockturtle::mig_network mig;
    if ( lorina::read_aiger( experiments::benchmark_path( benchmark ), mockturtle::aiger_reader( mig ) ) == lorina::return_code::success )
    {
      ::benchmark<mockturtle::mig_network> b( mig, benchmark );

      f0.run( b );
      f1.run( b );
    }
    else
    {
      fmt::print( "[i] parsing benchmark failed\n" );
    }
  }

  return 0;
}
