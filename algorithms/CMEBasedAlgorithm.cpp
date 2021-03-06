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
#include "CMEBasedAlgorithm.h"

// #define IMPORT_REAL_DATA

#ifdef IMPORT_REAL_DATA
    extern MIMOChannel *realChannel;
    extern MatrixXd *realSymbols;
    extern Noise *realNoise;
#endif

CMEBasedAlgorithm::CMEBasedAlgorithm(std::string name, Alphabet alphabet, uint L, uint Nr,uint N, uint iLastSymbolVectorToBeDetected, vector< ChannelMatrixEstimator * > channelEstimators, MatrixXd preamble, uint iFirstObservation, const MatrixXd &symbolVectors): UnknownChannelOrderAlgorithm(name, alphabet, L, Nr,N, iLastSymbolVectorToBeDetected, channelEstimators, preamble, iFirstObservation),_symbolVectors(symbolVectors)
{
}

void CMEBasedAlgorithm::run(MatrixXd observations,vector<double> noiseVariances)
{
    uint m,iTxAntenna,iRxAntenna,iDelay;
    uint nSymbolVectors = _symbolVectors.cols() - _preamble.cols();
    std::vector<double> CMEs(_candidateOrders.size());

#ifdef IMPORT_REAL_DATA
    MatrixXd channelMatrix = realChannel->at(_preamble.cols());
#endif
    double variance = noiseVariances[_symbolVectors.cols()-1];

    for(uint iChannelOrder=0;iChannelOrder<_candidateOrders.size();iChannelOrder++)
    {
        m = _candidateOrders[iChannelOrder];

        // channel estimation
        for(uint iSymbolVector=_preamble.cols();iSymbolVector<_iLastSymbolVectorToBeDetected;iSymbolVector++)
            _channelEstimators[iChannelOrder]->nextMatrix(observations.col(iSymbolVector),_symbolVectors.block(0,iSymbolVector-m+1,_nInputs,m),noiseVariances[iSymbolVector]);

        MatrixXd estimatedChannelMatrix = _channelEstimators[iChannelOrder]->lastEstimatedChannelMatrix();

        vector<VectorXd> hs(_nOutputs,VectorXd::Zero(_nInputs*m,1));

        MatrixXd C(nSymbolVectors,_nInputs*m);
        for(iTxAntenna=0;iTxAntenna<_nInputs;iTxAntenna++)
            for(iDelay=0;iDelay<m;iDelay++)
            {
                // symbols are transformed
                for(uint CmatrixRow=0;CmatrixRow<nSymbolVectors;CmatrixRow++)
                    C(CmatrixRow,iTxAntenna*m+iDelay) = _symbolVectors(iTxAntenna,_preamble.cols()-iDelay+CmatrixRow);

                // channel is transformed
                for(iRxAntenna=0;iRxAntenna<_nOutputs;iRxAntenna++)
                    hs[iRxAntenna](iTxAntenna*m+iDelay) = estimatedChannelMatrix(iRxAntenna,iTxAntenna+(m-1-iDelay)*_nInputs);
            }

        // CME
        double CME = 0.0;
        for(iRxAntenna=0;iRxAntenna<_nOutputs;iRxAntenna++)
        {
            VectorXd aux = observations.row(iRxAntenna).segment(_preamble.cols(),nSymbolVectors);
            VectorXd error = aux - C*hs[iRxAntenna];
            
            // CME += error'*error
            CME += error.dot(error);
        }
        CME /= variance;        
        CME += _nOutputs*log(fabs((C.transpose()*C).determinant()));
        CME /= 2.0;

        CMEs[iChannelOrder] = CME;
    } // for(uint iChannelOrder=0;iChannelOrder<_candidateOrders.size();iChannelOrder++)

    for(uint iChannelOrder=0;iChannelOrder<_candidateOrders.size();iChannelOrder++)
        _channelOrderAPPs[0].row(iChannelOrder).setConstant(CMEs[iChannelOrder]);
}

void CMEBasedAlgorithm::run(MatrixXd observations,vector<double> noiseVariances, MatrixXd trainingSequence)
{
    run(observations,noiseVariances);
}

MatrixXd CMEBasedAlgorithm::getDetectedSymbolVectors()
{
//     MatrixXd aux(1,1);
//     aux.resize(0,0);
//     return aux;
	throw RuntimeException("CMEBasedAlgorithm::getDetectedSymbolVectors: this method is not implemented.");
}

vector<MatrixXd> CMEBasedAlgorithm::getEstimatedChannelMatrices()
{
//     return vector<MatrixXd>(0);
	throw RuntimeException("CMEBasedAlgorithm::getEstimatedChannelMatrices: this method is not implemented.");
}

