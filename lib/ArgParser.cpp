#include "ArgParser.h"

namespace ArgumentParser {

    bool ArgParser::CheckCorrectness() const {
        auto check_correctness = []<typename K>(const std::map<std::string, std::shared_ptr<Argument<K>>>& arguments) {
            return std::all_of(arguments.begin(), arguments.end(), [](std::pair<std::string, std::shared_ptr<Argument<K>>> pair){
                return pair.second->IsCorrect();
            });
        };
        return check_correctness(int_arguments_) && check_correctness(string_arguments_);
    }

    void ArgParser::UpdatePositionalArgument(const std::string& value) {
        for (const auto& key : keys_) {
            if (key.second->type == StoreType::kIntArgument) {
                const auto& argument = int_arguments_[key.first];
                if (argument->positional_) {
                    argument->SetValue(std::stoi(value));
                }
            } else if (key.second->type == StoreType::kStringArgument) {
                const auto& argument = string_arguments_[key.first];
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
            const auto& key = keys_.at(argument_name);
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
                        case kFlagArgument:UpdateFlag(data[i]);
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
        auto new_flag = std::make_shared<Flag>(Flag());
        std::string short_flag_string{short_flag};
        auto key = std::make_shared<Key>(Key{short_flag_string, long_flag, "", StoreType::kFlagArgument});
        keys_[short_flag_string] = keys_[long_flag] = key;
        flags_[short_flag_string] = flags_[long_flag] = new_flag;

        return *new_flag;
    }

    Flag& ArgParser::AddFlag(const std::string& long_flag, const std::string& description) {
        auto new_flag = std::make_shared<Flag>(Flag());
        auto key = std::make_shared<Key>(Key{"", long_flag, description, StoreType::kFlagArgument});
        keys_[long_flag] = key;
        flags_[long_flag] = new_flag;

        return *new_flag;
    }

    Flag& ArgParser::AddFlag(char short_flag,
                             const std::string& long_flag,
                             const std::string& description) {
        auto new_flag = std::make_shared<Flag>(Flag());
        std::string short_flag_string{short_flag};
        auto key = std::make_shared<Key>(Key{short_flag_string, long_flag, description, StoreType::kFlagArgument});
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
            return *flags_.at(flag)->value_;
        } catch (const std::exception& exception) {
            throw parse_exception("There's no such flag");
        }
    }

    Argument<int>& ArgParser::AddIntArgument(const std::string& long_key) {
        auto argument = std::make_shared<Argument<int>>();
        int_arguments_[long_key] = argument;
        keys_[long_key] = std::make_shared<Key>(Key{"", long_key, "", StoreType::kIntArgument});

        return *argument;
    }

    Argument<int>& ArgParser::AddIntArgument(char short_key, const std::string& long_key) {
        auto argument = std::make_shared<Argument<int>>();
        std::string short_key_string{short_key};
        int_arguments_[short_key_string] = int_arguments_[long_key] = argument;
        keys_[short_key_string] = keys_[long_key] =
            std::make_shared<Key>(Key{short_key_string, long_key, "", StoreType::kIntArgument});

        return *argument;
    }

    Argument<int>& ArgParser::AddIntArgument(const std::string& long_key, const std::string& description) {
        auto argument = std::make_shared<Argument<int>>();
        int_arguments_[long_key] = argument;
        keys_[long_key] = std::make_shared<Key>(Key{"", long_key, description, StoreType::kIntArgument});

        return *argument;
    }

    Argument<std::string>& ArgParser::AddStringArgument(const std::string& long_key) {
        auto argument = std::make_shared<Argument<std::string>>();
        string_arguments_[long_key] = argument;
        keys_[long_key] = std::make_shared<Key>(Key{"", long_key, "", StoreType::kStringArgument});

        return *argument;
    }

    Argument<std::string>& ArgParser::AddStringArgument(char short_key, const std::string& long_key) {
        auto argument = std::make_shared<Argument<std::string>>();
        std::string short_key_string{short_key};
        string_arguments_[short_key_string] = string_arguments_[long_key] = argument;
        keys_[short_key_string] = keys_[long_key] =
            std::make_shared<Key>(Key{short_key_string, long_key, "", StoreType::kStringArgument});

        return *argument;
    }

    Argument<std::string>& ArgParser::AddStringArgument(char short_key,
                                                        const std::string& long_key,
                                                        const std::string& description) {
        auto argument = std::make_shared<Argument<std::string>>();
        std::string short_key_string{short_key};
        string_arguments_[short_key_string] = string_arguments_[long_key] = argument;
        keys_[short_key_string] = keys_[long_key] =
            std::make_shared<Key>(Key{short_key_string, long_key, description, StoreType::kStringArgument});

        return *argument;
    }

    int ArgParser::GetIntValue(const std::string& key, int index) {
        const auto& argument = int_arguments_.at(key);
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
        const auto& argument = string_arguments_.at(key);
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