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
  OBJDIR     = obj/x64/Release/BulletExampleBrowserLib
  TARGETDIR  = ../../bin
  TARGET     = $(TARGETDIR)/libBulletExampleBrowserLib_gmake_x64_release.a
  DEFINES   += -DB3_USE_CLEW -DGLEW_INIT_OPENGL11_FUNCTIONS=1 -DDYNAMIC_LOAD_X11_FUNCTIONS -DGLEW_DYNAMIC_LOAD_ALL_GLX_FUNCTIONS=1 -DGLEW_STATIC -DINCLUDE_CLOTH_DEMOS
  INCLUDES  += -I../../src/clew -I../../examples/ExampleBrowser -I../../src -I../../examples/ThirdPartyLibs -I../../examples/ThirdPartyLibs/optionalX11 -I../../examples/ThirdPartyLibs/glad
  CPPFLAGS  += -MMD -MP $(DEFINES) $(INCLUDES)
  CFLAGS    += $(CPPFLAGS) $(ARCH) -O2 -msse2 -ffast-math -m64 -fPIC
  CXXFLAGS  += $(CFLAGS) 
  LDFLAGS   += -s -m64 -L/usr/lib64
  RESFLAGS  += $(DEFINES) $(INCLUDES) 
  LIBS      += -ldl -lpthread
  LDDEPS    += 
  LINKCMD    = $(AR) -rcs $(TARGET) $(OBJECTS)
  define PREBUILDCMDS
  endef
  define PRELINKCMDS
  endef
  define POSTBUILDCMDS
  endef
endif

ifeq ($(config),debug64)
  OBJDIR     = obj/x64/Debug/BulletExampleBrowserLib
  TARGETDIR  = ../../bin
  TARGET     = $(TARGETDIR)/libBulletExampleBrowserLib_gmake_x64_debug.a
  DEFINES   += -D_DEBUG=1 -DB3_USE_CLEW -DGLEW_INIT_OPENGL11_FUNCTIONS=1 -DDYNAMIC_LOAD_X11_FUNCTIONS -DGLEW_DYNAMIC_LOAD_ALL_GLX_FUNCTIONS=1 -DGLEW_STATIC -DINCLUDE_CLOTH_DEMOS
  INCLUDES  += -I../../src/clew -I../../examples/ExampleBrowser -I../../src -I../../examples/ThirdPartyLibs -I../../examples/ThirdPartyLibs/optionalX11 -I../../examples/ThirdPartyLibs/glad
  CPPFLAGS  += -MMD -MP $(DEFINES) $(INCLUDES)
  CFLAGS    += $(CPPFLAGS) $(ARCH) -g -ffast-math -m64 -fPIC
  CXXFLAGS  += $(CFLAGS) 
  LDFLAGS   += -m64 -L/usr/lib64
  RESFLAGS  += $(DEFINES) $(INCLUDES) 
  LIBS      += -ldl -lpthread
  LDDEPS    += 
  LINKCMD    = $(AR) -rcs $(TARGET) $(OBJECTS)
  define PREBUILDCMDS
  endef
  define PRELINKCMDS
  endef
  define POSTBUILDCMDS
  endef
endif

OBJECTS := \
	$(OBJDIR)/clew.o \
	$(OBJDIR)/glx.o \
	$(OBJDIR)/gl.o \
	$(OBJDIR)/OpenGLExampleBrowser.o \
	$(OBJDIR)/OpenGLGuiHelper.o \
	$(OBJDIR)/b3Clock.o \
	$(OBJDIR)/ChromeTraceUtil.o \
	$(OBJDIR)/GwenParameterInterface.o \
	$(OBJDIR)/gwenUserInterface.o \
	$(OBJDIR)/GwenTextureWindow.o \
	$(OBJDIR)/GwenProfileWindow.o \
	$(OBJDIR)/GraphingTexture.o \
	$(OBJDIR)/CollisionShape2TriangleMesh.o \
	$(OBJDIR)/b3ResourcePath.o \
	$(OBJDIR)/GL_ShapeDrawer.o \
	$(OBJDIR)/InProcessExampleBrowser.o \

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
	@echo Linking BulletExampleBrowserLib
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
	@echo Cleaning BulletExampleBrowserLib
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

