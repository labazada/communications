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
// #include <Eigen/Array> 
#include <Eigen/Core> 

class ParticleFilter{
protected:
    const uint _capacity;
    uint _nParticles;
    Particle **_particles;
public:

    ParticleFilter(uint nParticles);
    virtual ~ParticleFilter();

    void clear();

    Particle *getParticle(uint n) const { return _particles[n];}
    virtual void keepParticles(std::vector<uint> resamplingIndexes,std::vector<uint> indexes);
    /**
     *    It performs resamling keeping only the particles given by the vector of indexes. It guarantees that the order of the particles in the resulting particle filter is the one specified by the vector of indexes.
     * @param resamplingIndexes
     */
    virtual void keepParticles(std::vector<uint> resamplingIndexes);

    virtual void addParticle(Particle *particle)
    {
        _particles[_nParticles++] = particle;
    }

    VectorXd getWeightsVector() const
    {
        VectorXd weights(_nParticles);
        for(uint i=0;i<_nParticles;i++)
            weights(i) = _particles[i]->getWeight();
        return weights;
    }

    void normalizeWeights()
    {
        double sum = 0.0;
        uint i;

        for(i=0;i<_nParticles;i++)
            sum += _particles[i]->getWeight();

        for(i=0;i<_nParticles;i++)
            _particles[i]->setWeight(_particles[i]->getWeight()/sum);
    }

    void normalizeWeights(std::vector<uint> indexes)
    {
        double sum = 0.0;
        uint i,nParticles=indexes.size();

        for(i=0;i<nParticles;i++)
            sum += _particles[indexes[i]]->getWeight();

        for(i=0;i<nParticles;i++)
            _particles[indexes[i]]->setWeight(_particles[indexes[i]]->getWeight()/sum);
    }

    uint capacity() const { return _capacity;}
    uint nParticles() const { return _nParticles;}
    Particle *getBestParticle() const { return _particles[iBestParticle()]; }
    uint iBestParticle() const;
    void printWeights() const;
};

#endif
