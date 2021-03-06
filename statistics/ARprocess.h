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
#ifndef ARPROCESS_H
#define ARPROCESS_H

/**
    @author Manu <manu@rustneversleeps>
*/

#include <vector>
#include <math.h>
#include <stdlib.h>
#include <types.h>
#include <Util.h>
#include <Random.h>
#include <StatUtil.h>
#include <Eigen/LU>

class ARprocess{

private:
    vector<double> _coefficients;
    double _noiseVariance;
    double _noiseMean;
    uint _nCoefficients, _rows, _columns, _iNextMatrix;
    uint _iterationsForConvergence;
    std::vector<MatrixXd> _buffer;   

    void CommonConstructorsCode(const MatrixXd &seed);   

public:
    ARprocess(MatrixXd seed,vector<double> coefficients,double noiseVariance);
    ARprocess(MatrixXd seed,uint order,double velocity,double carrierFrequency,double T);

    MatrixXd nextMatrix();
    
    static vector<double> parametersFromYuleWalker(uint order,double velocity,double carrierFrequency,double T,double &noiseVariance);

    uint rows() {return _rows;}
    uint cols() {return _columns;}
};

#endif
