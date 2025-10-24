//Eduardo Avila Summer 2025

#include <iostream>
#include <map>
#include <string>
#include "compiler.h"
#include "lexer.h"

using namespace std;

LexicalAnalyzer lexer;
map<string, int> location_table;

struct InstructionNode* parse_program();
struct InstructionNode* parse_var_section();
struct InstructionNode* parse_body();
struct InstructionNode* parse_stmt_list();
struct InstructionNode* parse_stmt();
struct InstructionNode* parse_assign_stmt();
struct InstructionNode* parse_if_stmt();
struct InstructionNode* parse_while_stmt();
struct InstructionNode* parse_for_stmt();
struct InstructionNode* parse_switch_stmt();
struct InstructionNode* parse_input_stmt();
struct InstructionNode* parse_output_stmt();
int parse_primary();
ArithmeticOperatorType parse_op();
ConditionalOperatorType parse_relop();
void parse_inputs();

int get_location(string var_name) {
    if (location_table.find(var_name) == location_table.end()) {
        location_table[var_name] = next_available;
        mem[next_available] = 0;
        next_available++;
    }
    return location_table[var_name];
}

int allocate_constant(int value) {
    int loc = next_available;
    mem[next_available] = value;
    next_available++;
    return loc;
}

struct InstructionNode* create_noop() {
    struct InstructionNode* node = new InstructionNode;
    node->type = NOOP;
    node->next = nullptr;
    return node;
}

struct InstructionNode* append_instruction(struct InstructionNode* first, struct InstructionNode* second) {
    if (first == nullptr) return second;
    
    struct InstructionNode* current = first;
    while (current->next != nullptr) {
        current = current->next;
    }
    current->next = second;
    return first;
}

struct InstructionNode* parse_generate_intermediate_representation() {
    return parse_program();
}

struct InstructionNode* parse_program() {
    parse_var_section();
    struct InstructionNode* body = parse_body();
    parse_inputs();
    return body;
}

struct InstructionNode* parse_var_section() {
    Token token = lexer.GetToken();
    while (token.token_type == ID) {
        get_location(token.lexeme);
        token = lexer.GetToken();
        if (token.token_type == COMMA) {
            token = lexer.GetToken();
        }
    }
    // token should be SEMICOLON
    return nullptr;
}

struct InstructionNode* parse_body() {
    Token token = lexer.GetToken(); // LBRACE
    struct InstructionNode* stmt_list = parse_stmt_list();
    token = lexer.GetToken(); // RBRACE
    return stmt_list;
}

struct InstructionNode* parse_stmt_list() {
    struct InstructionNode* stmt = parse_stmt();
    Token token = lexer.peek(1);
    
    if (token.token_type == ID || token.token_type == IF || token.token_type == WHILE || 
        token.token_type == FOR || token.token_type == SWITCH || token.token_type == INPUT || 
        token.token_type == OUTPUT) {
        struct InstructionNode* rest = parse_stmt_list();
        return append_instruction(stmt, rest);
    }
    
    return stmt;
}

struct InstructionNode* parse_stmt() {
    Token token = lexer.peek(1);
    
    switch (token.token_type) {
        case ID:
            return parse_assign_stmt();
        case IF:
            return parse_if_stmt();
        case WHILE:
            return parse_while_stmt();
        case FOR:
            return parse_for_stmt();
        case SWITCH:
            return parse_switch_stmt();
        case INPUT:
            return parse_input_stmt();
        case OUTPUT:
            return parse_output_stmt();
        default:
            return nullptr;
    }
}

struct InstructionNode* parse_assign_stmt() {
    Token id_token = lexer.GetToken(); // ID
    Token equal_token = lexer.GetToken(); // EQUAL
    
    struct InstructionNode* node = new InstructionNode;
    node->type = ASSIGN;
    node->assign_inst.left_hand_side_index = get_location(id_token.lexeme);
    node->next = nullptr;
    
