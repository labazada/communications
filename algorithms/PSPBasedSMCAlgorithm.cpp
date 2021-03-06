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
#include "PSPBasedSMCAlgorithm.h"

// #define DEBUG
// #define IMPORT_REAL_DATA

#include <defines.h>


#include <KalmanEstimator.h>

#ifdef IMPORT_REAL_DATA
	extern MIMOChannel *realChannel;
	extern MatrixXd *realSymbols;
	extern Noise *realNoise;
#endif

PSPBasedSMCAlgorithm::PSPBasedSMCAlgorithm(std::string name, Alphabet alphabet, uint L, uint Nr,uint N, uint iLastSymbolVectorToBeDetected, uint m, ChannelMatrixEstimator* channelEstimator, MatrixXd preamble, uint smoothingLag, uint nParticles, ResamplingAlgorithm* resamplingAlgorithm, const MatrixXd& channelMatrixMean, const MatrixXd& channelMatrixVariances): SMCAlgorithm(name, alphabet, L, Nr,N, iLastSymbolVectorToBeDetected, m, channelEstimator, preamble, smoothingLag, nParticles, resamplingAlgorithm, channelMatrixMean, channelMatrixVariances)
{
}

void PSPBasedSMCAlgorithm::initializeParticles()
{
	// we begin with only one particle
	_particleFilter->addParticle(new ParticleWithChannelEstimation(1.0,_nInputs,_iLastSymbolVectorToBeDetected+_d,_channelEstimator->clone()));

    _particleFilter->getParticle(0)->setSymbolVectors(0,_preamble.cols(),_preamble);
}

