#include "ArgParser.h"

namespace ArgumentParser {
    ArgParser::~ArgParser() {
        ClearMap(int_arguments_);
        ClearMap(string_arguments_);
        ClearMap(flags_);
        ClearMap(keys_);
    }

    template<typename K>
    void ArgParser::ClearMap(std::map<std::string, K*>& some_map) {
        for (const auto& item : some_map) {
            if (item.second != nullptr) {
                K* tmp = item.second;
                auto* key = keys_.at(item.first);
                if (!key->short_key.empty()) {
                    some_map.at(key->short_key) = nullptr;
                }
                if (!key->long_key.empty()) {
                    some_map.at(key->long_key) = nullptr;
                }
                delete tmp;
            }
        }
    }

    bool ArgParser::CheckCorrectness() const {
        auto check_correctness = []<typename K>(const std::map<std::string, K*>& arguments) {
            for (const auto& argument: arguments) {
                if (!argument.second->IsCorrect()) {
                    return false;
                }
            }
            return true;
        };
        return check_correctness(int_arguments_) && check_correctness(string_arguments_);
    }

    void ArgParser::UpdatePositionalArgument(const std::string& value) {
        for (const auto& key : keys_) {
            if (key.second->type == StoreType::kIntArgument) {
                Argument<int>* argument = int_arguments_[key.first];
                if (argument->positional_) {
                    argument->SetValue(std::stoi(value));
                }
            } else if (key.second->type == StoreType::kStringArgument) {
                Argument<std::string>* argument = string_arguments_[key.first];
                if (argument->positional_) {
                    argument->SetValue(value);
                }
            }
        }
    }

    void ArgParser::UpdateShortFlags(const std::string& raw_key) {
        for (int i = 1; i < raw_key.length(); i++) {
            if (std::string{raw_key[i]} == help_->short_key) {
                found_help_ = true;
            } else {
                *flags_[{raw_key[i]}]->value_ = true;
            }
        }
    }

    void ArgParser::UpdateFlag(const std::string& raw_key) {
        std::string long_key = raw_key.substr(2);
        if (help_->long_key == long_key) {
            found_help_ = true;
        } else {
            *flags_[raw_key.substr(2)]->value_ = true;
        }
    }

    void ArgParser::SetArgument(const std::string& argument_name, const std::string& value) {
        try {
            Key* key = keys_.at(argument_name);
            if (key->type == StoreType::kIntArgument) {
                int_arguments_[argument_name]->SetValue(std::stoi(value));
            } else {
                string_arguments_[argument_name]->SetValue(value);
            }
        } catch (const std::exception& exception) {
            throw parse_exception("There's no such argument as [" + argument_name + "]");
        }
    }

    void ArgParser::UpdateArgument(const std::string& equation) {
        SetArgument(
            {equation.begin() + (equation[1] == '-') + 1, equation.begin() + equation.find('=')},
            {equation.begin() + equation.find('=') + 1, equation.end()}
        );
    }

    void ArgParser::UpdateArgument(const std::string& raw_argument_name, const std::string& value) {
        SetArgument(
            raw_argument_name.substr(1 + (raw_argument_name[1] == '-')),
            value
        );
    }

    bool ArgParser::Parse(const std::vector<std::string>& data) {
        for (int i = 1; i < data.size(); i++) {
            if (data[i][0] == '-') {
                if (data[i].find('=') != std::string::npos) {
                    UpdateArgument(data[i]);
                } else if (data[i][1] != '-') {
                    if (data[i].length() > 2) {
                        UpdateShortFlags(data[i]);
                    } else {
                        switch (keys_.at(data[i].substr(1))->type) {
                            case kIntArgument:
                            case kStringArgument:
                                if (i == data.size() - 1) {
                                    throw parse_exception("Not enough values");
                                }
                                UpdateArgument(data[i], data[i + 1]);
                                i++;
                                break;
                            case kFlagArgument:UpdateShortFlags(data[i]);
                                break;
                        }
                    }
                } else {
                    if (data[i].substr(2) == help_->long_key) {
                        found_help_ = true;
                        continue;
                    }
                    switch (keys_.at(data[i].substr(2))->type) {
                        case kIntArgument:
                        case kStringArgument:
                            if (i == data.size() - 1) {
                                throw parse_exception("Not enough values");
                            }
                            UpdateArgument(data[i], data[i + 1]);
                            i++;
                            break;
                        case kFlagArgument:
                            UpdateFlag(data[i]);
                            break;
                    }
                }
            } else {
                UpdatePositionalArgument(data[i]);
            }
        }

        return CheckCorrectness();
    }

    bool ArgParser::Parse(int argc, char** argv) {
        std::vector<std::string> data(argc);
        for (int i = 0; i < argc; i++) {
            data[i] = std::string(argv[i]);
        }

        return Parse(data);
    }

    Flag& ArgParser::AddFlag(char short_flag, const std::string& long_flag) {
        auto* new_flag = new Flag;
        std::string short_flag_string{short_flag};
        auto* key = new Key{short_flag_string, long_flag, "", StoreType::kFlagArgument};
        keys_[short_flag_string] = keys_[long_flag] = key;
        flags_[short_flag_string] = flags_[long_flag] = new_flag;

        return *new_flag;
    }

