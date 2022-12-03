#pragma once

#include <string>
#include <map>
#include <utility>
#include <vector>
#include <optional>
#include <sstream>

namespace ArgumentParser {
    class argument_parser_exception : public std::exception {
     private:
        std::string message_;
     public:
        explicit argument_parser_exception(std::string message) : message_(std::move(message)) {}
        [[nodiscard]] const char* what() const noexcept override {
            return message_.c_str();
        }
    };

    class parse_exception : public argument_parser_exception {
     public:
        explicit parse_exception(std::string message) : argument_parser_exception(std::move(message)) {}
    };

    class settings_exception : public argument_parser_exception {
     public:
        explicit settings_exception(std::string message) : argument_parser_exception(std::move(message)) {}
    };

    enum StoreType {
        kIntArgument, kStringArgument, kFlagArgument
    };

    template<typename T>
    class Argument {
     public:
        std::vector<T>* values_ = new std::vector<T>;
        T* value_ = new T;
        std::optional<T> default_value_ = std::nullopt;
        std::optional<std::vector<T>> default_values_ = std::nullopt;

        uint64_t min_number_of_values_ = 1;
        uint64_t number_of_values_ = 0;
        bool positional_ = false;
        bool multi_value_ = false;
        bool stored_ = false;

        ~Argument() {
            if (stored_) {
                if (multi_value_) {
                    delete value_;
                } else {
                    delete values_;
                }
            } else {
                delete value_;
                delete values_;
            }
        }

        Argument<T>& Positional() {
            positional_ = true;
            return *this;
        }

        Argument<T>& Default(const T& default_value) {
            if (multi_value_) {
                throw settings_exception("You can't store single value in multi-value argument");
            }
            default_value_ = default_value;
            return *this;
        }

        Argument<T>& Default(const std::vector<T>& default_value) {
            if (!multi_value_) {
                throw settings_exception("You can't store multi-value value in single-value argument");
            }
            default_values_ = default_value;
            return *this;
        }

        Argument<T>& MultiValue(uint64_t min_number_of_values = 0) {
            min_number_of_values_ = min_number_of_values;
            multi_value_ = true;
            return *this;
        }

        Argument<T>& StoreValue(T& value) {
            if (multi_value_) {
                throw settings_exception("You can't store single value in multi-value argument");
            }
            delete value_;
            value_ = &value;
            stored_ = true;
            return *this;
        }

        Argument<T>& StoreValues(std::vector<T>& values) {
            if (!multi_value_) {
                throw settings_exception("You can't store multi-value value in single-value argument");
            }
            delete values_;
            values_ = &values;
            stored_ = true;
            return *this;
        }

        void SetValue(const T& value) {
            number_of_values_++;
            if (multi_value_) {
                values_->push_back(value);
            } else {
                *value_ = value;
            }
        }

        bool IsCorrect() {
            return number_of_values_ >= min_number_of_values_
                || (number_of_values_ == 0 && (default_value_ != std::nullopt || default_values_ != std::nullopt));
        }
    };

    class Flag {
     public:
        bool* value_ = new bool{false};
        bool stored_ = false;

        ~Flag() {
            if (!stored_) {
                delete value_;
            }
        }

        Flag& Default(bool default_value) {
            *this->value_ = default_value;
            return *this;
        }

        Flag& StoreValue(bool& store_value) {
            delete this->value_;
            this->value_ = &store_value;
            stored_ = true;
            return *this;
        }
    };

    struct Key {
        std::string short_key;
        std::string long_key;
        std::string description;

        StoreType type;

        static std::string Concat(const Key& key) {
            std::stringstream result;
            if (!key.short_key.empty()) {
                result << '-' << key.short_key;
            }
            if (!key.long_key.empty()) {
                result << "\t--" << key.long_key;
            }
            if (!key.description.empty()) {
                result << '\t' << key.description;
            }
            return result.str();
        }
    };

    class ArgParser {
        // Your Implementation here!
     private:

        std::string name_;

        std::optional<Key> help_{std::nullopt};
        bool found_help_{false};

        std::map<std::string, Key*> keys_;
        std::map<std::string, Argument<int>*> int_arguments_;
        std::map<std::string, Argument<std::string>*> string_arguments_;
        std::map<std::string, Flag*> flags_;

        [[nodiscard]] bool CheckCorrectness() const;

        template<typename K>
        void ClearMap(std::map<std::string, K*>& some_map);

        void UpdatePositionalArgument(const std::string& value);

        void UpdateShortFlags(const std::string& raw_key);

        void UpdateFlag(const std::string& raw_key);

        void SetArgument(const std::string& argument_name, const std::string& value);

        void UpdateArgument(const std::string& equation);

        void UpdateArgument(const std::string& raw_argument_name, const std::string& value);
     public:
        explicit ArgParser(std::string name) : name_(std::move(name)) {};

        ~ArgParser();

        bool Parse(const std::vector<std::string>& data);

        bool Parse(int argc, char** argv);

        Flag& AddFlag(char short_flag, const std::string& long_flag);

        Flag& AddFlag(const std::string& long_flag, const std::string& description);

        Flag& AddFlag(char short_flag, const std::string& long_flag, const std::string& description);

        void SetFlag(const std::string& flag);

        bool GetFlag(const std::string& flag);

        Argument<int>& AddIntArgument(const std::string& long_key);

        Argument<int>& AddIntArgument(char short_key, const std::string& long_key);

        Argument<int>& AddIntArgument(const std::string& long_key, const std::string& description);

        Argument<std::string>& AddStringArgument(const std::string& long_key);

        Argument<std::string>& AddStringArgument(char short_key, const std::string& long_key);

        Argument<std::string>& AddStringArgument(char short_key,
                                                 const std::string& long_key,
                                                 const std::string& description);

        int GetIntValue(const std::string& key, int index = 0);

        std::string GetStringValue(const std::string& key, int index = 0);

        void AddHelp(char short_key, const std::string& long_key, const std::string& description);

        [[nodiscard]] bool Help() const;

        std::string HelpDescription();
    };

} // namespace ArgumentParser