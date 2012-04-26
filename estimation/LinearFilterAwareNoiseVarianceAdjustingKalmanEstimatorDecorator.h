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


#ifndef LINEARFILTERAWARENOISEVARIANCEADJUSTINGKALMANESTIMATORDECORATOR_H
#define LINEARFILTERAWARENOISEVARIANCEADJUSTINGKALMANESTIMATORDECORATOR_H

#include <KalmanEstimatorDecorator.h>

#include <LinearDetector.h>

class LinearFilterAwareNoiseVarianceAdjustingKalmanEstimatorDecorator : public KalmanEstimatorDecorator
{
protected:
	LinearDetector *_linearDetector;
	double _symbolsVariance;
	
	double computeExtraVariance(double noiseVariance);
public:
	LinearFilterAwareNoiseVarianceAdjustingKalmanEstimatorDecorator(KalmanEstimator *kalmanEstimator, LinearDetector *linearDetector, double symbolsVariance);
	
    virtual MatrixXd nextMatrix(const VectorXd& observations, const MatrixXd& symbolsMatrix, double noiseVariance);
    virtual double likelihood(const VectorXd& observations, const MatrixXd symbolsMatrix, double noiseVariance);
    virtual LinearFilterAwareNoiseVarianceAdjustingKalmanEstimatorDecorator* clone() const;
	
	void setLinearDetector(LinearDetector * linearDetector) { _linearDetector = linearDetector; }
};

#endif // LINEARFILTERAWARENOISEVARIANCEADJUSTINGKALMANESTIMATORDECORATOR_H
