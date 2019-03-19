#ifndef _KORALI_BASEPROBLEM_H_
#define _KORALI_BASEPROBLEM_H_

#include "parameters/base.h"
#include <vector>
#include "stdlib.h"

namespace Korali::Problem
{

class Base
{
  public:

	Base(size_t seed = 0);

	void addParameter(Korali::Parameter::Base* p);
  virtual double evaluateFitness(double* sample) = 0;

  size_t _parameterCount;
	size_t _seed;

  std::vector<Korali::Parameter::Base*> _parameters;

  double getPriorsLogProbabilityDensity(double *x);
  double getPriorsProbabilityDensity(double *x);
  void initializeParameters();
};

} // namespace Korali


#endif // _KORALI_BASEPROBLEM_H_
