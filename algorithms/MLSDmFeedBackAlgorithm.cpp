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
#include "MLSDmFeedBackAlgorithm.h"

MLSDmFeedBackAlgorithm::MLSDmFeedBackAlgorithm(std::string name, Alphabet alphabet, uint L, uint Nr,uint N, uint iLastSymbolVectorToBeDetected, vector< ChannelMatrixEstimator * > channelEstimators, MatrixXd preamble, uint iFirstObservation, uint smoothingLag, uint nParticles, ResamplingAlgorithm* resamplingAlgorithm, double ARcoefficient, double samplingVariance, double ARprocessVariance): MLSDmAlgorithm(name, alphabet, L, Nr,N, iLastSymbolVectorToBeDetected, channelEstimators, preamble, iFirstObservation, smoothingLag, nParticles, resamplingAlgorithm, ARcoefficient, samplingVariance, ARprocessVariance)
{
}

void MLSDmFeedBackAlgorithm::process(const MatrixXd& observations, vector<double> noiseVariances)
{
    MLSDmAlgorithm::process(observations, noiseVariances);

    ParticleWithChannelEstimationAndChannelOrderAPP *bestParticle = dynamic_cast<ParticleWithChannelEstimationAndChannelOrderAPP *> (_particleFilter->getBestParticle()->clone());

    uint nParticles = _particleFilter->capacity();

    delete _particleFilter;

    for(uint iObservationToBeProcessed=_iLastSymbolVectorToBeDetected+_d-2;iObservationToBeProcessed>=_startDetectionTime;iObservationToBeProcessed--)
    {
        for(uint iChannelOrder=0;iChannelOrder<bestParticle->nChannelMatrixEstimators();iChannelOrder++)
        {
            MatrixXd symbolVectors = bestParticle->getSymbolVectors(iObservationToBeProcessed-_candidateOrders[iChannelOrder]+1,iObservationToBeProcessed);
            bestParticle->getChannelMatrixEstimator(iChannelOrder)->nextMatrix(observations.col(iObservationToBeProcessed),symbolVectors,noiseVariances[iObservationToBeProcessed]);
        }
    }

    bestParticle->setWeight(1.0);

    _particleFilter = new ParticleFilter(nParticles);

    _particleFilter->addParticle(bestParticle);

    MLSDmAlgorithm::process(observations, noiseVariances);
}