void PSPBasedSMCAlgorithm::process(const MatrixXd& observations, vector< double > noiseVariances)
{
    uint nSymbolVectors = uint(pow((double)_alphabet.length(),(double)_nInputs));
    vector<tSymbol> testedVector(_nInputs);
    VectorXd computedObservations(_nOutputs);
    double normConst;

    tParticleCandidate *particleCandidates = new tParticleCandidate[_particleFilter->capacity()*nSymbolVectors] ;

    // "symbolVectorsMatrix" will contain all the symbols involved in the current observation
    MatrixXd symbolVectorsMatrix(_nInputs,_channelOrder);
    VectorXd symbolsVector(_nInputsXchannelOrder);

    uint iLastSymbolVectorStartWithinStackedVector = _nInputsXchannelOrder - _nInputs;

    // at first, there is only one particle
    for(uint iObservationToBeProcessed=_startDetectionTime;iObservationToBeProcessed<_iLastSymbolVectorToBeDetected+_d;iObservationToBeProcessed++)
    {
	  
#ifdef DEBUG
		cout << "================== iObservationToBeProcessed = " << iObservationToBeProcessed << " ===================" << endl;
#endif
        // it keeps track of the place where a new tParticleCandidate will be stored within the vector
        uint iCandidate = 0;

        normConst = 0.0;

        // the candidates from all the particles are generated
        for(uint iParticle=0;iParticle<_particleFilter->nParticles();iParticle++)
        {
#ifdef DEBUG
			cout << "***** Particle = " << iParticle << " *****" << endl;
#endif
            ParticleWithChannelEstimation *processedParticle = dynamic_cast<ParticleWithChannelEstimation *>(_particleFilter->getParticle(iParticle));

            symbolVectorsMatrix.block(0,0,_nInputs,_channelOrder-1) = processedParticle->getSymbolVectors(iObservationToBeProcessed-_channelOrder+1,iObservationToBeProcessed-1);

            symbolsVector = Util::toVector(symbolVectorsMatrix,columnwise);

            MatrixXd estimatedChannelMatrix = processedParticle->getChannelMatrixEstimator(_estimatorIndex)->lastEstimatedChannelMatrix();
			
#ifdef DEBUG
			cout << "last estimated channel matrix is" << endl << estimatedChannelMatrix << endl;
			cout << "real channel" << endl << realChannel->at(iObservationToBeProcessed) << endl;
#endif

			// FIXME: when the channel estimator is not a KF, the estimation it returns must be multiplied by the AR coefficient
//             estimatedChannelMatrix *= _ARcoefficient;

            for(uint iTestedVector=0;iTestedVector<nSymbolVectors;iTestedVector++)
            {
                // the corresponding testing vector is generated from the index
                _alphabet.int2symbolsArray(iTestedVector,testedVector);

                // current tested vector is copied in the m-th position
                for(uint k=0;k<_nInputs;k++)
                    symbolVectorsMatrix(k,_channelOrder-1) = symbolsVector(iLastSymbolVectorStartWithinStackedVector+k) = testedVector[k];

                particleCandidates[iCandidate].fromParticle = iParticle;
                particleCandidates[iCandidate].symbolVectorsMatrix = symbolVectorsMatrix;
                particleCandidates[iCandidate].weight = processedParticle->getWeight()*StatUtil::normalPdf(observations.col(iObservationToBeProcessed),estimatedChannelMatrix*symbolsVector,noiseVariances[iObservationToBeProcessed]);
// 				particleCandidates[iCandidate].weight = processedParticle->getWeight()*
// 					processedParticle->getChannelMatrixEstimator()->likelihood(
// 					observations.col(iObservationToBeProcessed),
// 					symbolsVector,
// 					noiseVariances[iObservationToBeProcessed]
// 					);
                normConst += particleCandidates[iCandidate].weight;

                iCandidate++;
            } // for(uint iTestedVector=0;iTestedVector<nSymbolVectors;iTestedVector++)

#ifdef DEBUG
			getchar();
#endif

        } // for(uint iParticle=0;iParticle<_particleFilter->nParticles();iParticle++)

        // a vector of size the number of generated candidates is declared...
        VectorXd weights(iCandidate);

        // ...to store their weights
        for(uint i=0;i<iCandidate;i++)
            weights(i) = particleCandidates[i].weight/normConst;

        // the candidates that are going to give particles are selected
        vector<uint> indexesSelectedCandidates = _resamplingAlgorithm->obtainIndexes(_particleFilter->capacity(),weights);

        // every survivor candidate is associated with an old particle
        vector<uint> indexesParticles(indexesSelectedCandidates.size());
        for(uint i=0;i<indexesSelectedCandidates.size();i++)
            indexesParticles[i] = particleCandidates[indexesSelectedCandidates[i]].fromParticle;

        // the chosen particles are kept without modification (yet)
        _particleFilter->keepParticles(indexesParticles);

        // every surviving particle is modified according to what it says its corresponding candidate
        for(uint iParticle=0;iParticle<_particleFilter->nParticles();iParticle++)
        {
            ParticleWithChannelEstimation *processedParticle = dynamic_cast<ParticleWithChannelEstimation *>(_particleFilter->getParticle(iParticle));

            // sampled symbols are copied into the corresponding particle
            processedParticle->setSymbolVector(iObservationToBeProcessed,particleCandidates[indexesSelectedCandidates[iParticle]].symbolVectorsMatrix.col(_channelOrder-1));

            // channel matrix is estimated by means of the particle channel estimator
            processedParticle->setChannelMatrix(_estimatorIndex,iObservationToBeProcessed,processedParticle->getChannelMatrixEstimator(_estimatorIndex)->nextMatrix(observations.col(iObservationToBeProcessed),particleCandidates[indexesSelectedCandidates[iParticle]].symbolVectorsMatrix,noiseVariances[iObservationToBeProcessed]));

            processedParticle->setWeight(particleCandidates[indexesSelectedCandidates[iParticle]].weight);
        } // for(uint iParticle=0;iParticle<_particleFilter->nParticles();iParticle++)

        _particleFilter->normalizeWeights();
    } // for(uint iObservationToBeProcessed=_startDetectionTime;iObservationToBeProcessed<_iLastSymbolVectorToBeDetected+_d;iObservationToBeProcessed++)

    delete[] particleCandidates;
}

std::vector<std::vector<bool> > PSPBasedSMCAlgorithm::imposeFixedNumberOfSurvivorsPerState(const tParticleCandidate *particleCandidates,uint nCandidates)
{
	uint nStates = uint(pow(double(_alphabet.length()),double(_nInputs*(_channelOrder-1))));
	
	std::vector<std::vector<bool> > statesMasks(nStates,std::vector<bool>(nCandidates,false));
	
	for(uint iState=0;iState<nStates;iState++)
	{
		MatrixXd state = _alphabet.int2eigenMatrix(iState,_nInputs,_channelOrder-1);
		
		for(uint iCandidate=0;iCandidate<nCandidates;iCandidate++)
			if(particleCandidates[iCandidate].symbolVectorsMatrix.block(0,particleCandidates[iCandidate].symbolVectorsMatrix.cols()-_channelOrder+1,_nInputs,_channelOrder-1) == state)
				statesMasks[iState][iCandidate] = true;
	}
	
	return statesMasks;
}