    Flag& ArgParser::AddFlag(const std::string& long_flag, const std::string& description) {
        auto* new_flag = new Flag;
        auto* key = new Key{"", long_flag, description, StoreType::kFlagArgument};
        keys_[long_flag] = key;
        flags_[long_flag] = new_flag;

        return *new_flag;
    }

    Flag& ArgParser::AddFlag(char short_flag,
                             const std::string& long_flag,
                             const std::string& description) {
        auto* new_flag = new Flag;
        std::string short_flag_string{short_flag};
        auto* key = new Key{short_flag_string, long_flag, description, StoreType::kFlagArgument};
        keys_[short_flag_string] = keys_[long_flag] = key;
        flags_[short_flag_string] = flags_[long_flag] = new_flag;

        return *new_flag;
    }

    void ArgParser::SetFlag(const std::string& flag) {
        try {
            *flags_.at(flag)->value_ = true;
        } catch (const std::exception& exception) {
            throw parse_exception("Not excepted flag: [" + flag + "]");
        }
    }

    bool ArgParser::GetFlag(const std::string& flag) {
        try {
            return flags_.at(flag);
        } catch (const std::exception& exception) {
            throw parse_exception("There's no such flag");
        }
    }

    Argument<int>& ArgParser::AddIntArgument(const std::string& long_key) {
        auto* argument = new Argument<int>;
        int_arguments_[long_key] = argument;
        keys_[long_key] = new Key{"", long_key, "", StoreType::kIntArgument};

        return *argument;
    }

    Argument<int>& ArgParser::AddIntArgument(char short_key, const std::string& long_key) {
        auto* argument = new Argument<int>;
        std::string short_key_string{short_key};
        int_arguments_[short_key_string] = int_arguments_[long_key] = argument;
        keys_[short_key_string] = keys_[long_key] = new Key{short_key_string, long_key, "", StoreType::kIntArgument};

        return *argument;
    }

    Argument<int>& ArgParser::AddIntArgument(const std::string& long_key, const std::string& description) {
        auto* argument = new Argument<int>;
        int_arguments_[long_key] = argument;
        keys_[long_key] = new Key{"", long_key, description, StoreType::kIntArgument};

        return *argument;
    }

    Argument<std::string>& ArgParser::AddStringArgument(const std::string& long_key) {
        auto* argument = new Argument<std::string>;
        string_arguments_[long_key] = argument;
        keys_[long_key] = new Key{"", long_key, "", StoreType::kStringArgument};

        return *argument;
    }

    Argument<std::string>& ArgParser::AddStringArgument(char short_key, const std::string& long_key) {
        auto* argument = new Argument<std::string>;
        std::string short_key_string{short_key};
        string_arguments_[short_key_string] = string_arguments_[long_key] = argument;
        keys_[short_key_string] = keys_[long_key] = new Key{short_key_string, long_key, "", StoreType::kStringArgument};

        return *argument;
    }

    Argument<std::string>& ArgParser::AddStringArgument(char short_key,
                                                        const std::string& long_key,
                                                        const std::string& description) {
        auto* argument = new Argument<std::string>;
        std::string short_key_string{short_key};
        string_arguments_[short_key_string] = string_arguments_[long_key] = argument;
        keys_[short_key_string] = keys_[long_key] =
            new Key{short_key_string, long_key, description, StoreType::kStringArgument};

        return *argument;
    }

    int ArgParser::GetIntValue(const std::string& key, int index) {
        auto* argument = int_arguments_.at(key);
        if (argument->number_of_values_ >= argument->min_number_of_values_) {
            if (argument->multi_value_) {
                return (*argument->values_)[index];
            }
            return *argument->value_;
        } else {
            if (argument->multi_value_) {
                return (*argument->default_values_)[index];
            }
            return *argument->default_value_;
        }
    }

    std::string ArgParser::GetStringValue(const std::string& key, int index) {
        auto* argument = string_arguments_.at(key);
        if (argument->number_of_values_ >= argument->min_number_of_values_) {
            if (argument->multi_value_) {
                return (*argument->values_)[index];
            }
            return *argument->value_;
        } else {
            if (argument->multi_value_) {
                return (*argument->default_values_)[index];
            }
            return *argument->default_value_;
        }
    }

    void ArgParser::AddHelp(char short_key, const std::string& long_key, const std::string& description) {
        help_ = {{short_key}, long_key, description};
    }

    bool ArgParser::Help() const {
        return found_help_;
    }

    std::string ArgParser::HelpDescription() {
        if (help_ == std::nullopt) {
            return "";
        }
        std::stringstream result;
        result << name_ << "\n";
        for (const auto& key : keys_) {
            if (!key.second->short_key.empty()) {
                result << '-' << key.second->short_key;
            }
            if (!key.second->long_key.empty()) {
                result << "\t--" << key.second->long_key;
            }
            if (!key.second->description.empty()) {
                result << "\t" << key.second->description;
            }
            result << '\n';
        }
        return result.str();
    }
}
