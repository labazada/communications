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
#include "KnownChannelAlgorithm.h"

KnownChannelAlgorithm::KnownChannelAlgorithm(string name, Alphabet alphabet,int L,int N, int iLastSymbolVectorToBeDetected, const MIMOChannel &channel): Algorithm(name, alphabet, L, N, iLastSymbolVectorToBeDetected),_channel(channel)
{
}


KnownChannelAlgorithm::~KnownChannelAlgorithm()
{
}

void KnownChannelAlgorithm::Run(tMatrix observations,vector<double> noiseVariances, tMatrix trainingSequence)
{
    this->Run(observations,noiseVariances);
}

vector<tMatrix> KnownChannelAlgorithm::GetEstimatedChannelMatrices()
{
    vector<tMatrix> res;

    // returns an empty vector
    return res;
}
