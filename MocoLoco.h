#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <string>
#include <cmath>
#include <iterator>
#include <list>
#include <gsl/gsl_sf.h>
#include <gsl/gsl_cdf.h>
#include <gsl/gsl_sf_bessel.h>
#include <vector>
#include <algorithm>
#include <map>
#include <typeinfo> 
#include <sstream>
#include <numeric>
#include "./TwoBit/twobit.h"
#include "./TwoBit/twobit.c"
#include <getopt.h>
#include <unordered_map>

using namespace std;

string BED_FILE; 		//initializing const char variarible for Bed_file input reading
int half_length = 150; 		//default half_length 150
string TWOBIT_FILE;	//initializing const char variable for Twobit_file input reading
string JASPAR_FILE;
string alias_file = "multifasta_";
string MFASTA_FILE;
string ordering;
const unsigned int overhead = 25;
const double pseudoc = 0.01;
bool DS = 1;
string kmers = "6,8,10";
string dist = "1,2,3";
unsigned int top_N = 10;


class bed_class { //creation public class of bed_class type        

	private:	//field definition

		string chr_coord;
		unsigned int start_coord;
		unsigned int end_coord;
		bool flag;
		string sequence;

		void read_line(string);
		void flag_control(unsigned int, unsigned int);

	public:				//field definition
		
		bed_class(string seq){

			chr_coord = "MULTIFASTA";
			start_coord = 0;
			end_coord = 0;
			sequence = seq;
		}

		bed_class(unsigned int p, string line, TwoBit* tb,unsigned int n_line){

			read_line(line);					//reading bed line
			flag_control(start_coord,end_coord);			//controlling coordinates
			centering_function(start_coord, end_coord, p, overhead);		//centering the coordinates
			extract_seq(tb, n_line);				//extracting the sequence

		}
		string return_sequence(bed_class);
		string return_chr_coord_GEP();
		unsigned int return_start_coord_GEP();
		void centering_function(unsigned int, unsigned int, unsigned int, const unsigned int);
		void extract_seq(TwoBit*, unsigned int);
		string return_chr_coord();
		unsigned int return_start_coord();
		unsigned int return_end_coord();

};

class matrix_class {

	private: //field definition

		string matrix_name;
		string tf_name;
		vector<vector<double>> matrix;
		vector<vector<double>> norm_matrix;
		vector<vector<double>> inverse_norm_matrix;
		vector<vector<double>> matrix_log;
		vector<vector<double>> inverse_matrix_log;
		vector<double> col_sum;		


		void matrix_normalization_pseudoc(vector<vector<double>>, double);
		void matrix_normalization(vector<vector<double>>);
		void matrix_logarithmic(vector<vector<double>>);
		void read_JASPAR(string);
		vector<vector<double>> reverse_matrix(vector<vector<double>>);
		vector<double> find_col_sum(vector<vector<double>>);
		void print_debug_matrix(vector<vector<double>>, string);



	public:
		matrix_class(string JASPAR_FILE){

			read_JASPAR(JASPAR_FILE);
			matrix_normalization_pseudoc(matrix, pseudoc);			//Calling matrix normalization function
			matrix_normalization(norm_matrix);
			matrix_logarithmic(norm_matrix);
			inverse_norm_matrix = reverse_matrix(norm_matrix);
			inverse_matrix_log = reverse_matrix(matrix_log);

		}

		void debug_matrix(matrix_class);
		vector<vector<double>> return_log_matrix();
		vector<vector<double>> return_inverse_log_matrix();

};


class oligo_class{

	private:


		vector<double> oligo_scores;	
		vector<double> o_matrix_mins;	
		vector<double> o_matrix_maxes;
		double min_possible_score;
		double max_possible_score;
		double best_score;
		double best_score_normalized;
		string global_sequence;
		string best_oligo_seq;
		unsigned int local_position;
		string chr_coord_oligo;
		unsigned int start_coord_oligo;
		unsigned int end_coord_oligo;
		char strand;

		void find_minmax(vector<vector<double>>);
		unsigned int find_best_score(vector<double>);
		void find_coordinate(unsigned int, unsigned int, string, unsigned int);
		void find_best_sequence(string, unsigned int, unsigned int);
		void best_score_normalization();

	public:
		oligo_class(vector<vector<double>> matrix, string sequence, string chr_coord_GEP, unsigned int start_coord_GEP, char strand_sign){

			global_sequence = sequence;
			strand = strand_sign;
			find_minmax(matrix);		
			shifting(matrix, sequence, 0);
			local_position = find_best_score(oligo_scores);
			best_score_normalization();
			find_best_sequence(sequence, local_position, matrix[0].size());
			find_coordinate(local_position, matrix[0].size(), chr_coord_GEP, start_coord_GEP);
		}

		void shifting(vector<vector<double>>, string, unsigned int);
		void oligos_vector_debug(oligo_class);
		unsigned int return_start_coord_oligo();
		double return_best_score_normalized();
};

