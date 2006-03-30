#include <iostream>
#include <cstdlib>
#include <vector>
#include "types.h"
#include <Alphabet.h>
#include <Bits.h>
#include <ARprocess.h>
#include <ARchannel.h>
#include <ChannelDependentNoise.h>
#include <Modulator.h>
#include <Demodulator.h>
#include <Util.h>
#include <lapackpp/gmd.h>
#include <lapackpp/blas2pp.h>
#include <lapackpp/blas3pp.h>
#include <lapackpp/laslv.h>
#include <lapackpp/lavli.h>
#include <mylapack.h>

using namespace std;
// using namespace la;
int main(int argc,char* argv[])
{
    int longitudAlphabet = 2;
//     LaGenMatDouble matriz(4,2);
//     LaGenMatDouble matriz2(2,3);
//     LaGenMatDouble res(4,3);
//     matriz = 3;
//     matriz2 = 1;
//     matriz2(1,1) = 3;
//     Blas_Mat_Mat_Mult(matriz,matriz2,res);
// //     matriz2(2,2) = 18;
//     cout << "Hola, holita\n" << matriz << matriz2 << "a*b\n" << res;

//     cout << "El primer argumento " << argv[1] << "\n";
//     tBit secuenciasBits[][2] = {{0,0},{0,1},{1,0},{1,1}};
//     tSymbol simbolos[] = {-3,-1,1,3};
    vector<vector<tBit> > secuenciasBits(4,vector<tBit>(2));
    secuenciasBits[0][0] = 0; secuenciasBits[0][1] = 0;
    secuenciasBits[1][0] = 0; secuenciasBits[1][1] = 1;
    secuenciasBits[2][0] = 1; secuenciasBits[2][1] = 0;
    secuenciasBits[3][0] = 1; secuenciasBits[3][1] = 1;
    vector<tSymbol> simbolos(4);
    simbolos[0] = -3; simbolos[1] = -1; simbolos[2] = 1; simbolos[3] = 3;
    Alphabet pam4(2,4,secuenciasBits,simbolos);

    vector<tBit> secuenciaAbuscar(2);
    secuenciaAbuscar[0] = 1; secuenciaAbuscar[1] = 1;
    tSymbol simboloDevuelto = pam4[secuenciaAbuscar];

	// -------------- PAM2 -------------------
//     vector<vector<tBit> > secuenciasBits(2,vector<tBit>(1));
	secuenciasBits = *(new vector<vector<tBit> >(2,vector<tBit>(1)));
    secuenciasBits[0][0] = 0; secuenciasBits[1][0] = 1;
//     vector<tSymbol> simbolos(2);
	simbolos = *(new vector<tSymbol>(2));
    simbolos[0] = -1; simbolos[1] = 1;
    Alphabet pam2(1,2,secuenciasBits,simbolos);
	

	vector<tBit> secuenciaDevuelta;
	secuenciaDevuelta = pam4[-1];
	cout << simboloDevuelto << endl;
// 	cout << "el tama�o de la secuencia es " << secuenciaDevuelta[0];
	secuenciaDevuelta.resize(6);
	vector<tSymbol> secuenciaSimbolos(8);
	pam4.IntToSymbolsArray(13,&secuenciaSimbolos);
	cout << "Secuencia devuelta" << endl;
	for(int i=0;i<secuenciaSimbolos.size();i++)
		cout << secuenciaSimbolos[i];
	cout << endl;
	Bits bits(2,4);
	bits.Print();
	cout << "-----------" << endl;
	Bits bits2;

	tMatrix simbs = Modulator::Modulate(bits,pam2);

	cout << "Simbolos" << endl << simbs << "-------" << endl;

	Bits bitsDemodulados = Demodulator::Demodulate(simbs,pam2);

	cout << "Bits demodulados..." << endl;

	bitsDemodulados.Print();



// 	// ------------------------------ proceso AR ---------------------------------------
	vector<double> coeficientes(1);
	coeficientes[0] = 0.99999;
// 	tMatrix matrizInicial(generador.randnArray(2*3),2,3);
// 	ARprocess procesoAR(matrizInicial,coeficientes,0.001);
// 	for(int i=0;i<200;i++)
// 	{
// 		cout << procesoAR.NextMatrix() << endl << "----------" << endl;
// 	}
// 	// ---------------------------------------------------------------------------------

	ARchannel canal(2,3,2,5,0,0.1,coeficientes,0.001);
	for(int i=canal.Memory()-1;i<canal.Length();i++)
		cout << canal[i] << endl << "****************" << endl;

	ChannelDependentNoise ruido(canal);
	ruido.SetSNR(12,1);

	cout << "El ruido" << endl;
	ruido.Print();

	tMatrix observaciones = canal.Transmit(simbs,ruido);

	cout << "Las observaciones" << endl << observaciones;

// 	// ----------- operaciones entre objetos Bits ------------------------
// 	Bits otrosBits(4,50);
// 	cout << "igual a los demodulados: "<< (bitsDemodulados==bits) << "a los otros: " << (bits==otrosBits) << endl;
// 
// 	cout << "Diferencia: " << (bitsDemodulados-bits) << "a los otros: " << (bits-otrosBits) << endl;
// 	// -------------------------------------------------------------------

// 	bits2 = bits;
// 	bits2 = bits;
// 	bits2.Print();
	
// 	// ------------ Modulacion y demodulacion diferencial ------------
// 	Bits diffEncodBits = bits.DifferentialEncoding();
// 	diffEncodBits.Print();
// 	cout << "-----------" << endl;
// 	Bits diffDecodBits = diffEncodBits.DifferentialDecoding();
// 	diffDecodBits.Print();
// 	// --------------------------------------------------------------
// 
// 	// -------------- Operaciones con matrices ----------------------
// 	tMatrix A(2,4); A = 1;
// 	tMatrix A2(2,4); A2 = 4.1;
// 	tMatrix A3(2,4);
// 	tMatrix B(4,3);
// 	B = 2;
// 	tMatrix C = A*B;
// 	cout << A << endl << B << endl << C << endl;
// 	Util::Add(A,A2,A3);
// // 	cout << A << endl << A2 << endl << A3 << endl;
// 	// ----------------------------------------------------------------



// 
// // 	Random r;
// // 	cout << r.randn() << endl;
// 

// 
// 	cout << "Ruido" << endl;
// 	ruido.Print();
// 	cout << "-----------" <<endl;
// 	ruido.SetSNR(3,1);
// 	ruido.Print();
// 	vector<double> varianzas = ruido.Variances();
// 	for(int i=0;i<ruido.Length();i++)
// 		cout << varianzas[i] << endl;
// 
// 	cout << "una columna del ruido" << endl << ruido[3] << endl;
// 
// 
// 
// 	tMatrix sub = matrizAleatoria(*(new tRange(1,2)),*(new tRange(1,2)));
// 	cout << "------- (is submatrix view" << sub.is_submatrixview() << ")" << endl << sub << endl;
// 
// 	tMatrix otra(3,3);
// 	otra = 1;
// 	tVector v = otra.col(2);
// 
// 	cout << "El vector es " << endl << v;	
// 
// 	tVector res(4);
// 	Blas_Mat_Vec_Mult(matrizAleatoria,v,res);
// 
// 	cout << "matriz por vector" << endl << res;


	Random generador;
	double* arrayNormal = generador.randnArray(16);
	tMatrix matrizAleatoria(arrayNormal,4,4);
	cout << "Matriz aleatoria" << endl << matrizAleatoria << endl;

	tLongIntVector pivotes(matrizAleatoria.size(0));
	LUFactorizeIP(matrizAleatoria,pivotes);
	LaLUInverseIP(matrizAleatoria,pivotes);

	cout << "La inversa" << endl << matrizAleatoria;

    return 0;
}
