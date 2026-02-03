#ifndef PARAMETER_LOADER_HPP_
#define PARAMETER_LOADER_HPP_

#include <string>
#include <vector>
#include <unordered_map>
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <memory>
#include <algorithm>

/**
 * パラメータ設定が記述されたファイルを
 * パラメータ単位となる部分文字列に分解する
 * "1,2,3" -> ["1","2","3"]
 * 
 * パラメータファイルの構成
 * key1:value1,value2,value3; // -> ["value1", "value2", "value3"]
 * key2:(value1,value2),(value1,value2); // -> [["value1","value2"],["value3","value4"]]
 * key3:{(value1,value2),(value3,value4)},{(value101,value102),(value103,value104)};
 * // -> [[["value1","value2"],["value3","value4"]], [["value101","vaue102"],["value103","value104"]] ]
 */

class ParameterLoader {
public:
    struct Parameter {
        std::string raw_value;
        std::vector<std::string> parsed_values;
        std::vector<std::vector<std::string>> vector_values;
    };

    explicit ParameterLoader(const std::string& filename) {
        loadFromFile(filename);
        parseParameters();
    }

    std::vector<std::string> keys() const {
        std::vector<std::string> ret;
        for (const auto& param_pair : parameters) {
            const auto& key = param_pair.first;
            ret.push_back(key);
        }
        return ret;
    }

    // 単体の要素をprmから参照する場合に利用
    unsigned int set(const std::string& key) const {
        auto it = parameters.find(key);
        if (it == parameters.end()) {
            throw std::runtime_error("Key not found: " + key);
        }
        return atoi(it->second.parsed_values[0].c_str());
    }
    

    std::vector<std::string> get(const std::string& key) const {
        auto it = parameters.find(key);
        if (it == parameters.end()) {
            throw std::runtime_error("Key not found: " + key);
        }
        return it->second.parsed_values;
    }

    std::vector<std::vector<std::string>> getVector(const std::string& key) const {
        auto it = parameters.find(key);
        if (it == parameters.end()) {
            throw std::runtime_error("Key not found: " + key);
        }
        return it->second.vector_values;
    }

    void describe() const { // const : describe内で変数の書き換えを防ぐ
        for (const auto& param_pair : parameters) {
            const auto& key = param_pair.first;
            const auto& param = param_pair.second;
            
            std::cout << key << ": " << param.raw_value << " -> ";
            if (!param.vector_values.empty()) {
                for (const auto& vec : param.vector_values) {
                    std::cout << "{ ";
                    for (const auto& val : vec) {
                        std::cout << "[" << val << "] ";
                    }
                    std::cout << "} ";
                }
            } else {
                for (const auto& value : param.parsed_values) {
                    std::cout << "[" << value << "] ";
                }
            }
            std::cout << "\n";
        }
    }

private:
    std::unordered_map<std::string, Parameter> parameters;

    void loadFromFile(const std::string& filename) {
        std::ifstream file(filename);
        if (!file) {
            throw std::runtime_error("Cannot open file: " + filename);
        }

        std::string line, accumulator;
        while (std::getline(file, line)) { // 行ごとに検証する，accumulatorに貯めて，parseLineで処理する．
            // Skip empty lines and comments
            if (line.empty() || line[0] == '#') continue;
            
            // Remove whitespace
            line.erase(std::remove_if(line.begin(), line.end(), 
                [](char c) { return std::isspace(c); }), line.end());
            
            // Remove comments
            size_t commentPos = line.find('#');
            if (commentPos != std::string::npos) {
                line = line.substr(0, commentPos);
            }
            
            if (line.empty()) continue;

            accumulator += line;
            if (line.back() == ';') {
                parseLine(accumulator); // "key:value" を解析
                accumulator.clear();
            }
        }
    }

    void parseLine(const std::string& line) {
        size_t colonPos = line.find(':'); // : を見つける，なければnpos
        if (colonPos == std::string::npos) return;

        std::string key = line.substr(0, colonPos);
        std::string value = line.substr(colonPos + 1, line.length() - colonPos - 2); // -2 for removing ; + \n

        if (parameters.find(key) != parameters.end()) { // key already exists
            throw std::runtime_error("Duplicate key found: " + key);
        }
        parameters[key].raw_value = value; // valueを格納
    }

    void parseParameters() {
        for (std::pair<const std::string, Parameter>& param_pair : parameters) {
            Parameter& param = param_pair.second;
            if (param.raw_value.find('{') != std::string::npos) { // { があればvectorとして処理
                parseVectorFormat(param);
            } else if (param.raw_value.find('(') != std::string::npos) { // ( があればtupleとして処理
                parseTupleFormat(param);
            } else {
                parseSimpleFormat(param); // それ以外は単一値
            }
        }
    }

    static void parseSimpleFormat(Parameter& param) {
        std::stringstream ss(param.raw_value);
        std::string item;
        while (std::getline(ss, item, ',')) {
            if (!item.empty()) {
                param.parsed_values.push_back(item);
            }
        }
    }

    static void parseTupleFormat(Parameter& param) {
        std::string current;
        std::string number;
        bool inTuple = false;
        
        for (char c : param.raw_value) {
            if (c == '(') {
                inTuple = true;
                current.clear();
            } else if (c == ')') {
                inTuple = false;
                if (!number.empty()) {
                    current += number;
                    number.clear();
                }
                param.parsed_values.push_back(current);
                current.clear();
            } else if (c == ',') {
                if (inTuple) {
                    current += number + ",";
                    number.clear();
                }
            } else if (inTuple) {
                number += c;
            }
        }
    }

    static void parseVectorFormat(Parameter& param) {
        std::vector<std::string> currentVector;
        std::string current;
        std::string number;
        bool inTuple = false;
        
        for (char c : param.raw_value) {
            if (c == '{') {
                currentVector.clear();
            } else if (c == '}') {
                if (!currentVector.empty()) {
                    param.vector_values.push_back(currentVector);
                }
            } else if (c == '(') {
                inTuple = true;
                current.clear();
                number.clear();
            } else if (c == ')') {
                inTuple = false;
                if (!number.empty()) {
                    current += number;
                }
                if (!current.empty()) {
                    currentVector.push_back(current);
                }
            } else if (c == ',' && inTuple) {
                current += number + ",";
                number.clear();
            } else if (inTuple) {
                number += c;
            }
        }
    }
};

#endif