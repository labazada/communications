/***************************************************************************
 *   Copyright (C) 2006 by Manu   *
 *   manu@rustneversleeps   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#include "KnownSymbolsKalmanEstimator.h"

// #define DEBUG

KnownSymbolsKalmanEstimator::KnownSymbolsKalmanEstimator(const MatrixXd& initialEstimation, const MatrixXd& variances, uint N, vector<double> ARcoefficients, double ARvariance,const MatrixXd &symbols,uint startDetectionTime): KalmanEstimator(initialEstimation, variances, N, ARcoefficients, ARvariance),_presentTime(startDetectionTime),_symbols(symbols)
{
}

MatrixXd KnownSymbolsKalmanEstimator::nextMatrix(const VectorXd &observations, const MatrixXd &symbolsMatrix, double noiseVariance)
{
    _presentTime++;
	
	// if an all-zeros symbols matrix is passed (allegedly intentionally) that is fed into the Kalman Filter (rather than the true transmitted symbols)
	if(symbolsMatrix.isConstant(0.0))
		return KalmanEstimator::nextMatrix(observations, symbolsMatrix, noiseVariance);
	else
		return KalmanEstimator::nextMatrix(observations, _symbols.block(0,_presentTime-_channelOrder,_nInputs,_channelOrder), noiseVariance);
}

KnownSymbolsKalmanEstimator* KnownSymbolsKalmanEstimator::clone() const
{
	return new KnownSymbolsKalmanEstimator(*this);
}
