#include <argagg/argagg.hpp>

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>

int main(
  int argc,
  const char** argv)
{
  using argagg::parser_results;
  using argagg::parser;
  using std::cerr;
  using std::cout;
  using std::endl;
  using std::ofstream;
  using std::ostream;
  using std::ostringstream;
  using std::string;

  // Use an initializer list to define the argument parser. The first brace
  // starts the initializer list, the second brace starts the initializer list
  // for the `specs` vector in the `argagg::parser` struct.
  parser argparser {{
      // Each entry here is an initializer list for an `argagg::definition`
      // struct. The struct entities are the name of the flag, a vector/list of
      // strings that when matched will match this flag, the help string, and
      // the number of arguments this flag needs.
      {
        "help", {"-h", "--help"},
        "displays help information", 0},
      {
        "verbose", {"-v", "--verbose"},
        "increases verbosity", 0},
      {
        "sep", {"-s", "--sep"},
        "separator (default ',')", argagg::optional},
      {
        "output", {"-o", "--output"},
        "output filename (stdout if not specified)", 1},
    }};

  // Define our usage text.
  ostringstream usage;
  usage
    << "Joins all positional arguments together with a separator" << endl
    << endl
    << "Usage: " << argv[0] << " [options] ARG [ARG...]" << endl
    << endl;

  // Use our argument parser to... parse the command line arguments. If there
  // are any problems then just spit out the usage and help text and exit.
  argagg::parser_results args;
  try {
    args = argparser.parse(argc, argv);
  } catch (const std::exception& e) {
    cerr << usage.str() << argparser << endl
         << "Encountered exception while parsing arguments: " << e.what()
         << endl;
    return EXIT_FAILURE;
  }

  // If the help flag was specified then spit out the usage and help text and
  // exit.
  if (args["help"]) {
    cerr << usage.str() << argparser;
    return EXIT_SUCCESS;
  }

  // Respect verbosity. Okay, the logging here is a little ludicrous. The point
  // I want to show here is that you can quickly get the number of times an
  // option shows up.
  int verbose_level = args["verbose"].count();

  // Set up our verbose log output stream selector that selects stderr if the
  // requested log level is lower than or equal to the currently set verbose
  // level.
  ofstream dev_null;
  dev_null.open("/dev/null"); // portable? eh... simple? yes!
  auto vlog = [&](int level) -> ostream& {
      return verbose_level >= level ? cerr : dev_null;
    };

  vlog(1) << "verbose log level: " << verbose_level << endl;

  // Use comma as the separator unless one was specified.
  auto sep = args["sep"].as<string>(",");
  vlog(1) << "set separator to '" << sep << "'" << endl;

  // Determine output stream.
  ofstream output_file;
  ostream* output = &std::cout;
  if (args["output"]) {
    string filename = args["output"];
    output_file.open(filename);
    output = &output_file;
    vlog(1) << "outputting to file at '" << filename << "'" << endl;
  } else {
    vlog(1) << "outputting to stdout" << endl;
  }

  // Join the arguments.
  if (args.count() < 1) {
    vlog(0) << usage.str() << argparser << endl
            << "Not enough arguments" << endl;
    return EXIT_FAILURE;
  }
  for (auto& arg : args.pos) {
    vlog(2) << "writing argument" << endl;
    vlog(4) << "argument is '" << arg << "'" << endl;
    *output << arg;
    if (arg != args.pos.back()) {
      vlog(3) << "writing separator" << endl;
      *output << sep;
    }
  }
  vlog(4) << "writing endl" << endl;
  *output << endl;

  vlog(4) << "everything a-okay" << endl;
  return EXIT_SUCCESS;  
}
