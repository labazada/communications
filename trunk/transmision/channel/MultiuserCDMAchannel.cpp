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

MultiuserCDMAchannel::MultiuserCDMAchannel(int length, const tMatrix &spreadingCodes): StillMemoryMIMOChannel(spreadingCodes.cols(), spreadingCodes.rows(), 1, length),_spreadingCodes(spreadingCodes)
{
}


MultiuserCDMAchannel::~MultiuserCDMAchannel()
{
}

tMatrix MultiuserCDMAchannel::operator[](int n) const
{
    tMatrix spreadingCodesXcoeffs(_nOutputs,_nInputs);
    
    // spreadingCodesXcoeffs = _spreadingCodes * diag(getUsersCoefficientsAtTime(n))
    Blas_Mat_Mat_Mult(_spreadingCodes,LaGenMatDouble::from_diag(getUsersCoefficientsAtTime(n)),spreadingCodesXcoeffs);
    
    return spreadingCodesXcoeffs;
}