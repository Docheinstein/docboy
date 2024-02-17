#ifndef DOCBOYMACROS_H
#define DOCBOYMACROS_H

#ifdef ENABLE_DEBUGGER
class DebuggerBackend;
#define DEBUGGABLE_CLASS()                                                                                             \
    friend class DebuggerBackend;                                                                                      \
    friend class DebuggerFrontend;                                                                                     \
    friend class DebuggerHelpers;
#define IF_DEBUGGER(statement) statement
#define IF_ASSERTS_OR_DEBUGGER(statement) statement
#define ASSERTS_OR_DEBUGGER_ENABLED
#else
#define DEBUGGABLE_CLASS()
#define IF_DEBUGGER(statement)

#ifdef ENABLE_ASSERTS
#define IF_ASSERTS_OR_DEBUGGER(statement) statement
#define ASSERTS_OR_DEBUGGER_ENABLED
#else
#define IF_ASSERTS_OR_DEBUGGER(statement)
#endif // ENABLE_ASSERTS

#endif // ENABLE_DEBUGGER

#ifdef ENABLE_TESTS
#define TESTABLE_CLASS() friend class FramebufferRunner;
#define IF_TESTS(statement) statement
#else
#define TESTABLE_CLASS()
#define IF_TESTS(statement)
#endif

#endif // DOCBOYMACROS_H