    Token next_token = lexer.peek(1);
    if (next_token.token_type == ID || next_token.token_type == NUM) {
        int op1 = parse_primary();
        Token op_token = lexer.peek(1);
        
        if (op_token.token_type == PLUS || op_token.token_type == MINUS || 
            op_token.token_type == MULT || op_token.token_type == DIV) {
            ArithmeticOperatorType op = parse_op();
            int op2 = parse_primary();
            
            node->assign_inst.op = op;
            node->assign_inst.operand1_index = op1;
            node->assign_inst.operand2_index = op2;
        } else {
            node->assign_inst.op = OPERATOR_NONE;
            node->assign_inst.operand1_index = op1;
        }
    }
    
    Token semicolon = lexer.GetToken(); // SEMICOLON
    return node;
}

struct InstructionNode* parse_if_stmt() {
    Token if_token = lexer.GetToken(); // IF
    
    struct InstructionNode* cjmp_node = new InstructionNode;
    cjmp_node->type = CJMP;
    
    // Parse condition
    int op1 = parse_primary();
    ConditionalOperatorType relop = parse_relop();
    int op2 = parse_primary();
    
    cjmp_node->cjmp_inst.condition_op = relop;
    cjmp_node->cjmp_inst.operand1_index = op1;
    cjmp_node->cjmp_inst.operand2_index = op2;
    
    struct InstructionNode* body = parse_body();
    cjmp_node->next = body;
    
    struct InstructionNode* noop = create_noop();
    cjmp_node->cjmp_inst.target = noop;
    
    return append_instruction(cjmp_node, noop);
}

struct InstructionNode* parse_while_stmt() {
    Token while_token = lexer.GetToken(); // WHILE
    
    struct InstructionNode* cjmp_node = new InstructionNode;
    cjmp_node->type = CJMP;
    
    // Parse condition
    int op1 = parse_primary();
    ConditionalOperatorType relop = parse_relop();
    int op2 = parse_primary();
    
    cjmp_node->cjmp_inst.condition_op = relop;
    cjmp_node->cjmp_inst.operand1_index = op1;
    cjmp_node->cjmp_inst.operand2_index = op2;
    
    struct InstructionNode* body = parse_body();
    cjmp_node->next = body;
    
    struct InstructionNode* jmp_node = new InstructionNode;
    jmp_node->type = JMP;
    jmp_node->jmp_inst.target = cjmp_node;
    jmp_node->next = nullptr;
    
    struct InstructionNode* noop = create_noop();
    cjmp_node->cjmp_inst.target = noop;
    
    struct InstructionNode* body_with_jmp = append_instruction(body, jmp_node);
    cjmp_node->next = body_with_jmp;
    
    return append_instruction(cjmp_node, noop);
}

struct InstructionNode* parse_for_stmt() {
    Token for_token = lexer.GetToken(); // FOR
    Token lparen = lexer.GetToken(); // LPAREN
    
    struct InstructionNode* init = parse_assign_stmt();
    
    // Parse condition for while loop
    struct InstructionNode* cjmp_node = new InstructionNode;
    cjmp_node->type = CJMP;
    
    int op1 = parse_primary();
    ConditionalOperatorType relop = parse_relop();
    int op2 = parse_primary();
    
    cjmp_node->cjmp_inst.condition_op = relop;
    cjmp_node->cjmp_inst.operand1_index = op1;
    cjmp_node->cjmp_inst.operand2_index = op2;
    
    Token semicolon = lexer.GetToken(); // SEMICOLON
    
    struct InstructionNode* update = parse_assign_stmt();
    Token rparen = lexer.GetToken(); // RPAREN
    
    struct InstructionNode* body = parse_body();
    
    // Build the structure: init -> cjmp -> body -> update -> jmp -> noop
    struct InstructionNode* jmp_node = new InstructionNode;
    jmp_node->type = JMP;
    jmp_node->jmp_inst.target = cjmp_node;
    jmp_node->next = nullptr;
    
    struct InstructionNode* noop = create_noop();
    cjmp_node->cjmp_inst.target = noop;
    
    struct InstructionNode* body_with_update_jmp = append_instruction(body, append_instruction(update, jmp_node));
    cjmp_node->next = body_with_update_jmp;
    
    return append_instruction(init, append_instruction(cjmp_node, noop));
}

