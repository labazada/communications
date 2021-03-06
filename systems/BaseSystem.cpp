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
#include "BaseSystem.h"

#include <defines.h>
#include <bashcolors.h>
#include <typeinfo>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

#include <SingleUserPowerProfileDependentNoise.h>
#include <TimeInvariantChannel.h>
#include <BesselChannel.h>

extern uint32_t __mainSeedPassed;
extern uint32_t __statUtilSeedPassed;
extern uint __nFramesPassed;
extern bool __randomSeedHasBeenPassed;
extern bool __nFramesHasBeenPassed;

// #define EXPORT_REAL_DATA
// #define EXPORT_REAL_CHANNEL_ORDER

// #define PRINT_PARAMETERS
// #define PRINT_SYMBOLS_ACCOUNTED_FOR_DETECTION
// #define PRINT_SYMBOLS_ACCOUNTED_FOR_DETECTION_PER_FRAME

// #define STOP_AFTER_EACH_FRAME
// #define STOP_AFTER_EACH_SNR

#ifdef EXPORT_REAL_DATA
    MIMOChannel *realChannel;
    MatrixXd *realSymbols;
    Noise *realNoise;
#endif

#ifdef EXPORT_REAL_CHANNEL_ORDER
	int realChannelOrder;
#endif

BaseSystem::BaseSystem()
{
	_parametersFile.open("parameters.xml",ofstream::in);
	
	if(!_parametersFile.good())
		throw RuntimeException("BaseSystem::BaseSystem: cannot read from \"parameters.xml\"!!");
	
	// the xml file is read to memory...
	std::string parameters,parametersAux;
	getline(_parametersFile,parametersAux);
	while (_parametersFile) {
		parameters += parametersAux;
		getline(_parametersFile,parametersAux);
	}
	_parametersFile.close();
	
	// ...converted to a char* ...
	_parameters = new char[parameters.size()+1];
	strcpy (_parameters, parameters.c_str());
	
	// ...and then parsed
	_doc.parse<0>(_parameters);

	xml_node<> *thisSystemParameters = get_child(_doc.first_node(),"BaseSystem");
	
	if(!thisSystemParameters)
		throw RuntimeException("BaseSystem::BaseSystem: cannot find parameters for this system.");
	
	readParameterFromXML(thisSystemParameters,"randomSeeds",_randomSeeds);
	
	readParameterFromXML(thisSystemParameters,"loadSeeds",_loadSeeds);
	
	xml_node<> *mainRandomNode = get_child(thisSystemParameters,"mainRandom");
	if(!mainRandomNode)
		throw RuntimeException("BaseSystem::BaseSystem: cannot find parameter \"mainRandom\"");
	readParameterFromXML(mainRandomNode,"seed",_mainRandomSeed);
	readParameterFromXML(mainRandomNode,"haveStoredSample",_mainRandomHaveStoredSample);
	readParameterFromXML(mainRandomNode,"storedSample",_mainRandomStoredSample);
	
	
	readParameterFromXML(thisSystemParameters,"statUtilSeedToBeLoaded",_statUtilSeedToBeLoaded);
	
	readParameterFromXML(thisSystemParameters,"loadPerAlgorithmAndSNRseeds",_loadPerAlgorithmAndSNRseeds);
	readParameterFromXML(thisSystemParameters,"perAlgorithmAndSNRstatUtilSeedToBeLoaded",_perAlgorithmAndSNRstatUtilSeedToBeLoaded);

	readParameterFromXML(thisSystemParameters,"saveAtEveryFrame",_saveAtEveryFrame);
	readParameterFromXML(thisSystemParameters,"keepAllChannelEstimates",_keepAllChannelEstimates);
	
	try {
		readParameterFromXML(thisSystemParameters,"keepAllDetectedSymbols",_keepAllDetectedSymbols);
	} catch (RuntimeException)
	{
		// if this parameter has not been defined, a default value is set
		_keepAllDetectedSymbols = false;
	}
	
	readParameterFromXML(thisSystemParameters,"nFrames",_nFrames);
	readParameterFromXML(thisSystemParameters,"L",_L);
	readParameterFromXML(thisSystemParameters,"N",_N);
	readParameterFromXML(thisSystemParameters,"frameLength",_frameLength);
	readParameterFromXML(thisSystemParameters,"m",_m);
	
	try {
		readParameterFromXML(thisSystemParameters,"d",_d);
	} catch (RuntimeException)
	{
		std::cout << COLOR_WHITE << "assuming the smoothing lag is <channel order> - 1" << COLOR_NORMAL << std::endl;
		// smoothing factor
		_d = _m - 1;
	}
	
	readParameterFromXML(thisSystemParameters,"trainSeqLength",_trainSeqLength);
	readParameterFromXML(thisSystemParameters,"preambleLength",_preambleLength);
	readMultiValuedParameterFromXML(thisSystemParameters,"SNRs",_SNRs);
	
	readParameterFromXML(thisSystemParameters,"velocity",_velocity);
	readParameterFromXML(thisSystemParameters,"carrierFrequency",_carrierFrequency);
	readParameterFromXML(thisSystemParameters,"symbolRate",_symbolRate);
	
	xml_node<> *ARprocessNode = get_child(thisSystemParameters,"ARprocess");
	if(!ARprocessNode)
		throw RuntimeException("BaseSystem::BaseSystem: cannot find parameter \"ARprocess\"");
	readMultiValuedParameterFromXML(ARprocessNode,"coefficients",_ARcoefficients);
	readParameterFromXML(ARprocessNode,"variance",_ARvariance);
	
	bool overrideGivenARparametersWithYuleWalkerComputedOnes;
	readParameterFromXML(thisSystemParameters,"overrideGivenARparametersWithYuleWalkerComputedOnes",overrideGivenARparametersWithYuleWalkerComputedOnes);
	
	readParameterFromXML(thisSystemParameters,"noiseClassToBeInstantiated",_noiseClassToBeInstantiated);
	readParameterFromXML(thisSystemParameters,"channelClassToBeInstantiated",_channelClassToBeInstantiated);
	
	// the training sequence is included in the frame
	assert(_frameLength>_trainSeqLength);

	// ==================================== derived parameters ====================================

	
	// "in principle" the algorithms require as many smoothing symbol vectors as "d" (though it's not always like that, .e.g,  when estimating the channel order)
	_nSmoothingSymbolsVectors = _d;
	
    // BER and MSE computing
    _symbolsDetectionWindowStart = _trainSeqLength;
    _MSEwindowStart = _frameLength*9/10;
	
	// it's assumed that velocity is given in km/h...
	_velocity /= 3.6;
	
	_T = 1.0/_symbolRate;

	// ---------------------------------------------------------------------------------------------
	
	if(overrideGivenARparametersWithYuleWalkerComputedOnes)
	{
		// the AR process parameters are overwritten with those computed from the "velocity", "carrierFrequency" and "symbolRate" using Yule-Walker equations
		_ARcoefficients = ARprocess::parametersFromYuleWalker(_ARcoefficients.size(),_velocity,_carrierFrequency,_T,_ARvariance);
		std::cout << COLOR_ALERT << "setting AR coefficients" << COLOR_NORMAL << " (" << _ARcoefficients << ") " << COLOR_ALERT << 
									"and variance" << COLOR_NORMAL << " (" << _ARvariance << COLOR_NORMAL << ") " << COLOR_ALERT << "computed from Yule-Walker equations:" << COLOR_NORMAL << std::endl;
	}
	
	if(!_randomSeeds)
	{
		StatUtil::getRandomGenerator().setSeed(4135925433);
		StatUtil::_particlesInitializerRandomGenerator.setSeed(2484546298);
		_randomGenerator.setSeed(3763650855);
	}

#ifdef EXPORT_REAL_CHANNEL_ORDER
	realChannelOrder = _m;
#endif

    // alphabet is defined
    std::vector<std::vector<tBit> > alphabetBitSequences(2,std::vector<tBit>(1));
    alphabetBitSequences[0][0] = 0; alphabetBitSequences[1][0] = 1;
    std::vector<tSymbol> alphabetSymbols(2);
    alphabetSymbols[0] = -1; alphabetSymbols[1] = 1;
    _alphabet = new Alphabet(alphabetBitSequences,alphabetSymbols);

    // host name is concatenated into the file name
    char hostname[HOSTNAME_LENGTH];
    gethostname(hostname,HOSTNAME_LENGTH);

    // get present time of the system
    time_t presentTime;
    time(&presentTime);
	char *presentTimeString = ctime(&presentTime);
    presentTimeString[strlen(presentTimeString)-1] = '\0';
    for(int i=strlen(presentTimeString)-1;i>=0;i--)
        if(presentTimeString[i]==' ')
            presentTimeString[i]='_';

	// the name of the results file is built
	_resultsFile = std::string("res_") + std::string(hostname) + std::string("_") + std::string(presentTimeString);
	
	// the name for the temporal file is obtained from the final one	
	_tmpResultsFile = std::string("tmp_") + _resultsFile;

    // a specific preamble is generated...
    _preamble = MatrixXd::Zero(1,1);
    _preamble.resize(_N,_preambleLength);
    if(_preamble.size()>0)
        _preamble.setConstant(-1.0);
    
    // which symbols are to be taken into account when detecting
    _isSymbolAccountedForDetection = vector<vector<bool> >(_N,vector<bool>(_frameLength));

    // the preamble symbols before symbolsDetectionWindowStart are ignored for detection
    for(uint iTime=0;iTime<_symbolsDetectionWindowStart;iTime++)
        for(uint iInput=0;iInput<_N;iInput++)
            _isSymbolAccountedForDetection[iInput][iTime] = false;        
  
    for(uint iTime=_symbolsDetectionWindowStart;iTime<_frameLength;iTime++)
        for(uint iInput=0;iInput<_N;iInput++)
            _isSymbolAccountedForDetection[iInput][iTime] = true;
		
	_isChannelEstimateAccountedForMSE = vector<bool>(_frameLength,true);
	for(uint iTime=0;iTime<_MSEwindowStart;iTime++)
		_isChannelEstimateAccountedForMSE[iTime] = false;
	
    
#ifdef PRINT_SYMBOLS_ACCOUNTED_FOR_DETECTION
    cout << "isSymbolAccountedForDetection" << endl;
    Util::print(isSymbolAccountedForDetection);
#endif    
    
    // ambiguity resolution
    uint *firstPermutation = new uint[_N];
    for(uint i=0;i<_N;i++) firstPermutation[i] = i;
    _permutations = Util::permutations(firstPermutation,_N);
    delete[] firstPermutation;

    _peMatrices.reserve(_nFrames);
    _MSEMatrices.reserve(_nFrames);
	
    _overallPeTimeEvolution.resize(_SNRs.size());
    _overallErrorsNumberTimeEvolution.resize(_SNRs.size());
	
    _mainRandoms.reserve(_nFrames);
    _statUtilRandoms.reserve(_nFrames);
	
#ifdef SAVE_ALL_SEEDS
	_perAlgorithmAndSNRstatUtilRandoms.reserve(_nFrames);
#endif

#ifdef KEEP_ALL_CHANNEL_MATRICES
	_channelMatrices.reserve(_nFrames);
#endif
	
#ifdef SAVE_CHANNEL_ESTIMATES_VARIANCES
		_channelEstimatesVariances.reserve(_nFrames);
#endif
		
#ifdef SAVE_NOISE_VARIANCES
	_presentFrameNoiseVariances = std::vector<double>(_SNRs.size());
	_noiseVariances.reserve(_nFrames);
#endif
	
	if(_keepAllChannelEstimates)
		_channelEstimates.reserve(_nFrames);

	if(_keepAllDetectedSymbols)
		_allDetectedSymbols.reserve(_nFrames);

    _channel = NULL;
    _powerProfile = NULL;
	_noise = NULL;
}

