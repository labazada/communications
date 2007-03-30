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
#include "PSPAlgorithm.h"

// #define DEBUG3

PSPAlgorithm::PSPAlgorithm(string name, Alphabet alphabet, int L, int N, int K, int m, ChannelMatrixEstimator* channelEstimator, tMatrix preamble, int smoothingLag, int firstSymbolVectorDetectedAt, double ARcoefficient, int nSurvivors): KnownChannelOrderAlgorithm(name, alphabet, L, N, K, m, channelEstimator, preamble),_rAllSymbolRows(0,_N-1),_inputVector(N),_stateVector(N*(m-1)),_nSurvivors(nSurvivors),_d(smoothingLag),_startDetectionTime(preamble.cols()),_trellis(alphabet,N,m),_detectedSymbolVectors(new tMatrix(N,K+smoothingLag)),_firstSymbolVectorDetectedAt(firstSymbolVectorDetectedAt),_ARcoefficient(ARcoefficient)
{
    if(preamble.cols() < (m-1))
        throw RuntimeException("PSPAlgorithm::PSPAlgorithm: preamble dimensions are wrong.");

    _exitStage = new PSPPath*[_trellis.Nstates()];
    _arrivalStage = new PSPPath*[_trellis.Nstates()];
    _bestArrivingPaths = new PSPPathCandidate*[_trellis.Nstates()];

    for(int i=0;i<_trellis.Nstates();i++)
    {
    	_exitStage[i] = new PSPPath[_nSurvivors];
    	_arrivalStage[i] = new PSPPath[_nSurvivors];
    	_bestArrivingPaths[i] = new PSPPathCandidate[_nSurvivors];
    }

    _estimatedChannelMatrices.reserve(_K+_d-_preamble.cols());
}


PSPAlgorithm::~PSPAlgorithm()
{
    for(int i=0;i<_trellis.Nstates();i++)
    {
    	delete[] _exitStage[i];
    	delete[] _arrivalStage[i];
    	delete[] _bestArrivingPaths[i];
    }
    delete[] _exitStage;
    delete[] _arrivalStage;
    delete[] _bestArrivingPaths;
    delete _detectedSymbolVectors;
}

void PSPAlgorithm::ProcessOneObservation(const tVector &observations,double noiseVariance)
{
	int iState,iSurvivor;

	#ifdef DEBUG
		cout << "Al principio de ProcessOneObservation" << endl;
	#endif

	for(iState=0;iState<_trellis.Nstates();iState++)
	{
		// if the first survivor is empty, we assume that so are the remaining ones
		if(!_exitStage[iState][0].IsEmpty())
        {
            #ifdef DEBUG2
                cout << "Llamando a Deploy para el estado " << iState << endl;
            #endif
			DeployState(iState,observations,noiseVariance);
        }
	}

	#ifdef DEBUG
		cout << "Despues del 1er bucle en ProcessOneObservation" << endl;
	#endif

	// the best path arriving at each state is generated from the stored PathCandidate
	for(iState=0;iState<_trellis.Nstates();iState++)
	{
		for(iSurvivor=0;iSurvivor<_nSurvivors;iSurvivor++)
		{
			PSPPathCandidate &bestPathCandidate = _bestArrivingPaths[iState][iSurvivor];
			if(bestPathCandidate.NoPathArrived())
				continue;

			PSPPath &sourcePath = _exitStage[bestPathCandidate._fromState][bestPathCandidate._fromSurvivor];
			ChannelMatrixEstimator * newChannelMatrixEstimator = sourcePath.GetChannelMatrixEstimator()->Clone();

			#ifdef DEBUG
				cout << "Antes de llamar a NextMatrix" << endl;
			#endif

			newChannelMatrixEstimator->NextMatrix(observations,bestPathCandidate._detectedSymbolVectors,noiseVariance);

			#ifdef DEBUG
				cout << "Despues de llamar a NextMatrix" << endl;
			#endif

			_arrivalStage[iState][iSurvivor].Update(sourcePath,bestPathCandidate._newSymbolVector,bestPathCandidate._cost,vector<ChannelMatrixEstimator *>(1,newChannelMatrixEstimator));

			#ifdef DEBUG
				cout << "Despues de llamar a Update" << endl;
			#endif
		}
	}

	#ifdef DEBUG
		cout << "Despues del 2� bucle en ProcessOneObservation" << endl;
	#endif

	// _arrivalStage becomes _exitStage for the next iteration
	PSPPath **aux = _exitStage;
	_exitStage = _arrivalStage;
	_arrivalStage = aux;

	// the _arrivalStage (old _exitStage) and the best arriving paths get cleaned
	for(iState=0;iState<_trellis.Nstates();iState++)
	{
		for(iSurvivor=0;iSurvivor<_nSurvivors;iSurvivor++)
		{
			_arrivalStage[iState][iSurvivor].Clean();
			_bestArrivingPaths[iState][iSurvivor].Clean();
		}
        #ifdef DEBUG2
            PrintState(iState);
        #endif
	}
}