struct InstructionNode* parse_switch_stmt() {
    Token switch_token = lexer.GetToken(); // SWITCH
    Token id_token = lexer.GetToken(); // ID
    Token lbrace = lexer.GetToken(); // LBRACE
    
    int switch_var = get_location(id_token.lexeme);
    struct InstructionNode* first_case = nullptr;
    struct InstructionNode* current = nullptr;
    struct InstructionNode* noop = create_noop();
    
    Token token = lexer.peek(1);
    while (token.token_type == CASE) {
        lexer.GetToken(); // CASE
        Token num_token = lexer.GetToken(); // NUM
        Token colon = lexer.GetToken(); // COLON
        
        int case_value = allocate_constant(stoi(num_token.lexeme));
        
        struct InstructionNode* cjmp_node = new InstructionNode;
        cjmp_node->type = CJMP;
        cjmp_node->cjmp_inst.condition_op = CONDITION_NOTEQUAL;
        cjmp_node->cjmp_inst.operand1_index = switch_var;
        cjmp_node->cjmp_inst.operand2_index = case_value;
        
        struct InstructionNode* case_body = parse_body();
        struct InstructionNode* jmp_to_end = new InstructionNode;
        jmp_to_end->type = JMP;
        jmp_to_end->jmp_inst.target = noop;
        jmp_to_end->next = nullptr;
        
        cjmp_node->next = append_instruction(case_body, jmp_to_end);
        
        if (first_case == nullptr) {
            first_case = cjmp_node;
            current = cjmp_node;
        } else {
            current->cjmp_inst.target = cjmp_node;
            current = cjmp_node;
        }
        
        token = lexer.peek(1);
    }
    
    // Handle default case if present
    if (token.token_type == DEFAULT) {
        lexer.GetToken(); // DEFAULT
        Token colon = lexer.GetToken(); // COLON
        struct InstructionNode* default_body = parse_body();
        
        if (current != nullptr) {
            current->cjmp_inst.target = default_body;
        }
        append_instruction(default_body, noop);
    } else {
        if (current != nullptr) {
            current->cjmp_inst.target = noop;
        }
    }
    
    Token rbrace = lexer.GetToken(); // RBRACE
    
    return append_instruction(first_case, noop);
}

struct InstructionNode* parse_input_stmt() {
    Token input_token = lexer.GetToken(); // INPUT
    Token id_token = lexer.GetToken(); // ID
    Token semicolon = lexer.GetToken(); // SEMICOLON
    
    struct InstructionNode* node = new InstructionNode;
    node->type = IN;
    node->input_inst.var_index = get_location(id_token.lexeme);
    node->next = nullptr;
    
    return node;
}

struct InstructionNode* parse_output_stmt() {
    Token output_token = lexer.GetToken(); // OUTPUT
    Token id_token = lexer.GetToken(); // ID
    Token semicolon = lexer.GetToken(); // SEMICOLON
    
    struct InstructionNode* node = new InstructionNode;
    node->type = OUT;
    node->output_inst.var_index = get_location(id_token.lexeme);
    node->next = nullptr;
    
    return node;
}

int parse_primary() {
    Token token = lexer.GetToken();
    
    if (token.token_type == ID) {
        return get_location(token.lexeme);
    } else if (token.token_type == NUM) {
        return allocate_constant(stoi(token.lexeme));
    }
    
    return -1;
}

ArithmeticOperatorType parse_op() {
    Token token = lexer.GetToken();
    
    switch (token.token_type) {
        case PLUS: return OPERATOR_PLUS;
        case MINUS: return OPERATOR_MINUS;
        case MULT: return OPERATOR_MULT;
        case DIV: return OPERATOR_DIV;
        default: return OPERATOR_NONE;
    }
}

ConditionalOperatorType parse_relop() {
    Token token = lexer.GetToken();
    
    switch (token.token_type) {
        case GREATER: return CONDITION_GREATER;
        case LESS: return CONDITION_LESS;
        case NOTEQUAL: return CONDITION_NOTEQUAL;
        default: return CONDITION_GREATER; // default
    }
}

void parse_inputs() {
    Token token = lexer.GetToken();
    while (token.token_type == NUM) {
        inputs.push_back(stoi(token.lexeme));
        token = lexer.GetToken();
    }
}