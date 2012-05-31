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


#include "SERawareKalmanEstimatorDecorator.h"

#define DEBUG

#include <math.h>

SERawareKalmanEstimatorDecorator::SERawareKalmanEstimatorDecorator(KalmanEstimator *kalmanEstimator,const std::vector<double> &noiseVariances,const std::vector<double> &SERs,const std::vector<double> &possibleErrors):
KalmanEstimatorDecorator(kalmanEstimator),_noiseVariances(noiseVariances),_SERs(SERs),_possibleErrors(possibleErrors)
{
	assert(noiseVariances.size()==SERs.size());
	
	// just checking BPSK is in use
	assert(possibleErrors.size()==3);
	
#ifdef DEBUG
	computeExtraVariance(0.01);
	computeExtraVariance(0.5);
	computeExtraVariance(0.0007);
#endif
}

MatrixXd SERawareKalmanEstimatorDecorator::nextMatrix(const VectorXd& observations, const MatrixXd& symbolsMatrix, double noiseVariance)
{
	return KalmanEstimatorDecorator::nextMatrix(observations, symbolsMatrix, noiseVariance + computeExtraVariance(noiseVariance));
}

double SERawareKalmanEstimatorDecorator::likelihood(const VectorXd& observations, const MatrixXd symbolsMatrix, double noiseVariance)
{
	return KalmanEstimatorDecorator::likelihood(observations, symbolsMatrix, noiseVariance + computeExtraVariance(noiseVariance));
}

SERawareKalmanEstimatorDecorator* SERawareKalmanEstimatorDecorator::clone() const
{
	return new SERawareKalmanEstimatorDecorator(*this);
}

double SERawareKalmanEstimatorDecorator::computeExtraVariance(double noiseVariance)
{
	// we find out which variance from "_noiseVariances" is closer to this one
	uint iMin = 0;
	double minimumDistance = fabs(_noiseVariances[0]-noiseVariance);
	
	double distanceToCurrentVariance;
	
	for(uint i=1;i<_noiseVariances.size();i++)
	{
		distanceToCurrentVariance = fabs(_noiseVariances[i]-noiseVariance);
		if(distanceToCurrentVariance<minimumDistance)
		{
			iMin = i;
			minimumDistance = distanceToCurrentVariance;
		}
	}
	
	double errorsAutocorrelation = 0.0;
	for(uint i=1;i<_possibleErrors.size();i++)
		// NOTE: this is ultimately assuming BPSK
		errorsAutocorrelation += pow(_possibleErrors[i],2.0)*_SERs[iMin]/(_possibleErrors.size()-1);
	
#ifdef DEBUG
	cout << "noiseVariance = " << noiseVariance << " chosen noise variance: " << _noiseVariances[iMin] << " => SER = " << _SERs[iMin] << endl;
	cout << "errorsAutocorrelation = " << errorsAutocorrelation << endl;
#endif
	
	
	return 0.0;
}


