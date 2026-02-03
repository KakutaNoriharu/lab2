#ifndef PARAMETER_LOADER_HPP_
#define PARAMETER_LOADER_HPP_

#include <string>
#include <vector>
#include <unordered_map>
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <functional>

/**
 * パラメータ設定が記述されたファイルを
 * パラメータ単位となる部分文字列に分解する
 * * パラメータファイルの構成:
 * key1:value1,value2,value3; // -> L1: vector<string>
 * key2:(value1,value2),(value3,value4); // -> L2: vector<vector<string>>
 * key3:{(value1,value2),(value3,value4)},{(value101,value102),(value103,value104)};
 * // -> L3: vector<vector<vector<string>>>
 * 
 * update-log
 * 2025-10-07
 * ・Parameterの型にvector<vector<vector<string>>>を追加，パラメータファイルの","区切りで全てのstringを保存するようにした．
 */

class ParameterLoader {
public:
    struct Parameter {
        std::string raw_value;
        std::vector<std::string> parsed_values; // L1: "1,2,3"
        std::vector<std::vector<std::string>> vector_values; // L2: "(1,2),(3,4)"
        std::vector<std::vector<std::vector<std::string>>> vector_of_vector_values; // L3: "{(1,2),...},{(5,6),...}"
    };

    explicit ParameterLoader(const std::string& filename) {
        loadFromFile(filename);
        parseParameters();
    }

    // 既存のインターフェース
    std::vector<std::string> keys() const {
        std::vector<std::string> ret;
        for (const auto& param_pair : parameters) {
            ret.push_back(param_pair.first);
        }
        return ret;
    }

