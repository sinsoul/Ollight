#ifdef _MSC_VER

#  if(_MSC_VER == 1800)       // MSVC++ 12.0  (Visual Studio 2013)
#  elif(_MSC_VER == 1700)     // MSVC++ 11.0  (Visual Studio 2012)
#  elif(_MSC_VER == 1600)     // MSVC++ 10.0  (Visual Studio 2010)
#  elif(_MSC_VER == 1500)     // MSVC++ 9.0   (Visual Studio 2008)
#  elif(_MSC_VER == 1400)     // MSVC++ 8.0   (Visual Studio 2005)
#  elif(_MSC_VER == 1310)     // MSVC++ 7.1   (Visual Studio 2003)
#  elif(_MSC_VER == 1300)     // MSVC++ 7.0
#  elif(_MSC_VER == 1200)     // MSVC++ 6.0   (Visual Studio 6)
#    include "config-vc6.h"
#  elif(_MSC_VER == 1100)     // MSVC++ 5.0   (Visual Studio 5)
#  endif

#endif
