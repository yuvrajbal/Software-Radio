/*
Comp Eng 3DY4 (Computer Systems Integration Project)

Copyright by Nicola Nicolici
Department of Electrical and Computer Engineering
McMaster University
Ontario, Canada
*/

// run <./data/iq_samples.raw > >(aplay -f S16_LE -r 48000)

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

void rfFrontEnd(std::mutex &audio_mutex, std::mutex &rds_mutex, std::condition_variable &audio_cvar, std::condition_variable &rds_cvar, std::queue<std::vector<float>> &audioQueue, std::queue<std::vector<float>> &rdsQueue, float RFFS, float IFFS,int BLOCK_SIZE,int rf_decim, unsigned short int num_taps, int US, int audio_decim){
	
	std::cerr << "Starting FE thread \n";

	float Fc = 100000;	//RF cutoff 100Khz

	std::vector<float> i_state;
	i_state.clear();i_state.resize(num_taps-1,0.0);
	std::vector<float> q_state;
	q_state.clear();q_state.resize(num_taps-1,0.0);
	std::vector<float> fm_demod;

	std::vector<float> h;
	impulseResponseLPF(RFFS, Fc, num_taps, h);

	std::vector<float> I_block;
	I_block.clear();I_block.resize(BLOCK_SIZE/2,0.0);
	std::vector<float> Q_block;
	Q_block.clear();Q_block.resize(BLOCK_SIZE/2,0.0);
	std::vector<float> prev_state;
	prev_state.clear();prev_state.resize(2,0.0);
	std::vector<float> block_data(BLOCK_SIZE);
	std::vector<float> I;
	I.clear();I.resize(I_block.size()/rf_decim,0.0);
	std::vector<float> Q;
	Q.clear();Q.resize(Q_block.size()/rf_decim,0.0);

	// Repeated read from stdin and writing to Queues

	for(unsigned int block_id = 0; ; block_id++){

		// Read from stdin

		readStdinBlockData(BLOCK_SIZE, block_id, block_data);
		if ((std::cin.rdstate()) != 0) {
			std::cerr << "End of input stream reached" << "\n";
			exit(1);
		}
		//std::cerr << "Read block " << block_id << "\n";

		// Demodulate

		// Splits the data into I and Q samples
		for(int k = 0;k<BLOCK_SIZE/2;k++){
			I_block[k] = block_data[k*2];
			Q_block[k] = block_data[1+k*2];
		}


		convolveFIRinBlocks(I, I_block, h, i_state, BLOCK_SIZE/2, rf_decim);

		convolveFIRinBlocks(Q, Q_block, h, q_state, BLOCK_SIZE/2, rf_decim);

		fm_demod.clear();fm_demod.resize(I.size(),0.0);

		demod(fm_demod,I,Q,prev_state);
 
		//Figure out what goes where

		//Critical section

		//RDS CURRENTLY COMMENTED OUT

		std::unique_lock<std::mutex> audio_lock(audio_mutex);
		std::unique_lock<std::mutex> rds_lock(rds_mutex);
		//Lock if either queue is full
		//while((audioQueue.size() > 10)||(rdsQueue.size() > 10)){
		if((audioQueue.size() >= QUEUE_LENGTH)){			
			std::cerr << "Waiting, AudioQueue Full " << audioQueue.size() << "\n";
			audio_cvar.wait(audio_lock);
		}
		if((rdsQueue.size() >= QUEUE_LENGTH)){			
			std::cerr << "Waiting, RDSQueue Full " << rdsQueue.size() << "\n";
			rds_cvar.wait(rds_lock);
		}
		std::cerr << "pushing block " << block_id << " \n";
		audioQueue.push(fm_demod);
		rdsQueue.push(fm_demod);
		audio_lock.unlock();
		rds_lock.unlock();
		audio_cvar.notify_one();
		rds_cvar.notify_one();
		//Critical section ends
	}
}