class coordinator_class{ 					//Coordinator class to connect Matrix to Bed and Oligos_vector

	private:

		vector<vector<double>> matrix_log;
		vector<vector<double>> inverse_matrix_log;
		void centering_oligo();
		void best_strand(vector<oligo_class>);


	public:
		vector<oligo_class> oligos_vector;
		vector<bed_class> GEP; 
		void GEP_creation(string, string, vector<bed_class>&);
		void oligos_vector_creation(vector<oligo_class>&, vector<vector<double>>, vector<vector<double>>, vector<bed_class>);
		void print_debug_GEP(vector <bed_class>);

		coordinator_class(){
			GEP_creation(BED_FILE, TWOBIT_FILE, GEP);
			matrix_class M(JASPAR_FILE);
			matrix_log = M.return_log_matrix();
			inverse_matrix_log = M.return_inverse_log_matrix();
			oligos_vector_creation(oligos_vector, matrix_log, inverse_matrix_log, GEP);
			best_strand(oligos_vector);
			centering_oligo();
		}

};

class multifasta_class{

	private:

		vector<string> sequences;

		void length_control(vector<string>);
		void extract_sequences(string);
		void GEP_creation_MF(vector<string>);

	public:

		vector<bed_class> GEP;

		multifasta_class(string MFASTA_FILE){

			extract_sequences(MFASTA_FILE);
			length_control(sequences);
			GEP_creation_MF(sequences);
		}

};

class p_value_class{

	private:

		vector<unsigned int> K_vec;
		vector<unsigned int> N1_vec;
		vector<unsigned int> N2_vec;
		unsigned int T;
		vector<double> p_value_vec;
		string reverse_bases;
		string oligo;
		string oligo_RC;
		multimap<pair<unsigned int, unsigned int>,pair<string,string>> vertical_multimap;
		unordered_map<string,unsigned int>::iterator it_N1_plus;
		unordered_map<string,unsigned int>::iterator it_N1_minus;
		unsigned int total_oligo_N2;
		unsigned int position;
		unsigned int rank;
		unsigned int sum_top_N;
		double p_val;
		double p_val_log10;
		multimap<double,pair<string,string>> p_value_sort;
		multimap<double,pair<string,vector<unsigned int>>> p_value_KNT;

		multimap<pair<unsigned int,unsigned int>, pair<string,string>>  multimap_creation(map<pair<string,string>,pair<unsigned int,unsigned int>>);
		void filling_KNT_vectors(unordered_map<string,unsigned int>);
		void N2_calculation(unordered_map<string,unsigned int>);
		bool check_palindrome2(string);
		void calculating_p_value();
		double check_p_value(double);
		void sorting_p_value();
		void checking_ordering(map<pair<string,string>,pair<unsigned int,unsigned int>>, unsigned int, ofstream&, unsigned int);
		void print_debug_p_value_DS(map<pair<string,string>,pair<unsigned int, unsigned int>>, unsigned int, ofstream&, unsigned int);
		void print_debug_p_value_SS(map<pair<string,string>,pair<unsigned int, unsigned int>>, unsigned int, ofstream&, unsigned int);
		void print_debug_occurrences_DS(map<pair<string,string>,pair<unsigned int, unsigned int>>, unsigned int, ofstream&, unsigned int, vector<double>);
		void print_debug_occurrences_SS(map<pair<string,string>,pair<unsigned int, unsigned int>>, unsigned int, ofstream&, unsigned int, vector<double>);

	public:

		p_value_class(map<pair<string,string>,pair<unsigned int,unsigned int>> pair_map, unordered_map<string,unsigned int> orizzontal_map, unsigned int t, unsigned int position, ofstream &outfile, unsigned int freq){
		
			T = t;
			vertical_multimap = multimap_creation(pair_map);
			N2_calculation(orizzontal_map);
			filling_KNT_vectors(orizzontal_map);
			calculating_p_value();
			sorting_p_value();
			checking_ordering(pair_map, position, outfile, freq);		
		}

		multimap<double,pair<string,vector<unsigned int>>> return_p_value_KNT();
		multimap<pair<unsigned int,unsigned int>,pair<string,string>> return_vertical_multimap();
		unsigned int return_sum_top_N();
		vector<unsigned int> return_K_vec();
		vector<unsigned int> return_N1_vec();
		vector<unsigned int> return_N2_vec();
		unsigned int return_T();
		vector<double> return_p_value_vec();
};

class hamming_class{
	
	private:


		multimap<pair<unsigned int,unsigned int>, pair<string,string>> vertical_multimap;
		vector<string> best_oligos;
		string real_best_oligo;
		unsigned int real_best_oligo_occurrences;
		vector<string> similar_oligos;
		vector<unsigned int> similar_oligos_occurrences;
		double tot_similar_occurrences;
		double FREQUENCE_1;
		double FREQUENCE_2;
		vector<vector<unsigned int>> PWM_hamming;

