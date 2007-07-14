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

// #define DEBUG
// #define MISMO_CANAL
// #define EXPORT_REAL_DATA

#define MSE_TIME_EVOLUTION_COMPUTING

#define HOSTNAME_LENGTH 50
#define DATE_LENGTH 100
#define SPRINTF_BUFFER 50

#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <vector>
#include <unistd.h>
#include <string>
#include <math.h>
#include <sys/time.h>
#include <algorithm>

#include <types.h>
#include <defines.h>
#include <Alphabet.h>
#include <Bits.h>
#include <ARprocess.h>

#include <StatUtil.h>
#include <Util.h>
#include <TransmissionUtil.h>

#include <ARchannel.h>
#include <ARoneChannelOrderPerTransmitAtennaMIMOChannel.h>
#include <SparklingMemoryARMIMOChannel.h>
#include <BesselChannel.h>
#include <EstimatedMIMOChannel.h>

#include <ExponentialPowerProfile.h>
#include <FlatPowerProfile.h>
#include <ContinuousPowerProfile.h>
#include <ConstantMeanDSPowerProfile.h>

#include <ChannelDependentNoise.h>
#include <NullNoise.h>
#include <Modulator.h>
#include <Demodulator.h>
#include <KalmanFilter.h>

#include <KalmanEstimator.h>
#include <RLSEstimator.h>
#include <LMSEstimator.h>
#include <OneChannelOrderPerTransmitAtennaWrapperEstimator.h>
#include <APPbasedChannelOrderEstimator.h>
#include <KnownChannelChannelMatrixEstimator.h>
#include <KnownSymbolsKalmanEstimator.h>

#include <RMMSEDetector.h>
#include <MMSEDetector.h>
#include <DecorrelatorDetector.h>

#include <DSISoptAlgorithm.h>
#include <LinearFilterBasedSMCAlgorithm.h>
#include <LinearFilterBasedMKFAlgorithm.h>
#include <ViterbiAlgorithm.h>
#include <KnownSymbolsKalmanBasedChannelEstimator.h>
#include <UnknownChannelOrderAlgorithm.h>
#include <ISIS.h>
#include <USIS.h>
#include <USIS2SISAlgorithm.h>
#include <MaximumProbabilityCriterion.h>
#include <UniformRelatedCriterion.h>
#include <PSPAlgorithm.h>
#include <PSPBasedSMCAlgorithm.h>
#include <UPSPBasedSMCAlgorithm.h>
#include <ChannelOrderEstimatorSMCAlgorithm.h>
#include <TriangularizationBasedSMCAlgorithm.h>
#include <LinearFilterBasedAlgorithm.h>
#include <SISoptAlgorithm.h>

#include <Particle.h>
#include <ParticleWithChannelEstimation.h>
#include <ResamplingCriterion.h>
#include <MultinomialResamplingAlgorithm.h>
#include <ResidualResamplingAlgorithm.h>
#include <WithThresholdResamplingAlgorithmWrapper.h>
#include <WithoutReplacementResamplingAlgorithm.h>
#include <BestParticlesResamplingAlgorithm.h>

#include <lapackpp/gmd.h>
#include <lapackpp/gmi.h>
#include <lapackpp/blas1pp.h>
#include <lapackpp/blas2pp.h>
#include <lapackpp/blas3pp.h>
#include <lapackpp/laslv.h>
#include <lapackpp/lavli.h>
#include <lapackpp/sybmd.h>
#include <lapackpp/sybfd.h>

#include <SMCSystem.h>

using namespace std;

#ifdef EXPORT_REAL_DATA
	MIMOChannel *realChannel;
	tMatrix *realSymbols;
	Noise *realNoise;
#endif

