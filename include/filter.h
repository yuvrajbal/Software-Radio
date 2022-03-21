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
void impulseResponseLPF(float, float, unsigned short int, std::vector<float> &);
void convolveFIR(std::vector<float> &, const std::vector<float> &, const std::vector<float> &);
void blockProcess(std::vector<float> &, const std::vector<float> &, const std::vector<float> &, float, std::vector<float> &, std::vector<float> &, std::vector<float> &, const float, const float);

#endif // DY4_FILTER_H
