/*
 * @file
 * @brief
 * Defines a very simple command line argument parser.
 *
 * @copyright
 * Copyright (c) 2017 Viet The Nguyen
 *
 * @copyright
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * @copyright
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * @copyright
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */
#pragma once
#ifndef ARGAGG_ARGAGG_ARGAGG_HPP
#define ARGAGG_ARGAGG_ARGAGG_HPP

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <iterator>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>


/**
 * @brief
 * There are only two hard things in Computer Science: cache invalidation and
 * naming things (Phil Karlton).
 *
 * The names of types have to be succint and clear. This has turned out to be a
 * more difficult thing than I expected. Here you'll find a quick overview of
 * the type names you'll find in this namespace (and thus "library").
 *
 * When a program is invoked it is passed a number of "command line arguments".
 * Each of these "arguments" is a string (C-string to be more precise). An
 * "option" is a command line argument that has special meaning. This library
 * recognizes a command line argument as a potential option if it starts with a
 * dash ('-') or double-dash ('--').
 *
 * A "parser" is a set of "definitions" (not a literal std::set but rather a
 * std::vector). A parser is represented by the argagg::parser struct.
 *
 * A "definition" is a structure with four components that define what
 * "options" are recognized. The four components are the name of the option,
 * the strings that represent the option, the option's help text, and how many
 * arguments the option should expect. "Flags" are the individual strings that
 * represent the option ("-v" and "--verbose" are flags for the "verbose"
 * option). A definition is represented by the argagg::definition struct.
 *
 * Note at this point that the word "option" can be used interchangeably to
 * mean the notion of an option and the actual instance of an option given a
 * set of command line arguments. To be unambiguous we use a "definition" to
 * represent the notion of an option and an "option result" to represent an
 * actual option parsed from a set of command line arguments. An "option
 * result" is represented by the argagg::option_result struct.
 *
 * There's one more wrinkle to this: an option can show up multiple times in a
 * given set of command line arguments. For example, "-n 1 -n 2 -n 3". This
 * will parse into three distinct argagg::option_result instances, but all of
 * them correspond to the same argagg::definition. We aggregate these into the
 * argagg::option_results struct which represents "all parser results for a
 * given option definition". This argagg::option_results is basically a
 * std::vector of argagg::option_result.
 *
 * Options aren't the only thing parsed though. Positional arguments are also
 * parsed. Thus a parser produces a result that contains both option results
 * and positional arguments. The parser results are represented by the
 * argagg::parser_results struct. All option results are stored in a mapping
 * from option name to the argagg::option_results. All positional arguments are
 * simply stored in a vector of C-strings.
 */
namespace argagg {


/**
 * @brief
 * This exception is thrown when a long option is parsed and is given an
 * argument using the "=" syntax but the option doesn't expect an argument.
 */
struct unexpected_argument_error
: public std::invalid_argument {

  /**
   * @brief
   * Explicit constructor which passes "what" string argument to
   * std::invalid_argument constructor.
   */
  explicit unexpected_argument_error(const std::string& what)
  : std::invalid_argument(what)
  {
  }

};


/**
 * @brief
 * This exception is thrown when an option is parsed unexpectedly such as when
 * an argument was expected for a previous option or if an option was found
 * that has not been defined.
 */
struct unexpected_option_error
: public std::invalid_argument {

  /**
   * @brief
   * Explicit constructor which passes "what" string argument to
   * std::invalid_argument constructor.
   */
  explicit unexpected_option_error(const std::string& what)
  : std::invalid_argument(what)
  {
  }

};


/**
 * @brief
 * This exception is thrown when an option requires an argument but is not
 * provided one. This can happen if another flag was found after the option or
 * if we simply reach the end of the command line arguments.
 */
struct option_lacks_argument_error
: public std::invalid_argument {

  /**
   * @brief
   * Explicit constructor which passes "what" string argument to
   * std::invalid_argument constructor.
   */
  explicit option_lacks_argument_error(const std::string& what)
  : std::invalid_argument(what)
  {
  }

};


/**
 * @brief
 * This exception is thrown when an option's flag is invalid. This can be the
 * case if the flag is not prefixed by one or two hyphens or contains non
 * alpha-numeric characters after the hypens. See is_valid_flag_definition()
 * for more details.
 */
struct invalid_flag
: public std::invalid_argument {

  /**
   * @brief
   * Explicit constructor which passes "what" string argument to
   * std::invalid_argument constructor.
   */
  explicit invalid_flag(const std::string& what)
  : std::invalid_argument(what)
  {
  }

};


/**
 * @brief
 * The set of template instantiations that convert C-strings to other types for
 * the option_result::as(), option_results::as(), parser_results::as(), and
 * parser_results::all_as() methods are placed in this namespace.
 */
namespace convert {