$(OBJDIR)/clew.o: ../../src/clew/clew.c
	@echo $(notdir $<)
	$(SILENT) $(CC) $(CFLAGS) -o "$@" -MF $(@:%.o=%.d) -c "$<"
$(OBJDIR)/glx.o: ../../examples/ThirdPartyLibs/glad/glx.c
	@echo $(notdir $<)
	$(SILENT) $(CC) $(CFLAGS) -o "$@" -MF $(@:%.o=%.d) -c "$<"
$(OBJDIR)/gl.o: ../../examples/ThirdPartyLibs/glad/gl.c
	@echo $(notdir $<)
	$(SILENT) $(CC) $(CFLAGS) -o "$@" -MF $(@:%.o=%.d) -c "$<"
$(OBJDIR)/OpenGLExampleBrowser.o: ../../examples/ExampleBrowser/OpenGLExampleBrowser.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(CXXFLAGS) -o "$@" -MF $(@:%.o=%.d) -c "$<"
$(OBJDIR)/OpenGLGuiHelper.o: ../../examples/ExampleBrowser/OpenGLGuiHelper.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(CXXFLAGS) -o "$@" -MF $(@:%.o=%.d) -c "$<"
$(OBJDIR)/b3Clock.o: ../../examples/Utils/b3Clock.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(CXXFLAGS) -o "$@" -MF $(@:%.o=%.d) -c "$<"
$(OBJDIR)/ChromeTraceUtil.o: ../../examples/Utils/ChromeTraceUtil.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(CXXFLAGS) -o "$@" -MF $(@:%.o=%.d) -c "$<"
$(OBJDIR)/GwenParameterInterface.o: ../../examples/ExampleBrowser/GwenGUISupport/GwenParameterInterface.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(CXXFLAGS) -o "$@" -MF $(@:%.o=%.d) -c "$<"
$(OBJDIR)/gwenUserInterface.o: ../../examples/ExampleBrowser/GwenGUISupport/gwenUserInterface.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(CXXFLAGS) -o "$@" -MF $(@:%.o=%.d) -c "$<"
$(OBJDIR)/GwenTextureWindow.o: ../../examples/ExampleBrowser/GwenGUISupport/GwenTextureWindow.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(CXXFLAGS) -o "$@" -MF $(@:%.o=%.d) -c "$<"
$(OBJDIR)/GwenProfileWindow.o: ../../examples/ExampleBrowser/GwenGUISupport/GwenProfileWindow.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(CXXFLAGS) -o "$@" -MF $(@:%.o=%.d) -c "$<"
$(OBJDIR)/GraphingTexture.o: ../../examples/ExampleBrowser/GwenGUISupport/GraphingTexture.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(CXXFLAGS) -o "$@" -MF $(@:%.o=%.d) -c "$<"
$(OBJDIR)/CollisionShape2TriangleMesh.o: ../../examples/ExampleBrowser/CollisionShape2TriangleMesh.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(CXXFLAGS) -o "$@" -MF $(@:%.o=%.d) -c "$<"
$(OBJDIR)/b3ResourcePath.o: ../../examples/Utils/b3ResourcePath.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(CXXFLAGS) -o "$@" -MF $(@:%.o=%.d) -c "$<"
$(OBJDIR)/GL_ShapeDrawer.o: ../../examples/ExampleBrowser/GL_ShapeDrawer.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(CXXFLAGS) -o "$@" -MF $(@:%.o=%.d) -c "$<"
$(OBJDIR)/InProcessExampleBrowser.o: ../../examples/ExampleBrowser/InProcessExampleBrowser.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(CXXFLAGS) -o "$@" -MF $(@:%.o=%.d) -c "$<"

-include $(OBJECTS:%.o=%.d)
