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
#include "KnownFlatChannelAndActiveUsersOptimalAlgorithm.h"

KnownFlatChannelAndActiveUsersOptimalAlgorithm::KnownFlatChannelAndActiveUsersOptimalAlgorithm(std::string name, Alphabet alphabet, uint L, uint Nr, uint N, uint iLastSymbolVectorToBeDetected, const MIMOChannel& channel, uint preambleLength, std::vector<std::vector<bool> > usersActivity): KnownFlatChannelOptimalAlgorithm(name, alphabet, L, Nr, N, iLastSymbolVectorToBeDetected, channel, preambleLength),_noTransmissionAlphabet(new Alphabet(vector<tSymbol>(1,0.0))),_usersActivity(usersActivity)
{
    if(_usersActivity.size()!=_nInputs || _usersActivity[0].size()!=(_iLastSymbolVectorToBeDetected-_preambleLength))
        throw RuntimeException("KnownFlatChannelAndActiveUsersOptimalAlgorithm::KnownFlatChannelAndActiveUsersOptimalAlgorithm: users activity vector has wrong dimensions.");
}

KnownFlatChannelAndActiveUsersOptimalAlgorithm::~KnownFlatChannelAndActiveUsersOptimalAlgorithm()
{
    delete _noTransmissionAlphabet;
}