BaseSystem::~BaseSystem()
{
    delete _alphabet;
	delete[] _parameters;
}

void BaseSystem::simulate()
{
	
	// the frame length in bits is
    _nBitsGenerated = (_frameLength+_nSmoothingSymbolsVectors)*_alphabet->nBitsPerSymbol();
	 //this is here just in case the value "_nSmoothingSymbolsVectors" is modified by a subclass of BaseSystem (e.g. "TesisOrdenCanalDesconocidoSystem")

// if a number of frames has been passed...
if(__nFramesHasBeenPassed)
{
	_nFrames = __nFramesPassed;
	cout << COLOR_LIGHT_BLUE << _nFrames << " frames are going to be simulated." << COLOR_NORMAL << endl;
}

    // for repeating simulations
    if(_loadSeeds)
	{
	
		if(__randomSeedHasBeenPassed)
		{
			_randomGenerator.setSeed(__mainSeedPassed);
			StatUtil::getRandomGenerator().setSeed(__statUtilSeedPassed);
		}else
		{
			_randomGenerator.setSeed(_mainRandomSeed);
			if(_mainRandomHaveStoredSample)
				_randomGenerator.setStoredSample(_mainRandomStoredSample);
			StatUtil::getRandomGenerator().setSeed(_statUtilSeedToBeLoaded);
		}

		cout << COLOR_LIGHT_BLUE << "seeds are being loaded..." << COLOR_NORMAL << endl;
		cout << COLOR_LIGHT_BLUE << "\t " << _randomGenerator.getSeed() << endl << "\t " << StatUtil::getRandomGenerator().getSeed() << COLOR_NORMAL << endl;

	}

    _iFrame = 0;
    while((_iFrame<_nFrames) && (!__done))
    {

        // the seeds are kept for saving later
		_mainRandoms.push_back(_randomGenerator);
		_statUtilRandoms.push_back(StatUtil::getRandomGenerator());

        // bits are generated ...
        Bits generatedBits(_N,_nBitsGenerated,_randomGenerator);

		// differential modulation
// 		bits = bits.differentialEncoding();

        // ... and then modulated by means of the alphabet
        MatrixXd symbolsWithoutPreamble = Modulator::modulate(generatedBits,*_alphabet);

        // the preamble is set before the symbols to be transmitted
        if(_preamble.size()>0)
        {
            _symbols.resize(_preamble.rows(),_preamble.cols()+symbolsWithoutPreamble.cols());
            _symbols << _preamble,symbolsWithoutPreamble;
        }else
            _symbols = symbolsWithoutPreamble;

        // all the above symbols must be processed except those generated due to the smoothing
        _iLastSymbolVectorToBeDetected = _symbols.cols() - _nSmoothingSymbolsVectors;

		// this method was introduced to initialize some variables that may be required in the build of the channel...
        buildSystemSpecificVariables();

		// ...carried out by this other method
		_channel =  buildChannel();

#ifdef PRINT_PARAMETERS
		std::cout << "the true symbols:" << std::endl << _symbols << std::endl;
        std::cout << "iLastSymbolVectorToBeDetected = " << _iLastSymbolVectorToBeDetected << std::endl;
#endif

#ifdef PRINT_SYMBOLS_ACCOUNTED_FOR_DETECTION_PER_FRAME
    cout << "isSymbolAccountedForDetection" << endl;
    Util::print(isSymbolAccountedForDetection);
#endif    

	_noise = buildNoise();
	
	assert(_noise!=NULL);

#ifdef EXPORT_REAL_DATA
            realSymbols = &_symbols;
            realChannel = _channel;
            realNoise = _noise;
#endif
			
        for(_iSNR=0;_iSNR<_SNRs.size();_iSNR++)
        {
            cout << COLOR_FRAME_NUMBER_SNR << "SNR = " << COLOR_NORMAL << _SNRs[_iSNR] << COLOR_FRAME_NUMBER_SNR << " [Frame " << COLOR_NORMAL << _iFrame << COLOR_FRAME_NUMBER_SNR << "]..." << COLOR_NORMAL << endl;

            // noise SNR is set
			_noise->setSNR(_SNRs[_iSNR]);

#ifdef PRINT_NOISE
			cout << "noise is" << endl;
			noise->print();
			cout << endl;
#endif

            // transmission
            _observations = _channel->transmit(_symbols,*_noise);

             addAlgorithms();

            // here the number of algoriths is known. So, the first iteration:
            if(_iFrame==0 && _iSNR==0)
                onlyOnce();
			

#ifdef SAVE_NOISE_VARIANCES
			uint iNoiseVariance = _frameLength/2;
			_presentFrameNoiseVariances[_iSNR] = _noise->variances()[iNoiseVariance];
// 			cout << COLOR_INFO << "noise variance at time " << COLOR_NORMAL << _frameLength/2 << ": " << _noise->variances()[iNoiseVariance] << endl;
#endif

            // algorithms are executed
            for(_iAlgorithm=0;_iAlgorithm<_algorithms.size();_iAlgorithm++)
            {
				if(_loadPerAlgorithmAndSNRseeds)
				{
					StatUtil::getRandomGenerator().setSeed(_perAlgorithmAndSNRstatUtilSeedToBeLoaded);

					cout << COLOR_LIGHT_PINK << "per ALGORITHM and SNR seeds are being loaded..." << COLOR_NORMAL << endl;
					cout << COLOR_LIGHT_PINK << "\t " << _randomGenerator.getSeed() << endl << "\t " << StatUtil::getRandomGenerator().getSeed() << COLOR_NORMAL << endl;
				}

#ifdef SAVE_ALL_SEEDS
				_thisFramePerAlgorithmAndSNRstatUtilRandoms[_iSNR][_iAlgorithm] = StatUtil::getRandomGenerator();
#endif
                // if there is training sequence
                if(_trainSeqLength!=0)
                    _algorithms[_iAlgorithm]->run(_observations,_noise->variances(),_symbols.block(0,_preambleLength,_N,_trainSeqLength));
                // if there is NOT training sequence
                else
                    _algorithms[_iAlgorithm]->run(_observations,_noise->variances());

				if(_algorithms[_iAlgorithm]->performsSymbolsDetection())
				{
					_detectedSymbols = _algorithms[_iAlgorithm]->getDetectedSymbolVectors();
					
					if(_keepAllDetectedSymbols)
						_presentFrameAllDetectedSymbols[_iSNR][_iAlgorithm] = _detectedSymbols;
					
					// if there is no training sequence
					if(_trainSeqLength==0)
						_pe = computeSER(_symbols.block(0,_preambleLength,_N,_frameLength),_detectedSymbols,_isSymbolAccountedForDetection,_iBestPermutation,_bestPermutationSigns);
					else
					{
						_pe = computeSERwithoutSolvingAmbiguity(_symbols.block(0,_preambleLength,_N,_frameLength),_detectedSymbols,_isSymbolAccountedForDetection);
// 						std::cout << "no need to solve ambiguity..." << std::endl;
					}
				}
				// if the algorithm doesn't perform symbols detection...
				else
				{
					// we assign a meaningless (flag) value to the probability of error
					_pe = FUNNY_VALUE;
					
					if(_keepAllDetectedSymbols)
						_presentFrameAllDetectedSymbols[_iSNR][_iAlgorithm] = MatrixXd::Constant(_N,_frameLength,FUNNY_VALUE);
				}

                beforeEndingAlgorithm();

                delete _algorithms[_iAlgorithm];
            }

            _algorithms.clear();
			
#ifdef STOP_AFTER_EACH_SNR
		getchar();
#endif
        } // for(uint iSNR=0;iSNR<SNRs.size();iSNR++)
		
		storeFrameResults();
		if(_saveAtEveryFrame || _iFrame==_nFrames-1 || __done || __saveData)
		{
			_f.open(_tmpResultsFile.c_str(),ofstream::trunc);
			saveFrameResults();
			_f.close();
			
			// the temporal file is renamed as the final
			std::string mvCommand = std::string(MV_COMMAND) + std::string(" ") + _tmpResultsFile + std::string(" ") + _resultsFile;
			int systemCommandReturn = system(mvCommand.c_str());
			cout << COLOR_INFO << "moving operation returned " << COLOR_NORMAL << systemCommandReturn << endl;
			
			// the flag variable is reset in case the corresponding signal is sent again
			__saveData = false;
		}

        // ---------------------------------------------------------

        _iFrame++;

        delete _channel;
		_channel = NULL;
		
        delete _noise;
		_noise = NULL;
		
#ifdef STOP_AFTER_EACH_FRAME
		getchar();
#endif
    } // while((iFrame<nFrames) && (!done))
    
    // a symbolic link pointing to the results file is created
	std::string lnCommand = std::string(LN_COMMAND) + std::string(" -sf ") + _resultsFile + std::string(" ") + std::string(SYMBOLIC_LINK_NAME);
	std::cout << COLOR_INFO << "created symbolic link: " << COLOR_NORMAL << "res_last" << " -> " << _resultsFile << COLOR_INFO << " (" << system(lnCommand.c_str()) << ") " << COLOR_NORMAL << std::endl;

    _overallPeMatrix *= 1.0/_iFrame;
    _overallMseMatrix *= 1.0/_iFrame;

    cout << "Overall SER:" << endl;
    Util::print(_overallPeMatrix);

    cout << "Overall MSE:" << endl;
    Util::print(_overallMseMatrix);
}

