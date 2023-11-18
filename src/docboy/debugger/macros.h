#ifndef DEBUGGERMACROS_H
#define DEBUGGERMACROS_H

// This file is included also if ENABLE_DEBUGGER is disabled.
// It defines some macros that can be used to use the Debugger
// with the rest of the code.

#ifdef ENABLE_DEBUGGER
class DebuggerBackend;
#define DEBUGGABLE_CLASS()                                                                                             \
    friend class DebuggerBackend;                                                                                      \
    friend class DebuggerFrontend;                                                                                     \
    friend class DebuggerHelpers;
#define IF_DEBUGGER(statement) statement
#define IF_ASSERTS_OR_DEBUGGER(statement) statement
#else
#define DEBUGGABLE_CLASS()
#define IF_DEBUGGER(statement)

#ifdef ENABLE_ASSERTS
#define IF_ASSERTS_OR_DEBUGGER(statement) statement
#else
#define IF_ASSERTS_OR_DEBUGGER(statement)
#endif // ENABLE_ASSERTS

#endif // ENABLE_DEBUGGER

#endif // DEBUGGERMACROS_H