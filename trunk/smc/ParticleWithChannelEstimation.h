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
#ifndef PARTICLEWITHCHANNELESTIMATION_H
#define PARTICLEWITHCHANNELESTIMATION_H

#include <Particle.h>

#define DO_NOT_STORE_CHANNEL_MATRICES

/**
	@author Manu <manu@rustneversleeps>
*/

#include <vector>
#include <ChannelMatrixEstimator.h>

class ParticleWithChannelEstimation : public Particle
{
protected:
	std::vector<ChannelMatrixEstimator *> _channelMatrixEstimators;

	#ifndef DO_NOT_STORE_CHANNEL_MATRICES
		tMatrix **_estimatedChannelMatrices;
	#endif
public:
    ParticleWithChannelEstimation(double weight, int symbolVectorLength, int nTimeInstants,ChannelMatrixEstimator *channelMatrixEstimator);

    ParticleWithChannelEstimation(double weight, int symbolVectorLength, int nTimeInstants,std::vector <ChannelMatrixEstimator *> channelMatrixEstimators);

	ParticleWithChannelEstimation(const ParticleWithChannelEstimation &particle);

    ~ParticleWithChannelEstimation();

    tMatrix GetChannelMatrix(int iChannelOrder,int n)
    {
    	#ifndef DO_NOT_STORE_CHANNEL_MATRICES
			return _estimatedChannelMatrices[iChannelOrder][n];
		#endif

		// return a matrix with the proper dimension (not initialized)
// 		return tMatrix(_channelMatrixEstimators[iChannelOrder]->Rows(),_channelMatrixEstimators[iChannelOrder]->Cols());
		return LaGenMatDouble::zeros(_channelMatrixEstimators[iChannelOrder]->Rows(),_channelMatrixEstimators[iChannelOrder]->Cols());
    }

    void SetChannelMatrix(int iChannelOrder,int n,const tMatrix &matrix)
    {
		#ifndef DO_NOT_STORE_CHANNEL_MATRICES
     		_estimatedChannelMatrices[iChannelOrder][n] = matrix;
     	#endif
    }

    ChannelMatrixEstimator *GetChannelMatrixEstimator(int iChannelOrder) { return _channelMatrixEstimators[iChannelOrder];}

    int NchannelMatrixEstimators() {return _channelMatrixEstimators.size();}

	ParticleWithChannelEstimation *Clone();
};

#endif
