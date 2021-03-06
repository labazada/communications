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
#include "CDMASystem.h"

#include <CDMAunknownActiveUsersSISoptWithNoUsersActivityKnowledge.h>
#include <KnownFlatChannelOptimalAlgorithm.h>
#include <KnownFlatChannelAndActiveUsersOptimalAlgorithm.h>
#include <UnknownActiveUsersLinearFilterBasedSMCAlgorithm.h>
#include <CDMAunknownActiveUsersSISopt.h>
#include <TimeInvariantChannel.h>
#include <MultiuserCDMAchannel.h>
#include <ViterbiAlgorithmWithAprioriProbabilities.h>
#include <PSPAlgorithmWithAprioriProbabilities.h>
#include <FRSsBasedUserActivityDetectionAlgorithm.h>

#include <math.h>
#include <algorithm>

using namespace std; 

#include <bashcolors.h>
#include <defines.h>
#include <SingleUserChannelDependentNoise.h>
#include <SingleUserPowerProfileDependentNoise.h>

#define PRINT_CODES_INFO
// #define PRINT_ACTIVITY_SAMPLING
// #define PRINT_INFO

// #define DEBUG

CDMASystem::CDMASystem(): SMCSystem()
{
	// parameters for the grid needed by the Finite Random Sets based algorithm ("FRSsBasedUserActivityDetectionAlgorithm")
	double firstCell,lastCell;
// 	uint nCells;
	
    if (_m!=1)
        throw RuntimeException("CDMASystem::CDMASystem: channel is not flat.");
	
	xml_node<> *thisSystemParameters = get_child(_doc.first_node(),"CDMASystem");
	
	if(!thisSystemParameters)
		throw RuntimeException("CDMASystem::CDMASystem: cannot find parameters for this system.");
	
	readParameterFromXML(thisSystemParameters,"userPersistenceProb",_userPersistenceProb);
	readParameterFromXML(thisSystemParameters,"newActiveUserProb",_newActiveUserProb);
	readParameterFromXML(thisSystemParameters,"userPriorProb",_userPriorProb);
	
	readParameterFromXML(thisSystemParameters,"adjustParticlesNumberFromSurvivors",_adjustParticlesNumberFromSurvivors);
	readParameterFromXML(thisSystemParameters,"adjustSurvivorsFromParticlesNumber",_adjustSurvivorsFromParticlesNumber);
	readParameterFromXML(thisSystemParameters,"minSignalToInterferenceRatio",_minSignalToInterferenceRatio);
	
	readParameterFromXML(thisSystemParameters,"nSurvivors",_nSurvivors);
	readParameterFromXML(thisSystemParameters,"iUserOfInterest",_iUserOfInterest);
	
	readParameterFromXML(thisSystemParameters,"maskUsedToComputeTheSER",_maskUsedToComputeTheSER);
	
	readParameterFromXML(thisSystemParameters,"forgettingFactor",_forgettingFactor);
	
	xml_node<> *FRSsBasedAlgorithmNode = get_child(thisSystemParameters,"FRSsBasedUserActivityDetectionAlgorithm");
	readParameterFromXML(FRSsBasedAlgorithmNode,"firstCell",firstCell);
	readParameterFromXML(FRSsBasedAlgorithmNode,"lastCell",lastCell);
		
	xml_node<> *parameterNode = get_child(FRSsBasedAlgorithmNode,"channelCoefficientsGrids");
	if(!parameterNode)
		throw RuntimeException("CDMASystem::readChannelCoefficientsGrids: cannot find parameter \"FRSsBasedUserActivityDetectionAlgorithm\"");
	
	uint nCells;
	std::string channelProbabilitiesFileName;
	
	// all the grids' parameters are read
	for (xml_node<> *nodeChild = parameterNode->first_node(); nodeChild; nodeChild = nodeChild->next_sibling())
	{
		readParameterFromXML(nodeChild,"nCells",nCells);	
		readParameterFromXML(nodeChild,"channelProbabilitiesFileName",channelProbabilitiesFileName);
		
		_nCells.push_back(nCells);
		_channelProbabilitiesFileNames.push_back(channelProbabilitiesFileName);
	}

	_usersActivityPdfs = std::vector<UsersActivityDistribution>(_N,UsersActivityDistribution(_userPersistenceProb,_newActiveUserProb,_userPriorProb));

	// first user starts transmitting something
	_usersActivityPdfs[0].setApriori(1.0);

    // a flat power profile is generated. Notice:
    //      i) that m should be 1, otherwise an exception would have been thrown
    //     ii) we only need to generate a coefficient per user, i.e., a 1xN vector
// 	FlatPowerProfile::setCoefficientsMean(5.0);
    _powerProfile = new FlatPowerProfile(1,_N,_m,1.0);

	_cdmaKalmanEstimator = NULL;
	_cdmaKnownChannelChannelMatrixEstimator = NULL;
	_cdmaRLSEstimator = NULL;

	_mmseDetector = new MMSEDetector(_L,_N,_alphabet->variance(),_N);
	    
    adjustParticlesSurvivors(_nParticles,_nSurvivors,_adjustParticlesNumberFromSurvivors,_adjustSurvivorsFromParticlesNumber);

	// in order to compute the BER/MSE/... of the user of interest only, a "dummy" permutation (a permutation for a set of 1 element) is generated
	_permutations = std::vector<std::vector<uint> >(1,std::vector<uint>(1,0));
	
	resetFramePieces();
		
#ifdef KEEP_EVERY_FRAME_USER_ACTIVITY
	_everyFrameUsersActivity.reserve(_nFrames);
#endif
	
#ifdef KEEP_EVERY_FRAME_SPREADING_CODES
	_everyFrameSpreadingCodes.reserve(_nFrames);
#endif

	_peActivityDetectionFrames.reserve(_nFrames);
	
#ifdef ESTIMATE_CHANNEL_TRANSITION_PROBABILITIES
	// this variable is only used by method "channelCoeffToCell"...which in turn is only needed when estimating the channel transition probabilities
	
	for(uint i=0;i<_nCells.size();i++)
	{
		_gridSteps.push_back((lastCell-firstCell)/(_nCells[i]-1));
		_estimatedChannelTransitionProbabilities.push_back(MatrixXd::Zero(_nCells[i],_nCells[i]));
		_estimatedChannelMarginalProbabilities.push_back(VectorXd::Zero(_nCells[i]));
	}
#endif

	for(uint i=0;i<_nCells.size();i++)
		_grids.push_back(Util::linspace(firstCell,lastCell,_nCells[i]));
	
	// ---------------- reading of the estimated channel transition probabilities
		
#ifndef ESTIMATE_CHANNEL_TRANSITION_PROBABILITIES
	std::ifstream f;
	
	_estimatedChannelTransitionProbabilities = std::vector<MatrixXd>(_nCells.size());
	_estimatedChannelMarginalProbabilities = std::vector<VectorXd>(_nCells.size());
	
	for(uint i=0;i<_nCells.size();i++)
	{
		f.open(_channelProbabilitiesFileNames[i].c_str(),std::ifstream::in);
		if(f.fail())
			throw RuntimeException("CDMASystem::CDMASystem: error reading file \"" + _channelProbabilitiesFileNames[i] + "\"");

		_estimatedChannelTransitionProbabilities[i] = Octave::fromOctaveFileStream(f);
		_estimatedChannelMarginalProbabilities[i] = Octave::fromOctaveFileStream(f);
		f.close();
		f.clear();
		
		VectorXd sums = _estimatedChannelTransitionProbabilities[i].rowwise().sum();
		double sum = _estimatedChannelMarginalProbabilities[i].sum();
	
		for(uint iRow=0;iRow<_estimatedChannelTransitionProbabilities[i].rows();iRow++)
		{
			_estimatedChannelMarginalProbabilities[i](iRow) = _estimatedChannelMarginalProbabilities[i](iRow)/sum;
			for(uint iCol=0;iCol<_estimatedChannelTransitionProbabilities[i].cols();iCol++)
				_estimatedChannelTransitionProbabilities[i](iRow,iCol) /= sums(iRow);
		}
	
// 		cout << "estimatedChannelTransitionProbabilities" << endl << _estimatedChannelTransitionProbabilities << endl;
	}
#endif
	
	// all the required gris are built
	for(uint i=0;i<_grids.size();i++)
		_grids[i] = Util::linspace(firstCell,lastCell,_nCells[i]);
}


