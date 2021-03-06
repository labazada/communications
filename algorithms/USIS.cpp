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
#include "USIS.h"

// #define DEBUG
// #define VIEJA

USIS::USIS(std::string name, Alphabet alphabet, uint L, uint Nr,uint N, uint iLastSymbolVectorToBeDetected, vector< ChannelMatrixEstimator * > channelEstimators,vector<LinearDetector *> linearDetectors, MatrixXd preamble, uint iFirstObservation, uint smoothingLag, uint nParticles, ResamplingAlgorithm* resamplingAlgorithm,ChannelOrderEstimator * channelOrderEstimator,double ARcoefficient,double samplingVariance,double ARprocessVariance): MultipleChannelEstimatorsPerParticleSMCAlgorithm(name, alphabet, L, Nr,N, iLastSymbolVectorToBeDetected, channelEstimators, preamble, iFirstObservation, smoothingLag, nParticles, resamplingAlgorithm),_linearDetectors(linearDetectors.size()),_channelOrderEstimator(channelOrderEstimator->clone()),_particleFilter(nParticles),_ARcoefficient(ARcoefficient),_samplingVariance(samplingVariance),_ARprocessVariance(ARprocessVariance),_processDoneExternally(false)
{
    if(linearDetectors.size()!=_candidateOrders.size())
        throw RuntimeException("USIS::USIS: number of detectors and number of channel matrix estimators (and candidate orders) are different.");

    for(uint iChannelOrder=0;iChannelOrder<_candidateOrders.size();iChannelOrder++)
        _linearDetectors[iChannelOrder] = linearDetectors[iChannelOrder]->clone();
}


USIS::~USIS()
{
    for(uint iChannelOrder=0;iChannelOrder<_candidateOrders.size();iChannelOrder++)
        delete _linearDetectors[iChannelOrder];

    delete _channelOrderEstimator;
}

void USIS::initializeParticles()
{
    // memory is reserved
    for(uint iParticle=0;iParticle<_particleFilter.capacity();iParticle++)
    {
        // a clone of each of the channel matrix estimators...
        vector<ChannelMatrixEstimator *> thisParticleChannelMatrixEstimators(_candidateOrders.size());

        //...and linear detectors is constructed
        vector<LinearDetector *> thisParticleLinearDetectors(_candidateOrders.size());

        for(uint iCandidateOrder=0;iCandidateOrder<_candidateOrders.size();iCandidateOrder++)
        {
            thisParticleChannelMatrixEstimators[iCandidateOrder] = _channelEstimators[iCandidateOrder]->clone();

            if(_randomParticlesInitilization)
                // the first matrix of the channel matrix estimator is initialized randomly
                thisParticleChannelMatrixEstimators[iCandidateOrder]->setFirstEstimatedChannelMatrix(Util::toMatrix(StatUtil::randnMatrix(_channelMeanVectors[iCandidateOrder],_channelCovariances[iCandidateOrder],StatUtil::_particlesInitializerRandomGenerator),rowwise,_nOutputs));
            
            thisParticleLinearDetectors[iCandidateOrder] = _linearDetectors[iCandidateOrder]->clone();
        }

        // ... and passed within a vector to each particle
        _particleFilter.addParticle(new ParticleWithChannelEstimationAndLinearDetectionAndChannelOrderEstimation(1.0/(double)_particleFilter.capacity(),_nInputs,_iLastSymbolVectorToBeDetected,thisParticleChannelMatrixEstimators,thisParticleLinearDetectors,_channelOrderEstimator->clone()));
    }
}

