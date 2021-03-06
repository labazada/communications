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
#include "TVT2007System.h"

// #define DEBUG

TVT2007System::TVT2007System()
 : ChannelOrderEstimationSystem()
{
    nSurvivors = 12;
	adjustSurvivorsFromParticlesNumber = true;
    adjustParticlesNumberFromSurvivors = false;

    forgettingFactor = 0.99;
    forgettingFactorDetector = 0.95;

    _powerProfile = new FlatPowerProfile(_L,_N,_m,1.0);
// 	powerProfile = new ExponentialPowerProfile(L,N,m,1.8e-6,1.0/500.0e3);

	adjustParticlesSurvivors(_nParticles,nSurvivors,adjustParticlesNumberFromSurvivors,adjustSurvivorsFromParticlesNumber);

	rmmseDetector = new RMMSEDetector(_L*(_d+1),_N*(_m+_d),_alphabet->variance(),forgettingFactorDetector,_N*(_d+1));

	rlsEstimator = new RLSEstimator(_powerProfile->means(),_N,forgettingFactor);
	for(uint iChannelOrder=0;iChannelOrder<_candidateChannelOrders.size();iChannelOrder++)
	{
		RLSchannelEstimators.push_back(new RLSEstimator(_channelOrderCoefficientsMeans[iChannelOrder],_N,forgettingFactor));
		kalmanChannelEstimators.push_back(new KalmanEstimator(_channelOrderCoefficientsMeans[iChannelOrder],_channelOrderCoefficientsVariances[iChannelOrder],_N,_ARcoefficients,_ARvariance));
		noForgetRLSchannelEstimators.push_back(new RLSEstimator(_channelOrderCoefficientsMeans[iChannelOrder],_N,1.0));
	}

    ResamplingCriterion resamplingCriterion(_resamplingRatio);
    withoutReplacementResamplingAlgorithm = new WithoutReplacementResamplingAlgorithm(resamplingCriterion);
	bestParticlesResamplingAlgorithm = new BestParticlesResamplingAlgorithm(resamplingCriterion);

    kalmanEstimator = new KalmanEstimator(_powerProfile->means(),_powerProfile->variances(),_N,_ARcoefficients,_ARvariance);
}


TVT2007System::~TVT2007System()
{
	delete _powerProfile;

	delete rmmseDetector;

	delete rlsEstimator;
	for(uint iChannelOrder=0;iChannelOrder<_candidateChannelOrders.size();iChannelOrder++)
	{
		delete RLSchannelEstimators[iChannelOrder];
		delete kalmanChannelEstimators[iChannelOrder];
		delete noForgetRLSchannelEstimators[iChannelOrder];
	}

	delete withoutReplacementResamplingAlgorithm;
	delete bestParticlesResamplingAlgorithm;

	delete kalmanEstimator;
}

void TVT2007System::addAlgorithms()
{
	ChannelOrderEstimationSystem::addAlgorithms();

	_algorithms.push_back(new CMEBasedAlgorithm("CME based algorithm (RLS no forget)",*_alphabet,_L,_L,_N,_iLastSymbolVectorToBeDetected,noForgetRLSchannelEstimators,_preamble,_preamble.cols(),_symbols));

	_algorithms.push_back(new TimeVaryingChannelCMEbasedAlgorithm("TimeVaryingChannelCMEbasedAlgorithm",*_alphabet,_L,_L,_N,_iLastSymbolVectorToBeDetected,noForgetRLSchannelEstimators,_preamble,_preamble.cols(),_symbols));

	_algorithms.push_back(new MLSDmAlgorithm("MLSDmAlgorithm",*_alphabet,_L,_L,_N,_iLastSymbolVectorToBeDetected,RLSchannelEstimators,_preamble,_preamble.cols(),_d,_nParticles,bestParticlesResamplingAlgorithm,_ARcoefficients[0],_firstSampledChannelMatrixVariance,_ARvariance));

	_algorithms.push_back(new MLSDmAlgorithm("MKF MLSDmAlgorithm",*_alphabet,_L,_L,_N,_iLastSymbolVectorToBeDetected,kalmanChannelEstimators,_preamble,_preamble.cols(),_d,_nParticles,bestParticlesResamplingAlgorithm,_ARcoefficients[0],_firstSampledChannelMatrixVariance,_ARvariance));

// 	algorithms.push_back(new PSPAlgorithm("PSPAlgorithm",*alphabet,L,L,N,iLastSymbolVectorToBeDetected,m,kalmanEstimator,preamble,d,iLastSymbolVectorToBeDetected+d,ARcoefficients[0],nSurvivors));

//     algorithms.push_back(new ViterbiAlgorithm("Viterbi",*alphabet,L,L,N,iLastSymbolVectorToBeDetected,*(dynamic_cast<StillMemoryMIMOChannel *> (channel)),preamble,d));
}

void TVT2007System::saveFrameResults()
{
    ChannelOrderEstimationSystem::saveFrameResults();
    Octave::toOctaveFileStream(nSurvivors,"nSurvivors",_f);
	Octave::toOctaveFileStream(forgettingFactor,"forgettingFactor",_f);
	Octave::toOctaveFileStream(forgettingFactorDetector,"forgettingFactorDetector",_f);
}