void PSPAlgorithm::Process(const tMatrix &observations,vector<double> noiseVariances)
{
	int iProcessedObservation,iBestState,iBestSurvivor;

    #ifdef DEBUG3
    	cout << "_startDetectionTime: " << _startDetectionTime <<  " y _firstSymbolVectorDetectedAt: " << _firstSymbolVectorDetectedAt << endl;
    #endif

    for(iProcessedObservation=_startDetectionTime;iProcessedObservation<_firstSymbolVectorDetectedAt;iProcessedObservation++)
    {
        #ifdef DEBUG2
    	    cout << "iProcessedObservation = " << iProcessedObservation << endl;
        #endif
		ProcessOneObservation(observations.col(iProcessedObservation),noiseVariances[iProcessedObservation]);
    }

    #ifdef DEBUG3
    	cout << "Despues del primer bucle en Process" << endl;
    #endif

    BestPairStateSurvivor(iBestState,iBestSurvivor);

    #ifdef DEBUG3
    	cout << "Antes del inject" << endl;
    #endif

    // the first detected vector is copied into "_detectedSymbolVectors"...
    _detectedSymbolVectors->col(_startDetectionTime).inject(_exitStage[iBestState][iBestSurvivor].GetSymbolVector(_startDetectionTime));

    #ifdef DEBUG3
    	cout << "Despues del inject" << endl;
    #endif

    #ifdef DEBUG2
        cout << "detectado " << endl << _exitStage[iBestState][iBestSurvivor].GetSymbolVector(_startDetectionTime) << endl;
    #endif

    // ... and the first estimated channel matrix into _estimatedChannelMatrices
    _estimatedChannelMatrices.push_back(_exitStage[iBestState][iBestSurvivor].GetChannelMatrix(_startDetectionTime));
//     _estimatedChannelMatrices.push_back(_exitStage[iBestState][iBestSurvivor].GetChannelMatrixEstimator()->LastEstimatedChannelMatrix());

    #ifdef DEBUG3
    	cout << "despues de _estimatedChannelMatrices.push_back" << endl;
    #endif

    for( iProcessedObservation=_firstSymbolVectorDetectedAt;iProcessedObservation<_K+_d;iProcessedObservation++)
    {
		ProcessOneObservation(observations.col(iProcessedObservation),noiseVariances[iProcessedObservation]);

        BestPairStateSurvivor(iBestState,iBestSurvivor);

        _detectedSymbolVectors->col(iProcessedObservation-_firstSymbolVectorDetectedAt+_preamble.cols()+1).inject(_exitStage[iBestState][iBestSurvivor].GetSymbolVector(iProcessedObservation-_firstSymbolVectorDetectedAt+_preamble.cols()+1));

    	_estimatedChannelMatrices.push_back(_exitStage[iBestState][iBestSurvivor].GetChannelMatrix(iProcessedObservation));
//     	_estimatedChannelMatrices.push_back(_exitStage[iBestState][iBestSurvivor].GetChannelMatrixEstimator()->LastEstimatedChannelMatrix());
    }

    #ifdef DEBUG3
    	cout << "antes del �ltimo bucle" << endl;
    	cout << "empieza en " << _K+_d-_firstSymbolVectorDetectedAt+_startDetectionTime+1 << " y termina en " << _K+_d << endl;
    	cout << "_detectedSymbolVectors->cols(): " << _detectedSymbolVectors->cols() << endl;
    #endif

    // last detected symbol vectors are processed
    for(iProcessedObservation=_K+_d-_firstSymbolVectorDetectedAt+_startDetectionTime+1;iProcessedObservation<_K+_d;iProcessedObservation++)
    {
		#ifdef DEBUG3
			cout << "iProcessedObservation: " << iProcessedObservation << endl;
		#endif

		#ifdef DEBUG3
			cout << "eso: " << _exitStage[iBestState][iBestSurvivor]._detectedSequence->cols();
		#endif

        _detectedSymbolVectors->col(iProcessedObservation).inject(_exitStage[iBestState][iBestSurvivor].GetSymbolVector(iProcessedObservation));

		#ifdef DEBUG4
			cout << "en el medio" << iProcessedObservation << endl;
		#endif

		_estimatedChannelMatrices.push_back(_exitStage[iBestState][iBestSurvivor].GetChannelMatrix(iProcessedObservation));
// 		_estimatedChannelMatrices.push_back(_exitStage[iBestState][iBestSurvivor].GetChannelMatrixEstimator()->LastEstimatedChannelMatrix());
        #ifdef DEBUG2
            cout << "detectado " << endl << _exitStage[iBestState][iBestSurvivor].GetSymbolVector(iProcessedObservation) << endl;
        #endif
    }

    #ifdef DEBUG3
    	cout << "Final de process" << endl;
    #endif

}

