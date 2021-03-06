#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include "logger.hpp"

korali::Logger::Logger()
{
 __verbosityLevel = 3;
 __outputFile = stdout;
}

korali::Logger::Logger(const  std::string verbosityLevel, FILE* file)
{
 __verbosityLevel = getVerbosityLevel(verbosityLevel);
 __outputFile = file;
}

void korali::Logger::setVerbosityLevel(const  std::string verbosityLevel)
{
 __verbosityLevel = getVerbosityLevel(verbosityLevel);
}

void korali::Logger::setConsoleOutputFile(FILE* file)
{
 __outputFile = file;
}

size_t korali::Logger::getVerbosityLevel(const  std::string verbosityLevel)
{
 if (verbosityLevel == "Silent") return 0;
 if (verbosityLevel == "Minimal") return 1;
 if (verbosityLevel == "Normal") return 2;
 if (verbosityLevel == "Detailed") return 3;
 return 0;
}

bool korali::Logger::isEnoughVerbosity(const std::string verbosityLevel)
{
  size_t messageLevel = getVerbosityLevel(verbosityLevel);

  if (messageLevel <= __verbosityLevel) return true;
  return false;
}

void korali::Logger::logData(const std::string verbosityLevel, const char* format, ... )
{
 if (isEnoughVerbosity(verbosityLevel) == false) return;

 char* outstr = 0;
 va_list ap;
 va_start(ap, format);
 vasprintf(&outstr, format, ap);

 fprintf(__outputFile, "%s", outstr);
 fflush(__outputFile);
 free(outstr);

}

void korali::Logger::logInfo(const std::string verbosityLevel, const char* format, ... )
{
 if (isEnoughVerbosity(verbosityLevel) == false) return;

 std::string newFormat = "[Korali] ";
 newFormat += format;

 char* outstr = 0;
 va_list ap;
 va_start(ap, format);
 vasprintf(&outstr, newFormat.c_str(), ap);

 fprintf(__outputFile, "%s", outstr);
 fflush(__outputFile);
 free(outstr);
}

void korali::Logger::logWarning(const std::string verbosityLevel, const char* format, ... )
{
 if (isEnoughVerbosity(verbosityLevel) == false) return;

 std::string newFormat = "[Korali] Warning: ";
 newFormat += format;

 char* outstr = 0;
 va_list ap;
 va_start(ap, format);
 vasprintf(&outstr, newFormat.c_str(), ap);

 if(__outputFile == stdout)
 {
  fprintf(stderr, "%s", outstr);
  fflush(stderr);
 }
 else
 {
  fprintf(__outputFile, "%s", outstr);
  fflush(__outputFile);
 }

 free(outstr);
}

void korali::Logger::logError(const char* format, ... )
{
 std::string newFormat = "[Korali] Error: ";
 newFormat += format;

 char* outstr = 0;
 va_list ap;
 va_start(ap, format);
 vasprintf(&outstr, newFormat.c_str(), ap);

 fprintf(stderr, "%s", outstr);
 fflush(stderr);
 free(outstr);

 std::abort();
}
