/*
Comp Eng 3DY4 (Computer Systems Integration Project)

Department of Electrical and Computer Engineering
McMaster University
Ontario, Canada
*/

#include "dy4.h"
#include "filter.h"
#include "genfunc.h"
#include  "math.h"
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

void convolveFIRinBlocks(float *y_ds, const std::vector<float> &xblock, const std::vector<float> &h, std::vector<float> &state, float blockSize, const int rf_decim)
{

	for (int n = 0; n < blockSize/rf_decim; n++) {
		for (int k = 0; k < h.size(); k++) {
			if ((n*rf_decim-k) >= 0) {
				y_ds[n] += h[k]*xblock[n*rf_decim-k];
			}
			else{
				if(abs(n*rf_decim-k) > state.size()){
					y_ds[n] += 0.0;
				}
				else{
					y_ds[n] += h[k]*state[state.size()+(n*rf_decim-k)];
				}
			}
		}
	}

	state = slice(xblock,xblock.size()-h.size()+1, xblock.size()-1);

}

void blockProcess(std::vector<float> &y_ds, const std::vector<float> &x, const std::vector<float> &h, float blockSize, std::vector<float> &state, std::vector<float> &xblock, std::vector<float> &filteredData, const int rf_decim){
	y_ds.clear(); y_ds.resize((x.size()+h.size())/10 - 1, 0.0);
	state.clear(); state.resize(h.size()-1, 0.0);
	filteredData.clear(); filteredData.resize(blockSize, 0.0);


	for (int m = 0; m < (int)(x.size()/blockSize); m++){

		xblock = slice(x,m*blockSize,(m+1)*blockSize-1);

		convolveFIRinBlocks(&y_ds[m*blockSize/rf_decim], xblock, h, state, blockSize, rf_decim);
	}

}
void demod(std::vector<float> &fm_demod,const std::vector<float> &I,const std::vector<float> &Q,std::vector<float> &prev_state){
	for(unsigned int k = 0;k<I.size();k++){
		if(pow(I[k])+pow(Q[k]) == 0){
			fm_demod[k] = 0;
			continue;
		}
		if(k==0){
			fm_demod[k] = (I[k]*(Q[k]-prev_state[0])-Q[k]*(I[k]-prev_state[1]))/(pow(I[k])+pow(Q[k]))
		}else{
			fm_demod[k] = (I[k]*(Q[k]-Q[k-1])-Q[k]*(I[k]-I[k-1]))/(pow(I[k])+pow(Q[k]))
		}
	prev_state = I[I.size()-1],Q[Q.size()-1];
	}
}