void PSPAlgorithm::Run(tMatrix observations,vector<double> noiseVariances)
{
    if(observations.cols()<(_startDetectionTime+1+_d))
        throw RuntimeException("PSPAlgorithm::Run: Not enough observations.");

    // the last N*(m-1) symbols of the preamble are copied into a c++ vector...
    int preambleLength = _preamble.rows()*_preamble.cols();
    vector<tSymbol> initialStateVector(_N*(_m-1));

    // (it must be taken into account that the number of columns of the preamble might be greater than m-1)
    int iFirstPreambleSymbolNeeded = (_preamble.cols()-(_m-1))*_N;
    for(int i=iFirstPreambleSymbolNeeded;i<preambleLength;i++)
        initialStateVector[i-iFirstPreambleSymbolNeeded] = _preamble(i % _N,i / _N);

    // ...in order to use the method "SymbolsVectorToInt" from "Alphabet" to obtain the initial state
    int initialState = _alphabet.SymbolsArrayToInt(initialStateVector);

	#ifdef DEBUG
    	cout << "initialState es " << initialState << endl;
    #endif

	// the initial state is initalized
    _exitStage[initialState][0] = PSPPath(_K+_d,0.0,_preamble,vector<vector<tMatrix> > (1,vector<tMatrix>(0)),vector<ChannelMatrixEstimator *>(1,_channelEstimator));

	#ifdef DEBUG
    	cout << "Antes de llamar a process" << endl;
    #endif

	Process(observations,noiseVariances);
}

void PSPAlgorithm::Run(tMatrix observations,vector<double> noiseVariances, tMatrix trainingSequence)
{
    if(observations.rows()!=_L || trainingSequence.rows()!=_N)
        throw RuntimeException("PSPAlgorithm::Run: Observations matrix or training sequence dimensions are wrong.");

    // to process the training sequence, we need both the preamble and the symbol vectors related to it
    tMatrix preambleTrainingSequence = Util::Append(_preamble,trainingSequence);

	_startDetectionTime = preambleTrainingSequence.cols();

    vector<tMatrix> trainingSequenceChannelMatrices = ProcessTrainingSequence(observations,noiseVariances,trainingSequence);

	// known symbol vectors are copied into the the vector with the final detected ones
	(*_detectedSymbolVectors)(_rAllSymbolRows,tRange(_preamble.cols(),_startDetectionTime-1)).inject(trainingSequence);

	// and so the channel matrices
	for(uint i=0;i<trainingSequenceChannelMatrices.size();i++)
		_estimatedChannelMatrices.push_back(trainingSequenceChannelMatrices[i]);

    // the last N*(m-1) symbols of the training sequence are copied into a c++ vector...
    int preambleTrainingSequenceLength = preambleTrainingSequence.rows()*preambleTrainingSequence.cols();
    vector<tSymbol> initialStateVector(_N*(_m-1));

    // (it must be taken into account that the number of columns of the preamble might be greater than m-1)
    int iFirstPreambleSymbolNeeded = (preambleTrainingSequence.cols()-(_m-1))*_N;
    for(int i=iFirstPreambleSymbolNeeded;i<preambleTrainingSequenceLength;i++)
        initialStateVector[i-iFirstPreambleSymbolNeeded] = preambleTrainingSequence(i % _N,i / _N);

    // ...in order to use the method "SymbolsVectorToInt" from "Alphabet" to obtain the initial state
    int initialState = _alphabet.SymbolsArrayToInt(initialStateVector);

	#ifdef DEBUG2
    	cout << "initialState es " << initialState << endl;
    #endif

	// the initial state is initalized
    _exitStage[initialState][0] = PSPPath(_K+_d,0.0,preambleTrainingSequence,vector<vector<tMatrix> > (1,trainingSequenceChannelMatrices),vector<ChannelMatrixEstimator *>(1,_channelEstimator));

	Process(observations,noiseVariances);
}