  /**
   * @brief
   * Explicit instantiations of this function are used to convert arguments to
   * types.
   */
  template <typename T>
  T arg(const char* arg);

}


/**
 * @brief
 * Represents a single option parse result.
 *
 * You can check if this has an argument by using the implicit boolean
 * conversion.
 */
struct option_result {

  /**
   * @brief
   * Argument parsed for this single option. If no argument was parsed this
   * will be set to nullptr.
   */
  const char* arg;

  /**
   * @brief
   * Converts the argument parsed for this single option instance into the
   * given type using the type matched conversion function
   * argagg::convert::arg(). If there was not an argument parsed for this
   * single option instance then a argagg::option_lacks_argument_error
   * exception is thrown.
   */
  template <typename T>
  T as() const
  {
    if (this->arg) {
      return convert::arg<T>(this->arg);
    } else {
      throw option_lacks_argument_error("option has no argument");
    }
  }

  /**
   * @brief
   * Converts the argument parsed for this single option instance into the
   * given type using the type matched conversion function
   * argagg::convert::arg(). If there was not an argument parsed for this
   * single option instance then the provided default value is returned
   * instead.
   */
  template <typename T>
  T as(const T& t) const
  {
    if (this->arg) {
      return convert::arg<T>(this->arg);
    } else {
      return t;
    }
  }

  /**
   * @brief
   * Since we have the argagg::option_result::as() API we might as well alias
   * it as an implicit conversion operator. This performs implicit conversion
   * using the argagg::option_result::as() method.
   */
  template <typename T>
  operator T () const
  {
    return this->as<T>();
  }

  /**
   * @brief
   * Implicit boolean conversion function which returns true if there is an
   * argument for this single option instance.
   */
  operator bool () const
  {
    return this->arg != nullptr;
  }

};


/**
 * @brief
 * Represents multiple option parse results for a single option. If treated as
 * a single parse result it defaults to the last parse result. Note that an
 * instance of this struct is always created even if no option results are
 * parsed for a given definition. In that case it will simply be empty.
 *
 * To check if the associated option showed up at all simply use the implicit
 * boolean conversion or check if count() is greater than zero.
 */
struct option_results {

  /**
   * @brief
   * All option parse results for this option.
   */
  std::vector<option_result> all;

  /**
   * @brief
   * Gets the number of times the option shows up.
   */
  std::size_t count() const
  {
    return this->all.size();
  }

  /**
   * @brief
   * Gets a single option parse result by index.
   */
  option_result& operator [] (std::size_t index)
  {
    return this->all[index];
  }

  /**
   * @brief
   * Gets a single option result by index.
   */
  const option_result& operator [] (std::size_t index) const
  {
    return this->all[index];
  }

  /**
   * @brief
   * Converts the argument parsed for the LAST option parse result for the
   * parent definition to the provided type. For example, if this was for "-f 1
   * -f 2 -f 3" then calling this method for an integer type will return 3. If
   * there are no option parse results then a std::out_of_range exception is
   * thrown. Any exceptions thrown by option_result::as() are not
   * handled.
   */
  template <typename T>
  T as() const
  {
    if (this->all.size() == 0) {
      throw std::out_of_range("no option arguments to convert");
    }
    return this->all.back().as<T>();
  }

  /**
   * @brief
   * Converts the argument parsed for the LAST option parse result for the
   * parent definition to the provided type. For example, if this was for "-f 1
   * -f 2 -f 3" then calling this method for an integer type will return 3. If
   * there are no option parse results then the provided default value is
   * returned instead.
   */
  template <typename T>
  T as(const T& t) const
  {
    if (this->all.size() == 0) {
      return t;
    }
    return this->all.back().as<T>(t);
  }

  /**
   * @brief
   * Since we have the option_results::as() API we might as well alias
   * it as an implicit conversion operator. This performs implicit conversion
   * using the option_results::as() method.
   */
  template <typename T>
  operator T () const
  {
    return this->as<T>();
  }

