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
void impulseResponseLPFUS(float Fs, float Fc, unsigned short int num_taps, std::vector<float> &h,const int US)
{
	int taps = num_taps*US;
	// allocate memory for the impulse response
	h.clear(); h.resize(taps, 0.0);
	float normCutoffFreq = Fc / (Fs*US/2);
	int i;
	for(i=0;i<taps;i++){
		if(i == (taps-1)/2){
			h[i] = normCutoffFreq;
		}
		else{
			h[i] = normCutoffFreq*(sin(PI*normCutoffFreq*(i-(taps-1)/2)))/(PI*normCutoffFreq*(i-(taps-1)/2));
		}
		h[i] = h[i]*(sin(i*PI/(taps)))*(sin(i*PI/(taps)));

	}
}

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
		h[i] = h[i]*(sin(i*PI/(num_taps)))*(sin(i*PI/(num_taps)));

	}
}

void impulseResponseBPF(std::vector<float> &h, float Fb, float Fe, float Fs, unsigned short int num_taps)
{
	h.clear(); h.resize(num_taps, 0.0);
	float normCenter = ((Fe+Fb)/2)/(Fs/2);
	float normPass = (Fe-Fb)/(Fs/2);
	for(int i = 0;i<num_taps-1;i++){
		if(i == (num_taps-1)/2){
			h[i] = normPass;
		}else{
			h[i] = normPass*sin(PI*(normPass/2)*(i-(num_taps-1)/2))/(PI*(normPass/2)*(i-(num_taps-1)/2));
		}
		h[i] = h[i]*cos(i*PI*normCenter);
		h[i] = h[i]*pow(sin(i*PI/num_taps),2);
	}
}



void Usconv_ds(std::vector<float> &y_ds, const std::vector<float> &xblock, const std::vector<float> &h, std::vector<float> &state, const int decim, const int Upsamp_size)
{
    for(int phase=0; phase < Upsamp_size; phase++ ){
        for(int n=phase ; n < y_ds.size(); n=n+Upsamp_size ){
            int xu=n;
            int indx=0;
            for(int k=0; k< h.size(); k=k+1 ){            //index k for loop
                if(((decim % Upsamp_size)*n -k) >= 0 && ((decim % Upsamp_size)*n -k) < xblock.size() ){
                    y_ds[n]+= h[k]*xblock[(decim % Upsamp_size)*n-k];
                }
                else if( state.size() >0  && ((decim % Upsamp_size)*n -k) < 0){
                    y_ds[n]+= h[k]*state[state.size()-1-indx];
                    indx++;
                }
            }
        }
    }
    state = slice(xblock,xblock.size()-h.size()+1, xblock.size()-1);

}



std::vector<float> slice(std::vector<float>temp,int lBound, int rBound)
{
    auto start = temp.begin() + lBound;
    auto end = temp.begin() + rBound + 1;
    std::vector<float>  sliced(rBound - lBound + 1);
    copy(start, end, sliced.begin());
    return sliced;
}

void upsample(std::vector<float> &y_us, const std::vector<float> &fm_demod, const int US)
{
	y_us.clear(); y_us.resize((fm_demod.size()*US), 0.0);
	for (int n = 0; n < fm_demod.size()*US; n++) {
		if(n%US!=0){
			y_us[n] = 0.0;
		}
		else{
			y_us[n] = fm_demod[n/US];
		}
	}
}

void resampler(std::vector<float> &y, const std::vector<float> &fm_demod, const std::vector<float> &h, std::vector<float> &state, const int DS, const int US)
{
	y.clear(); y.resize((fm_demod.size()*US/DS), 0.0);

	int phase;

	for (int n = 0; n < fm_demod.size()*US/DS; n++){
		phase = (n*DS)%US;
		for (int k = phase; k < h.size(); k+=US) {
			if ((n*DS-k)/US >= 0) {
				y[n] += h[k]*fm_demod[(n*DS-k)/US];
			}
			else{
				y[n] += h[k]*state[state.size()+((n*DS-k)/US)];
			}
		}
	}

	state = slice(fm_demod,fm_demod.size()-(h.size()/US)+1, fm_demod.size()-1);

}

void convolveFIRinBlocks(std::vector<float> &y_ds, const std::vector<float> &xblock, const std::vector<float> &h, std::vector<float> &state, float blockSize, const int rf_decim)
{
	y_ds.clear(); y_ds.resize((xblock.size())/rf_decim, 0.0);
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
void fmPll(std::vector<float> &ncoOut, std::vector<float> pllIn,float freq, float Fs,const float ncoScale, const float phaseAdjust, const float normBandWidth, std::vector<float> &prev_pll)
{

	float Cp = 2.666;
	float Ci = 3.555;
	float Kp = normBandWidth*Cp;
	float Ki = normBandWidth*normBandWidth*Ci;
	ncoOut.clear(); ncoOut.resize(pllIn.size()+1);

	float integrator = prev_pll[0];
	float phaseEst = prev_pll[1];
	float feedbackI = prev_pll[2];
	float feedbackQ = prev_pll[3];
	ncoOut[0] = prev_pll[4];
	float trigOffset = prev_pll[5];
	float errorI;
	float errorQ;
	float errorD;
	float trigArg;
	for(int k = 0;k<pllIn.size();k++){

		errorI = pllIn[k]*feedbackI;
		errorQ = pllIn[k]*(-feedbackQ);
		errorD = atan2(errorQ,errorI);
		integrator += Ki*errorD;
		phaseEst += Kp*errorD + integrator;
		trigOffset += 1;
		trigArg = 2*PI*(freq/Fs)*trigOffset + phaseEst;
		feedbackI = cos(trigArg);
		feedbackQ = sin(trigArg);
		ncoOut[k+1] = cos(trigArg*ncoScale + phaseAdjust);
	}
	prev_pll[0] = integrator;
	prev_pll[1] = phaseEst;
	prev_pll[2] = feedbackI;
	prev_pll[3] = feedbackQ;
	prev_pll[4] = ncoOut[ncoOut.size()-1];
	prev_pll[5] = trigOffset + pllIn.size();
}



void demod(std::vector<float> &fm_demod,const std::vector<float> &I,const std::vector<float> &Q,std::vector<float> &prev_state){
	for(int k = 0;k<I.size();k++){
		if(pow(I[k],2)+pow(Q[k],2) == 0){
			fm_demod[k] = 0;
			continue;
		}
		if(k==0){
			fm_demod[k] = (I[k]*(Q[k]-prev_state[0])-Q[k]*(I[k]-prev_state[1]))/(pow(I[k],2)+pow(Q[k],2));
		}else{
			fm_demod[k] = (I[k]*(Q[k]-Q[k-1])-Q[k]*(I[k]-I[k-1]))/(pow(I[k],2)+pow(Q[k],2));
		}
	prev_state[0] = *(I.end() - 1);
	prev_state[1] = *(Q.end() - 1);
	}
}