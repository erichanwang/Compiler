#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <sstream>

using namespace std;

// --- Data Types ---
enum class ValueType { STRING, NUMBER, BOOL, EMPTY };

struct Value {
    ValueType type = ValueType::EMPTY;
    string s_val;
    double n_val = 0;
    bool b_val = false;

    string to_string() const {
        switch (type) {
            case ValueType::STRING: return s_val;
            case ValueType::NUMBER: return std::to_string(n_val);
            case ValueType::BOOL: return b_val ? "true" : "false";
            default: return "EMPTY";
        }
    }
};

// --- Global State ---
map<string, Value> variables;

// --- Utility Functions ---
string trim(const string& str) {
    size_t first = str.find_first_not_of(" \t\n\r");
    if (string::npos == first) return "";
    size_t last = str.find_last_not_of(" \t\n\r");
    return str.substr(first, (last - first + 1));
}

Value parse_value(const string& str) {
    Value val;
    string trimmed = trim(str);
    if (trimmed.front() == '\"' && trimmed.back() == '\"') {
        val.type = ValueType::STRING;
        val.s_val = trimmed.substr(1, trimmed.length() - 2);
    } else if (variables.count(trimmed)) {
        return variables[trimmed];
    } else {
        try {
            val.n_val = stod(trimmed);
            val.type = ValueType::NUMBER;
        } catch (...) {
            // Not a number, treat as an unquoted string or error
            val.type = ValueType::STRING;
            val.s_val = trimmed;
        }
    }
    return val;
}

// --- Forward Declarations ---
void execute_block(const vector<string>& block);
Value evaluate_expression(const string& expr);

// --- Core Logic ---
void execute_line(const string& line) {
    string trimmed_line = trim(line);
    if (trimmed_line.empty()) return;

    stringstream ss(trimmed_line);
    string command;
    ss >> command;

    if (command == "print") {
        string rest_of_line;
        getline(ss, rest_of_line);
        Value val = evaluate_expression(rest_of_line);
        cout << val.to_string() << endl;
    } else if (command == "let") {
        string var_name, eq, rest_of_line;
        ss >> var_name >> eq;
        getline(ss, rest_of_line);
        if (eq == "=") {
            variables[var_name] = evaluate_expression(rest_of_line);
        }
    } else if (command == "input") {
        string var_name;
        ss >> var_name;
        string input_val;
        getline(cin, input_val);
        variables[var_name] = { ValueType::STRING, input_val };
    }
}

Value evaluate_expression(const string& expr) {
    string trimmed_expr = trim(expr);
    stringstream ss(trimmed_expr);
    string left_str, op, right_str;
    ss >> left_str >> op >> right_str;

    if (op.empty()) {
        return parse_value(trimmed_expr);
    }

    Value left = parse_value(left_str);
    Value right = parse_value(right_str);
    Value result;
    result.type = ValueType::BOOL;

    if (op == "==") {
        if (left.type == ValueType::NUMBER && right.type == ValueType::NUMBER) {
            result.b_val = left.n_val == right.n_val;
        } else {
            result.b_val = left.to_string() == right.to_string();
        }
    } else if (op == "!=") {
        if (left.type == ValueType::NUMBER && right.type == ValueType::NUMBER) {
            result.b_val = left.n_val != right.n_val;
        } else {
            result.b_val = left.to_string() != right.to_string();
        }
    } else if (op == ">") {
        result.b_val = left.n_val > right.n_val;
    } else if (op == "<") {
        result.b_val = left.n_val < right.n_val;
    } else if (op == ">=") {
        result.b_val = left.n_val >= right.n_val;
    } else if (op == "<=") {
        result.b_val = left.n_val <= right.n_val;
    }

    return result;
}

void execute_block(const vector<string>& block) {
    for (size_t i = 0; i < block.size(); ++i) {
        string trimmed_line = trim(block[i]);
        if (trimmed_line.rfind("if", 0) == 0) {
            size_t condition_start = trimmed_line.find('(') + 1;
            size_t condition_end = trimmed_line.rfind(')');
            string condition = trimmed_line.substr(condition_start, condition_end - condition_start);
            
            vector<string> if_block, else_block;
            int brace_count = 0;
            bool in_if = false, in_else = false;

            // Find the if block
            size_t j = i + 1;
            for (; j < block.size(); ++j) {
                if (trim(block[j]) == "{") {
                    if (!in_if) in_if = true;
                    brace_count++;
                } else if (trim(block[j]) == "}") {
                    brace_count--;
                    if (brace_count == 0) {
                        j++;
                        break;
                    }
                } else if (in_if) {
                    if_block.push_back(block[j]);
                }
            }

            // Check for an else block
            if (j < block.size() && trim(block[j]) == "else") {
                j++; // Move past "else"
                for (; j < block.size(); ++j) {
                     if (trim(block[j]) == "{") {
                        if (!in_else) in_else = true;
                        brace_count++;
                    } else if (trim(block[j]) == "}") {
                        brace_count--;
                        if (brace_count == 0) {
                            break;
                        }
                    } else if (in_else) {
                        else_block.push_back(block[j]);
                    }
                }
            }

            Value result = evaluate_expression(condition);
            if (result.type == ValueType::BOOL && result.b_val) {
                execute_block(if_block);
            } else {
                execute_block(else_block);
            }
            i = j; // Continue execution after the if/else block
        } else {
            execute_line(block[i]);
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " <source_file>" << endl;
        return 1;
    }

    ifstream source_file(argv[1]);
    if (!source_file.is_open()) {
        cerr << "Error: Could not open file " << argv[1] << endl;
        return 1;
    }

    vector<string> lines;
    string line;
    while (getline(source_file, line)) {
        lines.push_back(line);
    }

    execute_block(lines);

    return 0;
}
