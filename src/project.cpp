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

#define QUEUE_LENGTH 10

class demodData{
	public:
	demodData(std::vector<float> val){block = val;}
	std::vector<float> block;
	bool read = false;
};

void updateQueue(std::queue<demodData> &dataQueue){
	if(dataQueue.front().read){
		dataQueue.pop();
	}
}

void rfFrontEnd(std::mutex &audio_mutex, std::mutex &rds_mutex, std::condition_variable &audio_cvar, std::condition_variable &rds_cvar, std::queue<demodData> audioQueue, std::queue<demodData> rdsQueue, float RFFS, float IFFS,int BLOCK_SIZE,int rf_decim){
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
void monoStereo(std::mutex &audio_mutex, std::condition_variable &audio_cvar, std::queue<demodData> audioQueue, float RFFS, float IFFS, int BLOCK_SIZE){

	//Read from queue first
	//Critical section
	std::unique_lock<std::mutex> my_lock(audio_mutex);
	while(audioQueue.empty()){
		audio_cvar.wait(my_lock);
	}
	if(!(audioQueue.front()).read){
		std::vector<float> block = (audioQueue.front()).block;
		(audioQueue.front()).read = true;
		updateQueue(audioQueue);
	}
	my_lock.unlock();

	//Critical section ends
	//Process data after

	int mono0Decim = 5;
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

void RDS(std::mutex &rds_mutex, std::condition_variable &rds_cvar, std::queue<demodData> &rdsQueue){
	
	//Read from queue first
	//Critical section
	std::unique_lock<std::mutex> my_lock(rds_mutex);
	while(rdsQueue.empty()){
		rds_cvar.wait(my_lock);
	}
	if(!(rdsQueue.front()).read){
		std::vector<float> block = (rdsQueue.front()).block;
		(rdsQueue.front()).read = true;
		updateQueue(rdsQueue);
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

	//Suggested to use 2 separate queues for separate threads
	std::queue<demodData> audioQueue;
	std::queue<demodData> rdsQueue;
	std::mutex audio_mutex;
	std::mutex rds_mutex;
	std::condition_variable audio_cvar;
	std::condition_variable rds_cvar;

	std::thread tFrontEnd = std::thread(rfFrontEnd, 'a', std::ref(audio_mutex), std::ref(rds_mutex), std::ref(audio_cvar), std::ref(rds_cvar), std::ref(audioQueue), std::ref(rdsQueue), RFFS, IFFS, BLOCK_SIZE, rf_decim);
	std::thread tMonoStereo = std::thread(monoStereo, 'b', std::ref(audio_mutex), std::ref(audio_cvar), std::ref(audioQueue));
	std::thread tRDS = std::thread(RDS, 'c', std::ref(rds_mutex), std::ref(rds_cvar), std::ref(rdsQueue));

	tFrontEnd.join();
	tMonoStereo.join();
	tRDS.join();

	return 0;
}