void BaseSystem::onlyOnce()
{
    _overallPeMatrix = MatrixXd::Zero(_SNRs.size(),_algorithms.size());
    _presentFramePe = MatrixXd::Zero(_SNRs.size(),_algorithms.size());

    _overallMseMatrix = MatrixXd::Zero(_SNRs.size(),_algorithms.size());
    _presentFrameMSE = MatrixXd::Zero(_SNRs.size(),_algorithms.size());

    // Pe evolution
    for(uint i=0;i<_SNRs.size();i++)
    {
        _overallPeTimeEvolution[i] = MatrixXd(_algorithms.size(),_frameLength);
        _overallErrorsNumberTimeEvolution[i] = MatrixXi::Zero(_algorithms.size(),_frameLength);
    }

    // we fill the vector with the names of the algorithms
    for(uint iAlgorithm=0;iAlgorithm<_algorithms.size();iAlgorithm++)
        _algorithmsNames.push_back(_algorithms[iAlgorithm]->getName());

#ifdef SAVE_ALL_SEEDS
// 		_thisFramePerAlgorithmAndSNRstatUtilSeeds = std::vector<std::vector<uint32_t> > (_SNRs.size(),std::vector<uint32_t>(_algorithms.size(),0));
		_thisFramePerAlgorithmAndSNRstatUtilRandoms = std::vector<std::vector<Random> > (_SNRs.size(),std::vector<Random>(_algorithms.size()));
#endif

	if(_keepAllChannelEstimates)
		// channel estimations (iChannelMatrixRow,iChannelMatrixCol,iTimeInstant,iAlgorithmm,iSNR)
		_presentFrameChannelMatrixEstimates = std::vector<std::vector<std::vector<MatrixXd> > >(_SNRs.size(),std::vector<std::vector<MatrixXd> >(_algorithms.size()));
	
	if(_keepAllDetectedSymbols)
		_presentFrameAllDetectedSymbols = std::vector<std::vector<MatrixXd> >(_SNRs.size(),std::vector<MatrixXd>(_algorithms.size()));
	
#ifdef SAVE_CHANNEL_ESTIMATES_VARIANCES
		_presentFrameChannelEstimatesVariances = std::vector<std::vector<std::vector<MatrixXd> > >(_SNRs.size(),std::vector<std::vector<MatrixXd> >(_algorithms.size()));
#endif
}

