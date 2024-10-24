#ifndef DOCBOYMACROS_H
#define DOCBOYMACROS_H

#ifdef ENABLE_DEBUGGER
class DebuggerBackend;
#define DEBUGGABLE_CLASS()                                                                                             \
    friend class DebuggerBackend;                                                                                      \
    friend class DebuggerFrontend;                                                                                     \
    friend class DebuggerHelpers;
#else
#define DEBUGGABLE_CLASS()
#endif // ENABLE_DEBUGGER

#ifdef ENABLE_TESTS
#define TESTABLE_CLASS() friend class FramebufferRunner;
#else
#define TESTABLE_CLASS()
#endif

#endif // DOCBOYMACROS_H