int main(int argc,char* argv[])
{
    SMCSystem holita;
    holita.Simulate();
    exit(0);

    double pe,mse;
    uint iChannelOrder,iSNR;
    int lastSymbolVectorInstant;

    // GLOBAL PARAMETERS
    int nFrames = 1;
    int L=3,N=2,K=300;
    int trainSeqLength = 100;
    int nParticles = 200;
    double resamplingRatio = 0.9;
    char outputFileName[HOSTNAME_LENGTH+4] = "res_";
    int preambleLength = 10;

    // BER and MSE computing
    int BERwindowStart = trainSeqLength;
	BERwindowStart = preambleLength + K*3/10;
    int MSEwindowStart = 0;
    MSEwindowStart = preambleLength + K*9/10;

	cout << "BERwindowStart = " << BERwindowStart << " MSEwindowStart = " << MSEwindowStart << endl;

    // PSP
    int nSurvivors = 2;
    bool adjustParticlesNumberFromSurvivors = false;

    // - ONE CHANNEL ORDER SYSTEM
    int m = 3;
	int d = m - 1;

    // - ONE CHANNEL ORDER PER ANTENNA SYSTEM
    vector<int> antennasChannelOrders(N);
    antennasChannelOrders[0] = 1;
    antennasChannelOrders[1] = 3;

	// back smoothing
	int c = 0;
	int e = 50;

    // SNRs to be processed
    vector<int> SNRs;
//     SNRs.push_back(3);SNRs.push_back(6);SNRs.push_back(9);SNRs.push_back(12);SNRs.push_back(15);
    SNRs.push_back(15);

    // AR process parameters
    vector<double> ARcoefficients(1);
    ARcoefficients[0] = 0.99999;
    double ARvariance=0.0001;

	// system parameters for generating the AR process
	int ARprocessOrder = 2;
	double velocity = 50; // (Km/h)
	double carrierFrequency = 2e9; // (Hz)
	double symbolRate = 500e3; // (Hz)
	double T = 1.0/symbolRate; // (s)

    // channel parameters
	double channelVariance=1.0;


	vector<double> differentialDelays,powers;
	// suburban macro
// 	differentialDelays.push_back(0);differentialDelays.push_back(0.1408e-6);differentialDelays.push_back(0.0626e-6);
// 	differentialDelays.push_back(0.4015e-6);differentialDelays.push_back(1.3820e-6);differentialDelays.push_back(2.8280e-6);
// 	powers.push_back(0);powers.push_back(-2.6682);powers.push_back(-6.2147);
// 	powers.push_back(-10.4132);powers.push_back(-16.4735);powers.push_back(-22.1898);

	// urban macro
// 	differentialDelays.push_back(0);differentialDelays.push_back(0.3600e-6);differentialDelays.push_back(0.2527e-6);
// 	differentialDelays.push_back(1.0387e-6);differentialDelays.push_back(2.7300e-6);differentialDelays.push_back(4.5977e-6);
// 	powers.push_back(0);powers.push_back(-2.2204);powers.push_back(-1.7184);
// 	powers.push_back(-5.1896);powers.push_back(-9.0516);powers.push_back(-12.5013);

	// urban micro
	differentialDelays.push_back(0);differentialDelays.push_back(0.2840e-6);differentialDelays.push_back(0.2047e-6);
	differentialDelays.push_back(0.6623e-6);differentialDelays.push_back(0.8066e-6);differentialDelays.push_back(0.9227e-6);
	powers.push_back(0);powers.push_back(-1.2661);powers.push_back(-2.7201);
	powers.push_back(-4.2973);powers.push_back(-6.0140);powers.push_back(-8.4306);

	ConstantMeanDSPowerProfile powerProfile(L,N,differentialDelays,powers,T);


// 	ExponentialPowerProfile powerProfile(L,N,T,0.00001); // m = 6
// 	ExponentialPowerProfile powerProfile(L,N,T,0.01); // m = 3
// 	FlatPowerProfile powerProfile(L,N,m,channelVariance);
	powerProfile.Print();
	cout << powerProfile.Means();
	cout << powerProfile.Variances();
	cout << "memoria es " << powerProfile.Memory() << endl;

    // channel estimator parameters
	double firstSampledChannelMatrixVariance = 0.0;
    double forgettingFactor = 0.99;
    double muLMS = 0.05;

	// unknown channel order
	vector<int> candidateChannelOrders;
	candidateChannelOrders.push_back(2);candidateChannelOrders.push_back(3);candidateChannelOrders.push_back(4);
	candidateChannelOrders.push_back(5);candidateChannelOrders.push_back(6);candidateChannelOrders.push_back(7);

	if(find(candidateChannelOrders.begin(),candidateChannelOrders.end(),m)==candidateChannelOrders.end())
		throw RuntimeException("The memory of the channel is not one of the possible candidates.");

	// initial channel estimation for every channel order
	vector<tMatrix> channelOrderCoefficientsMeans(candidateChannelOrders.size());
	vector<tMatrix> channelOrderCoefficientsVariances(candidateChannelOrders.size());
	for(iChannelOrder=0;iChannelOrder<candidateChannelOrders.size();iChannelOrder++)
	{
		channelOrderCoefficientsMeans[iChannelOrder] = LaGenMatDouble::zeros(L,N*candidateChannelOrders[iChannelOrder]);

		channelOrderCoefficientsVariances[iChannelOrder] = LaGenMatDouble::ones(L,N*candidateChannelOrders[iChannelOrder]);
	}

    // linear detectors parameters
    double forgettingFactorDetector = 0.95;

    // alphabet is defined
    vector<vector<tBit> > secuenciasBits(2,vector<tBit>(1));
    secuenciasBits[0][0] = 0; secuenciasBits[1][0] = 1;
    vector<tSymbol> simbolos(2);
    simbolos[0] = -1; simbolos[1] = 1;
    Alphabet pam2(1,2,secuenciasBits,simbolos);

	// -----------------------------------------------------------------------------

	// a vector that will contain the names of the algorithms
	vector<string> algorithmsNames;

	// host name is concatenated into the file name
	char hostname[HOSTNAME_LENGTH];
	gethostname(hostname,HOSTNAME_LENGTH);
	strcat(outputFileName,hostname);

	// get present time of the system
	time_t presentTime;
	time(&presentTime);
	char presentTimeString[DATE_LENGTH];
	ctime_r(&presentTime,presentTimeString);
	presentTimeString[strlen(presentTimeString)-1] = '\0';
	for(int i=strlen(presentTimeString)-1;i>=0;i--)
		if(presentTimeString[i]==' ')
			presentTimeString[i]='_';

	// it is concatenated into the file name
	strcat(outputFileName,"_");
	strcat(outputFileName,presentTimeString);

    // a specific preamble is generated...
    tMatrix preamble(N,preambleLength);
    preamble = -1.0;

    // some useful ranges
    tRange rAllSymbolRows(0,N-1);

	// vectors of channel estimators and linear detectors for unknown channel order algorithms
	vector<ChannelMatrixEstimator *> kalmanChannelEstimators;
	vector<ChannelMatrixEstimator *> RLSchannelEstimators;
	vector<LinearDetector *> RMMSElinearDetectors;
	for(iChannelOrder=0;iChannelOrder<candidateChannelOrders.size();iChannelOrder++)
	{
		kalmanChannelEstimators.push_back(new KalmanEstimator(channelOrderCoefficientsMeans[iChannelOrder],N,ARcoefficients[0],ARvariance));

		RLSchannelEstimators.push_back(new RLSEstimator(channelOrderCoefficientsMeans[iChannelOrder],N,forgettingFactor));

		RMMSElinearDetectors.push_back(new RMMSEDetector(L*candidateChannelOrders[iChannelOrder],N*(2*candidateChannelOrders[iChannelOrder]-1),pam2.Variance(),forgettingFactorDetector,N*candidateChannelOrders[iChannelOrder]));
	}

	// channel order estimators
	APPbasedChannelOrderEstimator *channelOrderEstimator= new APPbasedChannelOrderEstimator(N,preamble,candidateChannelOrders,channelOrderCoefficientsMeans,ARcoefficients[0]);

	// the maximum of the candidate channel orders is computed
	int maxCandidateOrder = candidateChannelOrders[0];
	for(iChannelOrder=1;iChannelOrder<candidateChannelOrders.size();iChannelOrder++)
		if(candidateChannelOrders[iChannelOrder]>maxCandidateOrder)
			maxCandidateOrder = candidateChannelOrders[iChannelOrder];

	// the algorithms with the higher smoothing lag require
// 	int nSmoothingSymbolsVectors = maxCandidateOrder-1;
	int nSmoothingSymbolsVectors = 10;
	int nSmoothingBitsVectors = nSmoothingSymbolsVectors*pam2.NbitsBySymbol();

	// PSP
	if(adjustParticlesNumberFromSurvivors)
	{
		nParticles = (int)pow((double)pam2.Length(),N*(m-1))*nSurvivors;
        cout << "Number of particles adjusted to " << nParticles << endl;
    }

    // always the same resampling criterion and algorithms
    ResamplingCriterion criterioRemuestreo(resamplingRatio);
//     MultinomialResamplingAlgorithm algoritmoRemuestreo(criterioRemuestreo);
    ResidualResamplingAlgorithm algoritmoRemuestreo(criterioRemuestreo);

//     WithThresholdResamplingAlgorithmWrapper residualResamplingWithThreshold(new ResidualResamplingAlgorithm(criterioRemuestreo),0.2);
//     WithoutReplacementResamplingAlgorithm withoutReplacementResampling(criterioRemuestreo);
//     BestParticlesResamplingAlgorithm bestParticlesResampling(criterioRemuestreo);


	// ------------------------- test simulations ----------------------
// #include <resamplingRate.h>
// #include <backwardSmoothing.h>
// #include <backwardForwardSmoothing.h>
// #include <particlesNumber.h>
// #include <MMSEforwardSmoothing.h>
	// -----------------------------------------------------------------

	// USIS2SIS transition criterion(s)
    MaximumProbabilityCriterion USISmaximumProbabilityCriterion(0.8);
    UniformRelatedCriterion USISuniformRelatedCriterion(2.0);

    // ambiguity resolution
    uint *firstPermutation = new uint[N];
    for(int i=0;i<N;i++) firstPermutation[i] = i;
    vector<vector<uint> > permutations = Util::Permutations(firstPermutation,N);
    delete[] firstPermutation;

    // matrices for results
    vector<tMatrix> peMatrices, MSEMatrices;
    peMatrices.reserve(nFrames);
    MSEMatrices.reserve(nFrames);

    tMatrix overallPeMatrix,overallMseMatrix,presentFramePe,presentFrameMSE;

	// channel order APP evolution
    vector<vector<vector<tMatrix> > > channelOrderAPPsAlongTime;
    channelOrderAPPsAlongTime.reserve(nFrames);

    vector<vector<tMatrix> > presentFrameChannelOrderAPPsAlongTime;
    vector<int> iAlgorithmsPerformingChannelOrderAPPestimation;

	// BER time evolution
    vector<tMatrix> overallPeTimeEvolution(SNRs.size());
    vector<LaGenMatInt> overallErrorsNumberTimeEvolution(SNRs.size());

	// seeds
	vector<uint32_t> mainSeeds,statUtilSeeds;
	vector<LaGenMatLongInt> beforeRunStatUtilSeeds;
	mainSeeds.reserve(nFrames);
	statUtilSeeds.reserve(nFrames);
	beforeRunStatUtilSeeds.reserve(nFrames);
	LaGenMatLongInt presentFrameStatUtilSeeds;

#ifdef MSE_TIME_EVOLUTION_COMPUTING
	vector<tMatrix> presentFrameMSEtimeEvolution(SNRs.size());
	vector<vector<tMatrix> > MSEtimeEvolution;
	MSEtimeEvolution.reserve(nFrames);
#endif

    // we don't want the same bits to be generated over and over
#ifdef RANDOM_SEED
		Random randomGenerator;
#else
    	Random randomGenerator(0);
#endif

#define PARAMETERS_DEFINED

	// for repeating simulations
	randomGenerator.setSeed(2848936331);
	StatUtil::GetRandomGenerator().setSeed(2969730736);

    for(int iFrame=0;iFrame<nFrames;iFrame++)
    {

#ifdef MISMO_CANAL
	    cout << "hola" << endl;
		randomGenerator.setSeed(657796750);
		StatUtil::GetRandomGenerator().setSeed(2925137718);
#endif

		// the seeds are kept for saving later
	    mainSeeds.push_back(randomGenerator.getSeed());
	    statUtilSeeds.push_back(StatUtil::GetRandomGenerator().getSeed());

        // bits are generated ...
        Bits bits(N,K+nSmoothingBitsVectors,randomGenerator);

        // ... and then modulated by means of the alphabet
        tMatrix symbols = Modulator::Modulate(bits,pam2);

        // the preamble is set before the symbols to be transmitted
        symbols = Util::Append(preamble,symbols);

		// all the above symbols must be processed except those generated due to the smoothing
        lastSymbolVectorInstant = symbols.cols() - nSmoothingSymbolsVectors;

        // an AR channel is generated
// 	    ARchannel canal(N,L,m,symbols.cols(),ARprocess(powerProfile.GenerateChannelMatrix(randomGenerator),ARcoefficients,ARvariance));
// 	    ARchannel canal(N,L,m,symbols.cols(),ARprocess(powerProfile.GenerateChannelMatrix(randomGenerator),ARprocessOrder,velocity/3.6,carrierFrequency,T));


		BesselChannel canal(N,L,m,symbols.cols(),velocity,carrierFrequency,T,powerProfile);

		// "m" and "d" are obtained from the just built channel object ...
		m = canal.EffectiveMemory();
		d = m-1;

        KalmanEstimator kalmanEstimator(powerProfile.Means(),powerProfile.Variances(),N,ARcoefficients[0],ARvariance);
	    RLSEstimator rlsEstimator(powerProfile.Means(),N,forgettingFactor);
		LMSEstimator lmsEstimator(powerProfile.Means(),N,muLMS);
	    KnownChannelChannelMatrixEstimator knownChannelEstimator(canal,preambleLength,N);
	    KnownSymbolsKalmanEstimator knownSymbolsKalmanEstimator(powerProfile.Means(),powerProfile.Variances(),N,ARcoefficients[0],ARvariance,symbols,preambleLength);

		// wrapped channel estimators
// 		OneChannelOrderPerTransmitAtennaWrapperEstimator rlsWrapper(powerProfile.Means(),N,antennasChannelOrders,new RLSEstimator(OneChannelOrderPerTransmitAtennaMIMOChannel::WithZerosMatrixToWithoutZerosMatrix(powerProfile.Means(),N,antennasChannelOrders),N,forgettingFactor));

// 		OneChannelOrderPerTransmitAtennaWrapperEstimator lmsWrapper(powerProfile.Means(),N,antennasChannelOrders,new LMSEstimator(OneChannelOrderPerTransmitAtennaMIMOChannel::WithZerosMatrixToWithoutZerosMatrix(powerProfile.Means(),N,antennasChannelOrders),N,muLMS));

// 		OneChannelOrderPerTransmitAtennaWrapperEstimator kalmanWrapper(powerProfile.Means(),N,antennasChannelOrders,new KalmanEstimator(OneChannelOrderPerTransmitAtennaMIMOChannel::WithZerosMatrixToWithoutZerosMatrix(powerProfile.Means(),N,antennasChannelOrders),N,ARcoefficients[0],ARvariance));

		// ...and linear detectors
		RMMSEDetector rmmseDetector(L*(c+d+1),N*(m+c+d),pam2.Variance(),forgettingFactorDetector,N*(d+1));
		MMSEDetector mmseDetectorLarge(L*(c+d+1),N*(m+c+d),pam2.Variance(),N*(d+1));
		MMSEDetector mmseDetectorSmall(L*(c+d+1),N*(d+1),pam2.Variance(),N*(d+1));
	    MMSEDetector mmseDetectorXL(L*(c+e+1),N*(e+1),pam2.Variance(),N*(d+1),0);
	    DecorrelatorDetector decorrelatorDetector(L*(c+d+1),N*(d+1),pam2.Variance());

#ifdef MISMO_CANAL
	    Random randomGen1,randomGen2;
	    randomGenerator.setSeed(randomGen1.getSeed());
	    StatUtil::GetRandomGenerator().setSeed(randomGen2.getSeed());
#endif

		// noise is generated according to the channel
		ChannelDependentNoise ruido(&canal);

		// absence of noise
		NullNoise ruidoNulo(canal.Nr(),canal.Length());

#ifdef EXPORT_REAL_DATA
			realSymbols = &symbols;
			realChannel = &canal;
			realNoise = &ruido;
#endif

        for(iSNR=0;iSNR<SNRs.size();iSNR++)
        {
            cout << "SNR = " << SNRs[iSNR] << " [Frame " << iFrame << "]..." << endl;

			// noise SNR is set
            ruido.SetSNR(SNRs[iSNR],pam2.Variance());

            // transmission
            tMatrix observaciones = canal.Transmit(symbols,ruido);


	    	// viterbi
	        EstimatedMIMOChannel kalmanEstimatedChannel(N,L,m,symbols.cols(),&kalmanEstimator,symbols,observaciones,ruido.Variances());

            // algorithms are created
            vector<Algorithm *> algorithms;

            // ----------------------- ALGORITHMS TO RUN ----------------------------

//             algorithms.push_back(new DSISoptAlgorithm ("D-SIS opt",pam2,L,N,lastSymbolVectorInstant,m,&kalmanEstimator,preamble,d,nParticles,&algoritmoRemuestreo,powerProfile.Means(),powerProfile.Variances()));

//             algorithms.push_back(new SISoptAlgorithm ("SIS opt",pam2,L,N,lastSymbolVectorInstant,m,&kalmanEstimator,preamble,nParticles,&algoritmoRemuestreo,powerProfile.Means(),powerProfile.Variances()));

//             algorithms.push_back(new LinearFilterBasedSMCAlgorithm("LMS-D-SIS",pam2,L,N,lastSymbolVectorInstant,m,&lmsEstimator,&rmmseDetector,preamble,d,nParticles,&algoritmoRemuestreo,powerProfile.Means(),powerProfile.Variances(),ARcoefficients[0],firstSampledChannelMatrixVariance,ARvariance));

// 			algorithms.push_back(new LinearFilterBasedSMCAlgorithm("RLS-D-SIS",pam2,L,N,lastSymbolVectorInstant,m,&rlsEstimator,&rmmseDetector,preamble,c,d,nParticles,&algoritmoRemuestreo,powerProfile.Means(),powerProfile.Variances(),ARcoefficients[0],firstSampledChannelMatrixVariance,ARvariance));

// 	        algorithms.push_back(new LinearFilterBasedSMCAlgorithm("RLS-D-SIS (known channel)",pam2,L,N,lastSymbolVectorInstant,m,&knownChannelEstimator,&mmseDetectorSmall,preamble,c,d,nParticles,&algoritmoRemuestreo,powerProfile.Means(),powerProfile.Variances(),ARcoefficients[0],firstSampledChannelMatrixVariance,ARvariance));

// 	        algorithms.push_back(new TriangularizationBasedSMCAlgorithm("Cholesky",pam2,L,N,lastSymbolVectorInstant,m,&kalmanEstimator,preamble,d,nParticles,&algoritmoRemuestreo,powerProfile.Means(),powerProfile.Variances(),ARcoefficients[0],ARvariance));

	        algorithms.push_back(new LinearFilterBasedMKFAlgorithm("MKF (MMSE)",pam2,L,N,lastSymbolVectorInstant,m,&kalmanEstimator,&mmseDetectorSmall,preamble,c,d,d,nParticles,&algoritmoRemuestreo,powerProfile.Means(),powerProfile.Variances(),ARcoefficients[0],firstSampledChannelMatrixVariance,ARvariance));

// 	        algorithms.push_back(new LinearFilterBasedMKFAlgorithm("MKF (Decorrelator)",pam2,L,N,lastSymbolVectorInstant,m,&kalmanEstimator,&decorrelatorDetector,preamble,c,d,d,nParticles,&algoritmoRemuestreo,powerProfile.Means(),powerProfile.Variances(),ARcoefficients[0],firstSampledChannelMatrixVariance,ARvariance));

//             algorithms.push_back(new ViterbiAlgorithm("Viterbi",pam2,L,N,lastSymbolVectorInstant,canal,preamble,d));

//             algorithms.push_back(new KnownSymbolsKalmanBasedChannelEstimator("Kalman Filter (Known Symbols)",pam2,L,N,lastSymbolVectorInstant,m,&kalmanEstimator,preamble,symbols));

//             algorithms.push_back(new PSPAlgorithm("PSPAlgorithm",pam2,L,N,lastSymbolVectorInstant,m,&kalmanEstimator,preamble,d,lastSymbolVectorInstant+d,ARcoefficients[0],nSurvivors));

// 			algorithms.push_back(new PSPBasedSMCAlgorithm("PSP based SMC algorithm",pam2,L,N,lastSymbolVectorInstant,m,&kalmanEstimator,preamble,d,nParticles,&withoutReplacementResampling,powerProfile.Means(),powerProfile.Variances(),ARcoefficients[0]));

// 			algorithms.push_back(new PSPBasedSMCAlgorithm("PSP based SMC algorithm (best particles resampling)",pam2,L,N,lastSymbolVectorInstant,m,&kalmanEstimator,preamble,d,nParticles,&bestParticlesResampling,powerProfile.Means(),powerProfile.Variances(),ARcoefficients[0]));

			algorithms.push_back(new LinearFilterBasedAlgorithm("Kalman Filter + MMSE",pam2,L,N,lastSymbolVectorInstant,m,&kalmanEstimator,preamble,c,d,&mmseDetectorSmall,ARcoefficients[0]));

	        algorithms.push_back(new LinearFilterBasedAlgorithm("Kalman Filter (known symbols) + MMSE",pam2,L,N,lastSymbolVectorInstant,m,&knownSymbolsKalmanEstimator,preamble,c,d,&mmseDetectorSmall,ARcoefficients[0]));

							// -------- One channel order per antenna ------
//             algorithms.push_back(new DSISoptAlgorithm ("D-SIS opt (one channel order per antenna)",pam2,L,N,lastSymbolVectorInstant,m,&kalmanWrapper,preamble,d,nParticles,&algoritmoRemuestreo));
//
//             algorithms.push_back(new LinearFilterBasedSMCAlgorithm("LMS-D-SIS (one channel order per antenna)",pam2,L,N,lastSymbolVectorInstant,m,&lmsWrapper,&rmmseDetector,preamble,d,nParticles,&algoritmoRemuestreo,ARcoefficients[0],firstSampledChannelMatrixVariance,ARvariance));
//
//             algorithms.push_back(new LinearFilterBasedSMCAlgorithm("RLS-D-SIS (one channel order per antenna)",pam2,L,N,lastSymbolVectorInstant,m,&rlsWrapper,&rmmseDetector,preamble,d,nParticles,&algoritmoRemuestreo,ARcoefficients[0],firstSampledChannelMatrixVariance,ARvariance));
//
//             algorithms.push_back(new KnownSymbolsKalmanBasedChannelEstimator("Kalman Filter (Known Symbols) (one channel order per antenna)",pam2,L,N,lastSymbolVectorInstant,m,&kalmanWrapper,preamble,symbols));
							// ---------------------------------------------

//             algorithms.push_back(new ISIS("ISIS",pam2,L,N,lastSymbolVectorInstant,kalmanChannelEstimators,preamble,preamble.cols(),d,nParticles,&algoritmoRemuestreo,canal,symbols));

//             algorithms.push_back(new USIS("USIS",pam2,L,N,lastSymbolVectorInstant,RLSchannelEstimators,RMMSElinearDetectors,preamble,preamble.cols(),d,nParticles,&algoritmoRemuestreo,channelOrderEstimator,ARcoefficients[0],firstSampledChannelMatrixVariance,ARvariance/*,canal,symbols*/));

// 		        algorithms.push_back(new USIS2SISAlgorithm("USIS2SISAlgorithm",pam2,L,N,lastSymbolVectorInstant,RLSchannelEstimators,RMMSElinearDetectors,preamble,preamble.cols(),d,nParticles,&algoritmoRemuestreo,channelOrderEstimator,ARcoefficients[0],firstSampledChannelMatrixVariance,ARvariance,&USISmaximumProbabilityCriterion));

//             algorithms.push_back(new UPSPBasedSMCAlgorithm("Unknown channel order PSP based SMC algorithm",pam2,L,N,lastSymbolVectorInstant,RLSchannelEstimators,preamble,preamble.cols(),d,nParticles,&withoutReplacementResampling,ARcoefficients[0],firstSampledChannelMatrixVariance,ARvariance));

			// the RLS algorithm considering all posible channel orders
// 			for(iChannelOrder=0;iChannelOrder<candidateChannelOrders.size();iChannelOrder++)
// 			{
//
//                 char buffer[SPRINTF_BUFFER];
//
//                 if(candidateChannelOrders[iChannelOrder]<m)
//                 	sprintf(buffer," underestimated (m=%d)",candidateChannelOrders[iChannelOrder]);
//                 else if(candidateChannelOrders[iChannelOrder]>m)
//                 	sprintf(buffer," overestimated (m=%d)",candidateChannelOrders[iChannelOrder]);
//                 else buffer[0] = '\0';
//
// 				algorithms.push_back(new LinearFilterBasedSMCAlgorithm(string("RLS-D-SIS") + string(buffer),pam2,L,N,lastSymbolVectorInstant,candidateChannelOrders[iChannelOrder],RLSchannelEstimators[iChannelOrder],RMMSElinearDetectors[iChannelOrder],preamble,candidateChannelOrders[iChannelOrder]-1,nParticles,&algoritmoRemuestreo,channelOrderCoefficientsMeans[iChannelOrder],channelOrderCoefficientsVariances[iChannelOrder],ARcoefficients[0],firstSampledChannelMatrixVariance,ARvariance));
// 			}

	        // --------------------- testing simulations ------------------
// #include <resamplingRate.h>
// #include <backwardSmoothing.h>
// #include <backwardForwardSmoothing.h>
// #include <particlesNumber.h>
// #include <MMSEforwardSmoothing.h>
	        // ------------------------------------------------------------

			// ---------------------------------------------------------------------------------

            // here the number of algoriths is known. So, the first iteration:
            if(iFrame==0 && iSNR==0)
            {
                overallPeMatrix = LaGenMatDouble::zeros(SNRs.size(),algorithms.size());
                presentFramePe = LaGenMatDouble::zeros(SNRs.size(),algorithms.size());

                overallMseMatrix = LaGenMatDouble::zeros(SNRs.size(),algorithms.size());
                presentFrameMSE = LaGenMatDouble::zeros(SNRs.size(),algorithms.size());

				// Pe evolution
				for(uint i=0;i<SNRs.size();i++)
				{
					overallPeTimeEvolution[i] = tMatrix(algorithms.size(),K);
					overallErrorsNumberTimeEvolution[i] = LaGenMatInt::zeros(algorithms.size(),K);
				}

				// we fill the vector with the names of the algorithms
				for(uint iAlgorithm=0;iAlgorithm<algorithms.size();iAlgorithm++)
				{
					algorithmsNames.push_back(algorithms[iAlgorithm]->GetName());

                    // ...besides we find out whether the algorithm performs channel order APP estimation
					if(algorithms[iAlgorithm]->PerformsChannelOrderAPPEstimation())
						// +1 is because in Octave/Matlab there is no 0 index
						iAlgorithmsPerformingChannelOrderAPPestimation.push_back(iAlgorithm+1);
				}

				// we set the size of the results matrix for channel order APPs evolution according to the number of algorithms
                // counted above
				presentFrameChannelOrderAPPsAlongTime = vector<vector<tMatrix> >(iAlgorithmsPerformingChannelOrderAPPestimation.size(),vector<tMatrix>(SNRs.size(),LaGenMatDouble::zeros(candidateChannelOrders.size(),K)));

#ifdef MSE_TIME_EVOLUTION_COMPUTING
				for(uint i=0;i<SNRs.size();i++)
					presentFrameMSEtimeEvolution[i] = tMatrix(algorithms.size(),K);
#endif

	            presentFrameStatUtilSeeds = LaGenMatLongInt(SNRs.size(),algorithms.size());
            }

            // algorithms are executed
            for(uint iAlgorithm=0,iAlgorithmPerformingChannelOrderAPPestimation=0;iAlgorithm<algorithms.size();iAlgorithm++)
            {
	            // in order to repeat a concrete simulation...
	            StatUtil::GetRandomGenerator().setSeed(3720678788);

	            // the seed kept by the class StatUtil is saved
	            presentFrameStatUtilSeeds(iSNR,iAlgorithm) = StatUtil::GetRandomGenerator().getSeed();

	            algorithms[iAlgorithm]->Run(observaciones,ruido.Variances(),symbols(rAllSymbolRows,tRange(preambleLength,preambleLength+trainSeqLength-1)));
//                 algorithms[iAlgorithm]->Run(observaciones,ruido.Variances());(preambleLength,preambleLength+trainSeqLength-1)

                tMatrix detectedSymbols = algorithms[iAlgorithm]->GetDetectedSymbolVectors();

	            pe = TransmissionUtil::ComputeBERsolvingAmbiguity(bits,BERwindowStart,K,Demodulator::Demodulate(detectedSymbols,pam2),BERwindowStart,K,permutations);

                mse = algorithms[iAlgorithm]->MSE(canal.Range(preambleLength+MSEwindowStart,lastSymbolVectorInstant-1));

#ifdef MSE_TIME_EVOLUTION_COMPUTING
	            tVector mseAlongTime = TransmissionUtil::MSEalongTime(algorithms[iAlgorithm]->GetEstimatedChannelMatrices(),0,K-1,canal.Range(preambleLength,preambleLength+K-1),0,K-1);
	            for(int ik=0;ik<K;ik++)
		            presentFrameMSEtimeEvolution[iSNR](iAlgorithm,ik) = mseAlongTime(ik);
#endif

                cout << algorithms[iAlgorithm]->GetName() << ": Pe = " << pe << " , MSE = " << mse << endl;

                // the error probability is accumulated
                overallPeMatrix(iSNR,iAlgorithm) += pe;
                presentFramePe(iSNR,iAlgorithm) = pe;

                // and the MSE
                overallMseMatrix(iSNR,iAlgorithm) += mse;
                presentFrameMSE(iSNR,iAlgorithm) = mse;

                // for the algorithm performing channel order estimation...
                if(algorithms[iAlgorithm]->PerformsChannelOrderAPPEstimation())
                {
                    //...the probability of the different channel orders at each time instant is retrieved
                    presentFrameChannelOrderAPPsAlongTime[iAlgorithmPerformingChannelOrderAPPestimation][iSNR] = (dynamic_cast <ChannelOrderEstimatorSMCAlgorithm *>(algorithms[iAlgorithm]))->GetChannelOrderAPPsAlongTime();
                    iAlgorithmPerformingChannelOrderAPPestimation++;
                }

                // Pe evolution
                tMatrix transmittedSymbols = symbols(tRange(0,N-1),tRange(preambleLength,preambleLength+K-1));

	            if(detectedSymbols.rows()!=0)
	            {
					for(int k=0;k<K;k++)
						for(int iUser=0;iUser<N;iUser++)
							if(detectedSymbols(iUser,k)!=transmittedSymbols(iUser,k))
								overallErrorsNumberTimeEvolution[iSNR](iAlgorithm,k)++;
	            }

                delete algorithms[iAlgorithm];
            }
        } // for(int iSNR=0;iSNR<SNRs.size();iSNR++)


		// ----------------- VARIABLES SAVING ----------------------
		ofstream f(outputFileName,ofstream::trunc);

        // pe
		peMatrices.push_back(presentFramePe);
		Util::MatricesVectorToStream(peMatrices,"pe",f);

        // MSE
		MSEMatrices.push_back(presentFrameMSE);
		Util::MatricesVectorToStream(MSEMatrices,"mse",f);

		// channel order APPs evolution along time
		channelOrderAPPsAlongTime.push_back(presentFrameChannelOrderAPPsAlongTime);
		Util::MatricesVectoresVectoresVectorToStream(channelOrderAPPsAlongTime,"channelOrderAPPsAlongTime",f);
		Util::ScalarsVectorToStream(iAlgorithmsPerformingChannelOrderAPPestimation,"iAlgorithmsPerformingChannelOrderAPPestimation",f);

#ifdef MSE_TIME_EVOLUTION_COMPUTING
	    MSEtimeEvolution.push_back(presentFrameMSEtimeEvolution);
	    Util::MatricesVectoresVectorToStream(MSEtimeEvolution,"MSEtimeEvolution",f);
#endif

	    // seeds just before the run of the algorithms
// 	    beforeRunMainSeeds.push_back(presentFrameMainSeeds);
	    beforeRunStatUtilSeeds.push_back(presentFrameStatUtilSeeds);
// 	    Util::MatricesVectorToStream(beforeRunMainSeeds,"beforeRunMainSeeds",f);
	    Util::MatricesVectorToStream(beforeRunStatUtilSeeds,"beforeRunStatUtilSeeds",f);

		for(uint iSNR=0;iSNR<SNRs.size();iSNR++)
			for(uint i=0;i<algorithmsNames.size();i++)
				for(int j=0;j<K;j++)
					overallPeTimeEvolution[iSNR](i,j) = (double) overallErrorsNumberTimeEvolution[iSNR](i,j) / (double) (N*(iFrame+1));
		Util::MatricesVectorToStream(overallPeTimeEvolution,"peTimeEvolution",f);

        Util::ScalarToStream(iFrame+1,"nFrames",f);

		Util::StringsVectorToStream(algorithmsNames,"algorithmsNames",f);
        Util::ScalarToStream(L,"L",f);
        Util::ScalarToStream(N,"N",f);
        Util::ScalarToStream(m,"m",f);
        Util::ScalarToStream(K,"K",f);
        Util::ScalarToStream(trainSeqLength,"trainSeqLength",f);
        Util::ScalarToStream(nParticles,"nParticles",f);
        Util::ScalarToStream(nSurvivors,"nSurvivors",f);
        Util::ScalarToStream(resamplingRatio,"resamplingRatio",f);
        Util::ScalarToStream(d,"d",f);
        Util::ScalarToStream(BERwindowStart,"BERwindowStart",f);
        Util::ScalarToStream(MSEwindowStart,"MSEwindowStart",f);
		Util::ScalarsVectorToStream(candidateChannelOrders,"candidateOrders",f);
		Util::ScalarsVectorToStream(SNRs,"SNRs",f);
		Util::ScalarToStream(forgettingFactor,"forgettingFactor",f);
		Util::ScalarToStream(muLMS,"muLMS",f);
		Util::ScalarsVectorToStream(ARcoefficients,"ARcoefficients",f);
		Util::ScalarToStream(ARvariance,"ARvariance",f);
		Util::ScalarToStream(forgettingFactorDetector,"forgettingFactorDetector",f);
		Util::MatrixToStream(preamble,"preamble",f);
		Util::ScalarToStream(channelVariance,"channelVariance",f);
		Util::ScalarToStream(firstSampledChannelMatrixVariance,"firstSampledChannelMatrixVariance",f);
		Util::ScalarToStream(nSmoothingBitsVectors,"nSmoothingBitsVectors",f);
		Util::ScalarToStream(preambleLength,"preambleLength",f);
		Util::ScalarToStream(ARprocessOrder,"ARprocessOrder",f);
		Util::ScalarToStream(velocity,"velocity",f);
		Util::ScalarToStream(carrierFrequency,"carrierFrequency",f);
		Util::ScalarToStream(symbolRate,"symbolRate",f);
		Util::ScalarsVectorToStream(mainSeeds,"mainSeeds",f);
		Util::ScalarsVectorToStream(statUtilSeeds,"statUtilSeeds",f);

	    Util::MatricesVectorToStream(canal.Range(preambleLength,lastSymbolVectorInstant),"channel",f);

		f.close();
		// ---------------------------------------------------------

    } // for(int iFrame=0;iFrame<nFrames;iFrame++)

    overallPeMatrix *= 1.0/nFrames;
    overallMseMatrix *= 1.0/nFrames;

    cout << "Overall SER:" << endl;
    Util::Print(overallPeMatrix);

    cout << "Overall MSE:" << endl;
    Util::Print(overallMseMatrix);

    // memory is released
	for(iChannelOrder=0;iChannelOrder<candidateChannelOrders.size();iChannelOrder++)
	{
		delete kalmanChannelEstimators[iChannelOrder];
		delete RLSchannelEstimators[iChannelOrder];
		delete RMMSElinearDetectors[iChannelOrder];
	}
	delete channelOrderEstimator;
}
