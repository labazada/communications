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
#include "SMCAlgorithm.h"

SMCAlgorithm::SMCAlgorithm(string name, Alphabet alphabet,int L,int N, int K, ChannelMatrixEstimator *channelEstimator, tMatrix preamble,int smoothingLag,int nParticles,ResamplingCriterion resamplingCriterion,StdResamplingAlgorithm resamplingAlgorithm): KnownChannelOrderAlgorithm(name, alphabet, L, N, K, channelEstimator, preamble),
// _variables initialization
_d(smoothingLag),_allSymbolsRows(0,_N-1),_resamplingCriterion(resamplingCriterion),_resamplingAlgorithm(resamplingAlgorithm),_particleFilter(nParticles)
{
    // at first, we assume that all observations from the preamble need to be processed
    _startDetectionTime = _m - 1;
}

void SMCAlgorithm::InitializeParticles()
{
    // memory is reserved
    for(int iParticle=0;iParticle<_particleFilter.Nparticles();iParticle++)
    {
        _particleFilter.SetParticle(new ParticleWithChannelEstimation(1.0/(double)_particleFilter.Nparticles(),_N,_K,_channelEstimator->Clone()),iParticle);
    }
}

void SMCAlgorithm::Run(tMatrix observations,vector<double> noiseVariances)
{
    int nObservations = observations.cols();

    if(nObservations<(_startDetectionTime+1+_d))
        throw RuntimeException("SMCAlgorithm::Run: Not enough observations.");

    this->InitializeParticles();

    this->Process(observations,noiseVariances);
}

void SMCAlgorithm::Run(tMatrix observations,vector<double> noiseVariances, tMatrix trainingSequence)
{
    if(observations.rows()!=_L || trainingSequence.rows()!=_N)
        throw RuntimeException("Run: Observations matrix or training sequence dimensions are wrong.");

    int iParticle,j;

    // to process the training sequence, we need both the preamble and the symbol vectors related to it
    tMatrix preambleTrainingSequence = Util::Append(_preamble,trainingSequence);


    tRange rSymbolVectorsTrainingSequece(0,preambleTrainingSequence.cols()-1);

    vector<tMatrix> trainingSequenceChannelMatrices = ProcessTrainingSequence(observations,noiseVariances,trainingSequence);

    this->InitializeParticles();

    for(iParticle=0;iParticle<_particleFilter.Nparticles();iParticle++)
    {
		ParticleWithChannelEstimation *processedParticle = _particleFilter.GetParticle(iParticle);
	
        //the channel estimation given by the training sequence is copied into each particle...
        for(j=_m-1;j<trainingSequenceChannelMatrices.size();j++)
        {
            processedParticle->SetChannelMatrix(j,trainingSequenceChannelMatrices[j]);
        }

        //... the symbols are considered detected...
        processedParticle->SetSymbolVectors(rSymbolVectorsTrainingSequece,preambleTrainingSequence);
    }

    // the Process method must start in
    _startDetectionTime = trainingSequenceChannelMatrices.size();

    this->Process(observations,noiseVariances);
}

tMatrix SMCAlgorithm::GetDetectedSymbolVectors()
{
    // best particle is chosen
    int iBestParticle;
    Util::Max(_particleFilter.GetWeightsVector(),iBestParticle);

    return ((_particleFilter.GetParticle(iBestParticle))->GetAllSymbolVectors())(_allSymbolsRows,tRange(_m-1,_K-1));
}

vector<tMatrix> SMCAlgorithm::GetEstimatedChannelMatrices()
{
    vector<tMatrix> channelMatrices;
    channelMatrices.reserve(_K-_m+1);

    // best particle is chosen
    int iBestParticle;
    Util::Max(_particleFilter.GetWeightsVector(),iBestParticle);
    
    for(int i=_m-1;i<_K;i++)
        channelMatrices.push_back((_particleFilter.GetParticle(iBestParticle))->GetChannelMatrix(i));

    return channelMatrices;
}

void SMCAlgorithm::Resampling()
{
	tVector weigths = _particleFilter.GetWeightsVector();

    if(_resamplingCriterion.ResamplingNeeded(weigths))
    {
        vector<int> indexes = StatUtil::Discrete_rnd(_particleFilter.Nparticles(),weigths);
		_particleFilter.KeepParticles(indexes);
    }
}