void PSPAlgorithm::DeployState(int iState,const tVector &observations,double noiseVariance)
{
    double newCost;
    int arrivalState,iDisposableSurvivor;
    tVector computedObservations(_L),error(_L);

    // "symbolVectors" will contain all the symbols involved in the current observation
    tMatrix symbolVectors(_N,_m);

	// the state determines the first "_m" symbol vectors involved in the "observations"
	_alphabet.IntToSymbolsArray(iState,_stateVector);
	for(int i=0;i<_N*(_m-1);i++)
		symbolVectors(i % _N,i / _N) = _stateVector[i];

    // now we compute the cost for each possible input
    for(int iInput=0;iInput<_trellis.NpossibleInputs();iInput++)
    {
        arrivalState = _trellis(iState,iInput);

        // the decimal input is converted to a symbol vector according to the alphabet
        _alphabet.IntToSymbolsArray(iInput,_inputVector);

        // it's copied into "symbolVectors"
        for(int i=0;i<_N;i++)
            symbolVectors(i,_m-1) = _inputVector[i];

		for(int iSourceSurvivor=0;iSourceSurvivor<_nSurvivors;iSourceSurvivor++)
		{
			if(_exitStage[iState][iSourceSurvivor].IsEmpty())
				continue;

            #ifdef DEBUG2
                cout << "--------------- DeployState: (" << iState << "," << iSourceSurvivor << ") con la entrada " << iInput << "---------------" << endl;
            #endif

			tMatrix estimatedChannelMatrix = _exitStage[iState][iSourceSurvivor].GetChannelMatrixEstimator()->LastEstimatedChannelMatrix();
			estimatedChannelMatrix *= _ARcoefficient;

			// computedObservations = estimatedChannelMatrix * symbolVectors(:)
			Blas_Mat_Vec_Mult(estimatedChannelMatrix,Util::ToVector(symbolVectors,columnwise),computedObservations);

			// error = observations - computedObservations
			Util::Add(observations,computedObservations,error,1.0,-1.0);

			newCost = _exitStage[iState][iSourceSurvivor].GetCost() + Blas_Dot_Prod(error,error);

            iDisposableSurvivor = DisposableSurvivor(arrivalState);

			// if the given disposal survivor is empty
			if((_bestArrivingPaths[arrivalState][iDisposableSurvivor].NoPathArrived()) ||
				// or its cost is greater than the computed new cost
				(_bestArrivingPaths[arrivalState][iDisposableSurvivor].GetCost() > newCost))
					// the ViterbiPath object at the arrival state is updated with that from the exit stage, the
					// new symbol vector, and the new cost
				{
                #ifdef DEBUG2
                    cout << "(" << iState << "," << iSourceSurvivor << ") --- " << iInput << " --->" << "(" << arrivalState << "," << iDisposableSurvivor << ") coste: " << newCost << " (coste a�adido: " << Blas_Dot_Prod(error,error) << ")" << endl;
                #endif
					_bestArrivingPaths[arrivalState][iDisposableSurvivor]._fromState = iState;
					_bestArrivingPaths[arrivalState][iDisposableSurvivor]._fromSurvivor = iSourceSurvivor;
					_bestArrivingPaths[arrivalState][iDisposableSurvivor]._input = iInput;
					_bestArrivingPaths[arrivalState][iDisposableSurvivor]._cost = newCost;
					_bestArrivingPaths[arrivalState][iDisposableSurvivor]._newSymbolVector = symbolVectors.col(_m-1);
					_bestArrivingPaths[arrivalState][iDisposableSurvivor]._detectedSymbolVectors = symbolVectors;
				}
                #ifdef DEBUG2
                else{
                    cout << "NOOOOOOOOOOOOOOOO (" << iState << "," << iSourceSurvivor << ") --- " << iInput << " ---> " << "(" << arrivalState << "," << iDisposableSurvivor << ") coste: " << newCost << " (coste a�adido: " << Blas_Dot_Prod(error,error) << ")" << endl;
                }
                #endif
		} // for(int iSourceSurvivor=0;iSourceSurvivor<_nSurvivors;iSourceSurvivor++)
    } // for(int iInput=0;iInput<_trellis.NpossibleInputs();iInput++)
}

