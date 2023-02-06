// Copyright (c) 2018, 2019, 2020 Marko Mahnič
// License: MPL2. See LICENSE in the root of the project.

#pragma once

#include "environment.h"

#include "option.h"
#include "parseresult.h"

namespace argumentum {

ARGUMENTUM_INLINE Environment::Environment(
      Option& option, ParseResultBuilder& result, const ParserDefinition& parserDef )
   : mOption( option )
   , mResult( result )
   , mParserDef( parserDef )
{}

ARGUMENTUM_INLINE const ParserConfig::Data& Environment::get_config() const
{
   return mParserDef.mConfig.data();
}

ARGUMENTUM_INLINE const ParserDefinition& Environment::get_parser_def() const
{
   return mParserDef;
}

ARGUMENTUM_INLINE std::shared_ptr<IFormatHelp> Environment::get_help_formatter(
      const std::string& helpOption ) const
{
   return get_config().help_formatter( helpOption );
}

ARGUMENTUM_INLINE std::ostream* Environment::get_output_stream() const
{
   return get_config().output_stream();
}

ARGUMENTUM_INLINE void Environment::exit_parser()
{
   mResult.requestExit();
}

ARGUMENTUM_INLINE std::string Environment::get_option_name() const
{
   return mOption.getHelpName();
}

ARGUMENTUM_INLINE void Environment::add_error( std::string_view error )
{
   if ( error.empty() )
      mResult.addError( get_option_name(), ACTION_ERROR );
   else
      mResult.addError( get_option_name() + ": " + std::string( error ), ACTION_ERROR );
}

ARGUMENTUM_INLINE void Environment::notify_help_was_shown()
{
   mResult.signalHelpShown();
}

}   // namespace argumentum
