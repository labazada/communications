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
#include "LinearFilterBasedSMCAlgorithm.h"

#include <MMSEDetector.h>
#include <DecorrelatorDetector.h>
#include <bashcolors.h>

// #define DEBUG
// #include <realData.h>

LinearFilterBasedSMCAlgorithm::LinearFilterBasedSMCAlgorithm(std::string name, Alphabet alphabet,uint L,uint Nr,uint N, uint iLastSymbolVectorToBeDetected,uint m,  ChannelMatrixEstimator *channelEstimator,LinearDetector *linearDetector,MatrixXd preamble, uint SMCsmoothingLag, uint nParticles,ResamplingAlgorithm *resamplingAlgorithm,const MatrixXd &channelMatrixMean, const MatrixXd &channelMatrixVariances,double ARcoefficient,double samplingVariance,double ARprocessVariance, bool substractContributionFromKnownSymbols): SMCAlgorithm(name, alphabet, L, Nr,N, iLastSymbolVectorToBeDetected,m, channelEstimator, preamble, SMCsmoothingLag, nParticles, resamplingAlgorithm, channelMatrixMean, channelMatrixVariances)
,_linearDetector(linearDetector->clone()),_ARcoefficient(ARcoefficient),_samplingVariance(samplingVariance),_ARprocessVariance(ARprocessVariance),_subtractContributionFromKnownSymbols(substractContributionFromKnownSymbols)
{
}

LinearFilterBasedSMCAlgorithm::LinearFilterBasedSMCAlgorithm(std::string name, Alphabet alphabet,uint L,uint Nr,uint N, uint iLastSymbolVectorToBeDetected,uint m,MatrixXd preamble, uint SMCsmoothingLag, ParticleFilter *particleFilter, ResamplingAlgorithm *resamplingAlgorithm,double ARcoefficient,double samplingVariance, double ARprocessVariance, bool substractContributionFromKnownSymbols): SMCAlgorithm(name, alphabet, L, Nr,N, iLastSymbolVectorToBeDetected,m, preamble, SMCsmoothingLag, particleFilter, resamplingAlgorithm)
,_linearDetector(NULL),_ARcoefficient(ARcoefficient),_samplingVariance(samplingVariance),_ARprocessVariance(ARprocessVariance),_subtractContributionFromKnownSymbols(substractContributionFromKnownSymbols)
{
}

LinearFilterBasedSMCAlgorithm::~LinearFilterBasedSMCAlgorithm()
{
    delete _linearDetector;
}

void LinearFilterBasedSMCAlgorithm::initializeParticles()
{
    ChannelMatrixEstimator *channelMatrixEstimatorClone;
    
    // memory is reserved
    for(uint iParticle=0;iParticle<_particleFilter->capacity();iParticle++)
    {
        channelMatrixEstimatorClone = _channelEstimator->clone();
        if(_randomParticlesInitilization)
            channelMatrixEstimatorClone->setFirstEstimatedChannelMatrix(Util::toMatrix(StatUtil::randnMatrix(_channelMean,_channelCovariance),rowwise,_nOutputs));
		
        _particleFilter->addParticle(new ParticleWithChannelEstimationAndLinearDetection(1.0/(double)_particleFilter->capacity(),_nInputs,_iLastSymbolVectorToBeDetected,channelMatrixEstimatorClone,_linearDetector->clone()));

        _particleFilter->getParticle(iParticle)->setSymbolVectors(0,_preamble.cols(),_preamble);
    }
}

