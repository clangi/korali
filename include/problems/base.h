#ifndef _KORALI_BASEPROBLEM_H_
#define _KORALI_BASEPROBLEM_H_

#include "distributions/base.h"
#include <vector>
#include "stdlib.h"

namespace Korali
{

class BaseProblem
{
  public:

	BaseProblem(size_t seed = 0);

	void addParameter(BaseDistribution* p);
  virtual double evaluateFitness(double* sample) = 0;

  size_t _parameterCount;
	size_t _seed;

  std::vector<BaseDistribution*> _parameters;

  double getPriorsLogProbabilityDensity(double *x);
  double getPriorsProbabilityDensity(double *x);
  void initializeParameters();
};

} // namespace Korali


#endif // _KORALI_BASEPROBLEM_H_