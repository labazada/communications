#include "Random.h"
#include <math.h>

using namespace std;

double Random::randn ()
{
  static bool havesmpl = false;
  static double smpl;

  if(havesmpl)
  {
	  havesmpl = false;
	  return smpl;
  }
  else
  {
	  double x1, x2, w;

	  do
		{
		  x1 = 2.0 * (rand_r (&_seed) / (double) RAND_MAX) - 1.0;
		  x2 = 2.0 * (rand_r (&_seed) / (double) RAND_MAX) - 1.0;
		  w = x1 * x1 + x2 * x2;
		}
	  while (w >= 1.0);

	  w = sqrt (-2.0 * log (w) / w);

	  smpl = x2 * w;
	  havesmpl = true;

	  return x1 * w;
  }
}

// double* Random::randnArray(int n,double mean,double variance)
// {
// 	double* array = new double[n];
// 	double stdDv = sqrt(variance);
// 	for(int i=0;i<n;i++)
// 		array[i] = randn()*stdDv + mean;
// 	return array;
// }

complex<double> Random::complexRandn()
{
  double x1, x2, w, y1, y2;

  do
    {
      x1 = 2.0 * (rand_r (&_seed) / (double) RAND_MAX) - 1.0;
      x2 = 2.0 * (rand_r (&_seed) / (double) RAND_MAX) - 1.0;
      w = x1 * x1 + x2 * x2;
    }
  while (w >= 1.0);

  w = sqrt (-2.0 * log (w) / w);
  y1 = x1 * w;
  y2 = x2 * w;

  return complex<double> (y1, y2);
}