CDMASystem::~CDMASystem()
{
    delete _powerProfile;
    delete _cdmaKalmanEstimator;
	delete _cdmaKnownChannelChannelMatrixEstimator;
	delete _cdmaRLSEstimator;
    delete _mmseDetector;
}

void CDMASystem::addAlgorithms()
{	
#ifdef ESTIMATE_CHANNEL_TRANSITION_PROBABILITIES
	return;
#endif
	
	for(uint iGrid=0;iGrid<_grids.size();iGrid++)
	{
		std::stringstream name;
		name << "Finite Random Sets with " << _nCells[iGrid] << " cells";
		_algorithms.push_back(new FRSsBasedUserActivityDetectionAlgorithm(name.str(),*_alphabet,_L,1,_N,_iLastSymbolVectorToBeDetected,_m,_preamble,_spreadingCodes,_grids[iGrid],_usersActivityPdfs,_estimatedChannelTransitionProbabilities[iGrid],_estimatedChannelMarginalProbabilities[iGrid]));
	}

	// ...the same for an estimator that knows the codes if these also change across frames
	delete _cdmaKalmanEstimator;
	_cdmaKalmanEstimator = new CDMAKalmanEstimator(_powerProfile->means(),_powerProfile->variances(),_ARcoefficients,_ARvariance,_spreadingCodes);
	
    delete _cdmaKnownChannelChannelMatrixEstimator;
    _cdmaKnownChannelChannelMatrixEstimator = new CDMAKnownChannelChannelMatrixEstimator(_channel,_preambleLength,_N,_spreadingCodes);
	
	delete _cdmaRLSEstimator;
	_cdmaRLSEstimator = new CDMARLSEstimator(_powerProfile->means(),_N,_forgettingFactor,_spreadingCodes);
	
    _algorithms.push_back(new KnownFlatChannelOptimalAlgorithm ("CDMA optimal with known channel BUT no knowledge of users activity probabilities",*_alphabet,_L,1,_N,_iLastSymbolVectorToBeDetected,*_channel,_preambleLength));

    _algorithms.push_back(new KnownFlatChannelAndActiveUsersOptimalAlgorithm ("CDMA optimal (known channel and active users)",*_alphabet,_L,1,_N,_iLastSymbolVectorToBeDetected,*_channel,_preambleLength,_usersActivity));
     
    _algorithms.push_back(new CDMAunknownActiveUsersSISopt ("CDMA SIS-opt",*_alphabet,_L,1,_N,_iLastSymbolVectorToBeDetected,_m,_cdmaKalmanEstimator,_preamble,_d,_nParticles,_resamplingAlgorithm,_powerProfile->means(),_powerProfile->variances(),_usersActivityPdfs));
	
	_algorithms.push_back(new UnknownActiveUsersLinearFilterBasedSMCAlgorithm ("CDMA SIS Linear Filters",*_alphabet,_L,1,_N,_iLastSymbolVectorToBeDetected,_m,_cdmaKalmanEstimator,_mmseDetector,_preamble,_d,_nParticles,_resamplingAlgorithm,_powerProfile->means(),_powerProfile->variances(),_usersActivityPdfs));
	
	_algorithms.push_back(new ViterbiAlgorithmWithAprioriProbabilities("Viterbi with a priori probabilities (known channel)",*_alphabet,_L,1,_N,_iLastSymbolVectorToBeDetected,*(dynamic_cast<StillMemoryMIMOChannel *> (_channel)),_preamble,_d,_usersActivityPdfs));
	
	_algorithms.push_back(new PSPAlgorithmWithAprioriProbabilities("PSP",*_alphabet,_L,1,_N,_iLastSymbolVectorToBeDetected,_m,_cdmaKalmanEstimator,_preamble,_d,_iLastSymbolVectorToBeDetected+_d,_nSurvivors,_usersActivityPdfs));
	
	_algorithms.push_back(new KnownSymbolsKalmanBasedChannelEstimatorAlgorithm("Kalman Filter (Known Symbols)",*_alphabet,_L,1,_N,_iLastSymbolVectorToBeDetected,_m,_cdmaKalmanEstimator,_preamble,_symbols));
}

