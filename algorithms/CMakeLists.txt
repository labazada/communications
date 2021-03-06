include_directories(.
		    ${PROJECT_SOURCE_DIR}/util
		    ${PROJECT_SOURCE_DIR}/util/tree
		    ${PROJECT_SOURCE_DIR}/random
		    ${PROJECT_SOURCE_DIR}/statistics
		    ${PROJECT_SOURCE_DIR}/filters
		    ${PROJECT_SOURCE_DIR}/detection
		    ${PROJECT_SOURCE_DIR}/detection/trellis
		    ${PROJECT_SOURCE_DIR}/smc
		    ${PROJECT_SOURCE_DIR}/smc/resampling
		    ${PROJECT_SOURCE_DIR}/transmission
		    ${PROJECT_SOURCE_DIR}/transmission/channel
		    ${PROJECT_SOURCE_DIR}/estimation)
ADD_LIBRARY (algorithms STATIC
			Algorithm
			CDMAunknownActiveUsersSISoptWithNoUsersActivityKnowledge
			CDMAunknownActiveUsersSISopt
			CMEapplyingAlgorithm
			CMEBasedAlgorithm
			DSISoptAlgorithm
			ISIS
			KnownChannelAlgorithm
			KnownChannelOrderAlgorithm
			KnownFlatChannelAndActiveUsersOptimalAlgorithm
			KnownFlatChannelOptimalAlgorithm
			KnownSymbolsCMEapplyingAlgorithm
			KnownSymbolsKalmanBasedChannelEstimatorAlgorithm
			LinearFilterBasedAlgorithm
			LinearFilterKFBasedAlgorithm
			LinearFilterBasedAlgorithmWithKnownChannel
			SOSMMSEBasedAlgorithm
			LinearFilterBasedCMEapplyingAlgorithm
			LinearFilterBasedMKFAlgorithm
			LinearFilterBasedSMCAlgorithm
			MLSDmAlgorithm
			MLSDmFeedBackAlgorithm
			MultipleChannelEstimatorsPerParticleSMCAlgorithm
			PSPAlgorithm
			PSPAlgorithmWithAprioriProbabilities
			PSPBasedSMCAlgorithm
			SISoptAlgorithm
			SMCAlgorithm
			TimeVaryingChannelCMEbasedAlgorithm
			TriangularizationBasedSMCAlgorithm
			UnknownActiveUsersLinearFilterBasedSMCAlgorithm
			UnknownChannelAlgorithm
			UnknownChannelOrderAlgorithm
			USIS
			ViterbiAlgorithm
			ViterbiAlgorithmWithAprioriProbabilities
			OneChannelOrderPerOutputSMCAlgorithm
			FRSsBasedUserActivityDetectionAlgorithm)
TARGET_LINK_LIBRARIES (algorithms transmission smc trellis tree)
add_subdirectory(USIS2SIS)
