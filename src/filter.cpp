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

// function to compute the filtered output "y" by doing the convolution
// of the input data "x" with the impulse response "h"
void convolveFIR(std::vector<float> &y, const std::vector<float> &x, const std::vector<float> &h)
{
	y.clear(); y.resize(x.size()+h.size()-1, 0.0);
	unsigned int m;
	unsigned int n;
	for(m = 0;m<y.size();m++){
		for(n = 0;n<h.size();n++){
			if((m-n)>=0 && (m-n)< x.size()){
				y[m] += h[n]*x[m-n];
			}
		}
	}
}
