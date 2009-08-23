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
#include "USIS2SISAlgorithm.h"

// #define DEBUG2

USIS2SISAlgorithm::USIS2SISAlgorithm(string name, Alphabet alphabet, int L, int Nr,int N, int iLastSymbolVectorToBeDetected, vector< ChannelMatrixEstimator * > channelEstimators, vector< LinearDetector * > linearDetectors, tMatrix preamble, int iFirstObservation, int smoothingLag, int nParticles, ResamplingAlgorithm* resamplingAlgorithm, ChannelOrderEstimator* channelOrderEstimator, double ARcoefficient, double samplingVariance, double ARprocessVariance, TransitionCriterion *transitionCriterion): USIS(name, alphabet, L, Nr,N, iLastSymbolVectorToBeDetected, channelEstimators, linearDetectors, preamble, iFirstObservation, smoothingLag, nParticles, resamplingAlgorithm, channelOrderEstimator, ARcoefficient, samplingVariance, ARprocessVariance),_transitionCriterion(transitionCriterion)
{
}


USIS2SISAlgorithm::~USIS2SISAlgorithm()
{
}


void USIS2SISAlgorithm::BeforeResamplingProcess(int iProcessedObservation, const tMatrix& observations, const vector< double > &noiseVariances)
{
    tVector _weightedChannelOrderAPPs(_candidateOrders.size());
    _weightedChannelOrderAPPs = 0.0;

    for(int iParticle=0;iParticle<_particleFilter.capacity();iParticle++)
    {
        ParticleWithChannelEstimationAndLinearDetectionAndChannelOrderEstimation *processedParticle = dynamic_cast <ParticleWithChannelEstimationAndLinearDetectionAndChannelOrderEstimation *>(_particleFilter.getParticle(iParticle));

        tVector particleChannelOrderAPPs = processedParticle->GetChannelOrderEstimator()->getChannelOrderAPPsVector();
        Util::add(_weightedChannelOrderAPPs,particleChannelOrderAPPs,_weightedChannelOrderAPPs,1.0,processedParticle->getWeight());
    }

    #ifdef DEBUG
        cout << "Probabilidades globales para los órdenes de canal:" << endl << _weightedChannelOrderAPPs << endl;
    #endif

    // the maximum probability is obtained
    int iMax;
    Util::max(_weightedChannelOrderAPPs,iMax);

    #ifdef DEBUG
        cout << "La probabilidad más alta es la " << iMax << endl;
        cout << "Pasa del umbral: " << (_weightedChannelOrderAPPs(iMax)>_threshold) << endl;
    #endif

    // if the transition criterion is satisfied
    if(_transitionCriterion->MakeTransition(_weightedChannelOrderAPPs))
    {
        #ifdef DEBUG
            cout << "Pasa del umbral: " << endl;
            cout << "La probabilidad más alta es la " << iMax << endl;
        #endif

        LinearFilterBasedSMCAlgorithm knownChannelOrderAlgorithm(_name,_alphabet,_nOutputs,_nOutputs,_nInputs,_iLastSymbolVectorToBeDetected,_candidateOrders[iMax],_preamble,_candidateOrders[iMax]-1,&_particleFilter,_resamplingAlgorithm,_ARcoefficient,_samplingVariance,_ARprocessVariance);

        knownChannelOrderAlgorithm.setEstimatorIndex(iMax);
        knownChannelOrderAlgorithm.runFrom(iProcessedObservation,observations,noiseVariances);
        _processDoneExternally = true;

        // the APP of the selected channel order is set to 1.0
        _channelOrderAPPs(tRange(iMax,iMax),tRange(iProcessedObservation,_iLastSymbolVectorToBeDetected-1)) = 1.0;

        return;
    }
}