void CDMASystem::buildSystemSpecificVariables()
{
	// when users are not transmitting, their symbols are zero
	_usersActivity = vector<vector<bool> >(_symbols.rows(),vector<bool>(_frameLength));
    
    for(uint iUser=0;iUser<_symbols.rows();iUser++)
    {
		// at the first time instant the prior probability is used to decide which users are active
		_usersActivity[iUser][_trainSeqLength] = _usersActivityPdfs[iUser].sampleFromPrior();
		
#ifdef PRINT_ACTIVITY_SAMPLING
		cout << "user " << iUser << ": " << _usersActivity[iUser][trainSeqLength] << endl;
#endif
		// when users are not transmitting, their symbols are zero
		_symbols(iUser,_preambleLength+_trainSeqLength) = double(_usersActivity[iUser][_trainSeqLength])*_symbols(iUser,_preambleLength+_trainSeqLength);
    }
      
    // set of active users evolves according to the given probabilities
	for(uint iTime=_trainSeqLength+1;iTime<_frameLength;iTime++)
		for(uint iUser=0;iUser<_symbols.rows();iUser++)
		{   
			_usersActivity[iUser][iTime] = _usersActivityPdfs[iUser].sampleGivenItWas(_usersActivity[iUser][iTime-1]);             
			_symbols(iUser,_preambleLength+iTime) = _symbols(iUser,_preambleLength+iTime)*double(_usersActivity[iUser][iTime]);
		}
            
#ifdef PRINT_INFO
    cout << "symbols after generating users activity" << endl << symbols << endl;
#endif
	
	do
	{
		// spreading codes for the users are generated randomly
		_spreadingCodes = Util::sign(StatUtil::randnMatrix(_L,_N,0.0,1.0));

#ifdef PRINT_CODES_INFO
		std::cout << "generated spreadingCodes..." << std::endl << _spreadingCodes << std::endl;
		std::cout << "are codes are orthogonal? " << Util::areColsOrthogonal(_spreadingCodes) << std::endl;
		std::cout << "are codes different and NOT opposite? " << Util::areColsDifferentAndNotOpposite(_spreadingCodes) << std::endl;
#endif
// 	} while(false);
	} while(!Util::areColsDifferentAndNotOpposite(_spreadingCodes));
}

