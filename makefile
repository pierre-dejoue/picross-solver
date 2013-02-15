all: PicrossSolver.exe

CPPSOURCES = main.cpp picross_solver.cpp
CPPOBJECTS = $(CPPSOURCES:.cpp=.obj)

# Flags taken from VS Express debug build log
#CPPFLAGS =  /c /ZI /nologo /W3 /WX- /Od /Oy- /D CODE_ANALYSIS /D _MBCS /Gm  /EHsc /RTC1 /MDd /GS /fp:precise /Zc:wchar_t /Zc:forScope /Gd /TP
# Flags taken from VS Express release build log
CPPFLAGS = /c /Zi /nologo /W3 /WX- /O2 /Oi /Oy- /GL          /D _MBCS /Gm- /EHsc /MD /GS /Gy    /fp:precise /Zc:wchar_t /Zc:forScope /Gd /TP

main.obj: main.cpp picross_solver.h
	cl $(CPPFLAGS) $*.cpp

picross_solver.obj: picross_solver.cpp picross_solver.h
	cl $(CPPFLAGS) $*.cpp

PicrossSolver.exe:  $(CPPOBJECTS)
	link /nologo /OPT:REF /OPT:ICF /DEBUG /TLBID:1 /DYNAMICBASE /NXCOMPAT /MACHINE:x86 /LTCG /out:$@ $**

clean: dummy
	-@del PicrossSolver.exe
	-@del *.obj

dummy:
	