void USIS::process(const MatrixXd& observations, vector<double> noiseVariances)
{
    uint iParticle,iSmoothing,iRow,iSampledSymbol,iAlphabet,iSampled;
    uint iChannelOrder;
    uint m,d,Nm,nLinearFiltersNeeded,iLinearFilterNeeded;
    vector<vector<MatrixXd> > matricesToStack(_candidateOrders.size());
    uint _nInputs_maxOrder = _nInputs*_maxOrder;
    VectorXd sampledVector(_nInputs),sampledSmoothingVector(_nInputs_maxOrder);
    double proposal,s2q,sumProb,likelihoodsProd,sumLikelihoodsProd;
    vector<MatrixXd> channelOrderEstimatorNeededSampledMatrices(_candidateOrders.size());

    // each matrix in "symbolProb" contains the probabilities connected to a channelOrder: 
    // symbolProb(i,j) is the p(i-th symbol=alphabet[j]). They are initialized with zeros
    vector<MatrixXd> symbolProb(_candidateOrders.size(),MatrixXd::Zero(_nInputs_maxOrder,_alphabet.length()));

    // "overallSymbolProb" will combine the previous probabilities accordin to the APP of the channel order
    MatrixXd overallSymbolProb(_nInputs_maxOrder,_alphabet.length());

    // 2*_maxOrder-1 = m_{max} + d_{max}
    MatrixXd forWeightUpdateNeededSymbols(_nInputs,2*_maxOrder-1);

    // _maxOrder = d_{max} + 1
    MatrixXd noiseCovariances[_maxOrder];

    uint iObservationToBeProcessed = _startDetectionTime;
    while((iObservationToBeProcessed<_iLastSymbolVectorToBeDetected) && !_processDoneExternally)
    {
        // the stacked observations vector
        VectorXd stackedObservations = Util::toVector(observations.block(0,iObservationToBeProcessed,_nOutputs,_maxOrder),columnwise);

        // stacked noise covariance needs to be constructed
        MatrixXd stackedNoiseCovariance = MatrixXd::Zero(_nOutputs*_maxOrder,_nOutputs*_maxOrder);

        // the loop accomplishes 2 things:
        for(iSmoothing=0;iSmoothing<_maxOrder;iSmoothing++)
        {
            // i) construction of the stacked noise covariance
            for(iRow=0;iRow<_nOutputs;iRow++)
                stackedNoiseCovariance(iSmoothing*_nOutputs+iRow,iSmoothing*_nOutputs+iRow) = noiseVariances[iObservationToBeProcessed+iSmoothing];

            // ii) obtaining the noise covariances for each time instant from the variances
            noiseCovariances[iSmoothing] = noiseVariances[iObservationToBeProcessed+iSmoothing]*MatrixXd::Identity(_nOutputs,_nOutputs);
        }

        for(iParticle=0;iParticle<_particleFilter.capacity();iParticle++)
        {
            ParticleWithChannelEstimationAndLinearDetectionAndChannelOrderEstimation *processedParticle = dynamic_cast <ParticleWithChannelEstimationAndLinearDetectionAndChannelOrderEstimation *>(_particleFilter.getParticle(iParticle));

            for(iChannelOrder=0;iChannelOrder<_candidateOrders.size();iChannelOrder++)
            {
                m = _candidateOrders[iChannelOrder];
                d = m-1;
                Nm = _nInputs*m;
                matricesToStack[iChannelOrder] = vector<MatrixXd>(_maxOrder,MatrixXd(_nOutputs,Nm));

                // predicted channel matrices are sampled and stored in a vector in order to stack them
                matricesToStack[iChannelOrder][0] = _ARcoefficient*processedParticle->getChannelMatrixEstimator(iChannelOrder)->lastEstimatedChannelMatrix() + StatUtil::randnMatrix(_nOutputs,Nm,0.0,_samplingVariance);

                for(iSmoothing=1;iSmoothing<_maxOrder;iSmoothing++)
                    matricesToStack[iChannelOrder][iSmoothing] = _ARcoefficient*matricesToStack[iChannelOrder][iSmoothing-1] + StatUtil::randnMatrix(_nOutputs,Nm,0.0,_ARprocessVariance);

                // for sampling s_{t:t+d} we need to build
                nLinearFiltersNeeded = _maxOrder - m + 1; // linear filters

                // during the first iteration, we are going to use the real linear detector of this particle for this channel order
                LinearDetector *linearDetectorBeingProccessed = processedParticle->getLinearDetector(iChannelOrder);

                for(iLinearFilterNeeded=0;iLinearFilterNeeded<nLinearFiltersNeeded;iLinearFilterNeeded++)
                {
                    // matrices are stacked to give
					MatrixXd stackedChannelMatrix = TransmissionUtil::channelMatrices2stackedChannelMatrix(matricesToStack[iChannelOrder],m,iLinearFilterNeeded,d+iLinearFilterNeeded);

                    // the estimated stacked channel matrix is used to obtain soft estimations
                    // of the transmitted symbols
                    VectorXd softEstimations = linearDetectorBeingProccessed->detect(stackedObservations.segment(iLinearFilterNeeded*_nOutputs,_nOutputs*(d+1)),stackedChannelMatrix,stackedNoiseCovariance.block(iLinearFilterNeeded*_nOutputs,iLinearFilterNeeded*_nOutputs,_nOutputs*(d+1),_nOutputs*(d+1)));

                    MatrixXd filter = linearDetectorBeingProccessed->computedFilter();

                    // during the first iteration, we have used the real linear detector of this particle for this channel; during the remaining iterations we don't want the real linear detector to be modified
                    if(iLinearFilterNeeded==0)
                        linearDetectorBeingProccessed = linearDetectorBeingProccessed->clone();

                    // the sampling variance is computed
                    MatrixXd s2qAux = _alphabet.variance()*stackedChannelMatrix*stackedChannelMatrix.transpose() + stackedNoiseCovariance.block(iLinearFilterNeeded*_nOutputs,iLinearFilterNeeded*_nOutputs,_nOutputs*(d+1),_nOutputs*(d+1));

                    // the real symbol we are sampling (it depends on "iLinearFilterNeeded")
                    uint iSampledSymbolPos = iLinearFilterNeeded*_nInputs - 1;

                    // sampling
                    for(iSampledSymbol=0;iSampledSymbol<(_nInputs*(d+1));iSampledSymbol++)
                    {
                        iSampledSymbolPos++;
                        
                        s2q = _alphabet.variance()*(1.0 - 2.0*filter.col(iSampledSymbol).dot(stackedChannelMatrix.col(_nInputs*(m-1)+iSampledSymbol))) + filter.col(iSampledSymbol).dot(s2qAux*filter.col(iSampledSymbol));
                        
                        sumProb = 0.0;
                        // the probability for each posible symbol alphabet is computed
                        for(iAlphabet=0;iAlphabet<_alphabet.length();iAlphabet++)
                        {
                            symbolProb[iChannelOrder](iSampledSymbolPos,iAlphabet) = StatUtil::normalPdf(softEstimations(iSampledSymbol),_alphabet[iAlphabet],s2q);

                            // the computed pdf is accumulated for normalizing purposes
                            sumProb += symbolProb[iChannelOrder](iSampledSymbolPos,iAlphabet);
                        }

                        if(sumProb!=0)                  
                            for(iAlphabet=0;iAlphabet<_alphabet.length();iAlphabet++)
                                symbolProb[iChannelOrder](iSampledSymbolPos,iAlphabet) /= sumProb;
                        else
                        {                                
                            cout << "The sum of the probabilities is null." << endl;
                            for(iAlphabet=0;iAlphabet<_alphabet.length();iAlphabet++)
                                symbolProb[iChannelOrder](iSampledSymbolPos,iAlphabet) = 0.5;
                        }
                    }
                } // for(iLinearFilterNeeded=0;iLinearFilterNeeded<nLinearFiltersNeeded;iLinearFilterNeeded++)

                // the clone is dismissed
                delete linearDetectorBeingProccessed;

            } //for(iChannelOrder=0;iChannelOrder<_candidateOrders.size();iChannelOrder++)

            //the probabilities of the different channel orders are weighted according to the a posteriori probability of the channel order in the previous time instant
            overallSymbolProb = processedParticle->getChannelOrderEstimator()->getChannelOrderAPP(0)*symbolProb[0];
            for(iChannelOrder=1;iChannelOrder<_candidateOrders.size();iChannelOrder++)
                overallSymbolProb = overallSymbolProb + processedParticle->getChannelOrderEstimator()->getChannelOrderAPP(iChannelOrder)*symbolProb[iChannelOrder];

            proposal = 1.0;
            // the symbols are sampled from the above combined probabilities
            for(iSampledSymbol=0;iSampledSymbol<_nInputs*_maxOrder;iSampledSymbol++)
            {
                iSampled = StatUtil::discrete_rnd(overallSymbolProb.row(iSampledSymbol));

                sampledSmoothingVector(iSampledSymbol) = _alphabet[iSampled];

                proposal *= overallSymbolProb(iSampledSymbol,iSampled);
            }

            // sampled symbol vector is stored for the corresponding particle
            processedParticle->setSymbolVector(iObservationToBeProcessed,sampledSmoothingVector.head(_nInputs));

            sumLikelihoodsProd = 0.0;

            for(iChannelOrder=0;iChannelOrder<_candidateOrders.size();iChannelOrder++)
            {
                m = _candidateOrders[iChannelOrder];
                d = m-1;

                // all the symbol vectors involved in the smoothing are kept in "forWeightUpdateNeededSymbols":
                // i) the already known:
                forWeightUpdateNeededSymbols.block(0,0,_nInputs,m-1) = processedParticle->getSymbolVectors(iObservationToBeProcessed-m+1,iObservationToBeProcessed-1);

                // ii) the just sampled
                forWeightUpdateNeededSymbols.block(0,m-1,_nInputs,_maxOrder) = Util::toMatrix(sampledSmoothingVector,columnwise,_nInputs);

                likelihoodsProd = processedParticle->getChannelOrderEstimator()->getChannelOrderAPP(iChannelOrder);

                for(iSmoothing=0;iSmoothing<_maxOrder;iSmoothing++)
                {
                    VectorXd stackedSymbolVector = Util::toVector(forWeightUpdateNeededSymbols.block(0,iSmoothing,_nInputs,m),columnwise);
                    
                    likelihoodsProd *= StatUtil::normalPdf(observations.col(iObservationToBeProcessed+iSmoothing),matricesToStack[iChannelOrder][iSmoothing]*stackedSymbolVector,noiseCovariances[iSmoothing]);
                } // for(iSmoothing=0;iSmoothing<_maxOrder;iSmoothing++)
                
                // the estimation of the channel matrix is updated
                processedParticle->setChannelMatrix(iChannelOrder,iObservationToBeProcessed,
                (processedParticle->getChannelMatrixEstimator(iChannelOrder))->nextMatrix(observations.col(iObservationToBeProcessed),
                    forWeightUpdateNeededSymbols.block(0,0,_nInputs,m),noiseVariances[iObservationToBeProcessed]));
                
                // the computed likelihood is accumulated
                sumLikelihoodsProd += likelihoodsProd;

                // a vector of channel matrices is built to update the channel order estimator
                channelOrderEstimatorNeededSampledMatrices[iChannelOrder] = matricesToStack[iChannelOrder][0];
            } // for(iChannelOrder=0;iChannelOrder<_candidateOrders.size();iChannelOrder++)

            // the channel order estimator is updated
            processedParticle->getChannelOrderEstimator()->update(observations.col(iObservationToBeProcessed),channelOrderEstimatorNeededSampledMatrices,sampledSmoothingVector.head(_nInputs),noiseVariances[iObservationToBeProcessed]);

            // the weight is updated
            processedParticle->setWeight((sumLikelihoodsProd/proposal)*processedParticle->getWeight());

        } // for(iParticle=0;iParticle<_particleFilter.capacity();iParticle++)

        _particleFilter.normalizeWeights();

        // we find out which is the "best" particle at this time instant
        ParticleWithChannelEstimationAndLinearDetectionAndChannelOrderEstimation *bestParticle = dynamic_cast <ParticleWithChannelEstimationAndLinearDetectionAndChannelOrderEstimation *>(_particleFilter.getBestParticle());

        // its a posteriori channel order probabilities are stored
        for(uint i=0;i<_candidateOrders.size();i++)
            _channelOrderAPPs[0](i,iObservationToBeProcessed) = bestParticle->getChannelOrderEstimator()->getChannelOrderAPP(i);

        beforeResamplingProcess(iObservationToBeProcessed,observations,noiseVariances);

        // if it's not the last time instant
        if(iObservationToBeProcessed<(_iLastSymbolVectorToBeDetected-1))
            _resamplingAlgorithm->resampleWhenNecessary(&_particleFilter);

        iObservationToBeProcessed++;
    } // while((iObservationToBeProcessed<_iLastSymbolVectorToBeDetected) && !_processDoneExternally)
}

