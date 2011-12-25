#ifndef ALPHABET_H
#define ALPHABET_H

#include <iostream>
#include <algorithm>
#include <vector>
#include <types.h>

class Alphabet
{
    private:
        std::vector<tSymbol> _symbols;
        std::vector<std::vector<tBit> > _bitsSequences;
        uint _nBitsPerSymbol,_length;
        double _mean,_variance;
        
        void computeMeanAndVariance();
    public:
        Alphabet(std::vector<std::vector<tBit> > secuenciasBits,std::vector<tSymbol> simbolos);
        Alphabet(std::vector<tSymbol> simbolos);        
        uint nBitsPerSymbol() const { return _nBitsPerSymbol;}
        double variance() const { return _variance;}
        tSymbol operator [](std::vector<tBit> secuenciaBitsBuscada) const;
        tSymbol operator [](uint index) const { return _symbols[index];}
        std::vector<tBit> operator [](tSymbol simbolo) const;
        uint length() const { return _length;}
        void int2symbolsArray(uint number, std::vector<tSymbol> &res) const;
        uint symbolsArray2int(std::vector<tSymbol> symbolsVector) const;
        tSymbol hardDecision(double softEstimation) const;
        tSymbol opposite(const tSymbol symbol) const { return -1.0*symbol;}
		bool doesItBelong(const tSymbol symbol) const;
		VectorXd int2eigenVector(uint number, uint length) const;
		MatrixXd int2eigenMatrix(uint number, uint rows, uint cols) const;
		
		/*!
		  it builds a new alphabet from this one with one additional symbol \ref symbol
		  \param symbol the symbol to be added
		  \return a new \ref Alphabet object
		*/
		Alphabet buildNewAlphabetByAddingSymbol(tSymbol symbol) const;
};
#endif