  /**
   * @brief
   * Implicit boolean conversion function which returns true if there is at
   * least one parser result for this definition.
   */
  operator bool () const
  {
    return this->all.size() > 0;
  }

};


/**
 * @brief
 * Represents all results of the parser including options and positional
 * arguments.
 */
struct parser_results {

  /**
   * @brief
   * Returns the name of the program from the original arguments list. This is
   * always the first argument.
   */
  const char* program;

  /**
   * @brief
   * Maps from definition name to the structure which contains the parser
   * results for that definition.
   */
  std::unordered_map<std::string, option_results> options;

  /**
   * @brief
   * Vector of positional arguments.
   */
  std::vector<const char*> pos;

  /**
   * @brief
   * Used to check if an option was specified at all.
   */
  bool has_option(const std::string& name) const
  {
    const auto it = this->options.find(name);
    return ( it != this->options.end()) && it->second.all.size() > 0;
  }

  /**
   * @brief
   * Get the parser results for the given definition. If the definition never
   * showed up then the exception from the unordered_map access will bubble
   * through so check if the flag exists in the first place with has_option().
   */
  option_results& operator [] (const std::string& name)
  {
    return this->options.at(name);
  }

  /**
   * @brief
   * Get the parser results for the given definition. If the definition never
   * showed up then the exception from the unordered_map access will bubble
   * through so check if the flag exists in the first place with has_option().
   */
  const option_results& operator [] (const std::string& name) const
  {
    return this->options.at(name);
  }

  /**
   * @brief
   * Gets the number of positional arguments.
   */
  std::size_t count() const
  {
    return this->pos.size();
  }

  /**
   * @brief
   * Gets a positional argument by index.
   */
  const char* operator [] (std::size_t index) const
  {
    return this->pos[index];
  }

  /**
   * @brief
   * Gets a positional argument converted to the given type.
   */
  template <typename T>
  T as(std::size_t i = 0) const
  {
    return convert::arg<T>(this->pos[i]);
  }

  /**
   * @brief
   * Gets all positional arguments converted to the given type.
   */
  template <typename T>
  std::vector<T> all_as() const
  {
    std::vector<T> v(this->pos.size());
    std::transform(
      this->pos.begin(), this->pos.end(), v.begin(),
      [](const char* arg) {
        return convert::arg<T>(arg);
      });
    return v;
  }

};


/**
 * @brief
 * An option definition which essentially represents what an option is.
 */
struct definition {

  /**
   * @brief
   * Name of the option. Option parser results are keyed by this name.
   */
  const std::string name;

  /**
   * @brief
   * List of strings to match that correspond to this option. Should be fully
   * specified with hyphens (e.g. "-v" or "--verbose").
   */
  std::vector<const char*> flags;

  /**
   * @brief
   * Help string for this option.
   */
  const char* help;

  /**
   * @brief
   * Number of arguments this option requires. Must be 0 or 1. All other values
   * have undefined behavior. Okay, the code actually works with positive
   * values in general, but it's unorthodox command line behavior.
   */
  unsigned int num_args;

  /**
   * @brief
   * Returns true if this option does not want any arguments.
   */
  bool wants_no_arguments() const
  {
    return this->num_args == 0;
  }