void monoStereo(std::mutex &audio_mutex, std::condition_variable &audio_cvar, std::queue<std::vector<float>> &audioQueue, float RFFS, float IFFS, int BLOCK_SIZE, unsigned short int num_taps, int US, int audio_decim){

	std::cerr << "starting audiothread \n";

	int mono0Decim = 5;
	std::vector<float> state;
	state.clear();state.resize(num_taps-1,0.0);

	std::vector<float> h2;
	std::vector<float> audio;
	std::vector<float> mono_data;	
	std::vector<float> demod_us;
	std::vector<float> fm_demod;
	std::vector<float> prev_pll;
	prev_pll.clear();prev_pll.resize(6,0.0);
	prev_pll[0] = 0;
	prev_pll[1] = 0;
	prev_pll[2] = 1;
	prev_pll[3] = 0;
	prev_pll[4] = 1;
	prev_pll[5] = 0;

	// For stereo
	std::vector<float> hCarrier;
	std::vector<float> hChannel;
	float Fs = IFFS;
	float FbCarrier = 18500;
	float FeCarrier = 19500;
	float FbChannel = 22000;
	float FeChannel = 54000;
	std::vector<float> ncoOut;
	float freq = 19000;
	std::vector<float> mixer;
	mixer.clear(); mixer.resize(hChannel.size());
	std::vector<float> stereo_data;
	std::vector<float> left_stereo;
	std::vector<float> right_stereo;
	left_stereo.clear(); left_stereo.resize(stereo_data.size());
	right_stereo.clear(); right_stereo.resize(stereo_data.size());
	impulseResponseLPFUS(IFFS, 16000, num_taps, h2,US);
	
	for(int i = 0;i<h2.size();i++){
		h2[i] = h2[i]*US;
	}
	
	while(true){

		//Read from queue first
		//Critical section
		std::unique_lock<std::mutex> audio_lock(audio_mutex);
		if(audioQueue.empty()){
			std::cerr << "Waiting, AudioQueue Empty " << audioQueue.size() << "\n";
			audio_cvar.wait(audio_lock);
		}
		std::cerr << "reading block\n";
		fm_demod = audioQueue.front();
		audioQueue.pop();
		audio_lock.unlock();
		audio_cvar.notify_one();
		std::cerr << "popped block\n";

		//Critical section ends
		//Process data after

		upsample(demod_us,fm_demod,US);

		mono_data.clear();mono_data.resize(demod_us.size()/audio_decim,0.0);
		mono_data.resize((fm_demod.size()*US)/audio_decim,0.0);
		convolveFIRinBlocks(mono_data,demod_us,h2,state,demod_us.size(),audio_decim);

		// STEREO
		impulseResponseBPF(hCarrier,FbCarrier,FeCarrier,Fs,num_taps);

		impulseResponseBPF(hChannel,FbChannel,FeChannel,Fs,num_taps);

		// PLL
		fmPll(ncoOut,hCarrier,freq,Fs, 1.0, 0.0, 0.01, prev_pll);

		// NCO
		for(int i = 0;i<ncoOut.size();i++){
			ncoOut[i] = ncoOut[i]*2;
		}

		// Mixer
		for(int k = 0;k<mixer.size();k++){
			mixer[k] = hChannel[k]*ncoOut[k];
		}

		std::vector<short int> mono_output;
		mono_output.clear();mono_output.resize(mono_data.size());
		for(unsigned int k = 0;k<mono_data.size();k++){
			if(std::isnan(mono_data[k])) mono_output[k] = 0;
			else mono_output[k] = static_cast<short int>(mono_data[k]*16384);
		}
		fwrite(&mono_output[0],sizeof(short int),mono_output.size(),stdout);	
	}
}

void RDS(std::mutex &rds_mutex, std::condition_variable &rds_cvar, std::queue<std::vector<float>> &rdsQueue, float RFFS, float IFFS, int BLOCK_SIZE, unsigned short int num_taps, int US, int audio_decim){
	
	std::cerr << "starting rds thread\n";

	std::vector<float> h;
	float Fs = IFFS;
	float Fb = 54000;
	float Fe = 60000;
	impulseResponseBPF(h,Fb,Fe,Fs,num_taps);

	while(true){

		//Read from queue first
		//Critical section
		std::unique_lock<std::mutex> rds_lock(rds_mutex);
		while(rdsQueue.empty()){
			std::cerr << "Waiting, RDSQueue Empty " << rdsQueue.size() << "\n";
			rds_cvar.wait(rds_lock);
		}
		std::vector<float> block = (rdsQueue.front());
		rdsQueue.pop();
		rds_lock.unlock();

		//critical section ends
		//Process data after

		std::cerr << "foo\n";
	}
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
	float RFFS, IFFS, audioFS;
	int US, audio_decim;
	if(mode == 0){
		RFFS = 2400000;
		IFFS = 240000;
		audioFS = 48000;
		US = 1;
		audio_decim = 5;
	}
	else if(mode == 1){
		RFFS = 1152000;
		IFFS = 288000;
		audioFS = 48000;
		US = 1;
		audio_decim = 6;
	}
	else if(mode == 2){
		RFFS = 2400000;
		IFFS = 240000;
		audioFS = 44100;
		US = 147;
		audio_decim = 800;
	}
	else if(mode == 3){
		RFFS = 1920000;
	 	IFFS = 320000;
		audioFS = 44100;
		US = 441;
		audio_decim = 3200;
	}

	int rf_decim = 10;
	int BLOCK_SIZE =  1024 * rf_decim * 2 * audio_decim;
	unsigned short int num_taps = 101;

	//Suggested to use 2 separate queues for separate threads
	std::queue<std::vector<float>> audioQueue;
	std::queue<std::vector<float>> rdsQueue;
	std::mutex audio_mutex;
	std::mutex rds_mutex;
	std::condition_variable audio_cvar;
	std::condition_variable rds_cvar;

	std::thread tFrontEnd = std::thread(rfFrontEnd, std::ref(audio_mutex), std::ref(rds_mutex), std::ref(audio_cvar), std::ref(rds_cvar), std::ref(audioQueue), std::ref(rdsQueue), RFFS, IFFS, BLOCK_SIZE, rf_decim, num_taps, US, audio_decim);
	std::thread tMonoStereo = std::thread(monoStereo, std::ref(audio_mutex), std::ref(audio_cvar), std::ref(audioQueue), RFFS, IFFS, BLOCK_SIZE, num_taps, US, audio_decim);
	std::thread tRDS = std::thread(RDS, std::ref(rds_mutex), std::ref(rds_cvar), std::ref(rdsQueue), RFFS, IFFS, BLOCK_SIZE, num_taps, US, audio_decim);

	tFrontEnd.join();
	tMonoStereo.join();
	tRDS.join();

	std::cerr << "threads joined \n";

	return 0;
}
