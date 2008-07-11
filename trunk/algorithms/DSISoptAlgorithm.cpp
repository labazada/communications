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
#include "DSISoptAlgorithm.h"

// #define DEBUG

DSISoptAlgorithm::DSISoptAlgorithm(string name, Alphabet alphabet,int L,int N, int K,int m, ChannelMatrixEstimator *channelEstimator, tMatrix preamble, int smoothingLag, int nParticles,ResamplingAlgorithm *resamplingAlgorithm, const tMatrix &channelMatrixMean, const tMatrix &channelMatrixVariances): SMCAlgorithm(name, alphabet, L, N, K,m,  channelEstimator, preamble, smoothingLag, nParticles,resamplingAlgorithm,channelMatrixMean,channelMatrixVariances)
{
//     _randomParticlesInitilization = true;
}

void DSISoptAlgorithm::Process(const tMatrix &observations, vector< double > noiseVariances)
{
	uint k,iSmoothingVector;
	int iSmoothingLag,iParticle,iSampledVector;
	vector<tSymbol> testedVector(_N),testedSmoothingVector(_N*_d),sampledVector(_N);
	double auxLikelihoodsProd;
	ChannelMatrixEstimator *channelEstimatorClone;
	tRange rmMinus1FirstColumns(0,_m-2);

	// it selects all rows in the symbols Matrix
	tRange rAll;

	// it includes all symbol vectors involved in the smoothing
	tMatrix smoothingSymbolVectors(_N,_m+_d);

	uint nSymbolVectors = (int) pow((double)_alphabet.Length(),(double)_N);
	uint nSmoothingVectors = (int) pow((double)_alphabet.Length(),(double)(_N*_d));

	// a likelihood is computed for every possible symbol vector
	tVector likelihoods(nSymbolVectors);

    tRange rmPrecedentColumns(_startDetectionTime-_m+1,_startDetectionTime);
    tRange rmMinus1PrecedentColumns(_startDetectionTime-_m+1,_startDetectionTime-1);
    tRange rmColumns;

	// for each time instant
	for(int iObservationToBeProcessed=_startDetectionTime;iObservationToBeProcessed<_K;iObservationToBeProcessed++)
	{
		for(iParticle=0;iParticle<_particleFilter->Capacity();iParticle++)
		{
			ParticleWithChannelEstimation *processedParticle = _particleFilter->GetParticle(iParticle);

			// the m-1 already detected symbol vectors are copied into the matrix:
			smoothingSymbolVectors(rAll,rmMinus1FirstColumns).inject(processedParticle->GetSymbolVectors(rmMinus1PrecedentColumns));

			for(uint iTestedVector=0;iTestedVector<nSymbolVectors;iTestedVector++)
			{
				// the corresponding testing vector is generated from the index
				_alphabet.IntToSymbolsArray(iTestedVector,testedVector);

				// current tested vector is copied in the m-th position
				for(k=0;k<_N;k++)
					smoothingSymbolVectors(k,_m-1) = testedVector[k];

				likelihoods(iTestedVector) = 0.0;

				// every possible smoothing sequence is tested
				for(iSmoothingVector=0;iSmoothingVector<nSmoothingVectors;iSmoothingVector++)
				{
					// a testing smoothing vector is generated for the index
					_alphabet.IntToSymbolsArray(iSmoothingVector,testedSmoothingVector);

					// symbols used for smoothing are copied into "smoothingSymbolVectors"
					for(k=0;k<testedSmoothingVector.size();k++)
						smoothingSymbolVectors((_Nm+k)%_N,(_Nm+k)/_N) = testedSmoothingVector[k];

					// a clone of the channel estimator is generated because this must not be modified
					channelEstimatorClone = processedParticle->GetChannelMatrixEstimator(_estimatorIndex)->Clone();

                    rmColumns.set(0,_m-1);
					auxLikelihoodsProd = 1.0;

					for(iSmoothingLag=0;iSmoothingLag<=_d;iSmoothingLag++)
					{

						// the likelihood is computed and accumulated
						auxLikelihoodsProd *= channelEstimatorClone->Likelihood(observations.col(iObservationToBeProcessed+iSmoothingLag),smoothingSymbolVectors(rAll,rmColumns),noiseVariances[iObservationToBeProcessed+iSmoothingLag]);

						// a step in the Kalman Filter
						channelEstimatorClone->NextMatrix(observations.col(iObservationToBeProcessed+iSmoothingLag),smoothingSymbolVectors(rAll,rmColumns),noiseVariances[iObservationToBeProcessed+iSmoothingLag]);

                        rmColumns = rmColumns + 1;
					} // for(iSmoothingLag=0;iSmoothingLag<=_d;iSmoothingLag++)

					// memory of the clone is freed
					delete channelEstimatorClone;

					// the likelihood is accumulated in the proper position of vector:
					likelihoods(iTestedVector) += auxLikelihoodsProd;
				} // for(iSmoothingVector=0;iSmoothingVector<nSmoothingVectors;iSmoothingVector++)

			} // for(int iTestedVector=0;iTestedVector<nSymbolVectors;iTestedVector++)

			tVector probabilities(nSymbolVectors);
			try {
				// probabilities are computed by normalizing the likelihoods
				probabilities = Util::Normalize(likelihoods);
			} catch (AllElementsNullException) {
				// if all the likelihoods are null
				probabilities = 1.0/(double)nSymbolVectors;
			}

			// one sample from the discrete distribution is taken
			iSampledVector = StatUtil::Discrete_rnd(probabilities);

			// the above index is turned into a vector
			_alphabet.IntToSymbolsArray(iSampledVector,sampledVector);

			// sampled symbols are copied into the corresponding particle
			processedParticle->SetSymbolVector(iObservationToBeProcessed,sampledVector);

			// channel matrix is estimated by means of the particle channel estimator
			processedParticle->SetChannelMatrix(_estimatorIndex,iObservationToBeProcessed,processedParticle->GetChannelMatrixEstimator(_estimatorIndex)->NextMatrix(observations.col(iObservationToBeProcessed),processedParticle->GetSymbolVectors(rmPrecedentColumns),noiseVariances[iObservationToBeProcessed]));

			processedParticle->SetWeight(processedParticle->GetWeight()* Util::Sum(likelihoods));

		} // for(iParticle=0;iParticle<_nParticles;iParticle++)

		_particleFilter->NormalizeWeights();

		// if it's not the last time instant
		if(iObservationToBeProcessed<(_K-1))
            _resamplingAlgorithm->ResampleWhenNecessary(_particleFilter);

        rmPrecedentColumns = rmPrecedentColumns + 1;
        rmMinus1PrecedentColumns = rmMinus1PrecedentColumns + 1;

	} // for each time instant
}

