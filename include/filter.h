
/*
Comp Eng 3DY4 (Computer Systems Integration Project)
Department of Electrical and Computer Engineering
McMaster University
Ontario, Canada
*/

#ifndef DY4_FILTER_H
#define DY4_FILTER_H

// add headers as needed
#include <iostream>
#include <vector>

// declaration of a function prototypes
void impulseResponseLPFUS(float, float, unsigned short int, std::vector<float> &,const int);
void impulseResponseLPF(float, float, unsigned short int, std::vector<float> &);
void impulseResponseBPF(std::vector<float> &, float , float , float , unsigned short int);
std::vector<float> slice(std::vector<float>, int, int);
void convolveFIRinBlocks(std::vector<float> &, const std::vector<float> &, const std::vector<float> &, std::vector<float> &, const float, const int);
void blockProcess(std::vector<float> &, const std::vector<float> &, const std::vector<float> &, float, std::vector<float> &, std::vector<float> &, std::vector<float> &, const int);
void split_audio_into_channels(const std::vector<float> &, std::vector<float> &, std::vector<float> &);
void demod(std::vector<float> &,const std::vector<float> &,const std::vector<float> &,std::vector<float> &);
void upsample(std::vector<float> &, const std::vector<float> &, const int );
void resampler(std::vector<float> &, const std::vector<float> &, const std::vector<float> &, std::vector<float> &, const int , const int);
void fmPll(std::vector<float> &, std::vector<float> ,float , float ,const float , const float , const float , std::vector<float> &);


#endif // DY4_FILTER_H