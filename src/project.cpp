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

void rfFrontEnd(std::vector<float> &FMDemodData, float RFFS, float IFFS){



	const std::string in_fname = "../data/float32samples.bin";
	std::vector<float> audio_data;
	int BLOCK_SIZE =  1024 * 10 * 2;
	for( unsigned int block_id = 0; ; block_id++){
		std::vector<float> block_data(BLOCK_SIZE); 
		readStdinBlockData(BLOCK_SIZE, block_id, block_data);
		if ((std::cin.rdstate()) != 0) {
			std::cerr << "End of input stream reached" << std::end1;
			exit(1);
		}
		std::cerr << "Read block" << block_id << std::end1;
	}	


	//Are these correct?
	float Fs = 2400000;
	float Fc = 30000;
	unsigned short int num_taps = 101;
	float decim = 10;
	
	std::vector<float> h;
	impulseResponseLPF(Fs, Fc, num_taps, h);

	std::vector<float> mono_block, mono_buffer;
	mono_buffer.resize(BLOCK_SIZE);

	for(int i = 0; i < audio_data.size(); i += BLOCK_SIZE){
		blockFiltering(audio_data, mono_block, h, mono_buffer.size(), mono_buffer);

		//Decimate (has to be a better way to do this)
		FMDemodData.clear();
	}

	int numDecim = RFFS/IFFS;

	for(int i = 0; i < mono_block.size(); i++){
		if((i % numDecim) == 0)
		FMDemodData.push_back(mono_block.at(i));
	}
}


void monoStereo(std::vector<float> FMDemodData, float RFFS, float IFFS){

}

void RDS(){

}

int main(int argc, char* argv[])
{
	int mode = 0;
	if(mode = 0){
		float RFFs = 2400000;
		float IFFs = 240000;
		float audioFs = 48000;	
	} 
	if(mode = 1){
		float RFFs = 1152000;
		float IFFs = 288000;
		float audioFs = 48000;	
	} 
	if(mode = 2){
		float RFFs = 2400000;
		float IFFs = 240000;
		float audioFs = 44100;	
	} 
	if(mode = 3){
		float RFFs = 1920000;
		float IFFs = 320000;
		float audioFs = 44100;	
	} 
	

	std::vector<float> FMDemodData;

	rfFrontEnd(FMDemodData);

	monoStereo(FMDemodData);
	
	//RDS();
	
	return 0;
}
