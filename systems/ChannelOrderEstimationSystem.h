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
#ifndef CHANNELORDERESTIMATIONSYSTEM_H
#define CHANNELORDERESTIMATIONSYSTEM_H

#include <SMCSystem.h>

/**
	@author Manu <manu@rustneversleeps>
*/

#include <UnknownChannelOrderAlgorithm.h>

class ChannelOrderEstimationSystem : public SMCSystem
{
private:
	uint _iAlgorithmPerformingChannelOrderAPPestimation;
	uint _iAlgorithmPerformingOneChannelOrderPerOutputAPPestimation;
protected:
	vector<uint> _candidateChannelOrders;
	uint _iTrueChannelOrder;
	uint _iMaxChannelOrder;

	vector<MatrixXd> _channelOrderCoefficientsMeans;
	vector<MatrixXd> _channelOrderCoefficientsVariances;

	// channel order APP evolution for a single channel order
    vector<vector<vector<MatrixXd> > > _channelOrderAPPsAlongTime;
    vector<vector<MatrixXd> > _presentFrameChannelOrderAPPsAlongTime;
    vector<uint> _iAlgorithmsPerformingChannelOrderAPPestimation;

	// channel order APP evolution for one channel order per output
	vector<vector<vector<vector<MatrixXd> > > > _oneChannelOrderPerOutputAPPsAlongTime;
    vector<vector<vector<MatrixXd> > > _presentFrameOneChannelOrderPerOutputAPPsAlongTime;
    vector<uint> _iAlgorithmsPerformingOneChannelOrderPerOutputAPPestimation;

	virtual void onlyOnce();
    virtual void beforeEndingAlgorithm();
	virtual void addAlgorithms();
	virtual void storeFrameResults();
	virtual void saveFrameResults();
public:
    ChannelOrderEstimationSystem();
};

#endif
