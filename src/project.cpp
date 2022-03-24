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
void rfFrontEnd(std::vector<float> &mono_block, float RFFS, float IFFS,int BLOCK_SIZE,int rf_decim){
	for( unsigned int block_id = 0; ; block_id++){
		std::vector<float> block_data(BLOCK_SIZE);
		// Temporarily using RFFS instead of BLOCK_SIZE
		//readStdinBlockData(BLOCK_SIZE, block_id, audio_data);
		readStdinBlockData(BLOCK_SIZE, block_id, block_data);
		if ((std::cin.rdstate()) != 0) {
			std::cerr << "End of input stream reached" << "\n";
			exit(1);
		}
		std::cerr << "Read block" << block_id << "\n";
	}

	float Fc = 100000;	//RF cutoff 100Khz
	unsigned short int num_taps = 101;

	std::vector<float> h;
	impulseResponseLPF(RFFS, Fc, num_taps, h);

	//int numDecim = RFFS/IFFS;
	int mono0Decim = 5;
	std::vector<float> I_block(BLOCK_SIZE/2);
	std::vector<float> Q_block(BLOCK_SIZE/2);
	std::vector<float> I(I_block.size()/mono0Decim);
	std::vector<float> Q(Q_block.size()/mono0Decim);
	std::vector<float> i_state(num_taps-1);
	std::vector<float> q_state(num_taps-1);
	std::vector<float> state(num_taps-1);
	std::vector<float> prev_state(2);
	std::vector<float> fm_demod(I_block.size());
	std::vector<float> mono_data(I_block.size());
	split_audio_into_channels(block_data, I_block, Q_block);

	//I_block = slice(I_data, block_count*BLOCK_SIZE, (block_count + 1)*BLOCK_SIZE);
	//Q_block = slice(Q_data, block_count*BLOCK_SIZE, (block_count + 1)*BLOCK_SIZE);

	//Might have to change to filter single block
	convolveFIRinBlocks(I, I_block, h, i_state, BLOCK_SIZE, rf_decim);
	convolveFIRinBlocks(Q, Q_block, h, q_state, BLOCK_SIZE, rf_decim);

	demod(fm_demod,I,Q,prev_state);
	std::vector<float> audio(fm_demod.size());
	std::vector<float> h2(num_taps);
	//What is our FS? 240 K samples/sec ?
	impulseResponseLPF(IFFS, 16000, num_taps, h2);
	convolveFIRinBlocks(audio,fm_demod,h2,state,BLOCK_SIZE,mono0Decim);

	//Critical Segment
	//mono_data.insert(mono_data.end(), mono_block.begin(), mono_block.end());

	for(unsigned int k = 0;k<mono_data.size();k++){
		if(std::isnan(mono_data[k])) audio[k] = 0;
		else audio[k] = static_cast<short int>(mono_data[k]*16384);
	}
	fwrite(&audio[0],sizeof(short int),audio.size(),stdout);

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

	// *** Block of size RFFS being used temporarily to test 1s of audio

	//std::vector<float> FMDemodData;
	//rfFrontEnd(FMDemodData,RFFS,IFFS,BLOCK_SIZE,rf_decim);

	std::vector<float> mono_block;
	rfFrontEnd(mono_block,RFFS,IFFS,BLOCK_SIZE,rf_decim);

	//monoStereo(FMDemodData,RFFS,IFFS,BLOCK_SIZE);

	//RDS();


	return 0;
}
