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
#include "SingleUserPowerProfileDependentNoise.h"

#include <Eigen/Dense>

SingleUserPowerProfileDependentNoise::SingleUserPowerProfileDependentNoise(double alphabetVariance,uint nOutputs, uint length, const DelayPowerProfile &powerProfile): PowerProfileDependentNoise(alphabetVariance,nOutputs, length,powerProfile),_iUser(0)
{
	// we need the autocorrelation of the channel coefficients (rather than the variance)
// 	_powerProfileDependentVarianceFactor = (powerProfile.variances().col(_iUser).array() + powerProfile.means().col(_iUser).array()*powerProfile.means().col(_iUser).array()).sum()/double(_nOutputs);
	_powerProfileDependentVarianceFactor = _alphabetVariance * ( (powerProfile.variances().col(_iUser).array() + powerProfile.means().col(_iUser).array()*powerProfile.means().col(_iUser).array()).sum() ); // <- more fair implementation
	
// 	_iUserSNRcontribution = _alphabetVariance * ( (powerProfile.variances().col(_iUser).array() + powerProfile.means().col(_iUser).array()*powerProfile.means().col(_iUser).array()).sum());
// 	_powerProfileDependentVarianceFactor -= _iUserSNRcontribution;
}

// void SingleUserPowerProfileDependentNoise::setSNR(int SNR)
// {
// 	double newStdDev = sqrt(pow(10.0,((double)-SNR)/10.0)*_iUserSNRcontribution - _powerProfileDependentVarianceFactor);
// 	
// 	std::cout << "SingleUserPowerProfileDependentNoise::setSNR: standard deviation set to " << newStdDev << std::endl;
// 
// 	_matrix *= (newStdDev/_stdDev);
// 	_stdDev = newStdDev;
// }