double CDMASystem::computeActivityDetectionErrorRate(MatrixXd sourceSymbols, MatrixXd detectedSymbols) const
{
	return computeSelectedUsersActivityDetectionErrorRate(sourceSymbols.row(_iUserOfInterest),detectedSymbols.row(_iUserOfInterest));
// 	return computeSelectedUsersActivityDetectionErrorRate(sourceSymbols,detectedSymbols);
}

double CDMASystem::computeSelectedUsersActivityDetectionErrorRate(MatrixXd sourceSymbols, MatrixXd detectedSymbols) const
{
	if (_symbolsDetectionWindowStart!=0)
		throw RuntimeException("CDMASystem::computeActivityDetectionErrorRate: this is only implemented when the starting point for SER computing is zero (the beginning of the frame).");

	// the dimensions of "sourceSymbols" are the same as those of "detectedSymbols"
	assert(sourceSymbols.rows() == detectedSymbols.rows() && sourceSymbols.cols()== detectedSymbols.cols());

	uint nSymbolsRows = detectedSymbols.rows();

	std::vector<std::vector<bool> > mask(nSymbolsRows,std::vector<bool>(_frameLength,true));

	// in order to compute the probability of activity detection the symbol detected makes no difference: the only thing that matters is wether a symbol (any) was detected or not
	// for both the "sourceSymbols" and the "detectedSymbols", every symbol belonging to the alphabet is transformed into the first (for example) symbol of the alphabet
	for (uint i=0;i<sourceSymbols.rows();i++)
		for (uint j=0;j<sourceSymbols.cols();j++)
		{
			if (_alphabet->doesItBelong(sourceSymbols(i,j)))
				sourceSymbols(i,j) = _alphabet->operator[](0u);
			if (_alphabet->doesItBelong(detectedSymbols(i,j)))
				detectedSymbols(i,j) = _alphabet->operator[](0u);
		}

	double res = 0.0;

	for (uint iSignChange=1;iSignChange<_signChanges.size();iSignChange++)
	{
		res += (_signChanges[iSignChange]-_signChanges[iSignChange-1])*
				computeSERwithoutSolvingAmbiguity(
					sourceSymbols.block(0,_signChanges[iSignChange-1],nSymbolsRows,_signChanges[iSignChange]-_signChanges[iSignChange-1]),
					Util::applyPermutationOnRows(detectedSymbols,_permutations[_piecesBestPermuationIndexes[iSignChange-1]],vector<int>(nSymbolsRows,+1)).block(0,_signChanges[iSignChange-1],nSymbolsRows,_signChanges[iSignChange]-_signChanges[iSignChange-1]),
					Util::block(mask,0,_signChanges[iSignChange-1],nSymbolsRows,_signChanges[iSignChange]-_signChanges[iSignChange-1]));
	}

	res /= sourceSymbols.cols();

	return res;
}

void CDMASystem::beforeEndingAlgorithm()
{
	SMCSystem::beforeEndingAlgorithm();

	if(_algorithms[_iAlgorithm]->performsSymbolsDetection())
		_presentFramePeActivityDetection(_iSNR,_iAlgorithm) = computeActivityDetectionErrorRate(_symbols.block(0,_preambleLength,_N,_frameLength),_detectedSymbols);
	
	resetFramePieces();
}

