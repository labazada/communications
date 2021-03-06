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
#include "ChannelOrderEstimator.h"

ChannelOrderEstimator::ChannelOrderEstimator(uint N, std::vector<uint> candidateOrders):_nInputs(N),_candidateOrders(candidateOrders),_channelOrderAPPs(candidateOrders.size(),1.0/(double)candidateOrders.size())
{
}

ChannelOrderEstimator::ChannelOrderEstimator(std::vector<uint> candidateOrders, vector<double> channelOrderAPPs):_candidateOrders(candidateOrders),_channelOrderAPPs(channelOrderAPPs)
{
}

VectorXd ChannelOrderEstimator::getChannelOrderAPPsVector()
{
    VectorXd res(_channelOrderAPPs.size());
    for(uint i=0;i<_channelOrderAPPs.size();i++)
        res(i) = _channelOrderAPPs[i];
    return res;
}
