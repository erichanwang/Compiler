#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <sstream>

using namespace std;

// --- Data Types ---
enum class ValueType { STRING, NUMBER, BOOL, EMPTY, ERROR };

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
            case ValueType::ERROR: return "ERROR: " + s_val;
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

// --- Forward Declarations ---
void execute_block(const vector<string>& block);
Value evaluate_expression(const string& expr);

Value parse_value(const string& str) {
    Value val;
    string trimmed = trim(str);
    if (trimmed.empty()) return val;

    if (trimmed.length() >= 2 && trimmed.front() == '\"' && trimmed.back() == '\"') {
        val.type = ValueType::STRING;
        val.s_val = trimmed.substr(1, trimmed.length() - 2);
    } else if (variables.count(trimmed)) {
        return variables[trimmed];
    } else {
        try {
            size_t processed_chars;
            val.n_val = stod(trimmed, &processed_chars);
            if (processed_chars == trimmed.length()) {
                val.type = ValueType::NUMBER;
            } else {
                 val.type = ValueType::ERROR;
                 val.s_val = "Invalid number format: '" + trimmed + "'";
            }
        } catch (...) {
            val.type = ValueType::ERROR;
            val.s_val = "Unknown identifier: '" + trimmed + "'";
        }
    }
    return val;
}

Value evaluate_expression(const string& expr) {
    string trimmed_expr = trim(expr);
    
    string op;
    size_t op_pos = string::npos;

    if ((op_pos = trimmed_expr.find("==")) != string::npos) op = "==";
    else if ((op_pos = trimmed_expr.find("!=")) != string::npos) op = "!=";
    else if ((op_pos = trimmed_expr.find(">=")) != string::npos) op = ">=";
    else if ((op_pos = trimmed_expr.find("<=")) != string::npos) op = "<=";
    else if ((op_pos = trimmed_expr.find(">")) != string::npos) op = ">";
    else if ((op_pos = trimmed_expr.find("<")) != string::npos) op = "<";

    if (op.empty()) {
        return parse_value(trimmed_expr);
    }

    string left_str = trim(trimmed_expr.substr(0, op_pos));
    string right_str = trim(trimmed_expr.substr(op_pos + op.length()));

    Value left = evaluate_expression(left_str);
    Value right = evaluate_expression(right_str);
    Value result;
    result.type = ValueType::BOOL;

    if (op == "==") {
        if (left.type == ValueType::NUMBER && right.type == ValueType::NUMBER) result.b_val = left.n_val == right.n_val;
        else result.b_val = left.to_string() == right.to_string();
    } else if (op == "!=") {
        if (left.type == ValueType::NUMBER && right.type == ValueType::NUMBER) result.b_val = left.n_val != right.n_val;
        else result.b_val = left.to_string() != right.to_string();
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

void execute_line(const string& line) {
    string trimmed_line = trim(line);
    if (trimmed_line.empty()) return;

    if (trimmed_line.rfind("prt ", 0) == 0) {
        string rest_of_line = trim(trimmed_line.substr(4));
        Value val = evaluate_expression(rest_of_line);
        cout << val.to_string() << endl;
        return;
    }
    
    if (trimmed_line.rfind("input ", 0) == 0) {
        string var_name = trim(trimmed_line.substr(6));
        string input_val;
        getline(cin, input_val);
        variables[var_name] = { ValueType::STRING, input_val };
        return;
    }

    size_t equals_pos = trimmed_line.find('=');
    if (equals_pos != string::npos) {
        bool is_comparison = (equals_pos > 0 && (trimmed_line[equals_pos - 1] == '=' || trimmed_line[equals_pos - 1] == '!' || trimmed_line[equals_pos - 1] == '>' || trimmed_line[equals_pos - 1] == '<')) || (equals_pos < trimmed_line.length() - 1 && trimmed_line[equals_pos + 1] == '=');
        if (!is_comparison) {
            string var_name = trim(trimmed_line.substr(0, equals_pos));
            string value_str = trim(trimmed_line.substr(equals_pos + 1));
            if (!var_name.empty() && var_name.find(' ') == string::npos) {
                variables[var_name] = evaluate_expression(value_str);
            }
        }
    }
}

// --- Block Execution Logic ---
size_t find_block_end(const vector<string>& program, size_t start_line) {
    int brace_level = 0;
    bool started = false;
    for (size_t i = start_line; i < program.size(); ++i) {
        for (char c : program[i]) {
            if (c == '{') {
                brace_level++;
                started = true;
            }
            else if (c == '}') brace_level--;
        }
        if (started && brace_level == 0) return i;
    }
    return program.size() - 1;
}

vector<string> get_block_content(const vector<string>& program, size_t start_line, size_t end_line) {
    vector<string> content;
    string first_line = program[start_line];
    size_t first_brace = first_line.find('{');
    
    if (start_line == end_line) {
        size_t last_brace = first_line.rfind('}');
        if (first_brace != string::npos && last_brace != string::npos) {
            content.push_back(first_line.substr(first_brace + 1, last_brace - first_brace - 1));
        }
    } else {
        content.push_back(first_line.substr(first_brace + 1));
        for (size_t i = start_line + 1; i < end_line; ++i) {
            content.push_back(program[i]);
        }
        string last_line = program[end_line];
        size_t last_brace = last_line.rfind('}');
        if (last_brace != string::npos) {
            content.push_back(last_line.substr(0, last_brace));
        }
    }
    return content;
}

void execute_block(const vector<string>& block) {
    for (size_t i = 0; i < block.size(); ++i) {
        string trimmed_line = trim(block[i]);
        if (trimmed_line.rfind("if", 0) == 0) {
            bool condition_met_in_chain = false;
            size_t current_pos = i;

            // Handle 'if'
            size_t if_block_end = find_block_end(block, current_pos);
            size_t cond_start = block[current_pos].find('(') + 1;
            size_t cond_end = block[current_pos].rfind(')');
            string condition = block[current_pos].substr(cond_start, cond_end - cond_start);
            if (evaluate_expression(condition).b_val) {
                condition_met_in_chain = true;
                execute_block(get_block_content(block, current_pos, if_block_end));
            }
            current_pos = if_block_end;

            // Handle 'else if' and 'else'
            while (current_pos + 1 < block.size()) {
                string next_line = trim(block[current_pos + 1]);
                if (next_line.rfind("else if", 0) == 0) {
                    current_pos++;
                    size_t elseif_block_end = find_block_end(block, current_pos);
                    if (!condition_met_in_chain) {
                        cond_start = block[current_pos].find('(') + 1;
                        cond_end = block[current_pos].rfind(')');
                        condition = block[current_pos].substr(cond_start, cond_end - cond_start);
                        if (evaluate_expression(condition).b_val) {
                            condition_met_in_chain = true;
                            execute_block(get_block_content(block, current_pos, elseif_block_end));
                        }
                    }
                    current_pos = elseif_block_end;
                } else if (next_line.rfind("else", 0) == 0) {
                    current_pos++;
                    size_t else_block_end = find_block_end(block, current_pos);
                    if (!condition_met_in_chain) {
                        execute_block(get_block_content(block, current_pos, else_block_end));
                    }
                    current_pos = else_block_end;
                    break;
                } else {
                    break;
                }
            }
            i = current_pos;
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
