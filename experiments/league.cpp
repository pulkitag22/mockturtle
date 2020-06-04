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
    mockturtle::depth_view depth_ntk{ntk};

    mockturtle::mig_algebraic_depth_rewriting_params ps;
    ps.strategy = mockturtle::mig_algebraic_depth_rewriting_params::dfs;
    ps.allow_area_increase = false;
    mockturtle::mig_algebraic_depth_rewriting( depth_ntk, ps );

    ntk = cleanup_dangling( ntk );
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

template<typename Agent, uint32_t times>
class repeat
{
public:
  template<typename Ntk>
  void run( Ntk& ntk )
  {
    Agent a;
    for ( auto i = 0u; i < times; ++i )
    {
      a.run( ntk );
    }
  }
};

template<typename Agent0, typename Agent1>
class combine
{
public:
  template<typename Ntk>
  void run( Ntk& ntk )
  {
    Agent0().run( ntk );
    Agent1().run( ntk );
  }
};

class size_eval
{
public:
  template<typename Ntk>
  void run( Ntk& ntk )
  {
    std::cout << ntk.size() << std::endl;
  }
};

class network_size_evaluator
{
public:
  explicit network_size_evaluator()
  {
  }

  template<typename Ntk>
  void record( Ntk const& ntk )
  {
    sizes.emplace_back( ntk.size() );
  }

  void reset()
  {
    sizes.clear();
  }

  void report( std::string const& name = {} ) const
  {
    fmt::print( "[i] {}: ", name );
    for ( const auto& s : sizes )
    {
      fmt::print( "({})", s );
    }
    std::cout << std::endl;
  }

protected:
  std::vector<uint64_t> sizes;
};

class network_depth_evaluator
{
public:
  explicit network_depth_evaluator()
  {
  }

  template<typename Ntk>
  void record( Ntk const& ntk )
  {
    mockturtle::depth_view depth_ntk{ntk};
    depths.emplace_back( depth_ntk.depth() );
  }

  void reset()
  {
    depths.clear();
  }

  void report( std::string const& name = {} ) const
  {
    fmt::print( "[i] {}: ", name );
    for ( const auto& d : depths )
    {
      fmt::print( "({})", d );
    }
    std::cout << std::endl;
  }

protected:
  std::vector<uint64_t> depths;
};

class network_size_and_depth_evaluator
{
public:
  explicit network_size_and_depth_evaluator()
  {
  }

  template<typename Ntk>
  void record( Ntk const& ntk )
  {
    mockturtle::depth_view depth_ntk{ntk};
    data.emplace_back( std::make_tuple( depth_ntk.size(), depth_ntk.depth() ) );
  }

  void reset()
  {
    data.clear();
  }

  void report( std::string const& name = {} ) const
  {
    fmt::print( "[i] {}: ", name );
    for ( const auto& tup : data )
    {
      fmt::print( "({}, {})", std::get<0>( tup ), std::get<1>( tup ) );
    }
    std::cout << std::endl;
  }

protected:
  std::vector<std::tuple<uint64_t,uint64_t>> data;
};

namespace detail
{

template<typename Ntk, typename S, typename T>
void run_agents_on_network( Ntk& ntk, S& st, T&& agent )
{
  agent.run( ntk );
  st.record( ntk );
}

template<typename Ntk, typename S, typename T, typename ... Ts>
void run_agents_on_network( Ntk& ntk, S& st, T&& agent, Ts ... ts )
{
  agent.run( ntk );
  st.record( ntk );
  run_agents_on_network<Ntk, S, Ts...>( ntk, st, std::forward<Ts>( ts )... );
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
  }