void CDMASystem::onlyOnce()
{
	SMCSystem::onlyOnce();

	_presentFramePeActivityDetection = MatrixXd::Constant(_SNRs.size(),_algorithms.size(),FUNNY_VALUE);
	
	// the no sign changes is needed (the algorithm knows the channel), the value is set to 0
	_thisFrameNumberSignChanges = std::vector<std::vector<uint> >(_SNRs.size(),std::vector<uint>(_algorithms.size(),0u));
}

double CDMASystem::computeSER(const MatrixXd &sourceSymbols,const MatrixXd &detectedSymbols,const vector<vector<bool> > &mask,uint &iBestPermutation,vector<int> &bestPermutationSigns)
{
	return computeSelectedUsersSER(sourceSymbols.row(_iUserOfInterest),detectedSymbols.row(_iUserOfInterest),Util::row(mask,_iUserOfInterest),iBestPermutation,bestPermutationSigns);	
// 	return computeSelectedUsersSER(sourceSymbols,detectedSymbols,mask,iBestPermutation,bestPermutationSigns);
}

double CDMASystem::computeSelectedUsersSER(const MatrixXd &sourceSymbols,const MatrixXd &detectedSymbols,const vector<vector<bool> > &mask,uint &iBestPermutation,vector<int> &bestPermutationSigns)
{
	if (_symbolsDetectionWindowStart!=0)
		throw RuntimeException("CDMASystem::computeSER: this is only implemented when the starting point for SER computing is zero (the beginning of the frame).");

	uint nSymbolsRows = detectedSymbols.rows();
		
	// if this method is called, we dismiss the initialization in the constructor
	_piecesBestPermuationIndexes.clear();
	_piecesBestPermutationSigns.clear();

	double res = 0.0;

	// NOTE: even though ultimately only symbol vectors from "_symbolsDetectionWindowStart" will be taken into account for detection, we don't have to worry about it here, since the mask will take care of that. That being so, 
	// "_signChanges" actually contains all the sign changes that occur within the frame
	
	std::vector<uint> trueChannelSignChanges,channelEstimateSignChanges;
	
	// if the algorithm performs channel estimation we use ITS channel estimates to split the frame and tackle the ambiguity problem
	if(_algorithms[_iAlgorithm]->performsChannelEstimation())
	{
		trueChannelSignChanges = Util::getZeroCrossings(_channel->getChannelMatrices(),_iUserOfInterest,_preambleLength,_frameLength);
		channelEstimateSignChanges = Util::getZeroCrossings(_algorithms[_iAlgorithm]->getEstimatedChannelMatrices(),_iUserOfInterest,_preambleLength,_frameLength);
	}
	//...if it doesn't perform channel estimation, the ambiguity problem is gone
	else
	{
		channelEstimateSignChanges = std::vector<uint>(2);
		channelEstimateSignChanges[0] = 0; channelEstimateSignChanges[1] = _frameLength;
		
		trueChannelSignChanges = channelEstimateSignChanges;
	}
	
	// enough space for the set resulting from the union of "trueChannelSignChanges" and "channelEstimateSignChanges" is reserved
	_signChanges = std::vector<uint>(trueChannelSignChanges.size()+channelEstimateSignChanges.size());
	
	// the union set is obtained
	std::vector<uint>::iterator it = std::set_union (trueChannelSignChanges.begin(), trueChannelSignChanges.end(), channelEstimateSignChanges.begin(), channelEstimateSignChanges.end(), _signChanges.begin());
	
	// since some elements were in both sets (the intersection was not null), we resize the vector representing the union set to the appropriate size
	_signChanges.resize(it - _signChanges.begin());
	
	// the number of sign changes counted is saved
	_thisFrameNumberSignChanges[_iSNR][_iAlgorithm] = _signChanges.size();
	
	// a mask built for taking into account "real" transmitted symbols that are detected by the algorithm as symbols (i.e., activity)
	std::vector<std::vector<bool> > activityDetectedAsActivityMask = mask;
	
	// a mask built for taking into account "real" transmitted symbols no matter if they are actually detected as real symbols by the algorithm or not
	std::vector<std::vector<bool> > activityMask = mask;
	
	for (uint iUser=0;iUser<nSymbolsRows;iUser++)
		for(uint iTime=_trainSeqLength;iTime<_frameLength;iTime++)
		{
			assert(Util::isUserActive(sourceSymbols(iUser,iTime))==_usersActivity[iUser][iTime]);
			activityDetectedAsActivityMask[iUser][iTime] = mask[iUser][iTime] && _usersActivity[iUser][iTime] && Util::isUserActive(detectedSymbols(iUser,iTime));
			activityMask[iUser][iTime] = mask[iUser][iTime] && _usersActivity[iUser][iTime];
		}
		
	std::vector<std::vector<bool> > symbolsToAccountForWhenComputingSER;
	
	if(!_maskUsedToComputeTheSER.compare("all"))
		symbolsToAccountForWhenComputingSER = mask;
	else if(!_maskUsedToComputeTheSER.compare("activityDetectedAsActivity"))
		symbolsToAccountForWhenComputingSER = activityDetectedAsActivityMask;
	else if(!_maskUsedToComputeTheSER.compare("activity"))
		symbolsToAccountForWhenComputingSER = activityMask;
	else
		throw RuntimeException(std::string("CDMASystem::computeSelectedUsersSER: unknown maskUsedToComputeTheSER: \"") + _maskUsedToComputeTheSER + std::string("\"."));
	
	
	uint overallNumberAccountedSymbols = 0;

	for (uint iSignChange=1;iSignChange<_signChanges.size();iSignChange++)
	{
		// we need to find out how many symbols are gonna be taken into account within this subframe
		uint thisSubframeNumberAccountedSymbols = 0;
		for (uint i=0;i<nSymbolsRows;i++)
			for (uint j=_signChanges[iSignChange-1];j<_signChanges[iSignChange];j++)
				thisSubframeNumberAccountedSymbols += symbolsToAccountForWhenComputingSER[i][j];

		overallNumberAccountedSymbols += thisSubframeNumberAccountedSymbols;

		res += thisSubframeNumberAccountedSymbols*
				BaseSystem::computeSER(sourceSymbols.block(0,_signChanges[iSignChange-1],nSymbolsRows,_signChanges[iSignChange]-_signChanges[iSignChange-1]),
										detectedSymbols.block(0,_signChanges[iSignChange-1],nSymbolsRows,_signChanges[iSignChange]-_signChanges[iSignChange-1]),
										Util::block(symbolsToAccountForWhenComputingSER,0,_signChanges[iSignChange-1],nSymbolsRows,_signChanges[iSignChange]-_signChanges[iSignChange-1]),
										iBestPermutation,bestPermutationSigns);
		assert(iBestPermutation==0);
		
		// we need to store which the best permutations were along with their corresponding signs since it will be needed later by "computeActivityDetectionErrorRate" and "computeMSE"
		_piecesBestPermuationIndexes.push_back(iBestPermutation);
		_piecesBestPermutationSigns.push_back(bestPermutationSigns);
  }

	assert(overallNumberAccountedSymbols!=0);
	res /= overallNumberAccountedSymbols;

	return res;
}