void BaseSystem::beforeEndingAlgorithm()
{	
	if(_algorithms[_iAlgorithm]->performsChannelEstimation())
	{
		// the channel matrices estimated by the algorithm are stored
		std::vector<MatrixXd> estimatedChannelMatrices = _algorithms[_iAlgorithm]->getEstimatedChannelMatrices();
		std::vector<MatrixXd> toCheckEstimatedChannelMatrices(estimatedChannelMatrices.begin()+_MSEwindowStart,estimatedChannelMatrices.end());

// 		_mse = computeMSE(_channel->range(_preambleLength+_MSEwindowStart,_iLastSymbolVectorToBeDetected-1),toCheckEstimatedChannelMatrices);
		_mse = computeMSE(_channel->range(_preambleLength,_iLastSymbolVectorToBeDetected-1),_algorithms[_iAlgorithm]->getEstimatedChannelMatrices(),_isChannelEstimateAccountedForMSE);
	}else
	{
	  _mse = FUNNY_VALUE;
	}

	if(_keepAllChannelEstimates)
	{
		vector<MatrixXd> thisAlgorithmEstimatedChannelMatrices;
		
		if(_algorithms[_iAlgorithm]->performsChannelEstimation())
			// we get the channel matrices estimated by this algorithm
			thisAlgorithmEstimatedChannelMatrices = _algorithms[_iAlgorithm]->getEstimatedChannelMatrices();
		else
			// we generate a sequence of matrices initialized to a "funny" value
			thisAlgorithmEstimatedChannelMatrices = vector<MatrixXd>(_iLastSymbolVectorToBeDetected-_preambleLength,MatrixXd::Constant(_channel->channelCoefficientsMatrixRows(),_channel->channelCoefficientsMatrixCols(),FUNNY_VALUE));

		_presentFrameChannelMatrixEstimates[_iSNR][_iAlgorithm] = thisAlgorithmEstimatedChannelMatrices;
	}
	
#ifdef SAVE_CHANNEL_ESTIMATES_VARIANCES
		vector<MatrixXd> thisAlgorithmChannelEstimatesVariances;
		
		if(_algorithms[_iAlgorithm]->computesChannelEstimatesVariances())
			// we get the channel matrices estimated by this algorithm
			thisAlgorithmChannelEstimatesVariances = _algorithms[_iAlgorithm]->getChannelEstimatesVariances();
		else
			// we generate a sequence of matrices initialized to a "funny" value
			thisAlgorithmChannelEstimatesVariances = vector<MatrixXd>(_iLastSymbolVectorToBeDetected-_preambleLength,MatrixXd::Constant(_channel->channelCoefficientsMatrixRows(),_channel->channelCoefficientsMatrixCols(),FUNNY_VALUE));

		_presentFrameChannelEstimatesVariances[_iSNR][_iAlgorithm] = thisAlgorithmChannelEstimatesVariances;
		
#endif

    cout << COLOR_GREEN << _algorithms[_iAlgorithm]->getName() << COLOR_NORMAL << ": Pe = " << _pe << " , MSE = " << _mse << endl;

    // the error probability is accumulated
    _overallPeMatrix(_iSNR,_iAlgorithm) += _pe;
    _presentFramePe(_iSNR,_iAlgorithm) = _pe;

    // and the MSE
    _overallMseMatrix(_iSNR,_iAlgorithm) += _mse;
    _presentFrameMSE(_iSNR,_iAlgorithm) = _mse;

    // Pe evolution
    MatrixXd transmittedSymbols = _symbols.block(0,_preambleLength,_N,_frameLength);

    if(_detectedSymbols.rows()!=0)
    {
        for(uint k=0;k<_frameLength;k++)
            for(uint iUser=0;iUser<_N;iUser++)
                if(_detectedSymbols(iUser,k)!=transmittedSymbols(iUser,k))
                    _overallErrorsNumberTimeEvolution[_iSNR](_iAlgorithm,k)++;
    }
}