		void find_best_oligos();
		void checking_best_oligo(unsigned int);
		void find_distanced_oligos(string, unsigned int);
		string select_real_best_oligo(unsigned int);
		bool is_similar_oligo(string, string, unsigned int);
		void print_debug_hamming(unsigned int, ofstream&);
		double frquence_1_calculation(unsigned int);
		double frquence_2_calculation(unordered_map<string,unsigned int>, unordered_map<string,unsigned int>);
		unsigned int finding_orizzontal_occurrences(unordered_map<string,unsigned int>, unordered_map<string,unsigned int>);
		void PWM_hamming_creation();

	public:

		hamming_class(multimap<pair<unsigned int,unsigned int>, pair<string,string>> v_multimap, unsigned int distance, unsigned int position, unsigned int freq, unordered_map<string,unsigned int> orizzontal_map_plus, unordered_map<string,unsigned int> orizzontal_map_minus, ofstream& outfile){

			vertical_multimap = v_multimap;
			find_best_oligos();
			checking_best_oligo(distance);
			similar_oligos.emplace_back(real_best_oligo);
			similar_oligos_occurrences.emplace_back(real_best_oligo_occurrences);
			FREQUENCE_1 = frquence_1_calculation(freq);
			FREQUENCE_2 = frquence_2_calculation(orizzontal_map_plus, orizzontal_map_minus); 
			print_debug_hamming(position, outfile);
			PWM_hamming_creation();
		}

		string return_real_best_oligo();
		unsigned int return_similar_oligo_size();
		vector<vector<unsigned int>> return_PWM_hamming();

};

class map_class{

	private:
		
		vector<vector<map<pair<string,string>,pair<unsigned int, unsigned int>>>> vector_kmers_maps_plus;
		vector<vector<map<pair<string,string>,pair<unsigned int, unsigned int>>>> vector_kmers_maps_minus;
		vector<unordered_map<string,unsigned int>> orizzontal_plus_debug;
		vector<unordered_map<string,unsigned int>> orizzontal_minus_debug;
		vector<map<pair<string,string>,pair<unsigned int, unsigned int>>> maps_vector_positions_plus;
		vector<map<pair<string,string>,pair<unsigned int, unsigned int>>> maps_vector_positions_minus;
		unordered_map<string, unsigned int> orizzontal_plus;
		unordered_map<string, unsigned int> orizzontal_minus;
		map<pair<string,string>,pair<unsigned int, unsigned int>>  vertical_plus;
		map<pair<string,string>,pair<unsigned int, unsigned int>>  vertical_minus;
		string reverse_bases;
		vector<vector<unsigned int>> tot_freq_matrix;
		vector<unsigned int> tot_sum_vector;
		vector<vector<unsigned int>> tot_sum_matrix;
		vector<unsigned int> kmers_vector;
		vector<unsigned int> distance_vector;
		unsigned int sequences_number_T;
		vector<p_value_class> P_VALUE_VECTOR;
		vector<vector<p_value_class>> P_VALUE_MATRIX;
		vector<hamming_class> HUMMING_VECTOR;
		vector<vector<hamming_class>> HUMMING_MATRIX;

		vector<unsigned int> generic_vector_creation(string);
		void table_creation_orizzontal(vector<bed_class>);
		void table_creation_vertical(vector<bed_class>);
		void or_ver_kmer_count(string,unordered_map<string,unsigned int>&, unordered_map<string,unsigned int>&);
		void vertical_kmer_count(string,map<pair<string,string>,pair<unsigned int, unsigned int>>&,map<pair<string,string>,pair<unsigned int, unsigned int>>&, unsigned int &);
		void select_best(map<pair<string,string>,pair<unsigned int,unsigned int>>&);
		void print_debug_orizzontal();
		bool check_palindrome(string);
		void P_VALUE_MATRIX_creation();
		void HUMMING_MATRIX_creation();
		ofstream outfile_header(unsigned int);
		ofstream outfile_header_hamming(unsigned int);
		void TopN_sum_and_freq();
		void p_value_parameters_debug_p_val();
		void p_value_parameters_debug_occ();
		double check_p_value(double);
		void check_kmer_dist();
		void Outfile_PWM_hamming();
		void print_debug_PWM_hamming(ofstream&, unsigned int);

	public:

		map_class(vector<bed_class> GEP, string kmers, string dist){

			kmers_vector = generic_vector_creation(kmers);
			distance_vector = generic_vector_creation(dist);
			check_kmer_dist();			
			table_creation_orizzontal(GEP);
			table_creation_vertical(GEP);
			sequences_number_T = GEP.size();
			print_debug_orizzontal();
			P_VALUE_MATRIX_creation();
			if(ordering == "p"){
			p_value_parameters_debug_p_val();
			}
			else{
			p_value_parameters_debug_occ();
			}
			TopN_sum_and_freq();
			HUMMING_MATRIX_creation();
			Outfile_PWM_hamming();
		}
};


void GEP_path();
void command_line_parser(int, char **);
void display_help();
bool is_file_exist(string fileName, string buf);
void check_input_file();
