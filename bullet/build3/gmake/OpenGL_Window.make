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
  OBJDIR     = obj/x64/Release/OpenGL_Window
  TARGETDIR  = ../../bin
  TARGET     = $(TARGETDIR)/libOpenGL_Window_gmake_x64_release.a
  DEFINES   += -DGLEW_INIT_OPENGL11_FUNCTIONS=1 -DDYNAMIC_LOAD_X11_FUNCTIONS -DGLEW_DYNAMIC_LOAD_ALL_GLX_FUNCTIONS=1 -DGLEW_STATIC
  INCLUDES  += -I../../examples/ThirdPartyLibs/optionalX11 -I../../examples/ThirdPartyLibs/glad -I../../examples/ThirdPartyLibs -I../../src
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
  OBJDIR     = obj/x64/Debug/OpenGL_Window
  TARGETDIR  = ../../bin
  TARGET     = $(TARGETDIR)/libOpenGL_Window_gmake_x64_debug.a
  DEFINES   += -D_DEBUG=1 -DGLEW_INIT_OPENGL11_FUNCTIONS=1 -DDYNAMIC_LOAD_X11_FUNCTIONS -DGLEW_DYNAMIC_LOAD_ALL_GLX_FUNCTIONS=1 -DGLEW_STATIC
  INCLUDES  += -I../../examples/ThirdPartyLibs/optionalX11 -I../../examples/ThirdPartyLibs/glad -I../../examples/ThirdPartyLibs -I../../src
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
	$(OBJDIR)/glx.o \
	$(OBJDIR)/gl.o \
	$(OBJDIR)/SimpleCamera.o \
	$(OBJDIR)/GLInstancingRenderer.o \
	$(OBJDIR)/GLRenderToTexture.o \
	$(OBJDIR)/EGLOpenGLWindow.o \
	$(OBJDIR)/OpenSans.o \
	$(OBJDIR)/GLPrimitiveRenderer.o \
	$(OBJDIR)/GLFWOpenGLWindow.o \
	$(OBJDIR)/TwFonts.o \
	$(OBJDIR)/X11OpenGLWindow.o \
	$(OBJDIR)/SimpleOpenGL3App.o \
	$(OBJDIR)/SimpleOpenGL2Renderer.o \
	$(OBJDIR)/LoadShader.o \
	$(OBJDIR)/opengl_fontstashcallbacks.o \
	$(OBJDIR)/fontstash.o \
	$(OBJDIR)/SimpleOpenGL2App.o \
	$(OBJDIR)/stb_image_write.o \

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
	@echo Linking OpenGL_Window
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
	@echo Cleaning OpenGL_Window
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
$(OBJDIR)/SimpleCamera.o: ../../examples/OpenGLWindow/SimpleCamera.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(CXXFLAGS) -o "$@" -MF $(@:%.o=%.d) -c "$<"
$(OBJDIR)/GLInstancingRenderer.o: ../../examples/OpenGLWindow/GLInstancingRenderer.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(CXXFLAGS) -o "$@" -MF $(@:%.o=%.d) -c "$<"
$(OBJDIR)/GLRenderToTexture.o: ../../examples/OpenGLWindow/GLRenderToTexture.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(CXXFLAGS) -o "$@" -MF $(@:%.o=%.d) -c "$<"
$(OBJDIR)/EGLOpenGLWindow.o: ../../examples/OpenGLWindow/EGLOpenGLWindow.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(CXXFLAGS) -o "$@" -MF $(@:%.o=%.d) -c "$<"
$(OBJDIR)/OpenSans.o: ../../examples/OpenGLWindow/OpenSans.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(CXXFLAGS) -o "$@" -MF $(@:%.o=%.d) -c "$<"
$(OBJDIR)/GLPrimitiveRenderer.o: ../../examples/OpenGLWindow/GLPrimitiveRenderer.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(CXXFLAGS) -o "$@" -MF $(@:%.o=%.d) -c "$<"
$(OBJDIR)/GLFWOpenGLWindow.o: ../../examples/OpenGLWindow/GLFWOpenGLWindow.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(CXXFLAGS) -o "$@" -MF $(@:%.o=%.d) -c "$<"
$(OBJDIR)/TwFonts.o: ../../examples/OpenGLWindow/TwFonts.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(CXXFLAGS) -o "$@" -MF $(@:%.o=%.d) -c "$<"
$(OBJDIR)/X11OpenGLWindow.o: ../../examples/OpenGLWindow/X11OpenGLWindow.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(CXXFLAGS) -o "$@" -MF $(@:%.o=%.d) -c "$<"
$(OBJDIR)/SimpleOpenGL3App.o: ../../examples/OpenGLWindow/SimpleOpenGL3App.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(CXXFLAGS) -o "$@" -MF $(@:%.o=%.d) -c "$<"
$(OBJDIR)/SimpleOpenGL2Renderer.o: ../../examples/OpenGLWindow/SimpleOpenGL2Renderer.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(CXXFLAGS) -o "$@" -MF $(@:%.o=%.d) -c "$<"
$(OBJDIR)/LoadShader.o: ../../examples/OpenGLWindow/LoadShader.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(CXXFLAGS) -o "$@" -MF $(@:%.o=%.d) -c "$<"
$(OBJDIR)/opengl_fontstashcallbacks.o: ../../examples/OpenGLWindow/opengl_fontstashcallbacks.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(CXXFLAGS) -o "$@" -MF $(@:%.o=%.d) -c "$<"
$(OBJDIR)/fontstash.o: ../../examples/OpenGLWindow/fontstash.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(CXXFLAGS) -o "$@" -MF $(@:%.o=%.d) -c "$<"
$(OBJDIR)/SimpleOpenGL2App.o: ../../examples/OpenGLWindow/SimpleOpenGL2App.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(CXXFLAGS) -o "$@" -MF $(@:%.o=%.d) -c "$<"
$(OBJDIR)/stb_image_write.o: ../../examples/ThirdPartyLibs/stb_image/stb_image_write.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(CXXFLAGS) -o "$@" -MF $(@:%.o=%.d) -c "$<"

-include $(OBJECTS:%.o=%.d)
