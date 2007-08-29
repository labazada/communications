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
#include "PSPvsPSPBasedSMCSystem.h"

PSPvsPSPBasedSMCSystem::PSPvsPSPBasedSMCSystem()
 : SMCSystem()
{
    nSurvivors = 2;
    adjustParticlesNumberFromSurvivors = true;

    powerProfile = new FlatPowerProfile(L,N,m,1.0);

	if(adjustParticlesNumberFromSurvivors)
	{
		nParticles = (int)pow((double)alphabet->Length(),N*(m-1))*nSurvivors;
        cout << "Number of particles adjusted to " << nParticles << endl;
    }

    kalmanEstimator = new KalmanEstimator(powerProfile->Means(),powerProfile->Variances(),N,ARcoefficients[0],ARvariance);

    ResamplingCriterion resamplingCriterion(resamplingRatio);
    withoutReplacementResamplingAlgorithm = new WithoutReplacementResamplingAlgorithm(resamplingCriterion);
	bestParticlesResamplingAlgorithm = new BestParticlesResamplingAlgorithm(resamplingCriterion);
}


PSPvsPSPBasedSMCSystem::~PSPvsPSPBasedSMCSystem()
{
	delete powerProfile;
	delete kalmanEstimator;

	delete withoutReplacementResamplingAlgorithm;
	delete bestParticlesResamplingAlgorithm;
}

void PSPvsPSPBasedSMCSystem::AddAlgorithms()
{
	algorithms.push_back(new PSPAlgorithm("PSPAlgorithm",*alphabet,L,N,lastSymbolVectorInstant,m,kalmanEstimator,preamble,d,lastSymbolVectorInstant+d,ARcoefficients[0],nSurvivors));

	algorithms.push_back(new PSPBasedSMCAlgorithm("PSP based SMC algorithm (deterministic)",*alphabet,L,N,lastSymbolVectorInstant,m,kalmanEstimator,preamble,d,nParticles,bestParticlesResamplingAlgorithm,powerProfile->Means(),powerProfile->Variances(),ARcoefficients[0]));

	algorithms.push_back(new PSPBasedSMCAlgorithm("PSP based SMC algorithm (without replacement resampling)",*alphabet,L,N,lastSymbolVectorInstant,m,kalmanEstimator,preamble,d,nParticles,withoutReplacementResamplingAlgorithm,powerProfile->Means(),powerProfile->Variances(),ARcoefficients[0]));

	algorithms.push_back(new PSPBasedSMCAlgorithm("PSP based SMC algorithm (residual resampling)",*alphabet,L,N,lastSymbolVectorInstant,m,kalmanEstimator,preamble,d,nParticles,algoritmoRemuestreo,powerProfile->Means(),powerProfile->Variances(),ARcoefficients[0]));

    algorithms.push_back(new ViterbiAlgorithm("Viterbi",*alphabet,L,N,lastSymbolVectorInstant,*(dynamic_cast<StillMemoryMIMOChannel *> (channel)),preamble,d));
}

void PSPvsPSPBasedSMCSystem::BuildChannel()
{
//     channel = new ARchannel(N,L,m,symbols.cols(),ARprocess(powerProfile->GenerateChannelMatrix(randomGenerator),ARcoefficients,ARvariance));
	channel = new BesselChannel(N,L,m,symbols.cols(),50,2e9,1.0/500.0e3,*powerProfile);
}

void PSPvsPSPBasedSMCSystem::BeforeEndingFrame(int iFrame)
{
    SMCSystem::BeforeEndingFrame(iFrame);
    Util::ScalarToOctaveFileStream(nSurvivors,"nSurvivors",f);
}