double CDMASystem::computeMSE(const vector<MatrixXd> &realChannelMatrices,const vector<MatrixXd> &estimatedChannelMatrices,const std::vector<bool> &mask) const
{
	return computeSelectedUsersMSE(Util::keepCol(realChannelMatrices,_iUserOfInterest),Util::keepCol(estimatedChannelMatrices,_iUserOfInterest),mask);
// 	return computeSelectedUsersMSE(realChannelMatrices,estimatedChannelMatrices);
}


double CDMASystem::computeSelectedUsersMSE(const vector<MatrixXd> &realChannelMatrices,const vector<MatrixXd> &estimatedChannelMatrices,const std::vector<bool> &mask) const
{
  
	assert(realChannelMatrices.size()==estimatedChannelMatrices.size());

	double res = 0.0;
	
	uint overallNumberAccountedChannelEstimates = 0;

	for (uint iSignChange = 1;iSignChange<_signChanges.size();iSignChange++)
	{
		std::vector<MatrixXd>  toCheckRealChannelMatrices(realChannelMatrices.begin()+_signChanges[iSignChange-1],realChannelMatrices.begin()+_signChanges[iSignChange]);
		std::vector<MatrixXd>  toCheckEstimatedChannelMatrices(estimatedChannelMatrices.begin()+_signChanges[iSignChange-1],estimatedChannelMatrices.begin()+_signChanges[iSignChange]);

		uint thisSubframeNumberAccountedChannelEstimates = 0;
		
		// TODO: this is also computed by the "computeMSE" method -> it could return this in a parameter...
		for (uint j=_signChanges[iSignChange-1];j<_signChanges[iSignChange];j++)
			thisSubframeNumberAccountedChannelEstimates += mask[j];
		
		try
		{
			res += thisSubframeNumberAccountedChannelEstimates*BaseSystem::computeMSE(toCheckRealChannelMatrices,toCheckEstimatedChannelMatrices,
				Util::block(mask,_signChanges[iSignChange-1],_signChanges[iSignChange]-_signChanges[iSignChange-1]),
				_permutations[_piecesBestPermuationIndexes[iSignChange-1]],_piecesBestPermutationSigns[iSignChange-1]);		
		}
		catch (DivisionByZero e)
		{
			// this subframe is not accounted for in any way
			continue;
		}
		
		overallNumberAccountedChannelEstimates += thisSubframeNumberAccountedChannelEstimates;
	}

	// if overallNumberAccountedChannelEstimates==0, this will give NaN...which is a good thing register in the results file
	res /= overallNumberAccountedChannelEstimates;
	return res;
}

