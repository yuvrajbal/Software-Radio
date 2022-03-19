/*
Comp Eng 3DY4 (Computer Systems Integration Project)

Department of Electrical and Computer Engineering
McMaster University
Ontario, Canada
*/

#include "dy4.h"
#include "filter.h"
#include "genfunc.h"
// function to compute the impulse response "h" based on the sinc function
void impulseResponseLPF(float Fs, float Fc, unsigned short int num_taps, std::vector<float> &h)
{
	// allocate memory for the impulse response
	h.clear(); h.resize(num_taps, 0.0);
	float normCutoffFreq = Fc / (Fs/2);
	int i;
	for(i=0;i<num_taps;i++){
		if(i == (num_taps-1)/2){
			h[i] = normCutoffFreq;
		}
		else{
			h[i] = normCutoffFreq*(sin(PI*normCutoffFreq*(i-(num_taps-1)/2)))/(PI*normCutoffFreq*(i-(num_taps-1)/2));
		}
		h[i] = h[i]*(sin(i*PI/num_taps))*(sin(i*PI/num_taps));

	}
}

std::vector<float> slice(std::vector<float>temp,int lBound, int rBound)
{
    auto start = temp.begin() + lBound;
    auto end = temp.begin() + rBound + 1;
    std::vector<float>  sliced(rBound - lBound + 1);
    copy(start, end, sliced.begin());
    return sliced;
}

void convolveFIRinBlocks(float *y, const std::vector<float> &xblock, const std::vector<float> &h, std::vector<float> &state, float blockSize)
{

	for (int n = 0; n < blockSize; n++) {
		for (int k = 0; k < h.size(); k++) {
				if ((n-k) >= 0) {
					y[n] = y[n] + h[k]*xblock[n-k];
				}
				else{
					if(abs(n-k) > state.size()){
						y[n] = y[n] + 0.0;
					}
					else{
						y[n] = y[n] + h[k]*state[state.size()+(n-k)];
					}
				}
		}
	}

	state = slice(xblock,xblock.size()-h.size()+1, xblock.size()-1);

}

void blockProcess(std::vector<float> &y, const std::vector<float> &x, const std::vector<float> &h, float blockSize, std::vector<float> &state, std::vector<float> &xblock, std::vector<float> &filteredData, const float RFFS, const float IFFS){
	y.clear(); y.resize(x.size()+h.size()-1, 0.0);
	state.clear(); state.resize(h.size()-1, 0.0);
	filteredData.clear(); filteredData.resize(blockSize, 0.0);


	for (int m = 0; m < (int)(x.size()/blockSize); m++){

		xblock = slice(x,m*blockSize,(m+1)*blockSize-1);          //code is from lab 2 so still justs splits full .raw file

		convolveFIRinBlocks(&y[m*blockSize], xblock, h, state, blockSize);
	}

}