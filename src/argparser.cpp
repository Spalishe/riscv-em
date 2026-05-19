#include <cstdlib>
#include <exception>
#include <iostream>
#include <memory>
#include <ostream>
#include <string>
#include <unordered_map>
#include <vector>

namespace arp {
enum positionality {
  pos = true,
  nopos = false,
};

enum requirement {
  required = true,
  norequired = false,
};

class ArgparserArgument {
public:
  ArgparserArgument(const std::string &name, const std::string &description,
                    const requirement required, const positionality positional)
      : m_name(name), m_positional(positional), m_description(description),
        m_required(required) {}
  virtual ~ArgparserArgument() {}
  ArgparserArgument(const ArgparserArgument &) = delete;
  bool defined() { return m_defined; };
  bool required() { return m_required; };
  std::string &getName() { return m_name; };
  std::string &getDescription() { return m_description; };
  virtual int read(std::vector<std::string> args, int start) {
    m_defined = true;
    return 0;
  };
  bool positional() { return m_positional; }
  virtual const char *type() const { return "NONE"; }

  virtual std::string tostring() { return "Base class, no value"; };

protected:
  std::string m_name, m_description;
  bool m_defined = false, m_required = false;
  positionality m_positional;
};

class sint : public ArgparserArgument {
public:
  using ArgparserArgument::ArgparserArgument;

  int read(std::vector<std::string> args, int start) override {
    if (start >= args.size())
      return 0;
    m_defined = true;
    try {
      m_data = std::stoll(args[start]);
    } catch (std::exception ex) {
      std::cerr << "Cant convert argument " << start << " [" << args[start]
                << "] to number" << std::endl;
      exit(-1);
    }
    return 1; // Read 1 argument
  };

  long long val() { return m_data; };
  virtual std::string tostring() override { return std::to_string(m_data); };

  const char *type() const override { return "signed int"; }

private:
  long long m_data;
};

class str : public ArgparserArgument {
public:
  using ArgparserArgument::ArgparserArgument;

  int read(std::vector<std::string> args, int start) override {
    if (start >= args.size())
      return 0;
    m_defined = true;
    m_data = args[start];
    return 1; // Read 1 argument
  };
  virtual std::string tostring() override { return "\"" + m_data + "\""; };

  std::string val() { return m_data; };

  const char *type() const override { return "string"; }

private:
  std::string m_data;
};

class def : public ArgparserArgument {
public:
  using ArgparserArgument::ArgparserArgument;

  int read(std::vector<std::string> args, int start) override {
    m_defined = true;
    return 0; // Read 0 arguments
  };
  virtual std::string tostring() override {
    return m_defined ? "defined" : "undefined";
  };

  bool val() { return m_defined; };

  const char *type() const override { return "definition"; }
};

class Argparser {
public:
  Argparser(int argc, char *argv[]) {
    for (unsigned int c = 0; c < argc; c++) {
      m_args.emplace_back(argv[c]);
    }
  }
  template <typename T>
  std::shared_ptr<T>
  add(const std::string &name, const std::string &description,
      const requirement required, const positionality positional) {
    std::shared_ptr<T> v =
        std::make_shared<T>(name, description, required, positional);

    m_conf.insert_or_assign((*v.get()).getName(), v);

    if (positional) {
      m_conf_pos.emplace_back(v);
    }

    return std::move(v);
  }

  void setDescription(const std::string &desc) { m_desc = desc; }

  void printHelp() {
    std::cout << "Program help:\n"
              << m_args[0] << "\n"
              << m_desc << "\nUsage:\n";
    if (m_conf_pos.size() > 0) {
      std::cout << "Positional arguments:\n";
    }
    for (auto &v : m_conf_pos) {
      std::cout << "\t" << v->getName() << ": " << v->getDescription() << "\n";
    }
    std::cout << "Non-positional arguments:\n";
    for (auto &vp : m_conf) {
      auto &v = vp.second;
      if (v->positional())
        continue;
      std::cout << "\t" << v->getName() << ": " << v->getDescription() << "\n";
    }
    std::cout << "\t--help: show this message" << std::endl;
  }

  void parse() {
    if (m_is_parsed) {
      std::cerr << "Tried parsing already parsed arguments" << std::endl;
      return;
    }

    size_t cur_pos_id = 0;

    // start at 1 to ignore program
    for (size_t i = 1; i < m_args.size();) {
      if (m_args[i] == "--help") {
        printHelp();
        exit(0);
      }
      auto found = m_conf.find(m_args[i]);
      if (found != m_conf.end()) {
        // Non-positional arguments
        auto val = m_conf[m_args[i]];
        if (val->positional())
          continue;
        i++;
        i += val->read(m_args, i);
      } else if (cur_pos_id < m_conf_pos.size()) {
        // Positional arguments
        auto val = m_conf_pos[cur_pos_id++];
        i += val->read(m_args, i);
      } else {
        i++;
      }
    }

    // Validate that requirements are met
    for (auto &vp : m_conf) {
      auto &v = vp.second;
      if (v->required() && !v->defined()) {
        std::cout << "Required argument " << v->getName() << " not defined!"
                  << std::endl;
        printHelp();
        exit(-1);
      }
    }

    m_is_parsed = true;
  };

  friend std::ostream &operator<<(std::ostream &os, const Argparser &li);

private:
  std::unordered_map<std::string, std::shared_ptr<ArgparserArgument>> m_conf;
  std::vector<std::shared_ptr<ArgparserArgument>> m_conf_pos;
  std::string m_desc;

  std::vector<std::string> m_args;
  bool m_is_parsed = false;
};

std::ostream &operator<<(std::ostream &os, const arp::Argparser &ap) {
  os << "Argparser dump:\n";
  size_t i = 0;
  for (auto &arg : ap.m_conf) {
    i++;
    os << i << ": " << arg.second->getName() << " is [" << arg.second->type()
       << "] and is "
       << (arg.second->defined() ? "[defined] " : "[undefined] ");
    if (arg.second->defined()) {
      os << "and equal to [" << arg.second->tostring() << "]";
    }
    os << "\n";
  }
  return os;
};

} // namespace arp
