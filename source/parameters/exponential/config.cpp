#include "korali.h"

using json = nlohmann::json;

json Korali::Parameter::Exponential::getConfiguration()
{
 auto js = this->Korali::Parameter::Base::getConfiguration();

 js["Type"] = "Exponential";
 js["Mean"] = _mean;

 return js;
}

void Korali::Parameter::Exponential::setConfiguration(json& js)
{
 this->Korali::Parameter::Base::setConfiguration(js);

 _mean = consume(js, { "Mean" }, KORALI_NUMBER);
}