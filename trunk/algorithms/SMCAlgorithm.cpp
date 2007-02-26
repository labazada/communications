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

SMCAlgorithm::SMCAlgorithm(string name, Alphabet alphabet,int L,int N, int K,int m, ChannelMatrixEstimator *channelEstimator, tMatrix preamble,int smoothingLag,int nParticles,ResamplingAlgorithm *resamplingAlgorithm): KnownChannelOrderAlgorithm(name, alphabet, L, N, K,m, channelEstimator, preamble),
// _variables initialization
_particleFilter(new ParticleFilter(nParticles)),_particleFilterNeedToBeDeleted(true),_resamplingAlgorithm(resamplingAlgorithm),_d(smoothingLag),_allSymbolsRows(0,_N-1),_estimatorIndex(0)
{
    // at first, we assume that all observations from the preamble need to be processed
    _startDetectionTime = _preamble.cols();
}

SMCAlgorithm::SMCAlgorithm(string name, Alphabet alphabet,int L,int N, int K,int m, tMatrix preamble,int smoothingLag,ParticleFilter *particleFilter,ResamplingAlgorithm *resamplingAlgorithm): KnownChannelOrderAlgorithm(name, alphabet, L, N, K,m, preamble),
// _variables initialization
_particleFilter(particleFilter),_particleFilterNeedToBeDeleted(false),_resamplingAlgorithm(resamplingAlgorithm),_d(smoothingLag),_allSymbolsRows(0,_N-1),_estimatorIndex(0)
{
    // at first, we assume that all observations from the preamble need to be processed
    _startDetectionTime = _preamble.cols();
}

SMCAlgorithm::~SMCAlgorithm()
{
	if(_particleFilterNeedToBeDeleted)
		delete _particleFilter;
}

void SMCAlgorithm::SetEstimatorIndex(int n)
{
	if(_particleFilter==NULL)
		throw RuntimeException("SMCAlgorithm::SetEstimatorIndex: the particle filter is not set.");

	if(n>=_particleFilter->GetParticle(0)->NchannelMatrixEstimators())
		throw RuntimeException("SMCAlgorithm::SetEstimatorIndex: index is out of range.");

	_estimatorIndex = n;
}

void SMCAlgorithm::InitializeParticles()
{
    // memory is reserved
    for(int iParticle=0;iParticle<_particleFilter->Nparticles();iParticle++)
    {
        _particleFilter->SetParticle(new ParticleWithChannelEstimation(1.0/(double)_particleFilter->Nparticles(),_N,_K,_channelEstimator->Clone()),iParticle);
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

void SMCAlgorithm::RunFrom(int n,tMatrix observations,vector<double> noiseVariances)
{
    int nObservations = observations.cols();
	_startDetectionTime = n;

    if(nObservations<(_startDetectionTime+1+_d))
        throw RuntimeException("SMCAlgorithm::RunFrom: Not enough observations.");

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

    for(iParticle=0;iParticle<_particleFilter->Nparticles();iParticle++)
    {
		ParticleWithChannelEstimation *processedParticle = _particleFilter->GetParticle(iParticle);

        //the channel estimation given by the training sequence is copied into each particle...
        for(j=_preamble.cols();j<trainingSequenceChannelMatrices.size();j++)
        {
            processedParticle->SetChannelMatrix(_estimatorIndex,j,trainingSequenceChannelMatrices[j]);
        }

        //... the symbols are considered detected...
        processedParticle->SetSymbolVectors(rSymbolVectorsTrainingSequece,preambleTrainingSequence);
    }

    // the Process method must start in
    _startDetectionTime = _preamble.cols() + trainingSequence.cols();
//     _startDetectionTime = trainingSequenceChannelMatrices.size();

    this->Process(observations,noiseVariances);
}

tMatrix SMCAlgorithm::GetDetectedSymbolVectors()
{
    // best particle is chosen
    int iBestParticle;
    Util::Max(_particleFilter->GetWeightsVector(),iBestParticle);

    return ((_particleFilter->GetParticle(iBestParticle))->GetAllSymbolVectors())(_allSymbolsRows,tRange(_preamble.cols(),_K-1));
}

vector<tMatrix> SMCAlgorithm::GetEstimatedChannelMatrices()
{
    vector<tMatrix> channelMatrices;
    channelMatrices.reserve(_K-_preamble.cols());

    // best particle is chosen
    int iBestParticle;
    Util::Max(_particleFilter->GetWeightsVector(),iBestParticle);

    for(int i=_preamble.cols();i<_K;i++)
        channelMatrices.push_back((_particleFilter->GetParticle(iBestParticle))->GetChannelMatrix(_estimatorIndex,i));

    return channelMatrices;
}
