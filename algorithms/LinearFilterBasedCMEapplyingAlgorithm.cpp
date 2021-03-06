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
#include "LinearFilterBasedCMEapplyingAlgorithm.h"

LinearFilterBasedCMEapplyingAlgorithm::LinearFilterBasedCMEapplyingAlgorithm(std::string name, Alphabet alphabet, uint L, uint Nr,uint N, uint iLastSymbolVectorToBeDetected, vector< ChannelMatrixEstimator * > channelEstimators, MatrixXd preamble, vector< LinearDetector *> linearDetectors, double ARcoefficient, bool substractContributionFromKnownSymbols): CMEapplyingAlgorithm(name, alphabet, L, Nr,N, iLastSymbolVectorToBeDetected, channelEstimators, preamble),_algorithmAlreadyExecuted(channelEstimators.size(),false)
{
    for(uint iChannelOrder=0;iChannelOrder<_candidateOrders.size();iChannelOrder++)
        algorithms.push_back(new LinearFilterBasedAlgorithm("foo",alphabet,L,Nr,N,iLastSymbolVectorToBeDetected,_candidateOrders[iChannelOrder],channelEstimators[iChannelOrder],preamble,_candidateOrders[iChannelOrder]-1,linearDetectors[iChannelOrder],std::vector<double>(1,ARcoefficient),substractContributionFromKnownSymbols));
}


LinearFilterBasedCMEapplyingAlgorithm::~LinearFilterBasedCMEapplyingAlgorithm()
{
    for(uint iChannelOrder=0;iChannelOrder<_candidateOrders.size();iChannelOrder++)
        delete algorithms[iChannelOrder];
}

std::vector<MatrixXd> LinearFilterBasedCMEapplyingAlgorithm::estimatedChannelMatricesForChannelOrder(uint iChannelOrder, const MatrixXd& observations, const vector< double >& noiseVariances,const MatrixXd& trainingSequence)
{
    if(!_algorithmAlreadyExecuted[iChannelOrder])
    {
        algorithms[iChannelOrder]->run(observations,noiseVariances,trainingSequence);
        _algorithmAlreadyExecuted[iChannelOrder] = true;
    }

    return algorithms[iChannelOrder]->getEstimatedChannelMatrices();
}

MatrixXd LinearFilterBasedCMEapplyingAlgorithm::detectedSymbolsForChannelOrder(uint iChannelOrder, const MatrixXd& observations, const vector<double>& noiseVariances,const MatrixXd& trainingSequence)
{
    if(!_algorithmAlreadyExecuted[iChannelOrder])
    {
        algorithms[iChannelOrder]->run(observations,noiseVariances,trainingSequence);
        _algorithmAlreadyExecuted[iChannelOrder] = true;
    }

    return algorithms[iChannelOrder]->getDetectedSymbolVectors();
}
