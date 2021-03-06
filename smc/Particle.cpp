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
#include "Particle.h"

Particle::Particle(double weight,uint symbolVectorLength,uint nTimeInstants):_weight(weight),_symbolVectors(MatrixXd::Zero(symbolVectorLength,nTimeInstants))
{
}

Particle::~Particle()
{
}

void Particle::operator=(const Particle &particle)
{
	if(this!=&particle)
	{
		_weight = particle._weight;
		_symbolVectors = particle._symbolVectors;
	}
}

std::ostream& operator<< (std::ostream &out, Particle &particle)
{
	out << "////////////" << " Particle " << "////////////" << std::endl;
	out << "Sequence of symbol vectors:" << std::endl;
	out << particle._symbolVectors << std::endl;
	out << "weight = " << particle._weight << std::endl;
	out << "\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\" << std::endl;

	return out;
}
