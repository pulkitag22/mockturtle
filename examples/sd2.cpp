#include <iostream>
#include <unordered_set>

#include <kitty/kitty.hpp>

std::unordered_set<kitty::dynamic_truth_table, kitty::hash<kitty::dynamic_truth_table>> calculate_npn_class(uint8_t num_vars);

template<typename TT, typename = std::enable_if_t<kitty::is_complete_truth_table<TT>::value>>
kitty::dynamic_truth_table dual_of( const TT& tt ){
				auto numvars = tt.num_vars();
				auto tt1 = tt;
				auto tt2 = ~tt1;
				for ( auto i = 0u; i < numvars; i++ ){
								tt1 = flip( tt1, i );
				}

				return ~tt1;
}

template<typename TT, typename = std::enable_if_t<kitty::is_complete_truth_table<TT>::value>>
kitty::dynamic_truth_table extend_tt (const TT& tt, int num_vars){
				kitty::dynamic_truth_table extended_tt(num_vars+1);
				for (int i=0; i<(int)extended_tt.num_bits(); i++){
								if (kitty::get_bit(tt, i%(tt.num_bits())) == 1){
												kitty::set_bit(extended_tt, i);
								}
				}
				return extended_tt;
}

std::unordered_set<kitty::dynamic_truth_table, kitty::hash<kitty::dynamic_truth_table>> calculate_sd_class_fast(uint8_t num_vars){

				auto npn_class = calculate_npn_class(num_vars);
				std::unordered_set<kitty::dynamic_truth_table, kitty::hash<kitty::dynamic_truth_table>> :: iterator itr;
				std::unordered_set<kitty::dynamic_truth_table, kitty::hash<kitty::dynamic_truth_table>> classes;
				kitty::dynamic_truth_table new_tt(num_vars+1), a(num_vars+1), extended_tt(num_vars+1);
				create_nth_var( a, num_vars);
				itr = npn_class.begin();

				do {
								//res = kitty::is_selfdual(*itr);
								//print_binary(*itr);
								//std::cout<<res<<"\n";

								//if (res){
								//classes.insert(*itr);
								//} else if (!res){
								extended_tt = extend_tt(*itr, num_vars);
								new_tt = kitty::binary_or(kitty::binary_and(extended_tt, a), kitty::binary_and(~a, dual_of(extended_tt)));
								const auto entry = kitty::exact_npn_canonization(new_tt);
								classes.insert(std::get<0>(entry));
								//	}
								itr++;
				} while (itr != npn_class.end());
				std::cout << "[i] enumerated "
								<< ( 1 << ( 1 << num_vars ) ) << " functions into "
								<< classes.size() << " classes." << std::endl;
		/*		std::unordered_set<kitty::dynamic_truth_table, kitty::hash<kitty::dynamic_truth_table>> :: iterator itr_print;
				for (itr_print=classes.begin(); itr_print != classes.end(); itr_print++){
								auto class_tt = (*itr_print);
								print_binary(class_tt);
								std::cout<<"\n";
				}*/
				return classes;
}

std::unordered_set<kitty::dynamic_truth_table, kitty::hash<kitty::dynamic_truth_table>> calculate_sd_class(uint8_t num_vars){
				kitty::dynamic_truth_table tt(num_vars);
				std::unordered_set<kitty::dynamic_truth_table, kitty::hash<kitty::dynamic_truth_table>> classes;
				kitty::dynamic_truth_table new_tt(num_vars+1), a(num_vars+1), extended_tt(num_vars+1);
				create_nth_var( a, num_vars);
				bool res;
				do {
								res = kitty::is_selfdual(tt);
								if (res){
												const auto entry = kitty::exact_npn_canonization(tt);
												classes.insert(std::get<0>(entry));
								} else if (!res){
												extended_tt = extend_tt(tt, num_vars);
												new_tt = kitty::binary_or(kitty::binary_and(extended_tt, a), kitty::binary_and(~a, dual_of(extended_tt)));
												//if(kitty::is_selfdual(new_tt)){
												//std::cout<<"almost there."<<"\n";
												//}
												const auto entry = kitty::exact_npn_canonization(new_tt);
												classes.insert(std::get<0>(entry));
												//print_binary(std::get<0>(entry));
												//std::cout<<"\n";
								}
								kitty::next_inplace( tt );
				} while (!kitty::is_const0( tt ));
				std::cout << "[i] enumerated "
								<< ( 1 << ( 1 << tt.num_vars() ) ) << " functions into "
								<< classes.size() << " classes." << std::endl;

				/*	std::unordered_set<kitty::dynamic_truth_table, kitty::hash<kitty::dynamic_truth_table>> :: iterator itr_print;
						for (itr_print=classes.begin(); itr_print != classes.end(); itr_print++){
						auto class_tt = (*itr_print);
						print_binary(class_tt);
						std::cout<<"\n";
						}*/
				return classes;
}

int main () {
				const auto classes_3u_sd = calculate_sd_class_fast(4u);
				//const auto classes_3u_npn = calculate_npn_class(4u);
				/*kitty::dynamic_truth_table a(4u), b(4u), c(4u), d(4u);
				create_nth_var( a, 3u);
				create_nth_var( b, 2u);
				create_nth_var( c, 1u);
				create_nth_var( d, 0u);
				auto f1 = kitty::binary_or(kitty::binary_and(d, kitty::binary_or(kitty::binary_or(c, b), a)), kitty::binary_and(a, kitty::binary_and(b, kitty::binary_and(c, ~d))));
				print_binary(f1);*/
				return 0;
}

std::unordered_set<kitty::dynamic_truth_table, kitty::hash<kitty::dynamic_truth_table>> calculate_npn_class(uint8_t num_vars){
				/* compute NPN classe */
				std::unordered_set<kitty::dynamic_truth_table, kitty::hash<kitty::dynamic_truth_table>> classes;
				kitty::dynamic_truth_table tt( num_vars );
				do{
								/* apply NPN canonization and add resulting representative to set */
								const auto entry = kitty::exact_npn_canonization(tt);
								classes.insert(std::get<0>(entry));

								/* increment truth table */
								kitty::next_inplace( tt );
				} while ( !kitty::is_const0( tt ) );

				/*	std::cout << "[i] enumerated "
						<< ( 1 << ( 1 << tt.num_vars() ) ) << " functions into "
						<< classes.size() << " classes." << std::endl;*/
				return classes;
}