  /**
   * @brief
   * Returns true if this option requires arguments.
   */
  bool requires_arguments() const
  {
    return this->num_args > 0;
  }

};


/**
 * @brief
 * Checks whether or not a command line argument should be processed as an
 * option flag. This is very similar to is_valid_flag_definition() but must
 * allow for short flag groups (e.g. "-abc") and equal-assigned long flag
 * arguments (e.g. "--output=foo.txt").
 */
bool cmd_line_arg_is_option_flag(
  const char* s)
{
  auto len = std::strlen(s);

  // The shortest possible flag has two characters: a hyphen and an
  // alpha-numeric character.
  if (len < 2) {
    return false;
  }

  // All flags must start with a hyphen.
  if (s[0] != '-') {
    return false;
  }

  // Shift the name forward by a character to account for the initial hyphen.
  // This means if s was originally "-v" then name will be "v".
  const char* name = s + 1;

  // Check if we're dealing with a long flag.
  bool is_long = false;
  if (s[1] == '-') {
    is_long = true;

    // Just -- is not a valid flag.
    if (len == 2) {
      return false;
    }

    // Shift the name forward to account for the extra hyphen. This means if s
    // was originally "--output" then name will be "output".
    name = s + 2;
  }

  // The first character of the flag name must be alpha-numeric. This is to
  // prevent things like "---a" from being valid flags.
  len = std::strlen(name);
  if (!std::isalnum(name[0])) {
    return false;
  }

  // At this point in is_valid_flag_definition() we would check if the short
  // flag has only one character. At command line specification you can group
  // short flags together or even add an argument to a short flag without a
  // space delimiter. Thus we don't check if this has only one character
  // because it might not.

  // If this is a long flag then we expect all characters *up to* an equal sign
  // to be alpha-numeric or a hyphen. After the equal sign you are specify the
  // argument to a long flag which can be basically anything.
  if (is_long) {
    bool encountered_equal = false;
    return std::all_of(name, name + len, [&](const char& c) {
        if (encountered_equal) {
          return true;
        } else {
          if (c == '=') {
            encountered_equal = true;
            return true;
          }
          return std::isalnum(c) || c == '-';
        }
      });
  }

  // At this point we are not dealing with a long flag. We already checked that
  // the first character is alpha-numeric so we've got the case of a single
  // short flag covered. This might be a short flag group though and we might
  // be tempted to check that each character of the short flag group is
  // alpha-numeric. However, you can specify the argument for a short flag
  // without a space delimiter (e.g. "-I/usr/local/include") so you can't tell
  // if the rest of a short flag group is part of the argument or not unless
  // you know what is a defined flag or not. We leave that kind of processing
  // to the parser.
  return true;
}


/**
 * @brief
 * Checks whether a flag in an option definition is valid. I suggest reading
 * through the function source to understand what dictates a valid.
 */
bool is_valid_flag_definition(
  const char* s)
{
  auto len = std::strlen(s);

  // The shortest possible flag has two characters: a hyphen and an
  // alpha-numeric character.
  if (len < 2) {
    return false;
  }

  // All flags must start with a hyphen.
  if (s[0] != '-') {
    return false;
  }

  // Shift the name forward by a character to account for the initial hyphen.
  // This means if s was originally "-v" then name will be "v".
  const char* name = s + 1;

  // Check if we're dealing with a long flag.
  bool is_long = false;
  if (s[1] == '-') {
    is_long = true;

    // Just -- is not a valid flag.
    if (len == 2) {
      return false;
    }

    // Shift the name forward to account for the extra hyphen. This means if s
    // was originally "--output" then name will be "output".
    name = s + 2;
  }

  // The first character of the flag name must be alpha-numeric. This is to
  // prevent things like "---a" from being valid flags.
  len = std::strlen(name);
  if (!std::isalnum(name[0])) {
    return false;
  }

  // If this is a short flag then it must only have one character.
  if (!is_long && len > 1) {
    return false;
  }

  // The rest of the characters must be alpha-numeric, but long flags are
  // allowed to have hyphens too.
  return std::all_of(name + 1, name + len, [&](const char& c) {
      return std::isalnum(c) || (c == '-' && is_long);
    });
}


/**
 * @brief
 * Tests whether or not a valid flag is short. Assumes the provided cstring is
 * already a valid flag.
 */
bool flag_is_short(
  const char* s)
{
  return s[0] == '-' && std::isalnum(s[1]);
}


/**
 * @brief
 * A list of option definitions used to inform how to parse arguments.
 */
struct parser {

  /**
   * @brief
   * Vector of the option definitions which inform this parser how to parse
   * the command line arguments.
   */
  std::vector<definition> definitions;