double BaseSystem::computeSER(const MatrixXd& sourceSymbols, const MatrixXd& detectedSymbols, const std::vector< std::vector< bool > >& mask, uint &iBestPermutation, std::vector< int > &bestPermutationSigns)
{
	// best permutation and its signs are initialized...
    iBestPermutation = 0;
	bestPermutationSigns = vector<int>(sourceSymbols.rows(),1);

	// the dimensions of "sourceSymbols" are the same as those of "detectedSymbols"
	assert(sourceSymbols.rows() == detectedSymbols.rows() && sourceSymbols.cols()== detectedSymbols.cols());
	
	// ...and the same as those of the mask
	assert(detectedSymbols.rows() == uint(mask.size()) && detectedSymbols.cols() == uint(mask[0].size()));

    vector<int> thisPermutationSigns(sourceSymbols.rows());

    // min number of errors
    uint minErrors = sourceSymbols.rows()*sourceSymbols.cols();
    
    uint nAccountedSymbols = 0;
    uint iInput;

    for(uint iPermut=0;iPermut<_permutations.size();iPermut++)
    {
        uint permutationErrors = 0;
        
        for(uint iStream=0;iStream<_permutations[iPermut].size();iStream++)
        {
            iInput = _permutations[iPermut][iStream];
          
            uint currentStreamErrorsInverting=0,currentStreamErrorsWithoutInverting=0;
            
            for(uint iTime=0;iTime<sourceSymbols.cols();iTime++)
            {
                // if this symbol is not accounted for
                if(!mask[iStream][iTime])
                    continue;

                // if the symbols differ, an error happened...
                currentStreamErrorsWithoutInverting += (sourceSymbols(iStream,iTime) != detectedSymbols(iInput,iTime));
                
                // ...unless the symbol sign needs to be switched because of the ambiguity
                currentStreamErrorsInverting += (sourceSymbols(iStream,iTime) != _alphabet->opposite(detectedSymbols(iInput,iTime)));
                
                nAccountedSymbols++;
            }              

            if(currentStreamErrorsWithoutInverting<currentStreamErrorsInverting)
            {
                permutationErrors += currentStreamErrorsWithoutInverting;
                thisPermutationSigns[iStream] = 1;
            }
            else
            {
                permutationErrors += currentStreamErrorsInverting;
                thisPermutationSigns[iStream] = -1;
            }
        } // for(uint iStream=0;iStream<permutations[iPermut].size();iStream++)
        
        if(permutationErrors<minErrors)
        {
            minErrors = permutationErrors;
            iBestPermutation = iPermut;
			bestPermutationSigns = thisPermutationSigns;
        }
    } // for(uint iPermut=0;iPermut<_permutations.size();iPermut++)
    
    // every symbols should have been accounted as many times as permutations
	assert(nAccountedSymbols % _permutations.size() == 0);
    
    nAccountedSymbols /= _permutations.size();
    
    // if all the symbols were masked
    if(nAccountedSymbols==0)
      return 0.0;
    else
      return double(minErrors)/double(nAccountedSymbols);
}

