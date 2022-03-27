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
#include <thread>
#include <iostream>
#include <mutex>
#include <queue>
#include <condition_variable>


class demodData{
	public:
	demodData(std::vector<float> val){block = val;}
	std::vector<float> block;
	bool monoRead = false;
	bool RDSRead = false;
};

void updateQueue(std::queue<demodData> &dataQueue){
	if((dataQueue.front().monoRead) && (dataQueue.front().RDSRead)){
		dataQueue.pop();
	}
}

void rfFrontEnd(std::mutex &my_mutex, std::condition_variable &my_cvar, std::queue<demodData> dataQueue, std::queue<demodData> &block_data, float RFFS, float IFFS,int BLOCK_SIZE,int rf_decim){
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

	std::vector<float> fm_demod;
	fm_demod.clear();fm_demod.resize(I.size(),0.0);

	demod(fm_demod,I,Q,prev_state);

	std::vector<float> h2;
	impulseResponseLPF(IFFS, 16000, num_taps, h2);


	std::vector<float> audio;
	audio.clear();audio.resize(fm_demod.size()/mono0Decim,0.0);

	convolveFIRinBlocks(audio,fm_demod,h2,state,fm_demod.size(),mono0Decim);

	std::vector<float> mono_data;
	mono_data.clear();mono_data.resize(I_block.size()/mono0Decim);

	for(unsigned int k = 0;k<audio.size();k++){
		if(std::isnan(audio[k])) mono_data[k] = 0;
		else mono_data[k] = static_cast<short int>(audio[k]*16384);
	}
	//std::cerr << "output";
	fwrite(&mono_data[0],sizeof(short int),mono_data.size(),stdout);

	//Figure out what goes where

	//Critical section
	std::unique_lock<std::mutex> my_lock(my_mutex);
	dataQueue.push(demodData(fm_demod));
	my_lock.unlock();

	//Critical section ends

}
void monoStereo(std::mutex &my_mutex, std::condition_variable &my_cvar, std::queue<demodData> dataQueue, float RFFS, float IFFS, int BLOCK_SIZE){

	//Read from queue first
	//Critical section
	std::unique_lock<std::mutex> my_lock(my_mutex);
	while(dataQueue.empty()){
		my_cvar.wait(my_lock);
	}
	if(!(dataQueue.front()).monoRead){
		std::vector<float> block = (dataQueue.front()).block;
		(dataQueue.front()).monoRead = true;
		updateQueue(dataQueue);
	}
	my_lock.unlock();

	//Critical section ends
	//Process data after

}

void RDS(std::mutex &my_mutex, std::condition_variable &my_cvar, std::queue<demodData> &dataQueue){
	
	//Read from queue first
	//Critical section
	std::unique_lock<std::mutex> my_lock(my_mutex);
	while(dataQueue.empty()){
		my_cvar.wait(my_lock);
	}
	if(!(dataQueue.front()).RDSRead){
		std::vector<float> block = (dataQueue.front()).block;
		(dataQueue.front()).RDSRead = true;
		updateQueue(dataQueue);
	}
	my_lock.unlock();

	//critical section ends
	//Process data after
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

	/*
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
	*/

	std::queue<demodData> dataQueue;
	std::mutex my_mutex;
	std::condition_variable my_cvar;

	std::thread tFrontEnd = std::thread(rfFrontEnd, 'a', std::ref(my_mutex), std::ref(my_cvar), std::ref(dataQueue));
	std::thread tMonoStereo = std::thread(monoStereo, 'b', std::ref(my_mutex), std::ref(my_cvar), std::ref(dataQueue));
	std::thread tRDS = std::thread(RDS, 'c', std::ref(my_mutex), std::ref(my_cvar), std::ref(dataQueue));

	tFrontEnd.join();
	tMonoStereo.join();
	tRDS.join();

	return 0;
}
