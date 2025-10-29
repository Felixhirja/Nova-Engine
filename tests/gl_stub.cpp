#include <glad/glad.h>

namespace {
void DummyPushClientAttrib(GLbitfield) {}
void DummyPopClientAttrib() {}
void DummyEnableClientState(GLenum) {}
void DummyDisableClientState(GLenum) {}
void DummyVertexPointer(GLint, GLenum, GLsizei, const GLvoid*) {}
void DummyColorPointer(GLint, GLenum, GLsizei, const GLvoid*) {}
void DummyTexCoordPointer(GLint, GLenum, GLsizei, const GLvoid*) {}
void DummyDrawElements(GLenum, GLsizei, GLenum, const GLvoid*) {}
void DummyDrawArrays(GLenum, GLint, GLsizei) {}
void DummyMatrixMode(GLenum) {}
void DummyLoadIdentity() {}
void DummyRotated(GLdouble, GLdouble, GLdouble, GLdouble) {}
void DummyTranslated(GLdouble, GLdouble, GLdouble) {}
}

PFNGLPUSHCLIENTATTRIBPROC glad_glPushClientAttrib = DummyPushClientAttrib;
PFNGLPOPCLIENTATTRIBPROC glad_glPopClientAttrib = DummyPopClientAttrib;
PFNGLENABLECLIENTSTATEPROC glad_glEnableClientState = DummyEnableClientState;
PFNGLDISABLECLIENTSTATEPROC glad_glDisableClientState = DummyDisableClientState;
PFNGLVERTEXPOINTERPROC glad_glVertexPointer = DummyVertexPointer;
PFNGLCOLORPOINTERPROC glad_glColorPointer = DummyColorPointer;
PFNGLTEXCOORDPOINTERPROC glad_glTexCoordPointer = DummyTexCoordPointer;
PFNGLDRAWELEMENTSPROC glad_glDrawElements = DummyDrawElements;
PFNGLDRAWARRAYSPROC glad_glDrawArrays = DummyDrawArrays;
PFNGLMATRIXMODEPROC glad_glMatrixMode = DummyMatrixMode;
PFNGLLOADIDENTITYPROC glad_glLoadIdentity = DummyLoadIdentity;
PFNGLROTATEDPROC glad_glRotated = DummyRotated;
PFNGLTRANSLATEDPROC glad_glTranslated = DummyTranslated;
