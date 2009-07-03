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
#ifndef UTIL_H
#define UTIL_H

/**
    @author Manu <manu@rustneversleeps>
*/

#include <iomanip>
#include <math.h>
#include <algorithm>
#include <vector>
#include <fstream>
#include <types.h>
#include <exceptions.h>
#include "utilExceptions.h"
#include <lapackpp/gmd.h>
#include <lapackpp/blas1pp.h>
#include <lapackpp/sybmd.h>
#include <lapackpp/sybfd.h>

enum tOrder {rowwise,columnwise};

class Util{

public:

    static void add(const tMatrix &A,const tMatrix &B,tMatrix &C,double = 1.0,double = 1.0);
    static void add(const tVector &a,const tVector &b,tVector &c,double = 1.0,double = 1.0);
    static void mult(const tVector &a,const tVector &b,tMatrix &C,double = 1.0);
    static void transpose(const tMatrix &A,tMatrix &B);
    static tVector toVector(const tMatrix &matrix,tOrder order);
    static tMatrix toMatrix(const tVector &vector,tOrder order,int rows,int cols);
    static tMatrix toMatrix(const vector<double> &vector,tOrder order,uint rows,uint cols);
    static tMatrix toMatrix(const tVector &vector,tOrder order,uint rows);
    static tMatrix append(const tMatrix &A,const tMatrix &B);
    static tMatrix verticalAppend(const tMatrix &A,const tMatrix &B);
    static tVector normalize(const tVector &v);
    static double Sum(const tVector &v);
    static void max(const tVector &v,int &index);
    static void min(const tVector &v,int &index);
    static double squareErrorPaddingWithZeros(const tMatrix &A,const tMatrix &B);
    static double squareError(const tMatrix &A,const tMatrix &B);
    static void print(const tMatrix &A);
    static void MatrixToOctaveFileStream(tMatrix A,string name,ofstream &f);
    template<class T> static void MatricesVectorToOctaveFileStream(vector<T> matrices,string name,ofstream &f);
    static void LongIntMatricesVectorToOctaveFileStream(vector<LaGenMatLongInt> matrices,string name,ofstream &f);
    static void MatricesVectoresVectorToOctaveFileStream(vector<vector<tMatrix> > matrices,string name,ofstream &f);
    static void MatricesVectoresVectoresVectorToOctaveFileStream(vector<vector<vector<tMatrix> > > matrices,string name,ofstream &f);
    template<class T> static void ScalarToOctaveFileStream(T scalar,string name,ofstream &f);
    static void StringsVectorToOctaveFileStream(std::vector<string> strings,string name,ofstream &f);
    template<class T> static void scalarsVectorToOctaveFileStream(std::vector<T> vector,string name,ofstream &f);
    template<class T> static int max(const std::vector<T> &vector);
    template<class T> static void min(const std::vector<T> &vector,int &iMin);
    template<class T> static T Sum(const std::vector<T> &vector);
    static void elementWiseDiv(const tMatrix &A,const tMatrix &B,tMatrix &C);
    static void elementWiseMult(const tMatrix &A,const tMatrix &B,tMatrix &C);    
    template<class T> static void print(const std::vector<T> &vector);
    template<class T> static void print(const std::vector<std::vector<T> > &matrix);
    template<class T> static void print(const T* array,int nElements);
    static void shiftUp(tVector &v,int n);
    template<class T> static vector<vector<T> > Permutations(T *array, int nElements);
    static vector<int> SolveAmbiguity(const tMatrix &H1,const tMatrix &H2,const vector<vector<uint> > &permutations,int &iBestPermutation);
    static tMatrix applyPermutation(const tMatrix &symbols,const vector<uint> &permutation,const vector<int> &signs);
    static tMatrix cholesky(const tMatrix &matrix);
    template<class T> static void NextVector(vector<T> &vector,const vector<vector<T> > &alphabets);
    template<class T> static void HowManyTimes(const vector<T> &v,vector<int> &firstOccurrence,vector<int> &times);
    static std::vector<int> nMax(int n,const tVector &v);
    static tMatrix flipLR(const tMatrix &A);
    static tMatrix sign(const tMatrix &A);   
};

#endif