    // 単体の整数要素を参照する場合に利用
    unsigned int set(const std::string& key) const {
        auto it = parameters.find(key);
        if (it == parameters.end()) {
            throw std::runtime_error("Key not found: " + key);
        }
        if (it->second.parsed_values.empty()) {
            throw std::runtime_error("L1 value is empty for key: " + key);
        }
        // L1の最初の要素をintに変換
        return static_cast<unsigned int>(std::stoul(it->second.parsed_values[0]));
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

    std::vector<std::vector<std::vector<std::string>>> getVectorOfVectors(const std::string& key) const {
        auto it = parameters.find(key);
        if (it == parameters.end()) {
            throw std::runtime_error("Key not found: " + key);
        }
        return it->second.vector_of_vector_values;
    }

    void describe() const {
        for (const auto& param_pair : parameters) {
            const auto& key = param_pair.first;
            const auto& param = param_pair.second;
            
            std::cout << key << ": " << param.raw_value << " -> ";

            if (!param.vector_of_vector_values.empty()) {
                // L3表示
                std::cout << "L3 { ";
                for (const auto& l2_vec : param.vector_of_vector_values) {
                    std::cout << "{ ";
                    for (const auto& l1_vec : l2_vec) {
                        std::cout << "(";
                        for (size_t i = 0; i < l1_vec.size(); ++i) {
                            std::cout << l1_vec[i] << (i < l1_vec.size() - 1 ? "," : "");
                        }
                        std::cout << ") ";
                    }
                    std::cout << "} ";
                }
                std::cout << "}";
            } else if (!param.vector_values.empty()) {
                // L2表示
                std::cout << "L2 { ";
                for (const auto& vec : param.vector_values) {
                    std::cout << "(";
                    for (size_t i = 0; i < vec.size(); ++i) {
                        std::cout << vec[i] << (i < vec.size() - 1 ? "," : "");
                    }
                    std::cout << ") ";
                }
                std::cout << "}";
            } else {
                // L1表示
                std::cout << "L1 [ ";
                for (const auto& value : param.parsed_values) {
                    std::cout << value << " ";
                }
                std::cout << "]";
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
        while (std::getline(file, line)) {
            // Skip empty lines and comments
            if (line.empty() || line.front() == '#') continue;

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
                parseLine(accumulator);
                accumulator.clear();
            }
        }
    }

    void parseLine(const std::string& line) {
        size_t colonPos = line.find(':');
        if (colonPos == std::string::npos) return;

        std::string key = line.substr(0, colonPos);
        // ';' を取り除く
        std::string value = line.substr(colonPos + 1, line.length() - colonPos - 2); 

        if (parameters.count(key)) {
            throw std::runtime_error("Duplicate key found: " + key);
        }
        parameters[key].raw_value = value;
    }

    void parseParameters() {
        for (std::pair<const std::string, Parameter>& param_pair : parameters) {
            Parameter& param = param_pair.second;
            // 優先度の高い順に判定
            if (param.raw_value.find('{') != std::string::npos) { // L3: {} があれば
                parseVectorFormat(param);
            } else if (param.raw_value.find('(') != std::string::npos) { // L2: () があれば
                parseTupleFormat(param);
            } else { // L1: それ以外
                parseSimpleFormat(param);
            }
        }
    }

    // L1: "1,2,3" -> {"1","2","3"}
    static void parseSimpleFormat(Parameter& param) {
        std::stringstream ss(param.raw_value);
        std::string item;
        while (std::getline(ss, item, ',')) {
            if (!item.empty()) {
                param.parsed_values.push_back(item);
            }
        }
    }

    // ネストされた文字列をトップレベルのセパレータで分割するヘルパー関数
    // 例: "{(1,2),(3,4)},{(5,6),(7,8)}" をトップレベルのカンマで分割する
    static std::vector<std::string> splitTopLevel(const std::string& str, char separator, char open_brace, char close_brace) {
        std::vector<std::string> tokens;
        int brace_count = 0;
        size_t start_pos = 0;

        for (size_t i = 0; i < str.length(); ++i) {
            char c = str[i];
            if (c == open_brace) brace_count++;
            else if (c == close_brace) brace_count--;
            else if (c == separator && brace_count == 0) {
                // トップレベルのセパレータを見つけた
                if (i > start_pos) {
                    tokens.push_back(str.substr(start_pos, i - start_pos));
                }
                start_pos = i + 1;
            }
        }

        // 最後のトークンを追加
        if (str.length() > start_pos) {
            tokens.push_back(str.substr(start_pos));
        }

        return tokens;
    }

    // L1タプルをパースするヘルパー関数
    // 例: "(1,2)" -> {"1", "2"}
    static std::vector<std::string> parseTuple(const std::string& tuple_str) {
        std::vector<std::string> result;
        std::string content = tuple_str;
        
        // 外側の括弧 () を取り除く
        if (content.front() == '(' && content.back() == ')') {
            content = content.substr(1, content.length() - 2);
        } else {
            // 括弧がない場合はエラー (L2/L3ではタプル形式が必須のため)
            throw std::runtime_error("Invalid tuple format: " + tuple_str);
        }
        
        std::stringstream ss(content);
        std::string item;
        while (std::getline(ss, item, ',')) {
            if (!item.empty()) {
                result.push_back(item);
            }
        }
        return result;
    }

    // L2: "(1,2),(3,4)" -> {{"1","2"},{"3","4"}}
    static void parseTupleFormat(Parameter& param) {
        // L2はトップレベルで '()' のグループを ',' で分割
        // 例: "(1,2),(3,4)" -> ["(1,2)", "(3,4)"]
        std::vector<std::string> tuple_tokens = splitTopLevel(param.raw_value, ',', '(', ')');

        for (const std::string& token : tuple_tokens) {
            // 各トークンがタプル "(a,b)" の形式であると期待される
            param.vector_values.push_back(parseTuple(token));
        }
    }

    // L3: "{(1,2),(3,4)},{(5,6),(7,8)}}" -> {{{"1","2"},{"3","4"}},{{"5","6"},{"7","8"}}}
    static void parseVectorFormat(Parameter& param) {
        // L3はトップレベルで '{}' のグループを ',' で分割
        // 例: "{(1,2),(3,4)},{(5,6),(7,8)}" -> ["{(1,2),(3,4)}", "{(5,6),(7,8)}"]
        std::vector<std::string> block_tokens = splitTopLevel(param.raw_value, ',', '{', '}');

        for (const std::string& block_token : block_tokens) {
            std::string content = block_token;
            
            // 外側の括弧 {} を取り除く
            if (content.front() == '{' && content.back() == '}') {
                content = content.substr(1, content.length() - 2);
            } else {
                throw std::runtime_error("Invalid L3 block format: " + block_token);
            }

            // content は L2 形式の文字列になっているはず (例: "(1,2),(3,4)")
            std::vector<std::string> tuple_tokens = splitTopLevel(content, ',', '(', ')');

            std::vector<std::vector<std::string>> l2_result;
            for (const std::string& tuple_token : tuple_tokens) {
                // 各トークンを L1 タプルとしてパース
                l2_result.push_back(parseTuple(tuple_token));
            }

            if (!l2_result.empty()) {
                param.vector_of_vector_values.push_back(l2_result);
            }
        }
    }
};

#endif // PARAMETER_LOADER_HPP_