tMatrix PSPAlgorithm::GetDetectedSymbolVectors()
{
    return (*_detectedSymbolVectors)(_rAllSymbolRows,tRange(_preamble.cols(),_K-1));
}

vector<tMatrix> PSPAlgorithm::GetEstimatedChannelMatrices()
{
	#ifdef DEBUG3
		cout << "_d es " << _d << endl;
		cout << "_estimatedChannelMatrices: " << _estimatedChannelMatrices.size() << endl;
	#endif

	vector<tMatrix> res = _estimatedChannelMatrices;

	// the last "_d" estimated matrices need to be erased
	vector<tMatrix>::iterator tempIterator;
	tempIterator = res.end();
	tempIterator--;
	for(int i=0;i<_d;i++)
		res.erase(tempIterator);

	return res;
}

void PSPAlgorithm::BestPairStateSurvivor(int &bestState,int &bestSurvivor)
{
	if(_exitStage[0][0].IsEmpty())
		throw RuntimeException("PSPAlgorithm::BestState: [first state,first survivor] is empty.");

	int iSurvivor;
	bestState = 0;
	bestSurvivor = 0;
	double bestCost = _exitStage[bestState][bestSurvivor].GetCost();

	for(int iState=1;iState<_trellis.Nstates();iState++)
		for(iSurvivor=0;iSurvivor<_nSurvivors;iSurvivor++)
		{
			#ifdef DEBUG2
				cout << "[iState: " << iState << ",iSurvivor: " << iSurvivor << "] coste = " << _exitStage[iState][iSurvivor].GetCost() << endl;
			#endif
			if(_exitStage[iState][iSurvivor].GetCost() < bestCost)
			{
				bestState = iState;
				bestSurvivor = iSurvivor;
				bestCost = _exitStage[iState][iSurvivor].GetCost();
			}
		}

	#ifdef DEBUG2
		cout << "bestState es " << bestState << " y bestSurvivor " << bestSurvivor << endl;
	#endif
}

int PSPAlgorithm::DisposableSurvivor(int iState)
{
    int iWorstCost;
    double worstCost;

    if(_bestArrivingPaths[iState][0].NoPathArrived())
        return 0;

    iWorstCost = 0;
    worstCost = _bestArrivingPaths[iState][0]._cost;

    for(int iSurvivor=1;iSurvivor<_nSurvivors;iSurvivor++)
    {
        if(_bestArrivingPaths[iState][iSurvivor].NoPathArrived())
            return iSurvivor;

        if(_bestArrivingPaths[iState][iSurvivor]._cost > worstCost)
        {
            iWorstCost = iSurvivor;
            worstCost = _bestArrivingPaths[iState][iSurvivor]._cost;
        }
    }

    return iWorstCost;
}

void PSPAlgorithm::PrintState(int iState)
{
    cout << "--------------- State " << iState << " ---------------" << endl;
    for(int iSurvivor=0;iSurvivor<_nSurvivors;iSurvivor++)
        if(!_exitStage[iState][iSurvivor].IsEmpty())
            _exitStage[iState][iSurvivor].Print();
}