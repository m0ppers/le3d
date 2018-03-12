# this one is important
SET(CMAKE_SYSTEM_NAME Amiga)

# specify the cross compiler
SET(CMAKE_C_COMPILER   /home/mop/m68k-amigaos/bin/m68k-amigaos-gcc)
SET(CMAKE_CXX_COMPILER /home/mop/m68k-amigaos/bin/m68k-amigaos-g++)

# for whatever reason 68040 or hard-fpu and the like seem to produce insta-crashes using amiga gcc 6.2
SET(CMAKE_C_FLAGS "-noixemul -m68030 -m68881" CACHE STRING "" FORCE)
SET(CMAKE_CXX_FLAGS "-noixemul -m68030 -m68881" CACHE STRING "" FORCE)

SET(UNIX false)
SET(AMIGA true)

# where is the target environment 
#SET(CMAKE_FIND_ROOT_PATH  /opt/eldk-2007-01-19/ppc_74xx /home/alex/eldk-ppc74xx-inst)

# search for programs in the build host directories
#SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
# for libraries and headers in the target directories
#SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
#SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)