  /**
   * @brief
   * Parses the provided command line arguments and returns the results as
   * @ref parser_results.
   *
   * @note
   * This method is not thread-safe and assumes that no modifications are made
   * to the definitions member field during the extent of this method call.
   */
  parser_results parse(int argc, const char** argv)
  {
    // Initialize the parser results that we'll be returning. Store the program
    // name (assumed to be the first command line argument) and initialize
    // everything else as empty.
    std::unordered_map<std::string, option_results> options {};
    parser_results results {argv[0], std::move(options), {}};

    // Inspect each definition to see if its valid. You may wonder "why don't
    // you do this validation on construction?" I had thought about it but
    // realized that since I've made the parser an aggregate type (granted it
    // just "aggregates" a single vector) I would need to track any changes to
    // the definitions vector and re-run the validity check in order to
    // maintain this expected "validity invariant" on the object. That would
    // then require hiding the definitions vector as a private entry and then
    // turning the parser into a thin interface (by re-exposing setters and
    // getters) to the vector methods just so that I can catch when the
    // definition has been modified. It seems much simpler to just enforce the
    // validity when you actually want to parser because it's at the moment of
    // parsing that you know the definitions are complete.
    std::array<definition*, 256> short_map;
    std::fill(short_map.begin(), short_map.end(), nullptr);
    std::unordered_map<std::string, definition*> long_map {};
    for (auto& defn : this->definitions) {

      if (defn.flags.size() == 0) {
        std::ostringstream msg;
        msg << "option \"" << defn.name << "\" has no flag definitions";
        throw invalid_flag(msg.str());
      }

      for (auto& flag : defn.flags) {

        if (!is_valid_flag_definition(flag)) {
          std::ostringstream msg;
          msg << "flag \"" << flag << "\" specified for option \"" << defn.name
              << "\" is invalid";
          throw invalid_flag(msg.str());
        }

        if (flag_is_short(flag)) {
          const int short_flag_letter = flag[1];
          const auto existing_short_flag = short_map[short_flag_letter];
          bool short_flag_already_exists = (existing_short_flag != nullptr);
          if (short_flag_already_exists) {
            std::ostringstream msg;
            msg << "duplicate short flag \"" << flag
                << "\" found, specified by both option  \"" << defn.name
                << "\" and option \"" << existing_short_flag->name;
            throw invalid_flag(msg.str());
          }
          short_map[short_flag_letter] = &defn;
          continue;
        }

        // If we're here then this is a valid, long-style flag.
        const auto existing_long_flag = long_map.find(flag);
        bool long_flag_exists = (existing_long_flag != long_map.end());
        if (long_flag_exists) {
          std::ostringstream msg;
          msg << "duplicate long flag \"" << existing_long_flag->first
              << "\" found, specified by both option  \"" << defn.name
              << "\" and option \"" << existing_long_flag->second->name;
          throw invalid_flag(msg.str());
        }
        long_map.insert(std::make_pair(flag, &defn));
      }

      // Add an empty option result for each definition.
      option_results opt_results {{}};
      results.options.insert(
        std::move(std::make_pair(defn.name, opt_results)));
    }

    auto known_short_flag = [&](const char flag) {
        return short_map[flag] != nullptr;
      };

    auto get_definition_for_short_flag = [&](const char flag) {
        return short_map[flag];
      };


    // Don't start off ignoring flags. We only ignore flags after a -- shows up
    // in the command line arguments.
    bool ignore_flags = false;

    // Keep track of any options that are expecting arguments.
    const char* last_flag_expecting_args = nullptr;
    option_result* last_option_expecting_args = nullptr;
    unsigned int num_option_args_to_consume = 0;

    // Get pointers to pointers so we can treat the raw pointer array as an
    // iterator for standard library algorithms. This isn't used yet but can be
    // used to template this function to work on iterators over strings or
    // C-strings.
    const char** arg_i = argv + 1;
    const char** arg_end = argv + argc;

    while (arg_i != arg_end) {
      auto arg_i_cstr = *arg_i;
      auto arg_i_len = std::strlen(arg_i_cstr);

      // Some behavior to note: if the previous option is expecting an argument
      // then the next entry will be treated as a positional argument even if
      // it looks like a flag.
      bool treat_as_positional_argument = (
          ignore_flags
          || num_option_args_to_consume > 0
          || !cmd_line_arg_is_option_flag(arg_i_cstr)
        );
      if (treat_as_positional_argument) {

        // If last option is expecting some specific positive number of
        // arguments then give this argument to that option, *regardless of
        // whether or not the argument looks like a flag or is the special "--"
        // argument*.
        if (num_option_args_to_consume > 0) {
          last_option_expecting_args->arg = arg_i_cstr;
          --num_option_args_to_consume;
          ++arg_i;
          continue;
        }

        // Now we check if this is just "--" which is a special argument that
        // causes all following arguments to be treated as non-options and is
        // itselve discarded.
        if (std::strncmp(arg_i_cstr, "--", 2) == 0 && arg_i_len == 2) {
          ignore_flags = true;
          ++arg_i;
          continue;
        }

        // If there are no expectations for option arguments then simply use
        // this argument as a positional argument.
        results.pos.push_back(arg_i_cstr);
        ++arg_i;
        continue;
      }

      // Reset the "expecting argument" state.
      last_flag_expecting_args = nullptr;
      last_option_expecting_args = nullptr;
      num_option_args_to_consume = 0;

      // If we're at this point then we're definitely dealing with something
      // that is flag-like and has hyphen as the first character and has a
      // length of at least two characters. How we handle this potential flag
      // depends on whether or not it is a long-option so we check that first.
      bool is_long_flag = (arg_i_cstr[1] == '-');

      if (is_long_flag) {

        // Long flags have a complication: their arguments can be specified
        // using an '=' character right inside the argument. That means an
        // argument like "--output=foobar.txt" is actually an option with flag
        // "--output" and argument "foobar.txt".
        auto long_flag_arg = std::strchr(arg_i_cstr, '=');
        std::size_t flag_len = arg_i_len;
        if (long_flag_arg != nullptr) {
          flag_len = long_flag_arg - arg_i_cstr;
        }
        std::string long_flag_str(arg_i_cstr, flag_len);

        const auto existing_long_flag = long_map.find(long_flag_str);
        bool long_flag_exists = (existing_long_flag != long_map.end());
        if (!long_flag_exists) {
          std::ostringstream msg;
          msg << "found unexpected flag: " << long_flag_str;
          throw unexpected_option_error(msg.str());
        }

        auto defn = existing_long_flag->second;

        if (long_flag_arg != nullptr && defn->num_args == 0) {
          std::ostringstream msg;
          msg << "found argument for option not expecting an argument: "
              << arg_i_cstr;
          throw unexpected_argument_error(msg.str());
        }

        auto& opt_results = results.options[defn->name];
        option_result opt_result {nullptr};
        opt_results.all.push_back(std::move(opt_result));

        if (defn->requires_arguments()) {
          bool there_is_an_equal_delimited_arg = (long_flag_arg != nullptr);
          if (there_is_an_equal_delimited_arg) {
            // long_flag_arg would be "=foo" in the "--output=foo" case so we
            // increment by 1 to get rid of the equal sign.
            opt_results.all.back().arg = long_flag_arg + 1;
          } else {
            last_flag_expecting_args = arg_i_cstr;
            last_option_expecting_args = &(opt_results.all.back());
            num_option_args_to_consume = defn->num_args;
          }
        }

        ++arg_i;
        continue;
      }

      // If we've made it here then we're looking at either a short flag or a
      // group of short flags. Short flags can be grouped together so long as
      // they don't require any arguments unless the option that does is the
      // last in the group ("-o x -v" is okay, "-vo x" is okay, "-ov x" is
      // not). So starting after the dash we're going to process each character
      // as if it were a separate flag. Note "sf_idx" stands for "short flag
      // index".
      for (std::size_t sf_idx = 1; sf_idx < arg_i_len; ++sf_idx) {
        const auto short_flag = arg_i_cstr[sf_idx];

        if (!std::isalnum(short_flag)) {
          std::ostringstream msg;
          msg << "found non-alphanumeric character '" << arg_i_cstr[sf_idx]
              << "' in flag group '" << arg_i_cstr << "'";
          throw std::domain_error(msg.str());
        }

        if (!known_short_flag(short_flag)) {
          std::ostringstream msg;
          msg << "found unexpected flag '" << arg_i_cstr[sf_idx]
              << "' in flag group '" << arg_i_cstr << "'";
          throw unexpected_option_error(msg.str());
        }

        auto defn = get_definition_for_short_flag(short_flag);
        auto& opt_results = results.options[defn->name];

        // Create an option result with an empty argument (for now) and add it
        // to this option's results.
        option_result opt_result {nullptr};
        opt_results.all.push_back(std::move(opt_result));

        if (defn->requires_arguments()) {

          // If this short flag's option requires an argument and we're the
          // last flag in the short flag group then just put the parser into
          // "expecting argument for last option" state and move onto the next
          // command line argument.
          bool is_last_short_flag_in_group = (sf_idx == arg_i_len - 1);
          if (is_last_short_flag_in_group) {
            last_flag_expecting_args = arg_i_cstr;
            last_option_expecting_args = &(opt_results.all.back());
            num_option_args_to_consume = defn->num_args;
            break;
          }

          // If this short flag's option requires and argument and we're NOT
          // the last flag in the short flag group then we automatically
          // consume the rest of the short flag group as the argument for this
          // flag. This is how we get the POSIX behavior of being able to
          // specify a flag's arguments without a white space delimiter (e.g.
          // "-I/usr/local/include").
          opt_results.all.back().arg = arg_i_cstr + sf_idx + 1;
          break;
        }
      }

      ++arg_i;
      continue;
    }

    // If we're done with all of the arguments but are still expecting
    // arguments for a previous option then we haven't satisfied that option.
    // This is an error.
    if (num_option_args_to_consume > 0) {
      std::ostringstream msg;
      msg << "last option \"" << last_flag_expecting_args
          << "\" expects an argument but the parser ran out of command line "
          << "arguments to parse";
      throw option_lacks_argument_error(msg.str());
    }

    return results;
  }

};


namespace convert {