double BaseSystem::computeMSE(const vector<MatrixXd> &realChannelMatrices,const vector<MatrixXd> &estimatedChannelMatrices, const std::vector<bool> &mask) const
{
    uint nRealChannelMatrices = realChannelMatrices.size();
	
	assert(nRealChannelMatrices==estimatedChannelMatrices.size());
	assert(nRealChannelMatrices==mask.size());

    double mse = 0;
	uint nChannelEstimatesAccountedFor = 0;
	
	for(uint i=0;i<nRealChannelMatrices;i++)
	{
		if(!mask[i])
			continue;
		
		// the square error committed by the estimated matrix is normalized by the squared Frobenius norm (i.e. the sum of all the elements squared) of the real channel matrix.
		// also notice that if the channel is Sparkling memory, the channel matrices of the real channel may have different sizes
		mse += Util::squareErrorPaddingWithZeros(realChannelMatrices.at(i),estimatedChannelMatrices.at(i))/realChannelMatrices.at(i).squaredNorm();
		
		nChannelEstimatesAccountedFor++;	
	}

	if(nChannelEstimatesAccountedFor==0)
		throw DivisionByZero();
	else
		return mse/(double)nChannelEstimatesAccountedFor;
}

double BaseSystem::computeMSE(const vector<MatrixXd> &realchannelMatrices,const vector<MatrixXd> &estimatedChannelMatrices,const std::vector<bool> &mask,const vector<uint> &bestPermutation,const vector<int> &bestPermutationSigns) const
{
	vector<uint> realChannelMatricesPermutation = Util::computeInversePermutation(bestPermutation);

	// the signs permutation is given permuted WITH RESPECT TO THE ESTIMATED CHANNEL MATRICES: we have to permute them according to the best permutation with respect to the real channel matrices
	vector<int> realChannelMatricesSignsPermutation = Util::applyPermutation(Util::applyPermutation(bestPermutationSigns,realChannelMatricesPermutation),realChannelMatricesPermutation);

	vector<MatrixXd> permutedRealChannelMatrices(realchannelMatrices.size());

	for (uint i=0;i<realchannelMatrices.size();i++)
		permutedRealChannelMatrices[i] = Util::applyPermutationOnColumns(realchannelMatrices[i],realChannelMatricesPermutation,realChannelMatricesSignsPermutation);

	return BaseSystem::computeMSE(permutedRealChannelMatrices,estimatedChannelMatrices,mask);
}

