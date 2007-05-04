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
#ifndef PARTICLEFILTER_H
#define PARTICLEFILTER_H

/**
	@author Manu <manu@rustneversleeps>
*/

// #define DEBUG_PF

#include <Particle.h>
#include <ParticleWithChannelEstimation.h>
#include <StatUtil.h>

class ParticleFilter{
protected:
    int _capacity,_nParticles;
    ParticleWithChannelEstimation **_particles;
public:

    ParticleFilter(int nParticles);
    virtual ~ParticleFilter();

	ParticleWithChannelEstimation *GetParticle(int n) { return _particles[n];}
	virtual void KeepParticles(std::vector<int> resamplingIndexes,std::vector<int> indexes);
	/**
	 *    It performs resamling keeping only the particles given by the vector of indexes. It guarantees that the order of the particles in the resulting particle filter is the one specified by the vector of indexes.
	 * @param resamplingIndexes
	 */
	virtual void KeepParticles(std::vector<int> resamplingIndexes);

	virtual void AddParticle(ParticleWithChannelEstimation *particle)
	{
		_particles[_nParticles++] = particle;
	}

    tVector GetWeightsVector()
    {
        tVector weights(_nParticles);
        for(int i=0;i<_nParticles;i++)
            weights(i) = _particles[i]->GetWeight();
        return weights;
    }

    void NormalizeWeights()
    {
        double sum = 0.0;
        int i;

        for(i=0;i<_nParticles;i++)
            sum += _particles[i]->GetWeight();

        for(i=0;i<_nParticles;i++)
            _particles[i]->SetWeight(_particles[i]->GetWeight()/sum);
    }

    void NormalizeWeights(std::vector<int> indexes)
    {
        double sum = 0.0;
        int i,nParticles=indexes.size();

        for(i=0;i<nParticles;i++)
            sum += _particles[indexes[i]]->GetWeight();

        for(i=0;i<nParticles;i++)
            _particles[indexes[i]]->SetWeight(_particles[indexes[i]]->GetWeight()/sum);
    }

	int Capacity() { return _capacity;}
	int Nparticles() { return _nParticles;}
};

#endif
