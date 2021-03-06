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
#include "MultiuserCDMAchannel.h"

#include <defines.h>

MultiuserCDMAchannel::MultiuserCDMAchannel(const MIMOChannel* const channel, const MatrixXd &spreadingCodes): StillMemoryMIMOChannel(spreadingCodes.cols(), spreadingCodes.rows(), 1, channel->length()),_spreadingCodes(spreadingCodes),_channel(channel),_spreadingCodesAutocorrelationMatrix(spreadingCodes.transpose()*spreadingCodes)
{
	assert(channel->nOutputs()==1);
	assert(channel->nInputs()==spreadingCodes.cols());
}

MultiuserCDMAchannel::~MultiuserCDMAchannel()
{
  delete _channel;
}

MatrixXd MultiuserCDMAchannel::getTransmissionMatrix(const uint n) const
{
    return _spreadingCodes*Util::toVector(_channel->at(n),rowwise).asDiagonal();
}

double MultiuserCDMAchannel::signalToInterferenceRatio(uint iUserOfInterest, uint t) const
{
	double otherUsersPower = 0.0;
	double userOfInterestPower = 0.0;
	
	for(uint i=0;i<_nInputs;i++)
	{
		if(i==iUserOfInterest)
			userOfInterestPower = _spreadingCodesAutocorrelationMatrix(i,i)*(_channel->at(t)(0,i)*_channel->at(t)(0,i));
		else
			otherUsersPower += _spreadingCodesAutocorrelationMatrix(i,i)*(_channel->at(t)(0,i)*_channel->at(t)(0,i));
	}
	
	return userOfInterestPower/otherUsersPower;
}

std::vector<double> MultiuserCDMAchannel::signalToInterferenceRatio(uint iUserOfInterest) const
{
	std::vector<double> res(_length-effectiveMemory()+1,FUNNY_VALUE);
	
	for(uint i=effectiveMemory()-1;i<_length;i++)
		res[i] = signalToInterferenceRatio(iUserOfInterest,i);
	
	return res;
}