uint USIS::iBestChannelOrder(uint iBestParticle)
{
    ParticleWithChannelEstimationAndLinearDetectionAndChannelOrderEstimation *bestParticle = dynamic_cast <ParticleWithChannelEstimationAndLinearDetectionAndChannelOrderEstimation *>(_particleFilter.getParticle(iBestParticle));

    uint iMaxChannelOrderAPP = 0;
    double maxChannelOrderAPP = bestParticle->getChannelOrderEstimator()->getChannelOrderAPP(iMaxChannelOrderAPP);

    for(uint i=1;i<_candidateOrders.size();i++)
        if(bestParticle->getChannelOrderEstimator()->getChannelOrderAPP(i) > maxChannelOrderAPP)
        {
            maxChannelOrderAPP = bestParticle->getChannelOrderEstimator()->getChannelOrderAPP(i);
            iMaxChannelOrderAPP = i;
        }

    return iMaxChannelOrderAPP;
}

void USIS::beforeInitializingParticles(const MatrixXd &observations,vector<double> &noiseVariances,const MatrixXd &trainingSequence)
{
    for(uint iChannelOrder=0;iChannelOrder<_candidateOrders.size();iChannelOrder++)
        _linearDetectors[iChannelOrder]->stateStepsFromObservationsSequence(observations,_candidateOrders[iChannelOrder]-1,_preamble.cols(),_preamble.cols()+trainingSequence.cols());

    // the APP of the candidate channel orders are set accordingly
    _channelOrderAPPs[0].block(0,_preamble.cols(),_channelOrderAPPs[0].rows(),trainingSequence.cols()).setConstant(1.0/double(_candidateOrders.size()));
//     _channelOrderAPPs(tRange(),tRange(_preamble.cols(),_preamble.cols()+trainingSequence.cols()-1)) = 1.0/double(_candidateOrders.size());
}

void USIS::updateParticleChannelOrderEstimators(Particle *particle,const MatrixXd &observations,const std::vector<std::vector<MatrixXd> > &channelMatrices,vector<double> &noiseVariances,const MatrixXd &sequenceToProcess)
{
    ParticleWithChannelEstimationAndLinearDetectionAndChannelOrderEstimation* convertedParticle = dynamic_cast <ParticleWithChannelEstimationAndLinearDetectionAndChannelOrderEstimation *> (particle);

    convertedParticle->getChannelOrderEstimator()->computeProbabilities(observations,channelMatrices,noiseVariances,sequenceToProcess,_preamble.cols());
}
