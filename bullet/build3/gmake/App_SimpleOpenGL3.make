# GNU Make project makefile autogenerated by Premake
ifndef config
  config=release64
endif

ifndef verbose
  SILENT = @
endif

ifndef CC
  CC = gcc
endif

ifndef CXX
  CXX = g++
endif

ifndef AR
  AR = ar
endif

ifndef RESCOMP
  ifdef WINDRES
    RESCOMP = $(WINDRES)
  else
    RESCOMP = windres
  endif
endif

ifeq ($(config),release64)
  OBJDIR     = obj/x64/Release/App_SimpleOpenGL3
  TARGETDIR  = ../../bin
  TARGET     = $(TARGETDIR)/App_SimpleOpenGL3_gmake_x64_release
  DEFINES   += -DGLEW_INIT_OPENGL11_FUNCTIONS=1 -DDYNAMIC_LOAD_X11_FUNCTIONS -DGLEW_DYNAMIC_LOAD_ALL_GLX_FUNCTIONS=1 -DGLEW_STATIC
  INCLUDES  += -I../../examples/SimpleOpenGL3 -I../../src -I../../examples -I../../examples/ThirdPartyLibs/optionalX11 -I../../examples/ThirdPartyLibs/glad
  CPPFLAGS  += -MMD -MP $(DEFINES) $(INCLUDES)
  CFLAGS    += $(CPPFLAGS) $(ARCH) -O2 -msse2 -ffast-math -m64
  CXXFLAGS  += $(CFLAGS) 
  LDFLAGS   += -L../../bin -s -m64 -L/usr/lib64
  RESFLAGS  += $(DEFINES) $(INCLUDES) 
  LIBS      += ../../bin/libOpenGL_Window_gmake_x64_release.a ../../bin/libBullet3Common_gmake_x64_release.a -ldl -lpthread
  LDDEPS    += ../../bin/libOpenGL_Window_gmake_x64_release.a ../../bin/libBullet3Common_gmake_x64_release.a
  LINKCMD    = $(CXX) -o $(TARGET) $(OBJECTS) $(RESOURCES) $(ARCH) $(LIBS) $(LDFLAGS)
  define PREBUILDCMDS
  endef
  define PRELINKCMDS
  endef
  define POSTBUILDCMDS
  endef
endif

ifeq ($(config),debug64)
  OBJDIR     = obj/x64/Debug/App_SimpleOpenGL3
  TARGETDIR  = ../../bin
  TARGET     = $(TARGETDIR)/App_SimpleOpenGL3_gmake_x64_debug
  DEFINES   += -D_DEBUG=1 -DGLEW_INIT_OPENGL11_FUNCTIONS=1 -DDYNAMIC_LOAD_X11_FUNCTIONS -DGLEW_DYNAMIC_LOAD_ALL_GLX_FUNCTIONS=1 -DGLEW_STATIC
  INCLUDES  += -I../../examples/SimpleOpenGL3 -I../../src -I../../examples -I../../examples/ThirdPartyLibs/optionalX11 -I../../examples/ThirdPartyLibs/glad
  CPPFLAGS  += -MMD -MP $(DEFINES) $(INCLUDES)
  CFLAGS    += $(CPPFLAGS) $(ARCH) -g -ffast-math -m64
  CXXFLAGS  += $(CFLAGS) 
  LDFLAGS   += -L../../bin -m64 -L/usr/lib64
  RESFLAGS  += $(DEFINES) $(INCLUDES) 
  LIBS      += ../../bin/libOpenGL_Window_gmake_x64_debug.a ../../bin/libBullet3Common_gmake_x64_debug.a -ldl -lpthread
  LDDEPS    += ../../bin/libOpenGL_Window_gmake_x64_debug.a ../../bin/libBullet3Common_gmake_x64_debug.a
  LINKCMD    = $(CXX) -o $(TARGET) $(OBJECTS) $(RESOURCES) $(ARCH) $(LIBS) $(LDFLAGS)
  define PREBUILDCMDS
  endef
  define PRELINKCMDS
  endef
  define POSTBUILDCMDS
  endef
endif

OBJECTS := \
	$(OBJDIR)/glx.o \
	$(OBJDIR)/gl.o \
	$(OBJDIR)/main.o \

RESOURCES := \

SHELLTYPE := msdos
ifeq (,$(ComSpec)$(COMSPEC))
  SHELLTYPE := posix
endif
ifeq (/bin,$(findstring /bin,$(SHELL)))
  SHELLTYPE := posix
endif

.PHONY: clean prebuild prelink

all: $(TARGETDIR) $(OBJDIR) prebuild prelink $(TARGET)
	@:

$(TARGET): $(GCH) $(OBJECTS) $(LDDEPS) $(RESOURCES)
	@echo Linking App_SimpleOpenGL3
	$(SILENT) $(LINKCMD)
	$(POSTBUILDCMDS)

$(TARGETDIR):
	@echo Creating $(TARGETDIR)
ifeq (posix,$(SHELLTYPE))
	$(SILENT) mkdir -p $(TARGETDIR)
else
	$(SILENT) mkdir $(subst /,\\,$(TARGETDIR))
endif

$(OBJDIR):
	@echo Creating $(OBJDIR)
ifeq (posix,$(SHELLTYPE))
	$(SILENT) mkdir -p $(OBJDIR)
else
	$(SILENT) mkdir $(subst /,\\,$(OBJDIR))
endif

clean:
	@echo Cleaning App_SimpleOpenGL3
ifeq (posix,$(SHELLTYPE))
	$(SILENT) rm -f  $(TARGET)
	$(SILENT) rm -rf $(OBJDIR)
else
	$(SILENT) if exist $(subst /,\\,$(TARGET)) del $(subst /,\\,$(TARGET))
	$(SILENT) if exist $(subst /,\\,$(OBJDIR)) rmdir /s /q $(subst /,\\,$(OBJDIR))
endif

prebuild:
	$(PREBUILDCMDS)

prelink:
	$(PRELINKCMDS)

ifneq (,$(PCH))
$(GCH): $(PCH)
	@echo $(notdir $<)
ifeq (posix,$(SHELLTYPE))
	-$(SILENT) cp $< $(OBJDIR)
else
	$(SILENT) xcopy /D /Y /Q "$(subst /,\,$<)" "$(subst /,\,$(OBJDIR))" 1>nul
endif
	$(SILENT) $(CXX) $(CXXFLAGS) -o "$@" -MF $(@:%.o=%.d) -c "$<"
endif

$(OBJDIR)/glx.o: ../../examples/ThirdPartyLibs/glad/glx.c
	@echo $(notdir $<)
	$(SILENT) $(CC) $(CFLAGS) -o "$@" -MF $(@:%.o=%.d) -c "$<"
$(OBJDIR)/gl.o: ../../examples/ThirdPartyLibs/glad/gl.c
	@echo $(notdir $<)
	$(SILENT) $(CC) $(CFLAGS) -o "$@" -MF $(@:%.o=%.d) -c "$<"
$(OBJDIR)/main.o: ../../examples/SimpleOpenGL3/main.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(CXXFLAGS) -o "$@" -MF $(@:%.o=%.d) -c "$<"

-include $(OBJECTS:%.o=%.d)
