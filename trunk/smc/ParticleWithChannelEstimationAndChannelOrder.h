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
#ifndef PARTICLEWITHCHANNELESTIMATIONANDCHANNELORDER_H
#define PARTICLEWITHCHANNELESTIMATIONANDCHANNELORDER_H

#include <ParticleWithChannelEstimation.h>

/**
	@author Manu <manu@rustneversleeps>
*/

#include <vector>
#include <WithChannelOrderParticleAddon.h>

class ParticleWithChannelEstimationAndChannelOrder : public ParticleWithChannelEstimation, public WithChannelOrderParticleAddon
{
// protected:
// 	int _m;
public:
    ParticleWithChannelEstimationAndChannelOrder(double weight, int symbolVectorLength, int nTimeInstants, ChannelMatrixEstimator* channelMatrixEstimator,int m);

    ParticleWithChannelEstimationAndChannelOrder(double weight, int symbolVectorLength, int nTimeInstants, std::vector <ChannelMatrixEstimator *> channelMatrixEstimators,int m);

    ParticleWithChannelEstimationAndChannelOrder* Clone();
     ParticleWithChannelEstimationAndChannelOrder(const ParticleWithChannelEstimationAndChannelOrder& particle);

// 	int GetChannelOrder(){ return _m;}
// 	void SetChannelOrder(int m) { _m = m;}
};

#endif
