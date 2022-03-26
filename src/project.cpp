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
void rfFrontEnd(std::vector<float> &block_data, float RFFS, float IFFS,int BLOCK_SIZE,int rf_decim){
	//std::vector<float> block_data(BLOCK_SIZE);

	float Fc = 100000;	//RF cutoff 100Khz
	unsigned short int num_taps = 101;

	std::vector<float> h;
	impulseResponseLPF(RFFS, Fc, num_taps, h);

	//int numDecim = RFFS/IFFS;
	int mono0Decim = 5;

	std::vector<float> I_block;
	I_block.clear();I_block.resize(block_data.size()/2,0.0);
	std::vector<float> Q_block;
	Q_block.clear();Q_block.resize(block_data.size()/2,0.0);
	std::vector<float> i_state(num_taps-1);
	std::vector<float> q_state(num_taps-1);
	std::vector<float> state(num_taps-1);
	std::vector<float> prev_state(2);

	split_audio_into_channels(block_data, I_block, Q_block);

	//I_block = slice(I_data, block_count*BLOCK_SIZE, (block_count + 1)*BLOCK_SIZE);
	//Q_block = slice(Q_data, block_count*BLOCK_SIZE, (block_count + 1)*BLOCK_SIZE);

	//Might have to change to filter single block
	std::vector<float> I;
	I.clear();I.resize(I_block.size()/rf_decim,0.0);
	std::vector<float> Q;
	Q.clear();Q.resize(Q_block.size()/rf_decim,0.0);

	convolveFIRinBlocks(I, I_block, h, i_state, BLOCK_SIZE/2, rf_decim);

	convolveFIRinBlocks(Q, Q_block, h, q_state, BLOCK_SIZE/2, rf_decim);

	std::cerr << *std::max_element(I.begin(), I.begin()+BLOCK_SIZE/2) << std::endl;
	std::cerr << *std::max_element(Q.begin(), Q.begin()+BLOCK_SIZE/2) << std::endl;

	std::vector<float> fm_demod;
	fm_demod.clear();fm_demod.resize(I.size(),0.0);

	demod(fm_demod,I,Q,prev_state);

	std::cerr << *std::max_element(fm_demod.begin(), fm_demod.begin()+BLOCK_SIZE/2) << std::endl;

	//------------plotting fm_demod to verify-----------------------
	std::vector<float> vector_index;
	genIndexVector(vector_index, fm_demod.size());
	// log time data in the "../data/" subfolder in a file with the following name
	// note: .dat suffix will be added to the log file in the logVector function
	logVector("demod_time", vector_index, fm_demod);

	// take a slice of data with a limited number of samples for the Fourier transform
	// note: NFFT constant is actually just the number of points for the
	// Fourier transform - there is no FFT implementation ... yet
	// unless you wish to wait for a very long time, keep NFFT at 1024 or below
	std::vector<float> slice_data = \
		std::vector<float>(fm_demod.begin(), fm_demod.begin() + NFFT);
	// note: make sure that binary data vector is big enough to take the slice

	// declare a vector of complex values for DFT
  std::vector<std::complex<float>> Xf;
	// ... in-lab ...
	DFT(slice_data, Xf);
	// compute the Fourier transform
	// the function is already provided in fourier.cpp

	// compute the magnitude of each frequency bin
	// note: we are concerned only with the magnitude of the frequency bin
	// (there is NO logging of the phase response)
	std::vector<float> Xmag;
	// ... in-lab ...
	computeVectorMagnitude(Xf, Xmag);
	// compute the magnitude of each frequency bin
	// the function is already provided in fourier.cpp

	// log the frequency magnitude vector
	vector_index.clear();
	genIndexVector(vector_index, Xmag.size());
	logVector("demod_freq", vector_index, Xmag); // log only positive freq

	std::cerr << "Run: gnuplot -e 'set terminal png size 1024,768' ../data/example.gnuplot > ../data/example.png\n";
	//---------------------------------------------------------------------------

	std::vector<float> h2;
	impulseResponseLPF(IFFS, 16000, num_taps, h2);


	std::vector<float> audio;
	audio.clear();audio.resize(fm_demod.size()/mono0Decim,0.0);

	convolveFIRinBlocks(audio,fm_demod,h2,state,fm_demod.size(),mono0Decim);

	std::cerr << *std::max_element(audio.begin(), audio.end()) << std::endl;

	std::vector<float> mono_data;
	mono_data.clear();mono_data.resize(audio.size());

	for(unsigned int k = 0;k<audio.size();k++){
		if(std::isnan(audio[k])) mono_data[k] = 0;
		else mono_data[k] = static_cast<short int>(audio[k]*16384);
	}
	std::cerr << *std::max_element(mono_data.begin(), mono_data.begin()+audio.size()) << std::endl;
	std::cerr << audio.size() << std::endl;
	std::cerr << mono_data.size() << std::endl;

	fwrite(&mono_data[0],sizeof(short int),audio.size(),stdout);

}
void monoStereo(std::vector<float> FMDemodData, float RFFS, float IFFS, int BLOCK_SIZE){


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

		rfFrontEnd(block_data,RFFS,IFFS,BLOCK_SIZE,rf_decim);

		//mono(block_data,RFFS,IFFS,BLOCK_SIZE);

		//stereo(block_data,RFFS,IFFS,BLOCK_SIZE);

		//RDS();

	}


	return 0;
}
