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

#include "ResamplingAlgorithm.h"

// #define DEBUG

bool ResamplingAlgorithm::ResampleWhenNecessary(ParticleFilter *particleFilter)
{
    tVector weigths = particleFilter->GetWeightsVector();

    if(_resamplingCriterion.ResamplingNeeded(weigths))
    {
        vector<int> indexes = ObtainIndexes(particleFilter->Nparticles(),weigths);
        #ifdef DEBUG
            extern int iteracionActual;
            extern vector<double> MSEs;
        //       cout << "Indices elegidos" << endl;
        //       Util::Print(indexes);
            if(iteracionActual==0)
            {
                vector<int> firstOcurrence,times;
                Util::HowManyTimes(indexes,firstOcurrence,times);
                Util::Print(times);

                // se ordenan
                typedef struct
                {
                    int indice;
                    double MSE;
                } sortable;


                for(int i=0;i<times.size();i++)
                    cout << "La part�ula " << indexes[firstOcurrence[i]] << " se remuestrea " << times[i] << " veces. MSE = " << MSEs[indexes[firstOcurrence[i]]] << endl;
                cout << "Una tecla..."; getchar();
            }
        #endif
        particleFilter->KeepParticles(indexes);
        return true;
    }
    return false;
}