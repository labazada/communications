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
#ifndef MODULATOR_H
#define MODULATOR_H

/**
    @author Manu <manu@rustneversleeps>
*/

#include <vector>
#include <types.h>
#include <Bits.h>
#include <Alphabet.h>
#include <Util.h>

class Modulator{
public:
    Modulator();

    static tMatrix modulate(const Bits &bits,Alphabet alfabeto)
    {
        return Util::eigen2lapack(modulate_eigen(bits,alfabeto));
    }
    static MatrixXd modulate_eigen(const Bits &bits,Alphabet alfabeto);
};

#endif
