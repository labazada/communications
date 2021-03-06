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
#include "Rev2TVT2007System.h"

Rev2TVT2007System::Rev2TVT2007System(): TVT2007System()
{
//	uniqueRLSchannelEstimator.push_back(RLSchannelEstimators[candidateChannelOrders.size()-1]);
//	uniquekalmanChannelEstimator.push_back(kalmanChannelEstimators[candidateChannelOrders.size()-1]);
	uniquekalmanChannelEstimator.push_back(kalmanChannelEstimators[1]);
}

void Rev2TVT2007System::addAlgorithms()
{
	ChannelOrderEstimationSystem::addAlgorithms();

	_algorithms.push_back(new MLSDmAlgorithm("MKF MLSDmAlgorithm",*_alphabet,_L,_L,_N,_iLastSymbolVectorToBeDetected,uniquekalmanChannelEstimator,_preamble,_preamble.cols(),_d,_nParticles,bestParticlesResamplingAlgorithm,_ARcoefficients[0],_firstSampledChannelMatrixVariance,_ARvariance));

    _algorithms.push_back(new ViterbiAlgorithm("Viterbi",*_alphabet,_L,_L,_N,_iLastSymbolVectorToBeDetected,*(dynamic_cast<StillMemoryMIMOChannel *> (_channel)),_preamble,_d));
}

