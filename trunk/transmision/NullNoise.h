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
#ifndef NULLNOISE_H
#define NULLNOISE_H

#include <Noise.h>

/**
	@author Manu <manu@rustneversleeps>
*/
class NullNoise : public Noise
{
public:
    NullNoise(int nOutputs, int length);

    virtual double stdDevAt(int n) const {return 0.0;}
    virtual tVector operator [ ](int n) const;
	virtual void setSNR(int SNR,double alphabetVariance) {}
	virtual void print() const {std::cout << "Null Noise" << std::endl;}
};

#endif