  /**
   * @brief
   * Templated function for conversion to T using the std::strtol() function.
   * This is used for anything long length or shorter (long, int, short, char).
   */
  template <typename T>
  T long_(const char* arg)
  {
    char* garbage = nullptr;
    errno = 0;
    T ret = static_cast<T>(std::strtol(arg, &garbage, 0));
    if (errno == ERANGE) {
      throw std::out_of_range("argument numeric value out of range");
    }
    return ret;
  }


  /**
   * @brief
   * Templated function for conversion to T using the std::strtoll() function.
   * This is used for anything long long length or shorter (long long).
   */
  template <typename T>
  T long_long_(const char* arg)
  {
    char* garbage = nullptr;
    errno = 0;
    T ret = static_cast<T>(std::strtoll(arg, &garbage, 0));
    if (errno == ERANGE) {
      throw std::out_of_range("argument numeric value out of range");
    }
    return ret;
  }


#define DEFINE_CONVERSION_FROM_LONG_(TYPE) \
  template <> \
  TYPE arg(const char* arg) \
  { \
    return long_<TYPE>(arg); \
  }

  DEFINE_CONVERSION_FROM_LONG_(char)
  DEFINE_CONVERSION_FROM_LONG_(unsigned char)
  DEFINE_CONVERSION_FROM_LONG_(signed char)
  DEFINE_CONVERSION_FROM_LONG_(short)
  DEFINE_CONVERSION_FROM_LONG_(unsigned short)
  DEFINE_CONVERSION_FROM_LONG_(int)
  DEFINE_CONVERSION_FROM_LONG_(unsigned int)
  DEFINE_CONVERSION_FROM_LONG_(long)
  DEFINE_CONVERSION_FROM_LONG_(unsigned long)

#undef DEFINE_CONVERSION_FROM_LONG_


#define DEFINE_CONVERSION_FROM_LONG_LONG_(TYPE) \
  template <> \
  TYPE arg(const char* arg) \
  { \
    return long_long_<TYPE>(arg); \
  }