  template<typename Benchmark, typename Evaluator>
  auto run( Benchmark const& bm, Evaluator& eval )
  {
    auto ntk = bm.clone();
    eval.record( ntk );

    std::apply( [&]( auto&&... args ){
        detail::run_agents_on_network( ntk, eval, std::forward<decltype( args )>( args )... );
      },
      agents );

    return ntk;
  }

private:
  std::tuple<AgentTypes...> agents;
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

/* TODOs:
 * - time stamps
 * - plots
 * - cec
 * - flow modifiers
 */
int main()
{
  /* define some optimization flows */
  flow<mig_rw, mig_rw, mig_rw> f0;
  flow<mig_rs, mig_rs, mig_rs> f1;
  flow<mig_rw, mig_rs, mig_rw, mig_rs, mig_rw, mig_rs> f2;
  flow<mig_rw, mig_rw, mig_rw, mig_rs, mig_rs, mig_rs> f3;
  flow<mig_alg_rw, mig_rw, mig_rw, mig_rw, mig_rs, mig_rs, mig_rs> f4;

  /* flow with flow modifiers and evaluators */
  flow<size_eval,repeat<combine<combine<mig_rw,mig_rs>,size_eval>,3u>> f5;

  /* add benchmarks */
  for ( auto const& benchmark : experiments::epfl_benchmarks( experiments::adder | experiments::bar | experiments::cavlc | experiments::dec | experiments::priority ) )
  {
    fmt::print( "[i] processing {}\n", benchmark );

    mockturtle::mig_network mig;
    network_size_and_depth_evaluator eval;

    if ( lorina::read_aiger( experiments::benchmark_path( benchmark ), mockturtle::aiger_reader( mig ) ) == lorina::return_code::success )
    {
      ::benchmark<mockturtle::mig_network> b( mig, benchmark );

      {
        eval.reset();
        auto const new_ntk = f0.run( b, eval );
        eval.report( "(RW)^3" );
        auto const cec = experiments::abc_cec( new_ntk, benchmark );
        std::cout << "[i] cec = " << cec << std::endl;
      }

      {
        eval.reset();
        auto const new_ntk = f1.run( b, eval );
        eval.report( "(RS)^3" );
        auto const cec = experiments::abc_cec( new_ntk, benchmark );
        std::cout << "[i] cec = " << cec << std::endl;
      }

      {
        eval.reset();
        auto const new_ntk = f2.run( b, eval );
        eval.report( "(RW;RS)^3" );
        auto const cec = experiments::abc_cec( new_ntk, benchmark );
        std::cout << "[i] cec = " << cec << std::endl;
      }

      {
        eval.reset();
        auto const new_ntk = f3.run( b, eval );
        eval.report( "RW^3;RS^3" );
        auto const cec = experiments::abc_cec( new_ntk, benchmark );
        std::cout << "[i] cec = " << cec << std::endl;
      }

      {
        eval.reset();
        auto const new_ntk = f4.run( b, eval );
        eval.report( "ARW; (RW;RS)^3" );
        auto const cec = experiments::abc_cec( new_ntk, benchmark );
        std::cout << "[i] cec = " << cec << std::endl;
      }

      {
        eval.reset();
        auto const new_ntk = f5.run( b, eval );
        eval.report( "REPEAT[COMBINE[RW,RS],3]" );
        auto const cec = experiments::abc_cec( new_ntk, benchmark );
        std::cout << "[i] cec = " << cec << std::endl;
      }
    }
    else
    {
      fmt::print( "[i] parsing benchmark failed\n" );
    }
  }

#if 0
  /* does not compile */
  flow<mig_alg_rw, mig_rw, mig_rw, mig_rw, mig_rs, mig_rs, mig_rs> f2;

  /* add benchmarks */
  for ( auto const& benchmark : experiments::epfl_benchmarks( experiments::adder | experiments::bar | experiments::cavlc | experiments::dec | experiments::priority ) )
  {
    fmt::print( "[i] processing {}\n", benchmark );

    mockturtle::mig_network mig;
    mockturtle::depth_view<mockturtle::mig_network> depth_mig{mig};
    network_size_and_depth_evaluator eval;

    if ( lorina::read_aiger( experiments::benchmark_path( benchmark ), mockturtle::aiger_reader( mig ) ) == lorina::return_code::success )
    {
      ::benchmark<mockturtle::depth_view<mockturtle::mig_network>> b( depth_mig, benchmark );

      eval.reset();
      f2.run( b, eval );
      eval.report( "NEW FLOW" );
    }
    else
    {
      fmt::print( "[i] parsing benchmark failed\n" );
    }
  }
#endif

  return 0;
}
