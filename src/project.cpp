/*
Comp Eng 3DY4 (Computer Systems Integration Project)

Copyright by Nicola Nicolici
Department of Electrical and Computer Engineering
McMaster University
Ontario, Canada
*/

#include "dy4.h"
#include "filter.h"
#include "fourier.h"
#include "genfunc.h"
#include "iofunc.h"
#include "logfunc.h"
#include <cmath>
#include <algorithm>
void rfFrontEnd(std::vector<float> &block_data, float RFFS, float IFFS,int BLOCK_SIZE,const int rf_decim,unsigned int block_id,	std::vector<float> &i_state,	std::vector<float> &q_state,	std::vector<float> &state,	std::vector<float> &prev_state){

	float Fc = 100000;	//RF cutoff 100Khz
	unsigned short int num_taps = 101;

	std::vector<float> h;
	impulseResponseLPF(RFFS, Fc, num_taps, h);

	int mono0Decim = 5;

	std::vector<float> I_block;
	I_block.clear();I_block.resize(BLOCK_SIZE/2,0.0);
	std::vector<float> Q_block;
	Q_block.clear();Q_block.resize(BLOCK_SIZE/2,0.0);

	// Splits the data into I and Q samples
	for(int k = 0;k<BLOCK_SIZE/2;k++){
		I_block[k] = block_data[k*2];
		Q_block[k] = block_data[1+k*2];
	}

	std::vector<float> I;
	I.clear();I.resize(I_block.size()/rf_decim,0.0);

	std::vector<float> Q;
	Q.clear();Q.resize(Q_block.size()/rf_decim,0.0);

	// Convolution and IQ decimation
	convolveFIRinBlocks(I, I_block, h, i_state, I_block.size(), rf_decim);

	convolveFIRinBlocks(Q, Q_block, h, q_state, Q_block.size(), rf_decim);

	std::vector<float> fm_demod;
	fm_demod.clear();fm_demod.resize(I.size(),0.0);
	// Demod function
	demod(fm_demod,I,Q,prev_state);


	std::vector<float> h2;
	impulseResponseLPF(IFFS, 16000, num_taps, h2);

	std::vector<float> audio;
	audio.clear();audio.resize(fm_demod.size()/mono0Decim,0.0);

	convolveFIRinBlocks(audio,fm_demod,h2,state,fm_demod.size(),mono0Decim);


	std::vector<short int> mono_data;
	mono_data.clear();mono_data.resize(audio.size());

	for(unsigned int k = 0;k<audio.size();k++){
		if(std::isnan(audio[k])) mono_data[k] = 0;
		else mono_data[k] = static_cast<short int>(audio[k]*16384);
	}
	fwrite(&mono_data[0],sizeof(short int),mono_data.size(),stdout);

}

void mono(std::vector<float> FMDemodData, float RFFS, float IFFS, int BLOCK_SIZE){

		// std::vector<float> h2;
		// impulseResponseLPF(IFFS, 16000, num_taps, h2);
		//
		// std::vector<float> audio;
		// audio.clear();audio.resize(fm_demod.size()/mono0Decim,0.0);
		//
		// convolveFIRinBlocks(audio,fm_demod,h2,state,fm_demod.size(),mono0Decim);
		//
		//
		// std::vector<short int> mono_data;
		// mono_data.clear();mono_data.resize(audio.size());
		//
		// for(unsigned int k = 0;k<audio.size();k++){
		// 	if(std::isnan(audio[k])) mono_data[k] = 0;
		// 	else mono_data[k] = static_cast<short int>(audio[k]*16384);
		// }
		// fwrite(&mono_data[0],sizeof(short int),mono_data.size(),stdout);


}
void stereo(std::vector<float> FMDemodData, float RFFS, float IFFS, int BLOCK_SIZE){


}
void RDS(){

}

int main(int argc, char* argv[])
{
	int mode = 0;
	if(argc < 2){
		std::cerr << "Operating in default mode 0" << "\n";
	}else if(argc == 2){
		mode = atoi(argv[1]);
		if( mode > 3){
			std::cerr << "Wrong mode " << mode << "\n";
		}else{
			std::cout << "Operating in mode " << mode << "\n";
		}
	}else{
		std::cerr << " Mode should be a value from 0 to 3\n";
		std::cerr << " Exiting now\n";
		exit(1);
	}
	if(mode == 0){
		float RFFS = 2400000;
		float IFFS = 240000;
		float audioFS = 48000;
	}
	if(mode == 1){
		float RFFS = 1152000;
		float IFFS = 288000;
		float audioFS = 48000;
	}
	if(mode == 2){
		float RFFS = 2400000;
		float IFFS = 240000;
		float audioFS = 44100;
	}
	if(mode == 3){
		float RFFS = 1920000;
		float IFFS = 320000;
		float audioFS = 44100;
	}
	float RFFS = 2400000;
	float IFFS = 240000;
	float audioFS = 48000;
	int rf_decim = 10;
	int audio_decim = 5;
	int BLOCK_SIZE =  1024 * rf_decim * 2 * audio_decim;
	int num_taps = 101;
	std::vector<float> i_state;
	i_state.clear();i_state.resize(num_taps-1,0.0);
	std::vector<float> q_state;
	q_state.clear();q_state.resize(num_taps-1,0.0);
	std::vector<float> state;
	state.clear();state.resize(num_taps-1,0.0);
	std::vector<float> prev_state;
	prev_state.clear();prev_state.resize(2,0.0);
	// const std::string in_fname = "../data/iq_samples.raw";
	// std::vector<float> block_data;
	// for( unsigned int block_id = 0; ; block_id++){
	// 	std::vector<float> block_data(BLOCK_SIZE);
	// 	readStdinBlockData(BLOCK_SIZE, block_id, block_data);
	// 	if ((std::cin.rdstate()) != 0) {
	// 		std::cerr << "End of input stream reached" << "\n";
	// 		exit(1);
	// 	}
	// 	std::cerr << "Read block" << block_id << "\n";
	// }
	for( unsigned int block_id = 0; ; block_id++){
		std::vector<float> block_data(BLOCK_SIZE);
		readStdinBlockData(BLOCK_SIZE, block_id, block_data);
		if ((std::cin.rdstate()) != 0) {
			std::cerr << "End of input stream reached" << "\n";
			exit(1);
		}
		std::cerr << "Read block " << block_id << "\n";
		//std::vector<float> fm_demod;
		rfFrontEnd(block_data,RFFS,IFFS,BLOCK_SIZE,rf_decim,block_id,i_state,q_state,state,prev_state);

		//mono(block_data,RFFS,IFFS,BLOCK_SIZE);

		//stereo(block_data,RFFS,IFFS,BLOCK_SIZE);

		//RDS();

	}


	return 0;
}
