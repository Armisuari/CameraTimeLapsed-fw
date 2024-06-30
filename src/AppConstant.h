#pragma once

#if __has_include("Arduino.h")
  #include "Arduino.h"
#else
  #define log_w(...)
  #define log_i(...)
  #define log_e(...)
  #define log_d(...)
#endif