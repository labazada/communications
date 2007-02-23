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
#include "ParticleWithChannelEstimation.h"

using namespace std;

ParticleWithChannelEstimation::ParticleWithChannelEstimation(double weight, int symbolVectorLength, int nTimeInstants
,ChannelMatrixEstimator *channelMatrixEstimator): Particle(weight, symbolVectorLength, nTimeInstants)
,_channelMatrixEstimators(1)
{
    _channelMatrixEstimators[0] = channelMatrixEstimator;

//     _estimatedChannelMatrices = new tMatrix*[_channelMatrixEstimators.size()];
    _estimatedChannelMatrices = new tMatrix*[1];
    _estimatedChannelMatrices[0] = new tMatrix[_nTimeInstants];
}

ParticleWithChannelEstimation::ParticleWithChannelEstimation(double weight, int symbolVectorLength, int nTimeInstants,vector <ChannelMatrixEstimator *> channelMatrixEstimators):Particle(weight, symbolVectorLength, nTimeInstants),_channelMatrixEstimators(channelMatrixEstimators)
{
    _estimatedChannelMatrices = new tMatrix*[_channelMatrixEstimators.size()];
    for(uint i=0;i<_channelMatrixEstimators.size();i++)
        _estimatedChannelMatrices[i] = new tMatrix[_nTimeInstants];
}

ParticleWithChannelEstimation::ParticleWithChannelEstimation(const ParticleWithChannelEstimation &particle):Particle(particle),_channelMatrixEstimators(particle._channelMatrixEstimators.size())
{
    _estimatedChannelMatrices = new tMatrix*[particle._channelMatrixEstimators.size()];
    for(uint iChannelMatrixEstimator=0;iChannelMatrixEstimator<particle._channelMatrixEstimators.size();iChannelMatrixEstimator++)
    {
        _channelMatrixEstimators[iChannelMatrixEstimator] = particle._channelMatrixEstimators[iChannelMatrixEstimator]->Clone();

        _estimatedChannelMatrices[iChannelMatrixEstimator] = new tMatrix[_nTimeInstants];
        for(int i=0;i<_nTimeInstants;i++)
            _estimatedChannelMatrices[iChannelMatrixEstimator][i] = particle._estimatedChannelMatrices[iChannelMatrixEstimator][i];
    }
}

ParticleWithChannelEstimation::~ParticleWithChannelEstimation()
{
    for(uint i=0;i<_channelMatrixEstimators.size();i++)
    {
        delete _channelMatrixEstimators[i];
        delete[] _estimatedChannelMatrices[i];
    }
    delete[] _estimatedChannelMatrices;
}

ParticleWithChannelEstimation *ParticleWithChannelEstimation::Clone()
{
	return new ParticleWithChannelEstimation(*this);
}
