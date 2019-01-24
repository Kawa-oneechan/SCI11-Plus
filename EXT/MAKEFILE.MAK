######################################################################
#
# makefile
#
# Makefile for the SCI interpreter and assorted drivers and utilities.
#
#
######################################################################

############ DEFAULT TARGETS ############
# sierra   no debug
# sierram  no debug, menu bars enabled
# sci      debug
# scitestr debug, menu bars enabled

prod: sierra

clean:
	@+del *.o? > nul
	@+del *.map > nul
	@+del sierra.exe > nul
	@+del sierram.exe > nul
	@+del sci.exe > nul
	@+del scitestr.exe > nul

# Do not use builtins (default) dependencies or actions -- everything
# is explicitly declared here.
.SUFFIXES

# Execute commands directly, without invoking command.com.  
.NOSHELL

.INIT:
	#
	# Do not use any Microsoft files for the interpreter
	#
	@%setenv include=.;i:\interp\sci\ibm
	@%setenv lib=.;i:\interp\sci\ibm
	#
	@%set masmFlags = -Fm -c -Cp -nologo -W2
	# -Fm		create a linker map file
	# -c		assemble-only (don't link)
	# -Cp		preserve case of all identifiers
	# -nologo	supress NMAKE copyright message
	# -W2		don't warn about extended jumps


.DEINIT:
# lol


#
# Files to be compiled for speed optimization.  All others will be 
# optimized for space.
#
speed  = string.c list.c animate.c motion.c getpath.c resource.c altres.c


#
# The source files which do not vary between interpreters.
#
s1	= stdio.c stdioasm.s scifgets.c string.c stdlib.c ctype.c window.c
s2	= io.s config.c event.c script.c list.c altres.c animate.c armasm.s
s3	= math.s savevars.s restart.c sound.c motion.c flist.c
s4	= savegame.c getpath.c midi.s fardata.c message.c trig.c audio.c
s5	= aud.s sync.c errmsg.c platform.c info.c scale.c mono.s
s6	= ems.s  eventasm.s extmem.s  dialog.c  fileio.s  intrpt.c  intasm.s language.c mouse.c xms.s mouseasm.s
s7	= picture.c graph.s  graph256.s cels.s resname.c palette.c pk.c movie.c
s8	= object.c array.c start.c startasm.s

src	= $(s1) $(s2) $(s3) $(s4) $(s5) $(s6) $(s7) $(s8)
base	= $[f,,$(src),o]

#
# Debugging files 
#
nodbg	= debug.o pmachine.o pmachasm.o objasm.o dos.o debugasm.o \
	resource.o sci.o opcodes.o kerndisp.o kernel.o text.o volload.o
dbg	= debug.od pmachine.od pmachasm.od objasm.od dos.od debugasm.od  \
	resource.od sci.od opcodes.od kerndisp.od kernel.od text.od volload.od

#
# Tester files
#
notst	= menu.o
tst	= menu.ot

#
# This file must come last for the memory manager to work
#
lastNodbg	= memmgr.o
lastDbg		= memmgr.od


#
# User (debugless-vols) interpreter 
#
Objs	 = $(base) $(nodbg) $(notst) $(lastNodbg)
tObjs	 = $(base) $(nodbg) $(tst)   $(lastNodbg)

#
# Inhouse debug (file, tester and vols) interpreters
#
dObjs	 = $(base) $(dbg)   $(notst) $(lastDbg)
dtObjs = $(base) $(dbg)   $(tst)   $(lastDbg)

############ DEBUG-NODEBUG RESOURCES  ############

sciBaseSetup:
# let all source files know that we are compiling the interpreter
	@%set cDefs = -DSCI
	@%set mDefs = -DSCI -DIBM

sciNoDbgSetup:
	@%do sciBaseSetup

sciNoDbgTSetup:
	@%do sciNoDbgSetup
	@%set cDefs = $(cDefs) -DTESTER
	@%set mDefs = $(mDefs) -DTESTER

sciDbgSetup:
	@%do sciBaseSetup
	@%set cDefs = $(cDefs) -DDEBUG
	@%set mDefs = $(mDefs) -DDEBUG

