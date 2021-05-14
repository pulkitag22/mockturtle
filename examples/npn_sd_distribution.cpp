#include <percy/percy.hpp>
#include <cassert>
#include <cstdio>
#include <fstream>

using namespace percy;
using kitty::dynamic_truth_table;

int main(void)
{
    {
        chain c;
        spec spec;
        spec.set_primitive(AIG);

        dynamic_truth_table tt1(3);
        dynamic_truth_table tt2(3);
		tt1 = 0011110011000011;
		tt2 = 0000000011111111;

        spec[0] = tt1;
		spec[1] = tt2;
        auto result = synthesize(spec, c);
        assert(result == success);
        assert(c.get_nr_steps() == 1);
        assert(c.simulate()[0] == spec[0]);
        assert(c.simulate()[1] == spec[1]);
        assert(c.is_aig());
        
    }

    {
        chain c;
        spec spec;
        spec.set_primitive(AIG);
        kitty::dynamic_truth_table tt(3);
        for (int i = 0; i < 256; i++) {
            kitty::create_from_words(tt, &i, &i + 1);
            spec[0] = tt;
            const auto result = synthesize(spec, c);
            assert(result == success);
            assert(c.is_aig());
            assert(c.simulate()[0] == tt);
        }
    }

    return 0;
}
