#ifndef COMMON_H
#define COMMON_H

/* ****************************************************************************
 * Holds commonly used #defines used for uniformity and code readability
 * For any custom error codes, start high (e.g. 0xFFFFFFFF) and work down to
 * reduce the probability of overlapping return codes.
 * ***************************************************************************/

#define OK                      0
#define GENERAL_FAILURE         1
#define CATASTROPHIC_FAILURE    2

#define SET_FAILED_ON_SRC_LINE  failed_on_src_line = failed_on_src_line == 0 ? __LINE__ : failed_on_src_line


enum read_write_enum {
  READ_ONLY
  ,COMMIT_WRITE
};

typedef read_write_enum ReadOrWrite;

#define BIT_63                  0x8000000000000000
#define BIT_62                  0x4000000000000000
#define BIT_61                  0x2000000000000000
#define BIT_60                  0x1000000000000000
#define BIT_59                  0x800000000000000
#define BIT_58                  0x400000000000000
#define BIT_57                  0x200000000000000
#define BIT_56                  0x100000000000000
#define BIT_55                  0x80000000000000
#define BIT_54                  0x40000000000000
#define BIT_53                  0x20000000000000
#define BIT_52                  0x10000000000000
#define BIT_51                  0x8000000000000
#define BIT_50                  0x4000000000000
#define BIT_49                  0x2000000000000
#define BIT_48                  0x1000000000000
#define BIT_47                  0x800000000000
#define BIT_46                  0x400000000000
#define BIT_45                  0x200000000000
#define BIT_44                  0x100000000000
#define BIT_43                  0x80000000000
#define BIT_42                  0x40000000000
#define BIT_41                  0x20000000000
#define BIT_40                  0x10000000000
#define BIT_39                  0x8000000000
#define BIT_38                  0x4000000000
#define BIT_37                  0x2000000000
#define BIT_36                  0x1000000000
#define BIT_35                  0x800000000
#define BIT_34                  0x400000000
#define BIT_33                  0x200000000
#define BIT_32                  0x100000000
#define BIT_31                  0x80000000
#define BIT_30                  0x40000000
#define BIT_29                  0x20000000
#define BIT_28                  0x10000000
#define BIT_27                  0x8000000
#define BIT_26                  0x4000000
#define BIT_25                  0x2000000
#define BIT_24                  0x1000000
#define BIT_23                  0x800000
#define BIT_22                  0x400000
#define BIT_21                  0x200000
#define BIT_20                  0x100000
#define BIT_19                  0x80000
#define BIT_18                  0x40000
#define BIT_17                  0x20000
#define BIT_16                  0x10000
#define BIT_15                  0x8000
#define BIT_14                  0x4000
#define BIT_13                  0x2000
#define BIT_12                  0x1000
#define BIT_11                  0x800
#define BIT_10                  0x400
#define BIT_9                   0x200
#define BIT_8                   0x100
#define BIT_7                   0x80
#define BIT_6                   0x40
#define BIT_5                   0x20
#define BIT_4                   0x10
#define BIT_3                   0x8
#define BIT_2                   0x4
#define BIT_1                   0x2
#define BIT_0                   0x1

#define NUM_BYTES_IN_WORD       2
#define NUM_BYTES_IN_DWORD      4
#define NUM_BYTES_IN_QWORD      8

#define NUM_BITS_IN_BYTE        8
#define NUM_BITS_IN_WORD        16
#define NUM_BITS_IN_DWORD       32
#define NUM_BITS_IN_QWORD       64

enum log_level_enum {
  SILENT
  ,ILLUSTRATIVE
  ,VERBOSE
  ,EFFUSIVE
  ,DEBUG
};

typedef log_level_enum logLvlEnum;


#endif


