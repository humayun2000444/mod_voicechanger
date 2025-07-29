find_path(FREESWITCH_INCLUDE_DIRS switch.h
          PATH_SUFFIXES freeswitch
          PATHS /usr/local/freeswitch/include /usr/include /usr/local/include)

find_library(FREESWITCH_LIBRARIES NAMES freeswitch
             PATHS /usr/local/freeswitch/lib /usr/lib /usr/local/lib)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Freeswitch DEFAULT_MSG
                                  FREESWITCH_LIBRARIES FREESWITCH_INCLUDE_DIRS)