double BaseSystem::computeSymbolVectorErrorRate(const MatrixXd& sourceSymbols, const MatrixXd& detectedSymbols, const std::vector< std::vector< bool > >& mask) const
{ 
	assert( (sourceSymbols.rows() == detectedSymbols.rows()) && (static_cast<uint>(detectedSymbols.rows())== mask.size()) );
	assert( (sourceSymbols.cols()== detectedSymbols.cols()) && (static_cast<uint>(detectedSymbols.cols())== mask[0].size()) );

	// number of errors
	uint errors = 0;

	uint nAccountedVectors = 0;

	for (uint iTime=0;iTime<sourceSymbols.cols();iTime++)
	{
		// the first symbol of the vector is used to decide whether the entire vector should be accounted for detection or not
		if (!mask[0][iTime])
			continue;

		// if the vectors differ, an error happened...
		errors += sourceSymbols.col(iTime)!=detectedSymbols.col(iTime);
		
		nAccountedVectors++;
	}

	// if all the symbols were masked
	if (nAccountedVectors==0)
		return 0.0;
	else
		return (double)errors/(double)(nAccountedVectors);
}

void BaseSystem::storeFrameResults()
{
    // pe
    _peMatrices.push_back(_presentFramePe);

    // MSE
    _MSEMatrices.push_back(_presentFrameMSE);

#ifdef SAVE_ALL_SEEDS
// 	_perAlgorithmAndSNRstatUtilSeeds.push_back(_thisFramePerAlgorithmAndSNRstatUtilSeeds);		
    _perAlgorithmAndSNRstatUtilRandoms.push_back(_thisFramePerAlgorithmAndSNRstatUtilRandoms);
#endif

#ifdef KEEP_ALL_CHANNEL_MATRICES
    _channelMatrices.push_back(_channel->range(_preambleLength,_iLastSymbolVectorToBeDetected-1));
#endif

    if (_keepAllChannelEstimates)
        _channelEstimates.push_back(_presentFrameChannelMatrixEstimates);
	
	if(_keepAllDetectedSymbols)
		_allDetectedSymbols.push_back(_presentFrameAllDetectedSymbols);

#ifdef SAVE_CHANNEL_ESTIMATES_VARIANCES
    _channelEstimatesVariances.push_back(_presentFrameChannelEstimatesVariances);
#endif
	
#ifdef SAVE_NOISE_VARIANCES
	_noiseVariances.push_back(_presentFrameNoiseVariances);
#endif
}

void BaseSystem::saveFrameResults()
{
    // pe
    Octave::toOctaveFileStream(_peMatrices,"pe",_f);

    // MSE
    Octave::toOctaveFileStream(_MSEMatrices,"mse",_f);

    Octave::toOctaveFileStream(_iFrame+1,"nFrames",_f);
    Octave::toOctaveFileStream(_algorithmsNames,"algorithmsNames",_f);
    Octave::toOctaveFileStream(_L,"L",_f);
    Octave::toOctaveFileStream(_N,"N",_f);
    Octave::toOctaveFileStream(_m,"m",_f);
    Octave::toOctaveFileStream(_frameLength,"frameLength",_f);
    Octave::toOctaveFileStream(_trainSeqLength,"trainSeqLength",_f);
    Octave::toOctaveFileStream(_d,"d",_f);
    Octave::toOctaveFileStream(_symbolsDetectionWindowStart,"symbolsDetectionWindowStart",_f);
    Octave::toOctaveFileStream(_MSEwindowStart,"MSEwindowStart",_f);
    Octave::toOctaveFileStream(_SNRs,"SNRs",_f);
    Octave::toOctaveFileStream(_preamble,"preamble",_f);
    Octave::toOctaveFileStream(_nSmoothingSymbolsVectors,"nSmoothingSymbolsVectors",_f);    
    Octave::toOctaveFileStream(_preambleLength,"preambleLength",_f);
	
	Random::toOctaveFileStream(_mainRandoms,"mainRandoms",_f);
	Random::toOctaveFileStream(_statUtilRandoms,"statUtilRandoms",_f);

#ifdef SAVE_ALL_SEEDS
	Random::toOctaveFileStream(_perAlgorithmAndSNRstatUtilRandoms,"perAlgorithmAndSNRstatUtilRandoms",_f);
#endif
	
	// NOTE: this is only saved for the last frame!!
	Octave::toOctaveFileStream(_observations,"observations",_f);
	Octave::toOctaveFileStream(_noise->range(_preambleLength,_iLastSymbolVectorToBeDetected-1),"noise",_f);

	// static_cast is needed so that the method can recognize the value returned by the "block" method as a MatrixXd
	Octave::toOctaveFileStream(static_cast<MatrixXd> (_symbols.block(0,_preambleLength,_N,_frameLength)),"symbols",_f);
	
#ifdef KEEP_ALL_CHANNEL_MATRICES
	Octave::toOctaveFileStream(_channelMatrices,"channels",_f);
#else
	// only last channel is saved
    Octave::toOctaveFileStream(_channel->range(_preambleLength,_iLastSymbolVectorToBeDetected-1),"channel",_f);
#endif

	Octave::toOctaveFileStream(vector<std::string>(1,_channel->name()),"channelClass",_f);
    Octave::toOctaveFileStream(vector<std::string>(1,std::string(typeid(*_noise).name())),"noiseClass",_f);
    Octave::toOctaveFileStream(vector<std::string>(1,std::string(typeid(*this).name())),"systemClass",_f);

    if(_powerProfile!=NULL)
    {
        Octave::toOctaveFileStream(_powerProfile->tapsPowers(),"powerProfileVariances",_f);
        Octave::toOctaveFileStream(vector<std::string>(1,std::string(typeid(*_powerProfile).name())),"powerProfileClass",_f);
    }
    
	if(_keepAllChannelEstimates)
		Octave::toOctaveFileStream(_channelEstimates,"channelEstimates",_f);
	
	if(_keepAllDetectedSymbols)
		Octave::toOctaveFileStream(_allDetectedSymbols,"allDetectedSymbols",_f);
	
#ifdef SAVE_CHANNEL_ESTIMATES_VARIANCES
		Octave::toOctaveFileStream(_channelEstimatesVariances,"channelEstimatesVariances",_f);
#endif

	Octave::toOctaveFileStream(_velocity,"velocity",_f);
	Octave::toOctaveFileStream(_carrierFrequency,"carrierFrequency",_f);
	Octave::toOctaveFileStream(_symbolRate,"symbolRate",_f);
	Octave::toOctaveFileStream(_T,"T",_f);

	Octave::toOctaveFileStream(_ARcoefficients,"ARcoefficients",_f);
	Octave::toOctaveFileStream(_ARvariance,"ARvariance",_f);
	
#ifdef SAVE_NOISE_VARIANCES
	Octave::toOctaveFileStream(_noiseVariances,"noiseVariances",_f);
#endif
}