void CDMASystem::resetFramePieces()
{
	// by default (when "computeSER" is not called), we assume that there is no sign changes (the frame is not split)...
	_signChanges = std::vector<uint>(2);
	_signChanges[0] = 0; _signChanges[1] = _frameLength;
	
	// ...the best permutation is the first one...
	_piecesBestPermuationIndexes = std::vector<uint>(1,0);
	
	// ...and its corresponding signs are all +1
	_piecesBestPermutationSigns = std::vector<std::vector<int> >(1,std::vector<int>(_permutations[0].size(),+1));
}

void CDMASystem::storeFrameResults()
{
    SMCSystem::storeFrameResults();

    _peActivityDetectionFrames.push_back(_presentFramePeActivityDetection);
	
#ifdef KEEP_EVERY_FRAME_USER_ACTIVITY
	_everyFrameUsersActivity.push_back(_usersActivity);
#endif
	
#ifdef KEEP_EVERY_FRAME_SPREADING_CODES
	_everyFrameSpreadingCodes.push_back(_spreadingCodes);
#endif

	_everyFrameNumberSignChanges.push_back(_thisFrameNumberSignChanges);
}

void CDMASystem::saveFrameResults()
{
    SMCSystem::saveFrameResults();
    Octave::toOctaveFileStream(_peActivityDetectionFrames,"activityDetectionErrorRate",_f);
    Octave::toOctaveFileStream(_spreadingCodes,"spreadingCodes",_f);
	Octave::toOctaveFileStream(_nSurvivors,"nSurvivors",_f);
	Octave::toOctaveFileStream(_userPersistenceProb,"userPersistenceProb",_f);
	Octave::toOctaveFileStream(_newActiveUserProb,"newActiveUserProb",_f);
	Octave::toOctaveFileStream(_userPriorProb,"userPriorProb",_f);
	
#ifdef KEEP_EVERY_FRAME_USER_ACTIVITY
	Octave::toOctaveFileStream(_everyFrameUsersActivity,"usersActivity",_f);
#else
	Octave::toOctaveFileStream(_usersActivity,"usersActivity",_f);
#endif
	
	Octave::toOctaveFileStream(_signChanges,"signChanges",_f);
	Octave::toOctaveFileStream(_minSignalToInterferenceRatio,"minSignalToInterferenceRatio",_f);
	
#ifdef KEEP_EVERY_FRAME_SPREADING_CODES
	Octave::toOctaveFileStream(_everyFrameSpreadingCodes,"everyFrameSpreadingCodes",_f);
#else
	Octave::toOctaveFileStream(_spreadingCodes,"spreadingCodes",_f);
#endif
	
	Octave::toOctaveFileStream(_everyFrameNumberSignChanges,"everyFrameNumberSignChanges",_f);
	
#ifdef ESTIMATE_CHANNEL_TRANSITION_PROBABILITIES
	std::ofstream f;
	
	for(uint iGrid=0;iGrid<_grids.size();iGrid++)
	{
		f.open(_channelProbabilitiesFileNames[iGrid].c_str(),std::ofstream::trunc);
		Octave::toOctaveFileStream(_estimatedChannelTransitionProbabilities[iGrid],"estimatedChannelTransitionProbabilities",f);
		Octave::toOctaveFileStream(_estimatedChannelMarginalProbabilities[iGrid],"estimatedChannelMarginalProbabilities",f);
		f.close();
		f.clear();
	}
#endif
}

