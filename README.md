# SimpleDataStorageFormat

Header-only library for reading and writing files of my own format called "Simple Data Storage Format" (sdsf)

This format supports:
 - bool values
 - int values
 - float values
 - strings
 - arrays
 - composite values (aka structs)
 - raw binary values
 
 To use library user must:
  - include "simple_data_storage_format.h"
  - #define SDSF_IMPL *at least once* before including header file
  
 Usage example can be found at test/main.c
