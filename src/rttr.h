#pragma once

#include "rttr/registration.h"

#define MY_RTTR_REGISTRATION                                                   \
  static void RTTR_CAT(rttr_auto_register_reflection_function_, __LINE__)();   \
  namespace {                                                                  \
  struct RTTR_CAT(rttr__auto__register__, __LINE__) {                          \
    RTTR_CAT(rttr__auto__register__, __LINE__)() {                             \
      RTTR_CAT(rttr_auto_register_reflection_function_, __LINE__)();           \
    }                                                                          \
  };                                                                           \
  }                                                                            \
  static const RTTR_CAT(rttr__auto__register__, __LINE__)                      \
      RTTR_CAT(auto_register__, __LINE__);                                     \
  static void RTTR_CAT(rttr_auto_register_reflection_function_, __LINE__)()