Noise *CDMASystem::buildNoise() const
{
	if(!_noiseClassToBeInstantiated.compare(SingleUserPowerProfileDependentNoise::getXMLname()))
		return new SingleUserPowerProfileDependentNoise(_alphabet->variance(),_L,_channel->length(),*_powerProfile,_iUserOfInterest);
	else if(!_noiseClassToBeInstantiated.compare(SingleUserChannelDependentNoise::getXMLname()))
		return new SingleUserChannelDependentNoise(_alphabet->variance(),_channel,_iUserOfInterest);
	else
		return SMCSystem::buildNoise();
}

MIMOChannel *CDMASystem::buildChannel()
{
	double thisChannelMinimumSIR;
	
	MIMOChannel *channel = NULL;
	
	do
	{
		delete channel;

		// ------------------------------------------------------------------------------------------
		
		// the appropriate class to be instantiated is selected here
		if(!_channelClassToBeInstantiated.compare(MultiuserCDMAchannel::getXMLname() + "_" + ARchannel::getXMLname()))
			channel =  new MultiuserCDMAchannel(new ARchannel(_N,1,_m,_symbols.cols(),ARprocess(_powerProfile->generateChannelMatrix(_randomGenerator),_ARcoefficients,_ARvariance)),_spreadingCodes);
		else if(!_channelClassToBeInstantiated.compare(MultiuserCDMAchannel::getXMLname() + "_" + TimeInvariantChannel::getXMLname()))
			channel = new MultiuserCDMAchannel(new TimeInvariantChannel(_powerProfile->nInputs(),_powerProfile->nOutputs(),_m,_symbols.cols(),MatrixXd::Ones(_powerProfile->nOutputs(),_powerProfile->nInputs())),_spreadingCodes);
		else if(!_channelClassToBeInstantiated.compare(MultiuserCDMAchannel::getXMLname() + "_" + BesselChannel::getXMLname()))
			channel = new MultiuserCDMAchannel(new BesselChannel(_N,1,_m,_symbols.cols(),_velocity,_carrierFrequency,_T,*_powerProfile),_spreadingCodes);
		else
			throw RuntimeException(std::string("CDMASystem::buildChannel: MIMOChannel class \"") + _channelClassToBeInstantiated + std::string("\" is not supported within a multiuser CDMA system."));
		
		// ------------------------------------------------------------------------------------------
		
		// Signal to Interference Ratio's
		std::vector<double> SIRs = dynamic_cast<MultiuserCDMAchannel *> (channel)->signalToInterferenceRatio(_iUserOfInterest);
		
		// the minimum is obtained
		thisChannelMinimumSIR = 10*log10(SIRs[std::min_element(SIRs.begin(),SIRs.end())-SIRs.begin()]);
		std::cout << "minimum SIR = " << thisChannelMinimumSIR << std::endl;

	} while(thisChannelMinimumSIR<_minSignalToInterferenceRatio);
	
#ifdef ESTIMATE_CHANNEL_TRANSITION_PROBABILITIES
	accountForEstimatedChannelTransitionProbabilities(channel);
#endif
	
	return channel;
}

#ifdef ESTIMATE_CHANNEL_TRANSITION_PROBABILITIES

void CDMASystem::accountForEstimatedChannelTransitionProbabilities(const MIMOChannel * const channel)
{
	uint nRows = channel->channelCoefficientsMatrixRows();
	uint nCols = channel->channelCoefficientsMatrixCols();

	for(uint iGrid=0;iGrid<_grids.size();iGrid++)	
	{		
		for(uint i=_preambleLength+1;i<_iLastSymbolVectorToBeDetected;i++)
		{
			for(uint iRow=0;iRow<nRows;iRow++)
				for(uint iCol=0;iCol<nCols;iCol++)
				{
					uint previousCoeffCell = channelCoeffToCell(channel->at(i-1)(iRow,iCol),_grids[iGrid],_gridSteps[iGrid]);
					uint currentCoeffCell = channelCoeffToCell(channel->at(i)(iRow,iCol),_grids[iGrid],_gridSteps[iGrid]);
					
					_estimatedChannelTransitionProbabilities[iGrid](previousCoeffCell,currentCoeffCell)++;
					_estimatedChannelMarginalProbabilities[iGrid](currentCoeffCell)++;
				}
		}
	}
}

uint CDMASystem::channelCoeffToCell(double coeff, const std::vector< double > &grid, double gridStep) const
{
	if(coeff<grid[0])
		return 0;
	else if(coeff>grid[grid.size()-1])
		return (grid.size()-1);
	
	return round((coeff-grid[0])/gridStep);
}

#endif