  DEFINE_CONVERSION_FROM_LONG_LONG_(long long)
  DEFINE_CONVERSION_FROM_LONG_LONG_(unsigned long long)

#undef DEFINE_CONVERSION_FROM_LONG_LONG_


  template <>
  float arg(const char* arg)
  {
    char* garbage = nullptr;
    errno = 0;
    float ret = std::strtof(arg, &garbage);
    if (errno == ERANGE) {
      throw std::out_of_range("argument numeric value out of range");
    }
    return ret;
  }


  template <>
  double arg(const char* arg)
  {
    char* garbage = nullptr;
    errno = 0;
    double ret = std::strtod(arg, &garbage);
    if (errno == ERANGE) {
      throw std::out_of_range("argument numeric value out of range");
    }
    return ret;
  }


  template <>
  const char* arg(const char* arg)
  {
    return arg;
  }


  template <>
  std::string arg(const char* arg)
  {
    return std::string(arg);
  }

}


} // namespace argagg


/**
 * @brief
 * Writes the option help to the given stream.
 */
std::ostream& operator << (std::ostream& os, const argagg::parser& x)
{
  for (auto& definition : x.definitions) {
    os << "    ";
    for (auto& flag : definition.flags) {
      os << flag;
      if (flag != definition.flags.back()) {
        os << ", ";
      }
    }
    os << std::endl;
    os << "        " << definition.help << std::endl;
  }
  return os;
}


#endif // ARGAGG_ARGAGG_ARGAGG_HPP
