# this one is important
SET(CMAKE_SYSTEM_NAME Amiga)

# specify the cross compiler
SET(CMAKE_C_COMPILER   /home/mop/m68k-amigaos-gcc6/bin/vc)
SET(CMAKE_ASM-VASM_COMPILER /home/mop/m68k-amigaos-gcc6/bin/vasmm68k_mot)
SET(CMAKE_CXX_COMPILER /home/mop/m68k-amigaos-gcc6/bin/m68k-amigaos-g++)

SET(CMAKE_C_FLAGS "-c99 -cpu=68080 -lmieee" CACHE STRING "" FORCE)
SET(CMAKE_C_FLAGS_RELEASE "-O3" CACHE STRING "" FORCE)
SET(CMAKE_CXX_FLAGS "-noixemul -m68040" CACHE STRING "" FORCE)

SET(UNIX false)
SET(AMIGA true)

# where is the target environment 
#SET(CMAKE_FIND_ROOT_PATH  /opt/eldk-2007-01-19/ppc_74xx /home/alex/eldk-ppc74xx-inst)

# search for programs in the build host directories
#SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
# for libraries and headers in the target directories
#SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
#SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)