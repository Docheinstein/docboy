#ifndef DEBUGGERMACROS_H
#define DEBUGGERMACROS_H

// This file is included also if ENABLE_DEBUGGER is disabled.
// It defines some macros that can be used to use the Debugger
// with the rest of the code.

#ifdef ENABLE_DEBUGGER
class DebuggerBackend;
#define DEBUGGABLE_CLASS()                                                                                             \
    friend class DebuggerBackend;                                                                                      \
    friend class DebuggerFrontend;
#define DEBUGGER_ONLY(statement) statement
#define ASSERTS_OR_DEBUGGER_ONLY(statement) statement
#else
#define DEBUGGABLE_CLASS()
#define DEBUGGER_ONLY(statement)

#ifdef ENABLE_ASSERTS
#define ASSERTS_OR_DEBUGGER_ONLY(statement) statement
#else
#define ASSERTS_OR_DEBUGGER_ONLY(statement)
#endif // ENABLE_ASSERTS

#endif // ENABLE_DEBUGGER

#endif // DEBUGGERMACROS_H