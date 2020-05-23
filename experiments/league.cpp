#include "experiments.hpp"

#include <mockturtle/networks/mig.hpp>
#include <mockturtle/algorithms/cut_rewriting.hpp>
#include <mockturtle/algorithms/mig_algebraic_rewriting.hpp>
#include <mockturtle/algorithms/node_resynthesis/mig_npn.hpp>
#include <mockturtle/io/aiger_reader.hpp>
#include <mockturtle/views/depth_view.hpp>
#include <lorina/lorina.hpp>

#include <fmt/format.h>
#include <vector>

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

  Ntk network() const
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
  int64_t score{0};
};

template<class Ntk>
class agent
{
public:
  virtual ~agent() {}

  virtual std::string name() const = 0;
  virtual void run( Ntk ntk ) = 0;
  virtual void report() const = 0;
};

class dfs_depth_rewriting : public agent<mockturtle::mig_network>
{
public:
  virtual std::string name() const
  {
    return std::string{"mig::depth_rewriting[dfs]"};
  }
  
  virtual void run( mockturtle::mig_network ntk )
  {
    mockturtle::depth_view<mockturtle::mig_network> depth_ntk( ntk );
    
    mockturtle::stopwatch t( st.time_total );
    uint64_t const before = depth_ntk.size();

    mockturtle::mig_algebraic_depth_rewriting_params ps;
    ps.strategy = mockturtle::mig_algebraic_depth_rewriting_params::dfs;
    ps.allow_area_increase = false;
    mockturtle::mig_algebraic_depth_rewriting( depth_ntk, ps );
    
    st.score += ( before - depth_ntk.size() );
  }

  virtual void report() const
  {
    fmt::print( "[i] {} score: {:8d} time: {:>8.2f}\n",
                name(), st.score, mockturtle::to_seconds( st.time_total ) );
  }
  
protected:
  agent_statistics st;
};

class aggressive_depth_rewriting : public agent<mockturtle::mig_network>
{
public:
  virtual std::string name() const
  {
    return std::string{"mig::depth_rewriting[dfs]"};
  }
  
  virtual void run( mockturtle::mig_network ntk )
  {
    mockturtle::depth_view<mockturtle::mig_network> depth_ntk( ntk );
    
    mockturtle::stopwatch t( st.time_total );
    uint64_t const before = depth_ntk.size();

    mockturtle::mig_algebraic_depth_rewriting_params ps;
    ps.strategy = mockturtle::mig_algebraic_depth_rewriting_params::aggressive;
    ps.allow_area_increase = false;
    mockturtle::mig_algebraic_depth_rewriting( depth_ntk, ps );
    
    st.score += ( before - depth_ntk.size() );
  }

  virtual void report() const
  {
    fmt::print( "[i] {} score: {:8d} time: {:>8.2f}\n",
                name(), st.score, mockturtle::to_seconds( st.time_total ) );
  }
  
protected:
  agent_statistics st;
};

class selective_depth_rewriting : public agent<mockturtle::mig_network>
{
public:
  virtual std::string name() const
  {
    return std::string{"mig::depth_rewriting[dfs]"};
  }
  
  virtual void run( mockturtle::mig_network ntk )
  {
    mockturtle::depth_view<mockturtle::mig_network> depth_ntk( ntk );
    
    mockturtle::stopwatch t( st.time_total );
    uint64_t const before = depth_ntk.size();

    mockturtle::mig_algebraic_depth_rewriting_params ps;
    ps.strategy = mockturtle::mig_algebraic_depth_rewriting_params::selective;
    ps.allow_area_increase = false;
    mockturtle::mig_algebraic_depth_rewriting( depth_ntk, ps );
    
    st.score += ( before - depth_ntk.size() );
  }

  virtual void report() const
  {
    fmt::print( "[i] {} score: {:8d} time: {:>8.2f}\n",
                name(), st.score, mockturtle::to_seconds( st.time_total ) );
  }
  
protected:
  agent_statistics st;
};

class migarw : public agent<mockturtle::mig_network>
{
public:
  virtual std::string name() const
  {
    return std::string{"migarw"};
  }
  
  virtual void run( mockturtle::mig_network ntk )
  {
    mockturtle::depth_view<mockturtle::mig_network> depth_ntk( ntk );
    
    mockturtle::stopwatch t( st.time_total );
    uint64_t const before = depth_ntk.size();

    // mockturtle::mig_algebraic_depth_rewriting_params arw_ps;
    // arw_ps.strategy = mockturtle::mig_algebraic_depth_rewriting_params::aggressive;

    // mockturtle::mig_algebraic_depth_rewriting_stats arw_st;
    mockturtle::mig_algebraic_depth_rewriting( depth_ntk );
    
    st.score += ( before - depth_ntk.size() );
  }

  virtual void report() const
  {
    fmt::print( "[i] {} score: {:8d} time: {:>8.2f}\n",
                name(), st.score, mockturtle::to_seconds( st.time_total ) );
  }
  
protected:
  agent_statistics st;
};

class migrw : public agent<mockturtle::mig_network>
{
public:
  virtual std::string name() const
  {
    return std::string{"migrw"};
  }
  
  virtual void run( mockturtle::mig_network ntk )
  {
    mockturtle::stopwatch t( st.time_total );

    uint64_t const before = ntk.num_gates();
    mockturtle::mig_npn_resynthesis resyn;
    mockturtle::cut_rewriting_params rw_ps;
    rw_ps.cut_enumeration_ps.cut_size = 4;
    cut_rewriting_with_compatibility_graph( ntk, resyn, rw_ps );
    ntk = cleanup_dangling( ntk );
    st.score = before - ntk.num_gates();
  }

  virtual void report() const
  {
    fmt::print( "[i] {} score: {:8d} time: {:>8.2f}\n",
                name(), st.score, mockturtle::to_seconds( st.time_total ) );
  }
  
protected:
  agent_statistics st;
};

class migrw2 : public agent<mockturtle::mig_network>
{
public:
  virtual std::string name() const
  {
    return std::string{"migrw2"};
  }
  
  virtual void run( mockturtle::mig_network ntk )
  {
    mockturtle::stopwatch t( st.time_total );

    uint64_t const before = ntk.num_gates();
    mockturtle::mig_npn_resynthesis resyn;
    mockturtle::cut_rewriting_params rw_ps;
    rw_ps.cut_enumeration_ps.cut_size = 4;
    cut_rewriting( ntk, resyn, rw_ps );
    ntk = cleanup_dangling( ntk );
    st.score = before - ntk.num_gates();
  }

  virtual void report() const
  {
    fmt::print( "[i] {} score: {:8d} time: {:>8.2f}\n",
                name(), st.score, mockturtle::to_seconds( st.time_total ) );
  }
  
protected:
  agent_statistics st;
};

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

int main()
{
  league l;

  /* add agents */
  l.add_agent<dfs_depth_rewriting>();
  l.add_agent<aggressive_depth_rewriting>();
  l.add_agent<selective_depth_rewriting>();
  // l.add_agent<migrw>();
  // l.add_agent<migrw2>();
  
  /* add benchmarks */
  for ( auto const& benchmark : experiments::epfl_benchmarks( ~experiments::hyp ) )
  {
    mockturtle::mig_network mig;
    if ( lorina::read_aiger( experiments::benchmark_path( benchmark ), mockturtle::aiger_reader( mig ) ) == lorina::return_code::success )
    {
      l.add_benchmark( mig, benchmark );
    }
    else
    {
      fmt::print( "[i] parsing benchmark failed\n" );
    }
  }
  
  /* evaluate agents on benchmarks */
  l.run();
  
  return 0;
}
