/* kitty: C++ truth table library
	* Copyright (C) 2017-2020  EPFL
	*
	* Permission is hereby granted, free of charge, to any person
	* obtaining a copy of this software and associated documentation
	* files (the "Software"), to deal in the Software without
	* restriction, including without limitation the rights to use,
	* copy, modify, merge, publish, distribute, sublicense, and/or sell
	* copies of the Software, and to permit persons to whom the
	* Software is furnished to do so, subject to the following
	* conditions:
	*
	* The above copyright notice and this permission notice shall be
	* included in all copies or substantial portions of the Software.
	*
	* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
	* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
	* OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
	* NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
	* HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
	* WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
	* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
	* OTHER DEALINGS IN THE SOFTWARE.
	*/

#include <cstdint>
#include <iostream>
#include <unordered_set>

#include <kitty/kitty.hpp>

/* compile time constant for the number of variables */

int main()
{
								kitty::dynamic_truth_table a( 4 ), b( 4 ), c( 4 ), d( 4 );

								kitty::create_nth_var( a, 0 );
								kitty::create_nth_var( b, 1 );
								kitty::create_nth_var( c, 2 );
								kitty::create_nth_var( d, 3 );

								const auto sum = kitty::binary_and (a ^ b, c ^ d);
								const auto carry = kitty::ternary_majority( a, b, c );
									print_xmas_tree_for_function(carry);		
									std::cout<<"\n";
								auto output_tt = kitty::exact_npn_canonization(carry);
								print_xmas_tree_for_function(carry);
									std::cout<<"\n";
								print_xmas_tree_for_function(sum);		
									std::cout<<"\n";
								auto output_ttt = kitty::exact_npn_canonization(sum);
								print_xmas_tree_for_function(sum);
									std::cout<<"\n";


								return 0;
}
