/*
    Copyright 2012 Manu <manuavazquez@gmail.com>

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/


#ifndef LINKEDKALMANFILTERANDLINEARFILTERBASEDALGORITHM_H
#define LINKEDKALMANFILTERANDLINEARFILTERBASEDALGORITHM_H

#include <LinearFilterBasedAlgorithm.h>

#include <LinearFilterAwareNoiseVarianceAdjustingKalmanEstimator.h>


class LinkedKalmanFilterAndLinearFilterBasedAlgorithm : public LinearFilterBasedAlgorithm
{
public:
	LinkedKalmanFilterAndLinearFilterBasedAlgorithm(std::string name, Alphabet alphabet, uint L, uint Nr,uint N, uint iLastSymbolVectorToBeDetected, uint m, LinearFilterAwareNoiseVarianceAdjustingKalmanEstimator* channelEstimator, MatrixXd preamble, uint smoothingLag, LinearDetector *linearDetector,  std::vector<double> ARcoefficients, bool substractContributionFromKnownSymbols=false);
};

#endif // LINKEDKALMANFILTERANDLINEARFILTERBASEDALGORITHM_H