sciDbgTSetup:
	@%do sciDbgSetup
	@%set cDefs = $(cDefs) -DTESTER
	@%set mDefs = $(mDefs) -DTESTER

############ BUILD INTERPRETERS ##########

sierra:   sierraSetup sierra.exe

sierram:  sierramSetup sierram.exe

sci:      sciSetup sci.exe

scitestr: scitestrSetup scitestr.exe

sierraSetup:
	@%do sciNoDbgSetup

sierramSetup:
	@%do sciNoDbgTSetup

sciSetup:
	@%do sciDbgSetup

scitestrSetup:
	@%do sciDbgTSetup

sierra.exe: $(Objs)
	@%do Link

sierram.exe: $(tObjs)
	@%do Link 

sci.exe: $(dObjs)
	@%do Link 

scitestr.exe: $(dtObjs)
	@%do Link 


############ METHODS FOR BUILDING OBJECT FILES  ############

### We only do medium model assembly right now ###

.DEFAULT.s:		#don't worry about trying to make .s files

debugasm.od:	[debugasm.s]
	@%do	Assemble

memmgr.od:	[memmgr.s]
	@%do	Assemble

objasm.od:	[objasm.s]
	@%do	Assemble

opcodes.od:	[opcodes.s]
	@%do	Assemble

kerndisp.od:	[kerndisp.s]
	@%do	Assemble

pmachasm.od:	[pmachasm.s]
	@%do	Assemble

###### ASSEMBLY: conditionals and general rules######

.s.o:
	@%do	Assemble

Assemble:
	@%set srcFile = $<
	@%set objFile = $@
	@-:ml $(mDefs) $(masmFlags) -Fo$(objFile) -Ta$(srcFile)
	%if %status
		@%set errors = TRUE
		@%exit 1
	%endif

###### C COMPILATION: conditionals and general rules######

.DEFAULT.c:	# don't worry about trying to make .c files

debug.od:	[debug.c]
	%do Compile

dos.od:	[dos.c]
	%do Compile

resource.od:	[resource.c]
	%do Compile

sci.od:	[sci.c]
	%do Compile

kernel.od:	[kernel.c]
	%do Compile

text.od:	[text.c]
	%do Compile

volload.od:	[volload.c]
	%do Compile

pmachine.od:	[pmachine.c]
	%do Compile

menu.ot:	[menu.c]
	%do Compile

.c.o:
	%do Compile


Compile:
#
#	Set any debugging-related options
#
	@%set srcFile = $<
	@%set objFile = $@
	@%set cFlags = /Fm -AM -DLINT_ARGS -DIBM -Gs -J -W3 -Zl -Zp -c -nologo
	# -AM		medium model (large code, small data)
	# -DIBM		define IBM
	# -DLINT_ARGS	define LINT_ARGS, to enable argument checking
	# -Gs		disable stack checking
	# -J		chars are unsigned by default
	# -W3		give maximum number of error messages
	# -Zl		generate references to standard libraries in object file
	# -Zp		pack all structures
	# -c		compile only
	# -nologo	supress NMAKE copyright message (KAWA WAS HERE)
	#
	%if %member($<, $(speed))
		@%set cFlags = $(cFlags) -Oalt
	%else
		@%set cFlags = $(cFlags) -Os
	%endif
	@-:cl $(cDefs) $(cFlags) -Fo$(objFile) $(srcFile)
	%if %status
		@%set errors = TRUE
		@%exit 1
	%endif


################### LINKING EXECUTABLES ######################

makeExe = TRUE
errors = FALSE

lFlags = /NOI /MAP /NOE /CO
# Linker flags
# /NOI		case sensitive
# /MAP		create a link map
# /NOE		no external dictionary (whatever that means)
# /CO 		generate codeview information

Link:    
	%if $(makeExe) == TRUE
		-:link $(lFlags) <@<
$[s," +\n",$**]
$*.exe
$*.map
libh.lib implode.lib;
<
		%if %status
			@%set errors = TRUE
			@%exit 1
		%endif
	%endif
