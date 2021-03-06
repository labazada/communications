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
#ifndef WITHCHANNELESTIMATIONPARTICLEADDON_H
#define WITHCHANNELESTIMATIONPARTICLEADDON_H

/**
It implements the channel related part for a particle

	@author Manu <manu@rustneversleeps>
*/

#include <defines.h>
#include <types.h>
#include <vector>
#include <ChannelMatrixEstimator.h>

class WithChannelEstimationParticleAddon{
protected:
    std::vector<ChannelMatrixEstimator *> _channelMatrixEstimators;
    std::vector<std::vector<MatrixXd> > _estimatedChannelMatrices;
	
#ifdef SAVE_CHANNEL_ESTIMATES_VARIANCES
	std::vector<std::vector<MatrixXd> > _channelEstimatesVariances;
#endif
	
public:
    WithChannelEstimationParticleAddon(ChannelMatrixEstimator * channelMatrixEstimator, uint trajectorylength);
    WithChannelEstimationParticleAddon(std::vector <ChannelMatrixEstimator *> channelMatrixEstimators, uint trajectorylength);
    ~WithChannelEstimationParticleAddon();
    
    WithChannelEstimationParticleAddon(const WithChannelEstimationParticleAddon& withChannelEstimationParticleAddon);

    MatrixXd getChannelMatrix(uint iChannelOrder,uint n) const
    {
        return _estimatedChannelMatrices[iChannelOrder][n];
    }

    void setChannelMatrix(uint iChannelOrder,uint n,const MatrixXd &matrix)
    {
            _estimatedChannelMatrices[iChannelOrder][n] = matrix;
    }
    
#ifdef SAVE_CHANNEL_ESTIMATES_VARIANCES
	MatrixXd getChannelEstimatesVariances(uint n,uint iChannelOrder=0) const
	{
		return _channelEstimatesVariances[iChannelOrder][n];
	}
	
	void setChannelEstimatesVariances(uint n,const MatrixXd &matrix,uint iChannelOrder=0)
	{
		_channelEstimatesVariances[iChannelOrder][n] = matrix;
	}
#endif

    ChannelMatrixEstimator *getChannelMatrixEstimator(uint iChannelOrder) const
    { 
        return _channelMatrixEstimators[iChannelOrder];
    }

	//! It returns the channel matrix estimator for the first order (useful when only one channel order is present)
	/*!
	  \return the channel matrix estimator for the first channel order
	*/
    ChannelMatrixEstimator *getChannelMatrixEstimator() const
    { 
        return _channelMatrixEstimators[0];
    }

    uint nChannelMatrixEstimators() const {return _channelMatrixEstimators.size();}

};

#endif
