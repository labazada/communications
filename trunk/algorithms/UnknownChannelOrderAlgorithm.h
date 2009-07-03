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
#ifndef UNKNOWNCHANNELORDERALGORITHM_H
#define UNKNOWNCHANNELORDERALGORITHM_H

#include <UnknownChannelAlgorithm.h>

/**
	@author Manu <manu@rustneversleeps>
*/

#include <vector>
#include <ChannelMatrixEstimator.h>

class UnknownChannelOrderAlgorithm : public UnknownChannelAlgorithm
{
protected:
	vector<ChannelMatrixEstimator *> _channelEstimators;
    vector<int> _candidateOrders;
	int _maxOrder,_iFirstObservation,_nInputsXchannelOrderaxOrder;
	int *_channelOrder2index;
	tMatrix _preamble;

	tMatrix _channelOrderAPPs;
public:
    UnknownChannelOrderAlgorithm(string name, Alphabet alphabet, int L, int N, int iLastSymbolVectorToBeDetected,vector<ChannelMatrixEstimator *> channelEstimators,tMatrix preamble,int iFirstObservation);

    ~UnknownChannelOrderAlgorithm();

	virtual vector<vector<tMatrix> > EstimateChannelFromTrainingSequence(const tMatrix &observations,vector<double> noiseVariances,tMatrix trainingSequence);
	tMatrix GetChannelOrderAPPsAlongTime() { return _channelOrderAPPs(tRange(),tRange(_preamble.cols(),_iLastSymbolVectorToBeDetected-1));}
    bool PerformsChannelOrderAPPEstimation() const { return true;}
};

#endif
