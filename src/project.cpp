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

void rfFrontEnd(std::vector<float> &FMDemodData, float RFFS, float IFFS,int BLOCK_SIZE){


	const std::string in_fname = "../data/iq_samples.raw";
	std::vector<float> audio_data;
	for( unsigned int block_id = 0; ; block_id++){
		std::vector<float> block_data(BLOCK_SIZE);
		readStdinBlockData(BLOCK_SIZE, block_id, audio_data);
		if ((std::cin.rdstate()) != 0) {
			std::cerr << "End of input stream reached" << "\n";
			exit(1);
		}
		std::cerr << "Read block" << block_id << "\n";
	}


	//Are these correct?
	// float Fs = 2400000;
	// float Fc = 30000;
	// unsigned short int num_taps = 101;
	// float decim = 10;
	//
	// std::vector<float> h;
	// impulseResponseLPF(Fs, Fc, num_taps, h);
	//
	// std::vector<float> mono_block, mono_buffer;
	// mono_buffer.resize(BLOCK_SIZE);
	//
	// for(int i = 0; i < audio_data.size(); i += BLOCK_SIZE){
	// 	blockFiltering(audio_data, mono_block, h, mono_buffer.size(), mono_buffer);
	//
	// 	//Decimate (has to be a better way to do this)
	// 	FMDemodData.clear();
	// }
	//
	// int numDecim = RFFS/IFFS;
	//
	// for(int i = 0; i < mono_block.size(); i++){
	// 	if((i % numDecim) == 0)
	// 	FMDemodData.push_back(mono_block.at(i));
	// }
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
	int BLOCK_SIZE =  1024 * 10 * 2;
	std::vector<float> FMDemodData;

	rfFrontEnd(FMDemodData,RFFS,IFFS,BLOCK_SIZE);

	monoStereo(FMDemodData,RFFS,IFFS,BLOCK_SIZE);

	//RDS();
	std::vector<float> processed_data(BLOCK_SIZE);
	std::vector<float> audio(BLOCK_SIZE);
	for(unsigned int k = 0;k<processed_data.size();k++){
		if(std::isnan(processed_data[k])) audio[k] = 0;
		else audio[k] = static_cast<short int>(processed_data[k]*16384);
	}
	fwrite(&audio[0],sizeof(short int),audio.size(),stdout);


	return 0;
}
