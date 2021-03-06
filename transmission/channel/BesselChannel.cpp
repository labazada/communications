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
#include "BesselChannel.h"
#define EPSILON 1e-10

// #define DEBUG

BesselChannel::BesselChannel(uint nInputs, uint nOutputs, uint memory, uint length, double velocity, double carrierFrequency, double T, const DelayPowerProfile &powerProfile): StillMemoryMIMOChannel(nInputs, nOutputs, memory, length),_channelMatrices(length)
{
	if(powerProfile.memory()!=memory)
		throw RuntimeException("BesselChannel::BesselChannel: memory is wrong.");

	uint iRow,iCol,iTime;
	const double c = 3e8;

	double dopplerFrequency = velocity/(c/carrierFrequency);
	double normDopplerFrequency = T*dopplerFrequency;

	vector<double> autocorrelations(length);

	for(iTime=0;iTime<length;iTime++)
        _channelMatrices[iTime] = MatrixXd(nOutputs,nInputs*memory);   

	vector<double> tapsVariances = powerProfile.tapsPowers();
    vector<MatrixXd> Ls(memory);   
    MatrixXd covarianceMatrix(length,length);   

	for(uint iTap=0;iTap<tapsVariances.size();iTap++)
	{
		for(iTime=0;iTime<length;iTime++)
			autocorrelations[iTime] = tapsVariances[iTap]*jn(0,2.0*M_PI*normDopplerFrequency*double(iTime));
		
#ifdef DEBUG
		for(iTime=0;iTime<3;iTime++)
			cout << "autocorrelations[" << iTime << "] = " << autocorrelations[iTime] << endl;
#endif

		for(iRow=0;iRow<length;iRow++)
			for(iCol=iRow+1;iCol<length;iCol++)
				covarianceMatrix(iCol,iRow) = covarianceMatrix(iRow,iCol) = autocorrelations[iCol-iRow];

		// variances are set
		for(iRow=0;iRow<length;iRow++)
			covarianceMatrix(iRow,iRow) = tapsVariances[iTap] + EPSILON;

		// cholesky decomposition
        Ls[iTap] = Eigen::LLT<MatrixXd>(covarianceMatrix).matrixL();
	}

	for(iRow=0;iRow<nOutputs;iRow++)
		for(iCol=0;iCol<nInputs*memory;iCol++)
		{
            VectorXd sample = Ls[iCol/nInputs]*StatUtil::randnMatrix(length,1,0.0,1.0);

			for(iTime=0;iTime<length;iTime++)
// 				_channelMatrices[iTime](iRow,iCol) = sample(iTime);
				_channelMatrices[iTime](iRow,iCol) = sample(iTime) + DelayPowerProfile::getCoefficientsMean();
		}
}
