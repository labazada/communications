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
#ifndef LMSMUTESTSYSTEM_H
#define LMSMUTESTSYSTEM_H

#include <SMCSystem.h>

/**
    @author Manu <manu@rustneversleeps>
*/

#include <PSPAlgorithm.h>
#include <FlatPowerProfile.h>
#include <RMMSEDetector.h>
#include <RLSEstimator.h>
#include <LMSEstimator.h>
#include <NLMSEstimator.h>

class LMSmuTestSystem : public SMCSystem
{
protected:
    uint nSurvivors;
    bool adjustParticlesNumberFromSurvivors,adjustSurvivorsFromParticlesNumber;

    // joint channel and data estimation
    double forgettingFactorDetector;
    RMMSEDetector *rmmseDetector;

    vector<double> musLMS;
    vector<ChannelMatrixEstimator *> LMSchannelEstimators;

	virtual void saveFrameResults();
    virtual void addAlgorithms();
public:
    LMSmuTestSystem();
    ~LMSmuTestSystem();
};

#endif