xml_node<>* BaseSystem::get_child(xml_node<> *inputNode, std::string sNodeFilter)
{
    // cycles every child
    for (xml_node<> *nodeChild = inputNode->first_node(); nodeChild; nodeChild = nodeChild->next_sibling())
    {
        if (nodeChild->name() == sNodeFilter)
        {
//             cout << "node name " << nodeChild->name() << "\n";
//             cout << "nodeChild " << nodeChild << endl;
            // returns the desired child
            return nodeChild;
        }
        xml_node<> * x = get_child(nodeChild, sNodeFilter);
        if (x) 
          return x;
    }
    return 0;
}

template<class T> void BaseSystem::readParameterFromXML(xml_node<> *parentNode,std::string xmlName,T &parameter)
{
	xml_node<> *parameterNode = get_child(parentNode,xmlName); 
	if(!parameterNode)
		throw RuntimeException(std::string("BaseSystem::readParameterFromXML: cannot find parameter \"")+xmlName+"\"");
	
	std::istringstream value;
	value.str(parameterNode->value());
	value >> parameter;
}
template void BaseSystem::readParameterFromXML(xml_node<> *parentNode,std::string xmlName,int &parameter);

template<class T> void BaseSystem::readMultiValuedParameterFromXML(xml_node<> *parentNode,std::string xmlName,std::vector<T> &vector)
{
	xml_node<> *parameterNode = get_child(parentNode,xmlName);
	if(!parameterNode)
		throw RuntimeException(std::string("BaseSystem::readMultiValuedParameterFromXML: cannot find parameter \"")+xmlName+"\"");
	
	std::istringstream value;
	
	for (xml_node<> *nodeChild = parameterNode->first_node(); nodeChild; nodeChild = nodeChild->next_sibling())
	{
		T tmp;
		value.clear(); value.str(nodeChild->value()); value >> tmp;
		vector.push_back(tmp);
	}
}
template void BaseSystem::readMultiValuedParameterFromXML(xml_node<> *parentNode,std::string xmlName,std::vector<double> &vector);

Noise *BaseSystem::buildNoise() const
{
	if(!_noiseClassToBeInstantiated.compare(NullNoise::getXMLname()))
		return new NullNoise(_L,_channel->length());
	else if(!_noiseClassToBeInstantiated.compare(ChannelDependentNoise::getXMLname()))
		return new ChannelDependentNoise(_alphabet->variance(),_channel);
	else if(!_noiseClassToBeInstantiated.compare(PowerProfileDependentNoise::getXMLname()))
	{
		assert(_powerProfile!=NULL);
		return new PowerProfileDependentNoise(_alphabet->variance(),_L,_channel->length(),*_powerProfile);
	}
	else
		throw RuntimeException(std::string("BaseSystem::buildNoise: unknown Noise class \"") + _noiseClassToBeInstantiated + std::string("\" cannot be instantiated."));
		
}

MIMOChannel *BaseSystem::buildChannel()
{
	assert(_powerProfile!=NULL);
	
	if(!_channelClassToBeInstantiated.compare(TimeInvariantChannel::getXMLname()))
		return new TimeInvariantChannel(_N,_L,_m,_symbols.cols(),_powerProfile->generateChannelMatrix(_randomGenerator));
	else if(!_channelClassToBeInstantiated.compare(BesselChannel::getXMLname()))
		return new BesselChannel(_N,_L,_m,_symbols.cols(),_velocity,_carrierFrequency,_T,*_powerProfile);
	else if(!_channelClassToBeInstantiated.compare(ARchannel::getXMLname()))
		return new ARchannel(_N,_L,_m,_symbols.cols(),ARprocess(_powerProfile->generateChannelMatrix(_randomGenerator),_ARcoefficients,_ARvariance));
	else
		throw RuntimeException(std::string("BaseSystem::buildChannel: unknown MIMOChannel class \"") + _channelClassToBeInstantiated + std::string("\" cannot be instantiated."));
}