void LinearFilterBasedSMCAlgorithm::process(const MatrixXd &observations, vector<double> noiseVariances)
{
    uint iParticle,iSmoothing,iRow,iSampledSymbol,iAlphabet,iSampled;
    vector<MatrixXd> matricesToStack(_d+1,MatrixXd(_nOutputs,_nInputsXchannelOrder));
    VectorXd sampledVector(_nInputs),sampledSmoothingVector(_nInputs*(_d+1));
    double proposal,s2q,sumProb,likelihoodsProd;
    MatrixXd s2qAux(_nOutputs*(_d+1),_nOutputs*(_d+1)),symbolProb(_nInputs*(_d+1),_alphabet.length());
    VectorXd s2qAuxFilter(_nOutputs*(_d+1));
    MatrixXd forWeightUpdateNeededSymbols(_nInputs,_channelOrder+_d);
    VectorXd predictedNoiselessObservation(_nOutputs);

    if(_subtractContributionFromKnownSymbols)
		// the algorithm is supposed to operate substracting the contribution of the known symbols but this is not compatible with the current linear detector
		assert(_linearDetector->channelMatrixcols() == _nInputs*(_d+1));

#ifdef DEBUG
	VectorXd thisTimeInstantWeights(_particleFilter->capacity());
#endif
	
    for(uint iObservationToBeProcessed=_startDetectionTime;iObservationToBeProcessed<_iLastSymbolVectorToBeDetected;iObservationToBeProcessed++)
    {
#ifdef DEBUG
		cout << COLOR_INFO << "truly transmitted symbol vector" << endl << realSymbols->col(iObservationToBeProcessed) << COLOR_NORMAL << endl;
		getchar();
#endif
        // the stacked observations vector
        VectorXd stackedObservations = Util::toVector(observations.block(0,iObservationToBeProcessed,_nOutputs,_d+1),columnwise);

        // stacked noise covariance needs to be constructed
        MatrixXd stackedNoiseCovariance = MatrixXd::Zero(_nOutputs*(_d+1),_nOutputs*(_d+1));
        for(iSmoothing=0;iSmoothing<=_d;iSmoothing++)
            for(iRow=0;iRow<_nOutputs;iRow++)
                stackedNoiseCovariance((iSmoothing)*_nOutputs+iRow,(iSmoothing)*_nOutputs+iRow) = noiseVariances[iObservationToBeProcessed+iSmoothing];

        for(iParticle=0;iParticle<_particleFilter->capacity();iParticle++)
        {
#ifdef DEBUG
			cout << "--------- iParticle = " << iParticle << " ----------" <<  endl;
#endif
            // first of the predicted ones is obtained via a virtual method
            fillFirstEstimatedChannelMatrix(iParticle,matricesToStack[0]);

            for(iSmoothing=1;iSmoothing<=_d;iSmoothing++)
                // matricesToStack[iSmoothing] = _ARcoefficient * matricesToStack[iSmoothing-1] + rand(_nOutputs,_nInputsXchannelOrder)*_ARprocessVariance
                matricesToStack[iSmoothing] = _ARcoefficient*matricesToStack[iSmoothing-1] + StatUtil::randnMatrix(_nOutputs,_nInputsXchannelOrder,0.0,_ARprocessVariance);

            // matrices are stacked to give
            MatrixXd stackedChannelMatrix = channelMatrices2stackedChannelMatrix(matricesToStack);

            VectorXd softEstimations;

            // the estimated stacked channel matrix is used to obtain soft estimations of the transmitted symbols
            if(_subtractContributionFromKnownSymbols)
            {
                // transformed observations
                softEstimations =  dynamic_cast <WithLinearDetectionParticleAddon *> (_particleFilter->getParticle(iParticle))->getLinearDetector(_estimatorIndex)->
                detect(substractKnownSymbolsContribution(matricesToStack,_channelOrder,_d,stackedObservations,
														_particleFilter->getParticle(iParticle)->getSymbolVectors(iObservationToBeProcessed-_channelOrder+1,iObservationToBeProcessed-1)),
						stackedChannelMatrix.block(0,(_channelOrder-1)*_nInputs,stackedChannelMatrix.rows(),(_d+1)*_nInputs),stackedNoiseCovariance);
            } else
                softEstimations =  dynamic_cast <WithLinearDetectionParticleAddon *> (_particleFilter->getParticle(iParticle))->getLinearDetector(_estimatorIndex)->detect(stackedObservations,stackedChannelMatrix,stackedNoiseCovariance);

            // the evaluated proposal function (necessary for computing the weights) is initialized
            proposal = 1.0;

            // sampling
            for(iSampledSymbol=0;iSampledSymbol<(_nInputs*(_d+1));iSampledSymbol++)
            {
                s2q = dynamic_cast <WithLinearDetectionParticleAddon *> (_particleFilter->getParticle(iParticle))->getLinearDetector(_estimatorIndex)->nthSymbolVariance(iSampledSymbol,noiseVariances[iObservationToBeProcessed]);

                sumProb = 0.0;

                // the probability for each posible symbol alphabet is computed
                for(iAlphabet=0;iAlphabet<_alphabet.length();iAlphabet++)
                {
                    symbolProb(iSampledSymbol,iAlphabet) = StatUtil::normalPdf(softEstimations(iSampledSymbol),dynamic_cast <WithLinearDetectionParticleAddon *> (_particleFilter->getParticle(iParticle))->getLinearDetector(_estimatorIndex)->nthSymbolGain(iSampledSymbol)*_alphabet[iAlphabet],s2q);

                    // the computed pdf is accumulated for normalizing purposes
                    sumProb += symbolProb(iSampledSymbol,iAlphabet);
                }

                if(sumProb!=0)
                    for(iAlphabet=0;iAlphabet<_alphabet.length();iAlphabet++)
                        symbolProb(iSampledSymbol,iAlphabet) /= sumProb;
                else
                {
                    cout << "LinearFilterBasedSMCAlgorithm::process: " << COLOR_ALERT << "the sum of the probabilities is null." << COLOR_NORMAL << endl;
                    for(iAlphabet=0;iAlphabet<_alphabet.length();iAlphabet++)
                        symbolProb(iSampledSymbol,iAlphabet) = 1.0/double(_alphabet.length());
                }

                iSampled = StatUtil::discrete_rnd(symbolProb.row(iSampledSymbol));
                sampledSmoothingVector(iSampledSymbol) = _alphabet[iSampled];

                proposal *= symbolProb(iSampledSymbol,iSampled);
            }

#ifdef DEBUG
			cout << "sampledSmoothingVector.head(_nInputs): " << endl << sampledSmoothingVector.head(_nInputs) << endl;
#endif
            // sampled symbol vector is stored for the corresponding particle
            _particleFilter->getParticle(iParticle)->setSymbolVector(iObservationToBeProcessed,sampledSmoothingVector.head(_nInputs));

            // all the symbol vectors involved in the smoothing are kept in "forWeightUpdateNeededSymbols"
            // i) the already known:
            forWeightUpdateNeededSymbols.block(0,0,_nInputs,_channelOrder-1) = _particleFilter->getParticle(iParticle)->getSymbolVectors(iObservationToBeProcessed-_channelOrder+1,iObservationToBeProcessed-1);

            // ii) the just sampled
            forWeightUpdateNeededSymbols.block(0,_channelOrder-1,_nInputs,_d+1) = Util::toMatrix(sampledSmoothingVector,columnwise,_nInputs);

            likelihoodsProd = smoothedLikelihood(matricesToStack,forWeightUpdateNeededSymbols,iObservationToBeProcessed,observations,noiseVariances);

            // the weight is updated
            _particleFilter->getParticle(iParticle)->setWeight((likelihoodsProd/proposal)*_particleFilter->getParticle(iParticle)->getWeight());
			
#ifdef DEBUG
			thisTimeInstantWeights(iParticle) = (likelihoodsProd/proposal);
#endif

            // and the estimation of the channel matrix
            dynamic_cast <WithChannelEstimationParticleAddon *> (_particleFilter->getParticle(iParticle))->setChannelMatrix(_estimatorIndex,iObservationToBeProcessed,
                                                dynamic_cast <WithChannelEstimationParticleAddon *> (_particleFilter->getParticle(iParticle))->getChannelMatrixEstimator(_estimatorIndex)->nextMatrix(observations.col(iObservationToBeProcessed),
                                                    forWeightUpdateNeededSymbols.block(0,0,_nInputs,_channelOrder),noiseVariances[iObservationToBeProcessed]));            
        } // for(iParticle=0;iParticle<_particleFilter->capacity();iParticle++)
        
#ifdef DEBUG
			cout << "weight updates: " << endl;
			for(uint i=0;i<_particleFilter->capacity();i++)
				cout << "particle " << i << " weight = " << thisTimeInstantWeights(i) << endl;
#endif

        _particleFilter->normalizeWeights();

        // if it's not the last time instant
        if(iObservationToBeProcessed<(_iLastSymbolVectorToBeDetected-1))
            _resamplingAlgorithm->resampleWhenNecessary(_particleFilter);

    } // for(uint iObservationToBeProcessed=_startDetectionTime;iObservationToBeProcessed<_iLastSymbolVectorToBeDetected;iObservationToBeProcessed++)
}

void LinearFilterBasedSMCAlgorithm::beforeInitializingParticles(const MatrixXd &observations, const MatrixXd &trainingSequence)
{
    _linearDetector->stateStepsFromObservationsSequence(observations,_d,_preamble.cols(),_preamble.cols()+trainingSequence.cols());
}
